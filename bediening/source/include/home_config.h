#pragma once

// Home screen (animated launcher band, function tiles, toast): timings and layout.
// Plain constants only (host-testable). Pins and control timings are in config.h / control_config.h.

#include <stdint.h>

#include "control_config.h"

// ---- animation / progress timings (display only: they never decide what the launcher does) ----
constexpr uint32_t HOME_FRAME_MS = 83;       // 12 fps
constexpr uint32_t PREP_DOWN_MS = 25000;     // progress bar while the bucket is pulled down (nominal)
constexpr uint32_t PREP_UP_MS = 25000;       // progress bar while the rope is unwound (nominal), lock animation included
constexpr uint32_t LOCK_ANIM_MS = 1700;      // padlock animation when the launcher goes from down to up
constexpr uint32_t LAUNCHED_ANIM_MS = 1300;  // rocket flies out after the launch write is confirmed
constexpr uint32_t TOAST_SLIDE_MS = 200;
constexpr uint32_t BOARD_OFFLINE_TOAST_MS = 3000;  // a toast for a missing switchboard only after this long

static_assert(LOCK_ANIM_MS < PREP_UP_MS, "the lock animation must be shorter than the unwind phase");
static_assert(LAUNCHED_ANIM_MS < REARM_DELAY_MS + 1000, "launch animation should not outlast the re-arm countdown by much");

// ---- layout of the 160x128 home screen (px) ----
constexpr int HOME_BAND_X = 0, HOME_BAND_Y = 0, HOME_BAND_W = 160, HOME_BAND_H = 78;
constexpr int HOME_TILES_Y = 82, HOME_TILES_W = 160, HOME_TILES_H = 24;
constexpr int HOME_TILE_W = 20, HOME_TILE_GAP = 2, HOME_TILE_X0 = 4;
// Every canvas is 160 px wide. LVGL may pad canvas rows to LV_DRAW_BUF_STRIDE_ALIGN bytes (1 = no padding with the
// default config this project uses; other configs differ) while the pixel code writes tightly packed rows, so a canvas
// whose row size is not a multiple of it would shear the picture (checked at compile time in ui_home.cpp). The toast
// bar is drawn with a margin inside its canvas.
constexpr int HOME_TOAST_Y = 109, HOME_TOAST_W = 160, HOME_TOAST_H = 18, HOME_TOAST_MARGIN = 4;
constexpr int HOME_TOAST_TEXT_DY = -1;  // the font sits low in its line: raise the toast text one row to center it in the bar
constexpr int HOME_TEXT_X = 70;       // big / small word right of the band animation
constexpr int HOME_WORD_Y = 14, HOME_SMALL_Y = 34;
