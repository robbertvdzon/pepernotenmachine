#pragma once

// 12 indicator LEDs: L1..L8 on MCP23017 GPA0..7, L9..L12 on Nano D2..D5.
// Active LOW in hardware; callers only think in states (see led_pattern.h).
//
// Every LED can individually be OFF, ON, FLASH, FLASH_FAST, PULSE, RAMP_UP or
// RAMP_DOWN. A small dedicated task renders the animations (software PWM with
// 8 brightness levels, since the expander has no PWM) so timing stays steady
// even when the main loop is busy with the display or BLE.
//
// LED indices are 0-based (index 0 = L1).

#include <Arduino.h>

#include "led_pattern.h"

enum LedId : uint8_t {
    LED_L1_AUTOMATIC_MODE = 0,
    LED_L2_READY_TO_LAUNCH = 1,
    LED_L3_LAUNCHER_CONNECTION = 2,
    LED_L4_DECORATIVE_LIGHTING = 3,
    LED_L5_SWITCHBOARD_CONNECTION = 4,
    LED_L6_PANIC_MODE = 5,
    LED_L7_FLASHING_LIGHT = 6,
    LED_L8_FOG_STATUS = 7,
    LED_L9_RESERVED = 8,
    LED_L10_RESERVED = 9,
    LED_L11_RESERVED = 10,
    LED_L12_RESERVED = 11,
};

// Configures the pins (all LEDs off) and starts the LED task. Call after mcp.begin().
void leds_init();

void leds_set_state(uint8_t index, led_state_t state);
led_state_t leds_get_state(uint8_t index);

// Convenience for plain on/off.
void leds_set(uint8_t index, bool on);
void leds_all_off();
