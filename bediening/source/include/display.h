#pragma once

// ST7735S TFT (LovyanGFX) + LVGL 9 integration, landscape 160x128.
// LVGL is only ever touched from the main loop context.

#include <Arduino.h>

bool display_init();
// Runs LVGL timers/rendering. Returns the suggested ms until the next call.
uint32_t display_update();
