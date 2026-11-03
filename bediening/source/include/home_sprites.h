#pragma once

// Pixel art of the home screen (ported from tools/preview/make_preview.py): the animated launcher
// band, the function tiles and the toast background. Pure drawing into RGB565 surfaces, no LVGL.
// Text is NOT drawn here (LVGL labels), except the big countdown digit and the "?".

#include <stdint.h>

#include "gfx.h"
#include "home_model.h"

namespace sprites {

// 160x78 surface. `t_ms` = time since the band state began, `progress` 0..1000, `secsLeft` for Rearm.
void drawBand(const gfx::Surface& s, BandState st, uint32_t t_ms, int progress, int secsLeft);
// Color of the left stripe / accent text of a band state at that time (Rearm alternates amber / red).
uint32_t bandAccent(BandState st, uint32_t t_ms);

// 160x24 surface with all tiles; `now_ms` is a global clock (blinking tiles blink in sync).
void drawTiles(const gfx::Surface& s, const TileState* tiles, uint32_t now_ms);
// True if the picture of this tile changes over time (needs redrawing every frame).
bool tileAnimates(TileState st);

// 160x18 toast background with its colored bar, inset by HOME_TOAST_MARGIN (the text is a label on top).
void drawToast(const gfx::Surface& s, ToastLevel level);

// Big number (digits of 20x28 px, centered) with turn arrows left and right, for value screens; value < 0 draws "--".
// 160x40 surface.
void drawBigNumber(const gfx::Surface& s, int value, uint32_t color);

}  // namespace sprites
