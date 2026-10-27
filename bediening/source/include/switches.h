#pragma once

// The 8 physical switches on MCP23017 port B (closed to GND = active).
// Debounced; one event per debounced transition. Polled over I2C because the
// expander's interrupt pins are not wired.

#include <Arduino.h>

enum SwitchId : uint8_t {
    SW1_BACK = 0,    // momentary  - back / escape
    SW2_PANIC,       // maintained
    SW3_FOG,         // maintained
    SW4_FLASH,       // maintained - flashing light
    SW5_AUTO,        // maintained - automatic mode
    SW6_DECOR,       // maintained - decorative lighting
    SW7_SIREN,       // momentary
    SW8_LAUNCH,      // momentary
};

struct SwitchEvent {
    SwitchId id;
    bool active;     // true = closed (active), false = open
    uint32_t t_ms;   // millis() when the debounced change was accepted
};

void switches_init();
// Call often; internally rate-limited to SWITCH_SCAN_INTERVAL_MS.
void switches_update(uint32_t now);

// Debounced state. Only trustworthy while switches_valid() is true.
bool switches_is_active(SwitchId id);
// false until the first successful read, and while the expander is failing
// (switch state is then UNKNOWN, not "unchanged").
bool switches_valid();

// Pops the next debounced change; returns false when none are pending.
bool switches_poll_event(SwitchEvent& out);

const char* switches_name(SwitchId id);
bool switches_is_momentary(SwitchId id);
