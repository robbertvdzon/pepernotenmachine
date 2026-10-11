#pragma once
#include <stdint.h>

// Dispenser control API

typedef void (*dispenser_notify_cb_t)(uint8_t state);

// Initialize dispenser hardware
void dispenser_init(dispenser_notify_cb_t notifyCb);

// Set dispensing duration in seconds (1-20, default 4)
void dispenser_set_duration(uint8_t seconds);
uint8_t dispenser_get_duration(void);

// Start dispensing (1) or stop (0)
void dispenser_control(uint8_t command);