#pragma once
#include <stdint.h>

// Servo control API

// Initialize servo hardware (attach to pin)
void servo_init();

// Set servo angle in degrees (0-180)
void servo_set_angle(uint8_t angle);

// Get last set angle (0-180)
uint8_t servo_get_angle();
