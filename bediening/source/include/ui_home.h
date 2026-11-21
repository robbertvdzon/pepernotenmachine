#pragma once

// The animated home screen: launcher band (animation per state), seven function tiles and a toast
// row. What to show comes from home_model.h, the pixel art from home_sprites.h; this file only puts
// it on LVGL canvases and labels. LVGL is only ever touched from the main loop.

#include <lvgl.h>
#include <stdint.h>

#include "home_model.h"

// Creates the home screen objects on `parent` (the home screen).
void ui_home_build(lv_obj_t* parent);

// Call every loop pass. The model always runs (so progress and animations are right when the home
// screen comes back); pixels are only drawn while `visible`.
void ui_home_update(const HomeInputs& in, bool visible);
