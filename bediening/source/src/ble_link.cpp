#include "ble_link.h"

#include <NimBLEDevice.h>
#include <ctype.h>

#include "ble_address.h"
#include "ble_scanner.h"
#include "config.h"
#include "device_store.h"
#include "log.h"

static const char* TAG = "LINK";

namespace {

struct ValueSlot {
    uint8_t data[LINK_VALUE_MAX];
    uint8_t len;
    bool valid;
    uint32_t updatedMs;
    uint32_t version;  // bumped on every change, incl. invalidation
};

struct Link {
    DeviceKind kind;

    // ---- shared with the main loop and the NimBLE host task (guarded by link_mux)
    LinkState state;
    char detail[20];
    ValueSlot values[LINK_MAX_CHARS];
    StoredDevice saved;
    uint32_t configGen;

    // ---- BLE task only
    uint32_t appliedGen;
    bool configured;
    NimBLEClient* client;
    NimBLERemoteCharacteristic* chr[LINK_MAX_CHARS];
    bool sessionUp;
    uint32_t nextAttemptMs;
    uint32_t attempts;
    std::string address;   // "aa:bb:cc:dd:ee:ff"
    uint8_t addressType;

    QueueHandle_t cmdq;  // pending writes, executed by the BLE task

    // ---- main loop only
    uint32_t loggedVersion[LINK_MAX_CHARS];
};

struct WriteCmd {
    uint32_t seq;
    uint8_t ci;
    uint8_t len;
    uint8_t data[LINK_WRITE_MAX];
};
struct WriteSlot {
    uint32_t seq;
    WriteState state;
};
constexpr uint8_t WRITE_RING = 16;
WriteSlot write_ring[WRITE_RING];  // guarded by link_mux
uint32_t next_seq = 1;

Link links[2];
portMUX_TYPE link_mux = portMUX_INITIALIZER_UNLOCKED;
bool started = false;

inline uint8_t index_of(DeviceKind k) { return k == DeviceKind::Launcher ? 0 : 1; }

inline bool due(uint32_t now, uint32_t when) { return static_cast<int32_t>(now - when) >= 0; }

void set_state(Link& l, LinkState s, const char* detail) {
    bool changed;
    portENTER_CRITICAL(&link_mux);
    changed = (l.state != s) || strncmp(l.detail, detail, sizeof(l.detail)) != 0;
    l.state = s;
    strncpy(l.detail, detail, sizeof(l.detail) - 1);
    l.detail[sizeof(l.detail) - 1] = '\0';
    portEXIT_CRITICAL(&link_mux);
    if (changed) {
        LOGI(TAG, "%-11s -> %s%s%s", device_kind_name(l.kind), link_state_name(s), detail[0] ? ": " : "", detail);
    }
}

// Called from the NimBLE host task (notifications) and the BLE task (reads).
void store_value(uint8_t link_idx, uint8_t ci, const uint8_t* data, size_t len) {
    if (link_idx > 1 || ci >= LINK_MAX_CHARS) return;
    ValueSlot& v = links[link_idx].values[ci];
    const uint8_t n = static_cast<uint8_t>(len > LINK_VALUE_MAX ? LINK_VALUE_MAX : len);
    portENTER_CRITICAL(&link_mux);
    memcpy(v.data, data, n);
    v.len = n;
    v.valid = true;
    v.updatedMs = millis();
    v.version++;
    portEXIT_CRITICAL(&link_mux);
}

void invalidate_values(Link& l) {
    portENTER_CRITICAL(&link_mux);
    for (uint8_t i = 0; i < LINK_MAX_CHARS; i++) {
        if (l.values[i].valid) {
            l.values[i].valid = false;
            l.values[i].version++;
        }
        l.chr[i] = nullptr;
    }
    portEXIT_CRITICAL(&link_mux);
}

void set_write_state(uint32_t seq, WriteState st) {
    portENTER_CRITICAL(&link_mux);
    WriteSlot& w = write_ring[seq % WRITE_RING];
    if (w.seq == seq) w.state = st;
    portEXIT_CRITICAL(&link_mux);
}

// Executes the queued writes, once each. When the link is not up they fail (never wait, never retry).
void drain_commands(Link& l) {
    if (!l.cmdq) return;
    const bool up = l.sessionUp && l.client && l.client->isConnected();
    WriteCmd c;
    while (xQueueReceive(l.cmdq, &c, 0) == pdTRUE) {
        bool ok = false;
        if (up && c.ci < LINK_MAX_CHARS && l.chr[c.ci]) ok = l.chr[c.ci]->writeValue(c.data, c.len, true);
        set_write_state(c.seq, ok ? WriteState::Ok : WriteState::Failed);
        uint8_t count = 0;
        const CharSpec* specs = ble_char_specs(l.kind, count);
        LOGI(TAG, "%-11s write %s: %s", device_kind_name(l.kind), c.ci < count ? specs[c.ci].name : "?",
             ok ? "ok" : "FAILED");
    }
}

// Discovers the service, checks the required characteristics, subscribes and
// reads the current state. Subscribing happens BEFORE the initial read so no
// update can fall in the gap. Returns false (with `detail` set) if unusable.
bool setup_session(Link& l, uint8_t link_idx, const char*& detail) {
    detail = LINK_ERR_NONE;
    NimBLERemoteService* svc = l.client->getService(NimBLEUUID(std::string(ble_service_uuid(l.kind))));
    if (!svc) {
        detail = LINK_ERR_SERVICE_MISSING;
        return false;
    }

    uint8_t count = 0;
    const CharSpec* specs = ble_char_specs(l.kind, count);
    for (uint8_t i = 0; i < count; i++) {
        const CharSpec& spec = specs[i];
        NimBLERemoteCharacteristic* c = svc->getCharacteristic(NimBLEUUID(std::string(spec.uuid)));
        if (!c) {
            if (spec.required) {
                LOGW(TAG, "%s: required characteristic %s not found", device_kind_name(l.kind), spec.name);
                detail = LINK_ERR_CHAR_MISSING;
                return false;
            }
            LOGD(TAG, "%s: optional characteristic %s not present", device_kind_name(l.kind), spec.name);
            continue;
        }
        l.chr[i] = c;

        if (spec.write && !c->canWrite()) {
            if (spec.required) {
                LOGW(TAG, "%s: %s is not writable", device_kind_name(l.kind), spec.name);
                detail = LINK_ERR_NOT_WRITABLE;
                return false;
            }
        }

        if (spec.subscribe) {
            const uint8_t ci = i;
            const bool ok = c->canNotify() &&
                            c->subscribe(true,
                                         [link_idx, ci](NimBLERemoteCharacteristic*, uint8_t* d, size_t n, bool) {
                                             store_value(link_idx, ci, d, n);
                                         },
                                         true);
            if (!ok) {
                LOGW(TAG, "%s: subscribe to %s failed", device_kind_name(l.kind), spec.name);
                if (spec.required) {
                    detail = LINK_ERR_SUBSCRIBE;
                    return false;
                }
            }
        }

        if (spec.read) {
            NimBLEAttValue v = c->canRead() ? c->readValue() : NimBLEAttValue();
            if (v.size() == 0) {
                LOGW(TAG, "%s: initial read of %s failed", device_kind_name(l.kind), spec.name);
                if (spec.required) {
                    detail = LINK_ERR_READ;
                    return false;
                }
            } else {
                store_value(link_idx, i, v.data(), v.size());
            }
        }
    }
    return true;
}

void drop_client(Link& l) {
    if (!l.client) return;
    if (l.client->isConnected()) {
        l.client->disconnect();
        for (int i = 0; i < 40 && l.client->isConnected(); i++) vTaskDelay(pdMS_TO_TICKS(25));
    }
    NimBLEDevice::deleteClient(l.client);
    l.client = nullptr;
}

// (Re)applies the saved device to the link: new client, new address.
void apply_config(Link& l, uint32_t now) {
    drop_client(l);
    l.sessionUp = false;
    invalidate_values(l);

    StoredDevice dev;
    uint32_t gen;
    portENTER_CRITICAL(&link_mux);
    dev = l.saved;
    gen = l.configGen;
    portEXIT_CRITICAL(&link_mux);
    l.appliedGen = gen;

    l.configured = dev.valid;
    if (!l.configured) {
        set_state(l, LinkState::NotConfigured, LINK_ERR_NONE);
        return;
    }

    char text[18];
    ble_address_format(dev.addr, text, sizeof(text));
    l.address = text;
    for (size_t i = 0; i < l.address.size(); i++) l.address[i] = static_cast<char>(tolower(l.address[i]));
    l.addressType = dev.addrType;

    l.client = NimBLEDevice::createClient();
    if (!l.client) {
        l.configured = false;
        set_state(l, LinkState::Error, LINK_ERR_NO_CLIENT);
        return;
    }
    l.client->setConnectTimeout(LINK_CONNECT_TIMEOUT_S);
    l.client->setConnectionParams(LINK_CONN_INTERVAL_MIN, LINK_CONN_INTERVAL_MAX, LINK_CONN_LATENCY,
                                  LINK_SUPERVISION_TIMEOUT);
    l.nextAttemptMs = now;
    l.attempts = 0;
    LOGI(TAG, "%-11s configured: %s (%s)", device_kind_name(l.kind), l.address.c_str(),
         ble_address_type_name(l.addressType));
    set_state(l, LinkState::Searching, LINK_ERR_NONE);
}

void attempt_connection(Link& l, uint8_t link_idx) {
    // An Error stays visible between retries; otherwise we are simply searching.
    if (l.state != LinkState::Error) set_state(l, LinkState::Searching, LINK_ERR_NONE);

    l.attempts++;
    const bool ok = l.client->connect(NimBLEAddress(l.address, l.addressType), true);
    if (!ok) {
        l.nextAttemptMs = millis() + LINK_RETRY_GAP_MS;
        return;
    }

    LOGI(TAG, "%-11s link up after %lu attempt(s), setting up", device_kind_name(l.kind), (unsigned long)l.attempts);
    set_state(l, LinkState::Connecting, LINK_ERR_NONE);

    const char* detail = LINK_ERR_NONE;
    if (setup_session(l, link_idx, detail)) {
        l.sessionUp = true;
        l.attempts = 0;
        set_state(l, LinkState::Connected, LINK_ERR_NONE);
    } else {
        l.client->disconnect();
        invalidate_values(l);
        set_state(l, LinkState::Error, detail);
        l.nextAttemptMs = millis() + LINK_ERROR_BACKOFF_MS;
    }
}

void service_link(Link& l, uint8_t link_idx, uint32_t now) {
    drain_commands(l);
    if (l.appliedGen != l.configGen) apply_config(l, now);
    if (!l.configured || !l.client) return;

    const bool connected = l.client->isConnected();
    if (l.sessionUp && !connected) {
        // Established link was lost (out of range, power loss, ...). Everything
        // learned over it is now unknown; start reconnecting right away.
        l.sessionUp = false;
        invalidate_values(l);
        LOGW(TAG, "%-11s link LOST, reconnecting", device_kind_name(l.kind));
        set_state(l, LinkState::Searching, LINK_ERR_NONE);
        l.nextAttemptMs = now + LINK_RECONNECT_FIRST_DELAY_MS;
        return;
    }
    if (connected) return;                      // up (or being torn down after a failed setup)
    if (!due(now, l.nextAttemptMs)) return;
    if (ble_scanner_is_scanning()) return;      // a setup scan has priority; connect() would cancel it

    attempt_connection(l, link_idx);
}

void link_task(void*) {
    for (;;) {
        const uint32_t now = millis();
        for (uint8_t i = 0; i < 2; i++) service_link(links[i], i, now);
        vTaskDelay(pdMS_TO_TICKS(LINK_TASK_PERIOD_MS));
    }
}

void load_saved(Link& l) {
    StoredDevice dev;
    device_store_load(l.kind, dev);  // false -> dev stays invalid ("not configured")
    portENTER_CRITICAL(&link_mux);
    l.saved = dev;
    l.configGen++;
    portEXIT_CRITICAL(&link_mux);
}

}  // namespace

void ble_link_init() {
    if (started) return;
    for (uint8_t i = 0; i < 2; i++) {
        Link& l = links[i];
        l.kind = i == 0 ? DeviceKind::Launcher : DeviceKind::Switchboard;
        l.state = LinkState::NotConfigured;
        l.appliedGen = 0xFFFFFFFFu;  // forces the first apply_config()
        l.cmdq = xQueueCreate(8, sizeof(WriteCmd));
        load_saved(l);
    }
    if (!ble_stack_ensure_init()) {
        LOGE(TAG, "BLE stack failed to start, no connections possible");
        return;
    }
    const BaseType_t ok = xTaskCreatePinnedToCore(link_task, "ble_link", 8192, nullptr, 1, nullptr, 1);
    started = (ok == pdPASS);
    if (!started) LOGE(TAG, "failed to start BLE link task");
}

void ble_link_reload() {
    if (!started) return;
    for (uint8_t i = 0; i < 2; i++) load_saved(links[i]);
}

LinkState ble_link_state(DeviceKind kind) {
    if (kind != DeviceKind::Launcher && kind != DeviceKind::Switchboard) return LinkState::NotConfigured;
    LinkState s;
    portENTER_CRITICAL(&link_mux);
    s = links[index_of(kind)].state;
    portEXIT_CRITICAL(&link_mux);
    return s;
}

void ble_link_detail(DeviceKind kind, char* out, size_t cap) {
    if (!out || cap == 0) return;
    out[0] = '\0';
    if (kind != DeviceKind::Launcher && kind != DeviceKind::Switchboard) return;
    portENTER_CRITICAL(&link_mux);
    strncpy(out, links[index_of(kind)].detail, cap - 1);
    portEXIT_CRITICAL(&link_mux);
    out[cap - 1] = '\0';
}

bool ble_link_get_value(DeviceKind kind, uint8_t char_index, uint8_t* out, uint8_t cap, uint8_t* len,
                        uint32_t* age_ms) {
    if ((kind != DeviceKind::Launcher && kind != DeviceKind::Switchboard) || char_index >= LINK_MAX_CHARS) return false;
    Link& l = links[index_of(kind)];
    bool ok = false;
    portENTER_CRITICAL(&link_mux);
    // Only trustworthy while the link is fully up in this connection.
    if (l.state == LinkState::Connected && l.values[char_index].valid) {
        const ValueSlot& v = l.values[char_index];
        const uint8_t n = v.len < cap ? v.len : cap;
        if (out) memcpy(out, v.data, n);
        if (len) *len = n;
        if (age_ms) *age_ms = millis() - v.updatedMs;
        ok = true;
    }
    portEXIT_CRITICAL(&link_mux);
    return ok;
}

uint32_t ble_link_write(DeviceKind kind, uint8_t char_index, const uint8_t* data, uint8_t len) {
    if (!started || !data || len == 0 || len > LINK_WRITE_MAX || char_index >= LINK_MAX_CHARS) return 0;
    if (kind != DeviceKind::Launcher && kind != DeviceKind::Switchboard) return 0;
    Link& l = links[index_of(kind)];
    if (!l.cmdq || ble_link_state(kind) != LinkState::Connected) return 0;  // nothing is queued for a dead link

    WriteCmd c;
    c.ci = char_index;
    c.len = len;
    memcpy(c.data, data, len);
    portENTER_CRITICAL(&link_mux);
    c.seq = next_seq++;
    if (next_seq == 0) next_seq = 1;
    write_ring[c.seq % WRITE_RING].seq = c.seq;
    write_ring[c.seq % WRITE_RING].state = WriteState::Pending;
    portEXIT_CRITICAL(&link_mux);

    if (xQueueSend(l.cmdq, &c, 0) != pdTRUE) {
        set_write_state(c.seq, WriteState::Failed);
        return 0;
    }
    return c.seq;
}

WriteState ble_link_write_state(uint32_t seq) {
    if (seq == 0) return WriteState::Failed;  // "nothing was queued"
    WriteState st = WriteState::Unknown;
    portENTER_CRITICAL(&link_mux);
    if (write_ring[seq % WRITE_RING].seq == seq) st = write_ring[seq % WRITE_RING].state;
    portEXIT_CRITICAL(&link_mux);
    return st;
}

// Logs every value change so the data flow can be verified without a UI.
void ble_link_update(uint32_t) {
    if (!started) return;
    for (uint8_t i = 0; i < 2; i++) {
        Link& l = links[i];
        uint8_t count = 0;
        const CharSpec* specs = ble_char_specs(l.kind, count);
        for (uint8_t c = 0; c < count; c++) {
            ValueSlot snap;
            portENTER_CRITICAL(&link_mux);
            snap = l.values[c];
            portEXIT_CRITICAL(&link_mux);
            if (snap.version == l.loggedVersion[c]) continue;
            l.loggedVersion[c] = snap.version;
            if (!snap.valid) {
                LOGI(TAG, "%-11s %s = (no current value)", device_kind_name(l.kind), specs[c].name);
                continue;
            }
            char hex[LINK_VALUE_MAX * 3 + 1];
            hex[0] = '\0';
            for (uint8_t b = 0; b < snap.len; b++) snprintf(hex + b * 3, 4, "%02X ", snap.data[b]);
            LOGI(TAG, "%-11s %s = %s", device_kind_name(l.kind), specs[c].name, hex);
        }
    }
}
