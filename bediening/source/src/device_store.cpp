#include "device_store.h"

#include <Preferences.h>

#include "ble_address.h"
#include "log.h"

static const char* TAG = "STOR";
static const char* NAMESPACE = "devices";

static const char* key_for(DeviceKind kind) {
    switch (kind) {
        case DeviceKind::Launcher: return "launcher";
        case DeviceKind::Switchboard: return "switchboard";
        default: return nullptr;
    }
}

bool device_store_load(DeviceKind kind, StoredDevice& out) {
    out = StoredDevice();
    const char* key = key_for(kind);
    if (!key) return false;

    Preferences prefs;
    if (!prefs.begin(NAMESPACE, false)) {
        LOGE(TAG, "cannot open NVS namespace '%s'", NAMESPACE);
        return false;
    }

    bool ok = false;
    uint8_t buf[DEVICE_RECORD_MAX];
    const size_t len = prefs.getBytesLength(key);
    if (len == 0) {
        // nothing stored yet: normal on first boot
    } else if (len > sizeof(buf)) {
        LOGW(TAG, "stored %s record has bad size %u, ignoring", device_kind_name(kind), (unsigned)len);
    } else if (prefs.getBytes(key, buf, len) != len) {
        LOGW(TAG, "could not read %s record", device_kind_name(kind));
    } else {
        ok = device_record_decode(buf, len, out);
        if (!ok) LOGW(TAG, "stored %s record is invalid, ignoring", device_kind_name(kind));
    }
    prefs.end();
    return ok;
}

bool device_store_save(DeviceKind kind, const StoredDevice& dev) {
    const char* key = key_for(kind);
    if (!key) return false;

    uint8_t buf[DEVICE_RECORD_MAX];
    const size_t n = device_record_encode(dev, buf, sizeof(buf));
    if (n == 0) {
        LOGE(TAG, "refusing to save an invalid %s record", device_kind_name(kind));
        return false;
    }

    Preferences prefs;
    if (!prefs.begin(NAMESPACE, false)) {
        LOGE(TAG, "cannot open NVS namespace '%s'", NAMESPACE);
        return false;
    }
    const bool ok = prefs.putBytes(key, buf, n) == n;
    prefs.end();

    char addr[18];
    ble_address_format(dev.addr, addr, sizeof(addr));
    if (ok) {
        LOGI(TAG, "saved %s: %s (%s) '%s'", device_kind_name(kind), addr, ble_address_type_name(dev.addrType),
             dev.name);
    } else {
        LOGE(TAG, "failed to save %s", device_kind_name(kind));
    }
    return ok;
}

bool device_store_erase(DeviceKind kind) {
    const char* key = key_for(kind);
    if (!key) return false;

    Preferences prefs;
    if (!prefs.begin(NAMESPACE, false)) {
        LOGE(TAG, "cannot open NVS namespace '%s'", NAMESPACE);
        return false;
    }
    // remove() returns false when the key does not exist; that is fine for erase.
    prefs.remove(key);
    prefs.end();
    LOGI(TAG, "erased %s", device_kind_name(kind));
    return true;
}
