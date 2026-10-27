#pragma once

// Presentation layer: home screen (live connection state) and the setup flow
// (select / forget the launcher and the switchboard). LVGL is only ever touched
// from the main loop.
//
// Controls:
//   home screen    short press = actions menu (prepare launcher), hold (UI_LONG_PRESS_MS) = setup
//   menus          turn = move focus, push = select, Back (SW1) = go back
//   scan screen    Back stops the scan and goes back

#include <stdint.h>

void ui_init();
void ui_update(uint32_t now);

// Input (call from the main loop when the events arrive).
void ui_on_encoder(int32_t delta);   // detents turned since the last call
void ui_on_push(bool pressed);       // debounced encoder push switch state
void ui_on_back();                   // Back button pressed
