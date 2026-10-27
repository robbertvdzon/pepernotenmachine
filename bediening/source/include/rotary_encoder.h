#pragma once

// Rotary encoder rotation. Quadrature edges are handled by pin-change
// interrupts, so no turns are missed while the main loop is busy (e.g. an
// LVGL redraw). The push switch lives in buttons.cpp.

#include <Arduino.h>

void rotary_encoder_init();

// Returns the detents turned since the last call (+ = one direction,
// - = the other) and clears the counter. Safe to call from the main loop.
int32_t rotary_encoder_take_delta();
