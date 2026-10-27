#pragma once

// Pure LED state / animation logic (host-testable, no hardware access).
//
// All patterns are a function of a GLOBAL millisecond clock, so every LED in
// the same state animates in sync (two flashing LEDs blink together).
//
//   LED_OFF         0 %
//   LED_ON          100 %
//   LED_FLASH       1 Hz,  50 % duty   (1000 ms period)   searching / discovering
//   LED_FLASH_FAST  5 Hz,  50 % duty   (200 ms period)    error / attention
//   LED_PULSE       smooth 0 -> 100 -> 0 triangle (2000 ms period)
//   LED_RAMP_UP     0 -> 100 %, then jumps back to 0 (1500 ms period)
//   LED_RAMP_DOWN   100 -> 0 %, then jumps back to 100 (1500 ms period)

#include <stdint.h>

typedef enum {
    LED_OFF = 0,
    LED_ON,
    LED_FLASH,
    LED_FLASH_FAST,
    LED_PULSE,
    LED_RAMP_UP,
    LED_RAMP_DOWN
} led_state_t;

constexpr uint32_t LED_FLASH_PERIOD_MS = 1000;
constexpr uint32_t LED_FLASH_FAST_PERIOD_MS = 200;
constexpr uint32_t LED_PULSE_PERIOD_MS = 2000;
constexpr uint32_t LED_RAMP_PERIOD_MS = 1500;

// Software PWM brightness resolution (slots per PWM cycle). 8 levels at a
// 1 ms slot = 125 Hz PWM, flicker free; more levels would need a faster slot.
constexpr uint8_t LED_PWM_LEVELS = 8;

// States whose brightness takes intermediate values and therefore need PWM.
inline bool led_state_is_fading(led_state_t s) { return s == LED_PULSE || s == LED_RAMP_UP || s == LED_RAMP_DOWN; }

inline bool led_state_is_valid(int s) { return s >= LED_OFF && s <= LED_RAMP_DOWN; }

// Brightness 0..255 of `state` at time `t_ms` (any monotonic millisecond clock).
inline uint8_t led_pattern_brightness(led_state_t state, uint32_t t_ms) {
    switch (state) {
        case LED_OFF:
            return 0;
        case LED_ON:
            return 255;
        case LED_FLASH:
            return (t_ms % LED_FLASH_PERIOD_MS) < LED_FLASH_PERIOD_MS / 2 ? 255 : 0;
        case LED_FLASH_FAST:
            return (t_ms % LED_FLASH_FAST_PERIOD_MS) < LED_FLASH_FAST_PERIOD_MS / 2 ? 255 : 0;
        case LED_PULSE: {
            const uint32_t half = LED_PULSE_PERIOD_MS / 2;
            const uint32_t ph = t_ms % LED_PULSE_PERIOD_MS;
            const uint32_t v = ph < half ? ph : LED_PULSE_PERIOD_MS - ph;  // 0..half
            return static_cast<uint8_t>((v * 255 + half / 2) / half);
        }
        case LED_RAMP_UP:
            return static_cast<uint8_t>((t_ms % LED_RAMP_PERIOD_MS) * 255 / (LED_RAMP_PERIOD_MS - 1));
        case LED_RAMP_DOWN:
            return static_cast<uint8_t>(255 - (t_ms % LED_RAMP_PERIOD_MS) * 255 / (LED_RAMP_PERIOD_MS - 1));
    }
    return 0;
}

// Maps perceived brightness 0..255 to a PWM duty level 0..LED_PWM_LEVELS with
// gamma 2.0 (an LED at 50 % duty looks much brighter than "half").
inline uint8_t led_brightness_to_level(uint8_t b) {
    const uint32_t v = static_cast<uint32_t>(b) * b * LED_PWM_LEVELS;
    return static_cast<uint8_t>((v + 65025 / 2) / 65025);
}
