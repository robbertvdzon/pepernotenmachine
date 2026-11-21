#pragma once

// Logical UI buttons:
//   EncoderPush - rotary encoder's integrated push switch (direct GPIO, D7)
//   Back        - SW1, the separate back/escape button (via switches.cpp)
// Both are debounced and reported as press/release events.

#include <Arduino.h>

enum class ButtonId : uint8_t { EncoderPush = 0, Back = 1 };

struct ButtonEvent {
    ButtonId id;
    bool pressed;
    uint32_t t_ms;
};

void buttons_init();
// Call after switches_update() so Back sees the newest SW1 state.
void buttons_update(uint32_t now);

bool buttons_is_pressed(ButtonId id);
bool buttons_poll_event(ButtonEvent& out);
const char* buttons_name(ButtonId id);
