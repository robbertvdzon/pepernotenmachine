#pragma once
#include <stdint.h>

// Called whenever the outputs changed: bitmask, bit (n-1) set = output n on.
typedef void (*outputs_changed_cb_t)(uint8_t mask);

void outputs_init(outputs_changed_cb_t cb);

// number: 1..OUTPUT_COUNT, on: true = output active. Non-blocking.
bool outputs_set(uint8_t number, bool on);

void outputs_all_off();

uint8_t outputs_get_mask();
