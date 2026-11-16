#pragma once

// The start-up splash screen on LVGL: title, subtitle, the animated link row and the "Connecting..." line.
// What to show comes from splash_model.h, the pixel art from splash_sprites.h. LVGL is only ever touched from the main loop.

#include <lvgl.h>
#include <stdint.h>

#include "splash_model.h"

// Creates a new full-screen splash screen (the caller loads it and deletes it when it is left).
lv_obj_t* ui_splash_build();

// Call every loop pass while the splash is the current screen; redraws only what changed.
void ui_splash_update(const SplashView& view);
