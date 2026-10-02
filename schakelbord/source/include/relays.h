#pragma once
#include <stddef.h>
#include <stdint.h>

// Called whenever the relay states changed: one state byte per relay (RELAY_STATE_*).
typedef void (*relays_changed_cb_t)(const uint8_t* states, size_t count);

void relays_init(relays_changed_cb_t cb);

// number: 1..RELAY_COUNT, state: RELAY_STATE_*, interval_ms: only used for blinking (0 = default).
// Returns false when the arguments are invalid. Non-blocking.
bool relays_set(uint8_t number, uint8_t state, uint16_t interval_ms);

void relays_all_off();

// Copies RELAY_COUNT state bytes into `out`.
void relays_get_states(uint8_t* out);
