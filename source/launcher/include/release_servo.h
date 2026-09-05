#pragma once
#include <stdint.h>

// Release servo control API

// Initialize servo hardware (attach to pin)
void release_servo_init();

// Set servo angle in degrees (0-180)
void release_servo_set_angle(uint8_t angle);

// Get last set angle (0-180)
uint8_t release_servo_get_angle();
