#pragma once
#include <stdint.h>

// Called when the debounced input state changed: bitmask, bit (n-1) set = input n active (pin LOW).
typedef void (*digital_inputs_changed_cb_t)(uint8_t mask);

void digital_inputs_init(digital_inputs_changed_cb_t cb);
uint8_t digital_inputs_get_mask();
