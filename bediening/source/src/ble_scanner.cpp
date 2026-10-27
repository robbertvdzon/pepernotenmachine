#include "ble_scanner.h"

#include <NimBLEDevice.h>

#include "ble_address.h"
#include "ble_config.h"
#include "event_queue.h"
#include "log.h"

static const char* TAG = "BLE";

// What the NimBLE callback copies out (kept tiny; classification happens later).
struct RawHit {
    uint8_t addr[6];
    uint8_t addrType;
    int8_t rssi;
    bool launcherUuid;
    bool switchboardUuid;
    char name[DEVICE_NAME_MAX + 1];
};

static bool initialized = false;
static bool was_scanning = false;
static ScanEntry table[SCAN_MAX_ENTRIES];
static size_t table_count = 0;
static uint32_t table_overflow = 0;
static uint32_t other_devices = 0;  // unidentified advertisers seen this scan (not stored)
static uint32_t reported_queue_drops = 0;

static EventQueue<RawHit, 16> hit_queue;
static portMUX_TYPE hit_mux = portMUX_INITIALIZER_UNLOCKED;

// Runs on the NimBLE host task: copy and return, nothing else.
class ScanCallbacks : public NimBLEAdvertisedDeviceCallbacks {
   public:
    ScanCallbacks() : launcher_uuid_(std::string(LAUNCHER_SERVICE_UUID)), switchboard_uuid_(std::string(SWITCHBOARD_SERVICE_UUID)) {}

    void onResult(NimBLEAdvertisedDevice* dev) override {
        RawHit h;
        memset(&h, 0, sizeof(h));
        NimBLEAddress a = dev->getAddress();
        ble_address_from_native(a.getNative(), h.addr);
        h.addrType = dev->getAddressType();
        int rssi = dev->getRSSI();
        h.rssi = static_cast<int8_t>(rssi < -127 ? -127 : (rssi > 20 ? 20 : rssi));
        h.launcherUuid = dev->isAdvertisingService(launcher_uuid_);
        h.switchboardUuid = dev->isAdvertisingService(switchboard_uuid_);
        std::string name = dev->getName();
        strncpy(h.name, name.c_str(), DEVICE_NAME_MAX);

        portENTER_CRITICAL(&hit_mux);
        hit_queue.push(h);
        portEXIT_CRITICAL(&hit_mux);
    }

   private:
    NimBLEUUID launcher_uuid_;
    NimBLEUUID switchboard_uuid_;
};

static ScanCallbacks scan_callbacks;

// Completion is detected by polling isScanning() in update(); nothing to do here.
static void on_scan_done(NimBLEScanResults) {}

bool ble_stack_ensure_init() {
    if (!initialized) {
        NimBLEDevice::init("Control unit");
        initialized = NimBLEDevice::getInitialized();
        if (initialized) {
            LOGI(TAG, "NimBLE initialised");
        } else {
            LOGE(TAG, "NimBLE initialisation FAILED");
        }
    }
    return initialized;
}

static void clear_queue() {
    RawHit tmp;
    portENTER_CRITICAL(&hit_mux);
    while (hit_queue.pop(tmp)) {
    }
    portEXIT_CRITICAL(&hit_mux);
}

bool ble_scanner_start(uint32_t seconds) {
    if (!ble_stack_ensure_init()) return false;
    NimBLEScan* scan = NimBLEDevice::getScan();
    if (scan->isScanning()) {
        LOGW(TAG, "scan already running");
        return false;
    }

    table_count = 0;
    table_overflow = 0;
    other_devices = 0;
    clear_queue();

    scan->setAdvertisedDeviceCallbacks(&scan_callbacks, false);  // each device once per scan
    scan->setActiveScan(true);  // request scan responses so name AND service UUID arrive together
    scan->setInterval(100);     // ms
    scan->setWindow(80);        // ms
    scan->setMaxResults(0);     // we keep our own table; do not let NimBLE accumulate results
    scan->clearResults();

    const bool ok = scan->start(seconds, on_scan_done, false);
    if (ok) {
        was_scanning = true;
        LOGI(TAG, "scan started (%lu s, active)", (unsigned long)seconds);
    } else {
        LOGE(TAG, "scan failed to start");
    }
    return ok;
}

void ble_scanner_stop() {
    if (initialized && NimBLEDevice::getScan()->isScanning()) {
        NimBLEDevice::getScan()->stop();
        LOGI(TAG, "scan stop requested");
    }
}

bool ble_scanner_is_scanning() { return initialized && NimBLEDevice::getScan()->isScanning(); }

static void merge_hit(const RawHit& h, uint32_t now) {
    const Identity id = classify_advertisement(h.name, h.launcherUuid, h.switchboardUuid);

    // Only launchers, switchboards and identity conflicts are worth a table slot.
    // Everything else (phones, beacons, ...) is just counted, so a busy BLE
    // environment can never crowd the real devices out of the table.
    if (id.kind == DeviceKind::Unknown && !id.conflict) {
        other_devices++;
        return;
    }

    ScanEntry* e = nullptr;
    for (size_t i = 0; i < table_count; i++) {
        if (ble_address_equal(table[i].addr, h.addr) && table[i].addrType == h.addrType) {
            e = &table[i];
            break;
        }
    }
    const bool is_new = (e == nullptr);
    if (is_new) {
        if (table_count >= SCAN_MAX_ENTRIES) {
            table_overflow++;
            return;
        }
        e = &table[table_count++];
        *e = ScanEntry();
        memcpy(e->addr, h.addr, 6);
        e->addrType = h.addrType;
    }

    const DeviceKind old_kind = e->kind;
    e->kind = id.kind;
    e->strong = id.strong;
    e->conflict = id.conflict;
    e->rssi = h.rssi;
    e->lastSeenMs = now;
    if (h.name[0]) memcpy(e->name, h.name, sizeof(e->name));

    if (is_new || old_kind != e->kind) {
        char addr[18];
        ble_address_format(e->addr, addr, sizeof(addr));
        if (e->conflict) {
            LOGW(TAG, "identity CONFLICT %s (%s) name='%s' (name and service UUID disagree) - ignored", addr,
                 ble_address_type_name(e->addrType), e->name);
        } else if (e->kind != DeviceKind::Unknown) {
            LOGI(TAG, "found %-11s %s %s (%s) rssi=%d name='%s'", device_kind_name(e->kind),
                 e->strong ? "[service UUID]" : "[name only]", addr, ble_address_type_name(e->addrType), e->rssi,
                 e->name);
        }
    }
}

void ble_scanner_update(uint32_t now) {
    if (!initialized) return;

    for (;;) {
        RawHit h;
        portENTER_CRITICAL(&hit_mux);
        const bool got = hit_queue.pop(h);
        portEXIT_CRITICAL(&hit_mux);
        if (!got) break;
        merge_hit(h, now);
    }

    portENTER_CRITICAL(&hit_mux);
    const uint32_t drops = hit_queue.dropped();
    portEXIT_CRITICAL(&hit_mux);
    if (drops != reported_queue_drops) {
        reported_queue_drops = drops;
        LOGW(TAG, "scan hit queue overflow, %lu dropped in total", (unsigned long)drops);
    }

    const bool scanning = NimBLEDevice::getScan()->isScanning();
    if (was_scanning && !scanning) {
        unsigned launchers = 0, switchboards = 0, conflicts = 0;
        for (size_t i = 0; i < table_count; i++) {
            if (table[i].conflict) conflicts++;
            else if (table[i].kind == DeviceKind::Launcher) launchers++;
            else if (table[i].kind == DeviceKind::Switchboard) switchboards++;
        }
        LOGI(TAG, "scan finished: %u launcher(s), %u switchboard(s), %u conflict(s), %lu other device(s) ignored%s",
             launchers, switchboards, conflicts, (unsigned long)other_devices,
             table_overflow ? " (result table was full)" : "");
    }
    was_scanning = scanning;
}

size_t ble_scanner_count() { return table_count; }

const ScanEntry& ble_scanner_entry(size_t index) {
    static const ScanEntry empty;
    return index < table_count ? table[index] : empty;
}
