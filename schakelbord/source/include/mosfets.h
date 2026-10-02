#pragma once
#include <stddef.h>
#include <stdint.h>

// Called whenever a duty cycle changed: one percentage byte (0..100) per MOSFET.
typedef void (*mosfets_changed_cb_t)(const uint8_t* percent, size_t count);

void mosfets_init(mosfets_changed_cb_t cb);

// number: 1..MOSFET_COUNT, percent: 0..100 (values above 100 are clamped). Non-blocking.
bool mosfets_set(uint8_t number, uint8_t percent);

void mosfets_all_off();

// Copies MOSFET_COUNT percentage bytes into `out`.
void mosfets_get(uint8_t* out);
