#pragma once

// Persistent storage (ESP32 NVS via Preferences) of the ONE selected launcher
// and ONE selected switchboard. Each is stored as a small validated record
// (see device_record.h); a missing or corrupt record loads as "no device".

#include "ble_identity.h"
#include "device_record.h"

bool device_store_load(DeviceKind kind, StoredDevice& out);  // false = none stored
bool device_store_save(DeviceKind kind, const StoredDevice& dev);
bool device_store_erase(DeviceKind kind);
