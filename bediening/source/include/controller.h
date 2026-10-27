#pragma once

// Glue between the hardware modules and the pure control logic (control_logic.h):
// gathers the switch and device states, runs the logic once per loop pass, executes the
// resulting commands through the BLE link manager and drives L2, L4, L7 and L8.

#include <Arduino.h>

#include "control_logic.h"
#include "switches.h"

void controller_init();

// Feed every debounced switch change here (the launch button is an edge, not a level).
void controller_on_switch(const SwitchEvent& e);

// Operator picked "Prepare launcher" in the actions menu.
void controller_request_prepare();

// Call every main-loop pass.
void controller_update(uint32_t now);

// ---- for the UI ----------------------------------------------------------------------------
LauncherStatus controller_launcher_status();
// Launcher status as text for the display, <= 20 characters ("READY", "preparing: up slow", ...).
void controller_launcher_text(char* out, size_t cap);
// The latest message (e.g. "not prepared") while it is fresh (MESSAGE_SHOW_MS). Returns false when there is none.
bool controller_message(char* out, size_t cap, bool* problem);
