#pragma once
#include <stddef.h>
#include <stdint.h>

// Callbacks invoked by the BLE layer when a client writes a characteristic.
// They are called from the BLE host task and must not block.
struct ble_callbacks_t {
    void (*relay_write)(uint8_t number, uint8_t state, uint16_t intervalMs);  // number 1..N, state 0/1/2
    void (*mosfet_write)(uint8_t number, uint8_t percent);                    // number 1..N, percent 0..100
    void (*output_write)(uint8_t number, uint8_t state);                      // number 1..N, state 0/1
    void (*fog_control_write)(uint8_t command);                               // 0 = stop, 1 = start
    void (*fog_duration_write)(uint8_t seconds);
    void (*all_clients_disconnected)();                                       // Last client left: switch everything off
};

void ble_init(const ble_callbacks_t& callbacks);
bool ble_is_connected();

// Update the value of a characteristic (so reads return it) and notify subscribed clients where applicable.
// Safe to call before ble_init() (ignored).
void ble_update_relays(const uint8_t* states, size_t count);
void ble_update_mosfets(const uint8_t* percent, size_t count);
void ble_update_digital_outputs(uint8_t mask);
void ble_update_fog_state(uint8_t state);
void ble_update_fog_control(uint8_t active);
void ble_update_fog_duration(uint8_t seconds);  // No notification, read/write only
void ble_update_digital_inputs(uint8_t mask);
void ble_update_analog_inputs(const uint16_t* millivolts, size_t count);  // Big-endian uint16 per channel
