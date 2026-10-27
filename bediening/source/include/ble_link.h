#pragma once

// Keeps the saved launcher and the saved switchboard connected.
//
// - One BLE task owns every NimBLE client operation (connect() blocks), so the
//   UI and main loop never wait on Bluetooth.
// - A lost or never-established link is retried forever; the moment a device is
//   back in range it is reconnected automatically.
// - After EVERY (re)connect the services are rediscovered, notifications are
//   re-subscribed and the current state is read again. Nothing cached from a
//   previous connection is trusted.
// - Values are only reported while the link is up. On disconnect every value
//   becomes invalid, so old state is never shown as current.
// - A setup scan (ble_scanner) has priority: while it runs no connection is
//   initiated, because NimBLE's connect() would cancel the scan.
// - Writes (ble_link_write) are queued to the BLE task, executed once, and their result can
//   be followed. They are NEVER retried or replayed here: whether to repeat a command is a
//   decision for the control logic (idempotent state settings may be re-sent; one-shot
//   actions such as launch must not be).

#include <Arduino.h>

#include "ble_devices.h"
#include "link_state.h"

// Loads the saved devices and starts the task. Call after the NVS and LEDs are usable.
void ble_link_init();

// Re-reads the saved devices from NVS (after the setup flow saved or erased one)
// and reconnects accordingly.
void ble_link_reload();

// Call every main-loop pass: logs value changes (INFO) for observability.
void ble_link_update(uint32_t now);

LinkState ble_link_state(DeviceKind kind);

// Short error text for the Error state ("" otherwise). Copies into `out`.
void ble_link_detail(DeviceKind kind, char* out, size_t cap);

// Latest value of a characteristic (index = LauncherChar / SwitchboardChar).
// Returns false when the link is not up or the value was never read in this
// connection, i.e. there is NO current value. `age_ms` = time since the last update.
bool ble_link_get_value(DeviceKind kind, uint8_t char_index, uint8_t* out, uint8_t cap, uint8_t* len,
                        uint32_t* age_ms);

// ---- writing ---------------------------------------------------------------------------
constexpr uint8_t LINK_WRITE_MAX = 8;  // bytes per write

enum class WriteState : uint8_t { Unknown, Pending, Ok, Failed };

// Queues ONE write (with response) to a characteristic of a connected device. Returns a sequence
// number for ble_link_write_state(), or 0 when nothing was queued (link not Connected, queue full).
uint32_t ble_link_write(DeviceKind kind, uint8_t char_index, const uint8_t* data, uint8_t len);

// Pending until the BLE task has executed it; Ok / Failed afterwards. Unknown = result was
// overwritten by newer writes (only the last 16 are kept) or seq was never issued.
WriteState ble_link_write_state(uint32_t seq);
