#pragma once

// Glue between the hardware modules and the pure control logic (control_logic.h):
// gathers the switch and device states, runs the logic once per loop pass, executes the
// resulting commands through the BLE link manager and drives L2, L4, L6, L7 and L8.

#include <Arduino.h>

#include "control_logic.h"
#include "home_model.h"
#include "switches.h"

void controller_init();

// Feed every debounced switch change here (the launch button is an edge, not a level).
void controller_on_switch(const SwitchEvent& e);

// The driver chose "Check launcher" in the menu: the unit forgets what it believed and asks again.
void controller_request_check();
// The driver's answer to the check question (None is ignored).
void controller_answer(Answer a);

// Call every main-loop pass.
void controller_update(uint32_t now);

// ---- for the UI ----------------------------------------------------------------------------
// True while the check overlay must be shown (idle launcher, state known, driver not yet answered).
bool controller_needs_confirm(CheckReason* reason);
// What was believed when the unit last switched off: only a hint for the overlay's first choice.
Known controller_stored_state();
// ---- Maintenance menu (see control_logic.h) ----
// True while the UI is on one of the maintenance screens (automation pauses, SW8 is blocked).
void controller_set_maintenance(bool on);
// "Release bucket" was confirmed: writes 1 to LAUNCH once (a real launch).
void controller_request_release();
// The knob is held on the Dispense screen (call every pass; false whenever it is not).
void controller_set_dispense_held(bool held);
RunState controller_dispense_state();
// Motor control: +1 = knob held on "up", -1 = on "down", 0 = not held (call every pass). Runs the pull motor at +-512.
void controller_set_motor_hold(int8_t dir);
RunState controller_motor_state();
// The latest message and when it appeared (millis), for screens that show it.
Msg controller_last_message(uint32_t* at);

// Everything the animated home screen needs, except the time (the caller sets `now`).
void controller_home_inputs(HomeInputs* out);
