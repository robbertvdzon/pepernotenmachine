#pragma once

// Persistent storage (ESP32 NVS via Preferences) of the ONE selected launcher
// and ONE selected switchboard. Each is stored as a small validated record
// (see device_record.h); a missing or corrupt record loads as "no device".

#include "ble_identity.h"
#include "device_record.h"

bool device_store_load(DeviceKind kind, StoredDevice& out);  // false = none stored
bool device_store_save(DeviceKind kind, const StoredDevice& dev);
bool device_store_erase(DeviceKind kind);

// The last launcher state this unit believed in (0 unknown, 1 at rest, 2 armed). Only a HINT for
// the power-up question: the driver is always asked, whatever is stored here.
uint8_t launcher_memory_load();
bool launcher_memory_save(uint8_t state);
