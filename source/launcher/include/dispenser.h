#pragma once
#include <stdint.h>

// Dispenser control API

typedef void (*dispenser_notify_cb_t)(uint8_t state);

// Initialize dispenser hardware
void dispenser_init(dispenser_notify_cb_t notifyCb);

// Set dispensing duration in seconds (1-10, default 1)
void dispenser_set_duration(uint8_t seconds);

// Start dispensing (1) or stop (0)
void dispenser_control(uint8_t command);