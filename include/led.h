#pragma once
#include <stdint.h>
#include <stddef.h>

// Modes
#define LED_MODE_OFF 0
#define LED_MODE_ON 1
#define LED_MODE_SINE 2
#define LED_MODE_FLASH 3
#define LED_MODE_RAINBOW 4
#define LED_MODE_CROSSFADE 5

void led_init();
void led_set(uint8_t mode, uint8_t r, uint8_t g, uint8_t b);
// Parse raw write from BLE (data and len)
void led_set_from_bytes(const uint8_t* data, size_t len);

// Fill out 4-byte state: mode, r, g, b
void led_get_state(uint8_t out[4]);

// Call frequently from main loop
void led_update();
