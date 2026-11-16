#pragma once

// Start-up splash screen: timings, layout and colors. Plain constants only (host-testable).
// What the splash does is in splash_model.h, the link row pixel art in splash_sprites.h, the LVGL screen in ui_splash.cpp.

#include <stdint.h>

#include "splash_art.h"

// ---- behavior ----
constexpr uint32_t SPLASH_MIN_MS = 2000;           // visible at least this long, even when both devices connect sooner
constexpr uint32_t SPLASH_CONNECTED_HOLD_MS = 500; // after the LAST device connected, show both badges "connected" this long
constexpr uint32_t SPLASH_CONTINUE_AFTER_MS = 20000;  // a device is still missing: offer "push = continue"
constexpr uint32_t SPLASH_FRAME_MS = 83;           // 12 fps, like the home screen band

// ---- layout of the 160x128 screen (px) ----
// The artwork (generated, splash_art.h) takes the top SPLASH_ART_H rows; everything else sits below it on a background of the
// picture's own bottom-row color. The title overlaps the bottom of the picture a little (its tall letters reach into it) so that
// the connection row and the status line still fit: tune with the constants below, check with test/offtarget (frames).
constexpr int SPLASH_W = 160, SPLASH_H = 128;
constexpr int SPLASH_TITLE_Y = 61;       // top of the title label (font line = 26 px; letters' ink starts ~3 px lower)
constexpr int SPLASH_SUB_Y = 80;         // top of the subtitle label (font line = 21 px; caps ink ~6..18 px lower)
constexpr int SPLASH_ROW_Y = 99;         // top of the link row canvas (badge 1, arcs, badge 2)
constexpr int SPLASH_ROW_W = 96;         // canvas width: only the part that animates is redrawn and sent to the panel
constexpr int SPLASH_ROW_H = 20;         // canvas height
constexpr int SPLASH_VERSION_MARGIN = 2; // version text, small font, bottom right
// "push = continue" is not part of the block above: it appears at the very top of the screen, over the picture, on a
// semi-transparent patch of the splash background (so it stays readable on top of the artwork).
constexpr int SPLASH_CONTINUE_Y = 2;

// The title is drawn five times (an outline of four offsets + the colored letters on top): keep every label inside the screen.
constexpr int SPLASH_LABEL_X = 1, SPLASH_LABEL_W = 158;
constexpr int SPLASH_TITLE_LETTER_SPACE = 1;  // px between title letters: the outline needs air or the letters merge

// Link row: [1] (badge) )))  (badge) [2]
constexpr int SPLASH_BADGE_W = 16, SPLASH_BADGE_H = 18;
constexpr int SPLASH_DIGIT_SCALE = 2;  // 5x7 glyph -> 10x14
constexpr int SPLASH_ARCS_W = 26;      // space between the two badges for the signal arcs
constexpr int SPLASH_ARC_CX = 3;      // arc centre, measured from the right edge of badge 1: puts the arcs in the middle of that gap (host test checks)
constexpr int SPLASH_ROW_CONTENT_W = 2 * 5 * SPLASH_DIGIT_SCALE + 2 * 3 + 2 * SPLASH_BADGE_W + SPLASH_ARCS_W;  // digits, gaps, badges, arcs
static_assert(SPLASH_ROW_CONTENT_W + 4 <= SPLASH_ROW_W, "the link row must fit its canvas with a little air");

namespace splash_color {
constexpr uint32_t BACKGROUND = SPLASH_ART_BG_HEX;  // the artwork's bottom row color: no seam under the picture
constexpr uint32_t TITLE = 0xC62828;
constexpr uint32_t TITLE_OUTLINE = 0x2A0E0E;
constexpr uint32_t TEXT = 0x101010;
constexpr uint32_t TEXT_MUTED = 0x3A464E;
constexpr uint32_t BADGE = 0x101010;
constexpr uint32_t BADGE_DIM = 0x505050;    // rune at the dim end of the pulse
constexpr uint32_t RUNE = 0xFFFFFF;
constexpr uint32_t ARC_OFF = 0x7F919B;
constexpr uint32_t ARC_ON = 0x101010;
constexpr uint32_t LAUNCHER = 0xF28C28;     // badge 1 (same orange as the rest of the UI)
constexpr uint32_t SWITCHBOARD = 0x26A69A;  // badge 2 (same teal)
constexpr uint32_t OK = 0x1FA85A;
constexpr uint32_t ERROR = 0xE74C3C;
}  // namespace splash_color

static_assert(SPLASH_MIN_MS >= 1000, "the splash must be readable");
static_assert(SPLASH_CONTINUE_AFTER_MS > SPLASH_MIN_MS, "the 'continue' offer comes after the minimum time");
static_assert(SPLASH_ROW_W <= SPLASH_W && (SPLASH_W - SPLASH_ROW_W) % 2 == 0, "the link row canvas is centered");
static_assert(SPLASH_TITLE_Y >= 1 && SPLASH_ROW_Y + SPLASH_ROW_H <= SPLASH_H - 8, "the link row leaves room for the version text");
static_assert(SPLASH_TITLE_Y + 26 <= SPLASH_SUB_Y + 8 && SPLASH_SUB_Y + 21 <= SPLASH_ROW_Y + 4, "title / subtitle / row must follow each other");
static_assert(SPLASH_ART_W == SPLASH_W && SPLASH_ART_H <= SPLASH_TITLE_Y + 20, "the picture is full width and the title only overlaps it a little");
static_assert(SPLASH_LABEL_X >= 1 && SPLASH_LABEL_X + SPLASH_LABEL_W + 1 <= SPLASH_W, "outline copies must stay on the screen");
