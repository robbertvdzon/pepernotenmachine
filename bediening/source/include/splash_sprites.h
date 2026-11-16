#pragma once

// Pixel art of the start-up splash: the link row "1 [Bluetooth badge] ))) [Bluetooth badge] 2" with its animation.
// Pure drawing into an RGB565 surface (SPLASH_ROW_W x SPLASH_ROW_H), no LVGL. Text (title, "Connecting...") is LVGL labels.

#include <stdint.h>

#include "gfx.h"
#include "splash_model.h"

namespace splash_sprites {

// `t_ms` = time since the splash began. Waiting badges pulse and the arcs travel from badge 1 to badge 2; a connected badge is
// green; a badge with a problem blinks red and the arcs stop. When both badges are connected the arcs are solid green.
void drawLinkRow(const gfx::Surface& s, BadgeState b1, BadgeState b2, uint32_t t_ms);

// Number of arcs lit at that time (0..3) while waiting; used for drawing and for the host test.
int arcsLit(uint32_t t_ms);

}  // namespace splash_sprites
