#pragma once

// =============================================================================
// Control-logic configuration: WHICH switchboard relay does what, and the
// timings of the safety rules. Plain constants only (host-testable).
// =============================================================================

#include <stdint.h>

// Control-unit switch indices (identical to SwitchId in switches.h; checked in controller.cpp).
constexpr uint8_t CSW_PANIC = 1;   // SW2
constexpr uint8_t CSW_FOG = 2;     // SW3
constexpr uint8_t CSW_FLASH = 3;   // SW4 flashlights
constexpr uint8_t CSW_AUTO = 4;    // SW5 automatic mode
constexpr uint8_t CSW_DECOR = 5;   // SW6 illumination
constexpr uint8_t CSW_HORN = 6;    // SW7 airhorn (momentary: the relay follows the button)
constexpr uint8_t CSW_LAUNCH = 7;  // SW8
constexpr int8_t CSW_NONE = -1;    // device without a dedicated manual switch

// Switchboard relay numbers, 1..8 (0 = not wired). The device table with their behaviour is in control_patterns.h.
constexpr uint8_t RELAY_RED_FLASH = 1;        // red flashlight, outside right back, 220 V
constexpr uint8_t RELAY_ORANGE_INSIDE = 2;    // orange flashlight, inside, 220 V
constexpr uint8_t RELAY_STROBE_INSIDE = 3;    // strobe light, inside, 220 V
constexpr uint8_t RELAY_ILLUMINATION = 4;     // illumination, exterior, 220 V
constexpr uint8_t RELAY_ORANGE_RIGHT = 5;     // orange flashlight, outside right front, 24 V
constexpr uint8_t RELAY_AIRHORN = 6;          // airhorn, outside right front, 24 V
constexpr uint8_t RELAY_SIREN = 7;            // siren, outside left back, 12 V (very loud)
constexpr uint8_t RELAY_ORANGE_LEFT = 8;      // orange flashlight, outside left front, 12 V
// The fog machine hangs on the switchboard's fog output (D9), not on a relay.

// Idempotent state commands (relays) are re-sent until the switchboard confirms the state.
constexpr uint32_t RELAY_RESEND_MS = 500;
// Fog: while the fog switch is ON a new start is requested whenever the machine is ready and idle.
constexpr uint32_t FOG_START_INTERVAL_MS = 1000;  // minimum time between start requests
constexpr uint32_t FOG_STOP_RESEND_MS = 500;

// Launcher (fully automatic: power-up check -> prepare -> launch -> prepare again).
constexpr uint32_t LATCH_RELOCK_MS = 4000;          // the launcher latch re-locks this long after a launch
constexpr uint32_t REARM_DELAY_MS = 4000;           // after a launch, wait this long before preparing again
static_assert(REARM_DELAY_MS >= LATCH_RELOCK_MS, "never start preparing before the latch has re-locked");
constexpr uint32_t PREPARE_START_TIMEOUT_MS = 3000; // prepare written but the launcher never left idle = refused
constexpr uint32_t PREPARE_TIMEOUT_MS = 90000;      // preparation takes ~65 s nominally
constexpr uint32_t PREPARE_RETRY_MS = 3000;         // a prepare write that failed is tried again after this
constexpr uint8_t PREPARE_MAX_FAILS = 3;            // ... at most this many times in a row, then the driver is asked

// Feedback.
constexpr uint32_t LED_PENDING_SHOW_MS = 700;    // a relay command that is not confirmed after this = flash (shorter lags are not shown)
constexpr uint32_t LED_PENDING_ERROR_MS = 3000;  // ... and after this = fast flash
constexpr uint32_t REJECT_FLASH_MS = 1500;       // L2 fast-flash after a rejected launch
constexpr uint32_t MESSAGE_SHOW_MS = 3000;       // how long the UI shows a message

// ---- maintenance (menu "Maintenance": release the bucket, dispense by hand) ----
constexpr uint32_t DISPENSE_START_TIMEOUT_MS = 2000;  // the dispenser must report running within this time of the start write
constexpr uint32_t DISPENSE_STOP_RESEND_MS = 500;     // the stop (0x00) is repeated until the dispenser reports "stopped"
constexpr uint8_t DISPENSE_STOP_MIN_SENDS = 2;        // ... and is always sent at least twice (the start may be late or unreported)
constexpr uint32_t DISPENSE_STUCK_MS = 3000;          // stop still not confirmed after this long: tell the driver

// ---- dispenser time (menu "Maintenance" -> "Dispenser time"): seconds of the dispense cycle, stored by the launcher ----
constexpr uint8_t DURATION_MIN_S = 1;
constexpr uint8_t DURATION_MAX_S = 60;
constexpr uint32_t DURATION_VERIFY_TIMEOUT_MS = 3000;  // after the write, the read-back must be done within this time
