#pragma once

// UI palette (dark theme for the TFT) and fonts. Accent colors follow the
// Flutter app: orange = launcher, teal = switchboard, indigo = focus/accent.
// Plain constants only (no LVGL dependency).

#include <stdint.h>

namespace ui_color {
constexpr uint32_t BACKGROUND = 0x000000;
constexpr uint32_t CARD = 0x262626;  // neutral gray: any color tint seen on screen is the panel, not the palette
constexpr uint32_t TEXT = 0xFFFFFF;
constexpr uint32_t TEXT_MUTED = 0x8A94A6;
constexpr uint32_t TEXT_DIM = 0xC4CAD8;  // readable on both the dark card and the purple focus color
constexpr uint32_t LAUNCHER = 0xF28C28;
constexpr uint32_t SWITCHBOARD = 0x26A69A;
constexpr uint32_t ACCENT = 0x6C63C7;
constexpr uint32_t OK = 0x2ECC71;
constexpr uint32_t WARNING = 0xF5A623;
constexpr uint32_t ERROR = 0xE74C3C;
}  // namespace ui_color

// Fonts: see fonts.h.
