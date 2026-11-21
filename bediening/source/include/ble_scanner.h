#pragma once

// Non-blocking BLE scanner for the launcher and the switchboard.
//
// NimBLE reports advertisements on its own host task. The callback only COPIES
// the data into a small locked queue; ble_scanner_update() (main loop) merges
// it into the result table, classifies it (service UUID = strong, name = weak,
// see ble_identity.h) and logs it. Nothing here blocks the UI.
//
// NimBLE is initialised lazily on the first ble_scanner_start().

#include <Arduino.h>

#include "ble_identity.h"
#include "device_record.h"

struct ScanEntry {
    DeviceKind kind = DeviceKind::Unknown;
    bool strong = false;    // identified by service UUID (not just name)
    bool conflict = false;  // name and service UUID disagree
    uint8_t addr[6] = {0, 0, 0, 0, 0, 0};  // display order
    uint8_t addrType = 0;
    int8_t rssi = -127;
    char name[DEVICE_NAME_MAX + 1] = {0};
    uint32_t lastSeenMs = 0;
};

constexpr size_t SCAN_MAX_ENTRIES = 12;

// Initialises the NimBLE stack once (also used by the link manager). False on failure.
bool ble_stack_ensure_init();

// Starts a scan for `seconds` (non-blocking). False if already scanning or BLE failed.
bool ble_scanner_start(uint32_t seconds);
void ble_scanner_stop();
bool ble_scanner_is_scanning();

// Call every loop iteration.
void ble_scanner_update(uint32_t now);

size_t ble_scanner_count();
const ScanEntry& ble_scanner_entry(size_t index);
