#pragma once

// =============================================================================
// Control-logic configuration: WHICH switchboard relay does what, and the
// timings of the safety rules. Plain constants only (host-testable).
// =============================================================================

#include <stdint.h>

// Switchboard relay numbers, 1..8. 0 = not wired / disabled.
// NOTHING IS WIRED YET: these are placeholders, change them here when the wiring is known.
constexpr uint8_t RELAY_FLASHING_LIGHT = 1;    // SW4
constexpr uint8_t RELAY_DECORATIVE_LIGHT = 2;  // SW6
constexpr uint8_t RELAY_SIREN = 3;             // SW7, momentary: relay follows the button

// Idempotent state commands (relays) are re-sent until the switchboard confirms the state.
constexpr uint32_t RELAY_RESEND_MS = 500;
// Fog: while the fog switch is ON a new start is requested whenever the machine is ready and idle.
constexpr uint32_t FOG_START_INTERVAL_MS = 1000;  // minimum time between start requests
constexpr uint32_t FOG_STOP_RESEND_MS = 500;

// Launcher.
constexpr uint32_t LAUNCH_LOCKOUT_MS = 6000;        // latch re-locks after 4 s; no launch/prepare before this
constexpr uint32_t PREPARE_START_TIMEOUT_MS = 3000; // prepare written but the launcher never left idle
constexpr uint32_t PREPARE_TIMEOUT_MS = 90000;      // preparation takes ~65 s nominally

// Feedback.
constexpr uint32_t LED_PENDING_ERROR_MS = 3000;  // a command that is still not confirmed after this = fast flash
constexpr uint32_t REJECT_FLASH_MS = 1500;       // L2 fast-flash after a rejected launch / prepare
constexpr uint32_t MESSAGE_SHOW_MS = 3000;       // how long the UI shows a message
