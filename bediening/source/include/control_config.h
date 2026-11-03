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

// Launcher (fully automatic: power-up check -> prepare -> launch -> prepare again).
constexpr uint32_t LATCH_RELOCK_MS = 4000;          // the launcher latch re-locks this long after a launch
constexpr uint32_t REARM_DELAY_MS = 4000;           // after a launch, wait this long before preparing again
static_assert(REARM_DELAY_MS >= LATCH_RELOCK_MS, "never start preparing before the latch has re-locked");
constexpr uint32_t PREPARE_START_TIMEOUT_MS = 3000; // prepare written but the launcher never left idle = refused
constexpr uint32_t PREPARE_TIMEOUT_MS = 90000;      // preparation takes ~65 s nominally
constexpr uint32_t PREPARE_RETRY_MS = 3000;         // a prepare write that failed is tried again after this
constexpr uint8_t PREPARE_MAX_FAILS = 3;            // ... at most this many times in a row, then the driver is asked

// Panic switch (SW2): fires some bells / horns (relays) in random order, just for fun.
// NOTHING IS WIRED YET: placeholder relay numbers, must differ from the relays above.
constexpr uint8_t PANIC_RELAYS[] = {4, 5, 6, 7};
constexpr uint8_t PANIC_RELAY_COUNT = sizeof(PANIC_RELAYS) / sizeof(PANIC_RELAYS[0]);
constexpr uint32_t PANIC_ON_MIN_MS = 300;    // a relay stays on for ON_MIN .. ON_MIN+ON_SPAN
constexpr uint32_t PANIC_ON_SPAN_MS = 900;
constexpr uint32_t PANIC_GAP_MIN_MS = 150;   // pause before the next one: GAP_MIN .. GAP_MIN+GAP_SPAN
constexpr uint32_t PANIC_GAP_SPAN_MS = 750;
constexpr uint32_t PANIC_CLEANUP_MS = 2000;  // after the switch goes off the relays are forced off for this long

// Feedback.
constexpr uint32_t LED_PENDING_ERROR_MS = 3000;  // a command that is still not confirmed after this = fast flash
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
