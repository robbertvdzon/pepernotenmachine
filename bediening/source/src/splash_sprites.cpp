#include "splash_sprites.h"

#include "splash_config.h"

namespace splash_sprites {
namespace {

constexpr int ARC_STEP_MS = 260;

// The Bluetooth rune inside a badge (box of 8 x 12 px, centered on cx, top at y0), two pixels wide horizontally so it
// stays legible on a 160 px panel.
void rune(const gfx::Surface& s, int cx, int y0, uint32_t color) {
    const int top = y0, bot = y0 + 11;
    const int ya = y0 + 3, yb = y0 + 8;  // where the arms cross the spine's thirds
    const int xl = cx - 3, xr = cx + 3;
    for (int dx = 0; dx <= 1; dx++) {
        gfx::line(s, xl + dx, ya, xr + dx, yb, color);   // upper-left -> lower-right
        gfx::line(s, xr + dx, yb, cx + dx, bot, color);  // lower-right -> spine bottom
        gfx::line(s, cx + dx, bot, cx + dx, top, color); // the spine
        gfx::line(s, cx + dx, top, xr + dx, ya, color);  // spine top -> upper-right
        gfx::line(s, xr + dx, ya, xl + dx, yb, color);   // upper-right -> lower-left
    }
}

void badge(const gfx::Surface& s, int x, int y, BadgeState st, uint32_t ring, uint32_t t_ms) {
    uint32_t fill = splash_color::BADGE;
    uint32_t runeColor = splash_color::RUNE;
    switch (st) {
        case BadgeState::Waiting: {
            // triangle wave 0..256..0 over 1.2 s: the rune breathes
            const uint32_t ph = t_ms % 1200;
            const int tri = static_cast<int>(ph < 600 ? ph : 1200 - ph) * 256 / 600;
            runeColor = gfx::mix(splash_color::BADGE_DIM, splash_color::RUNE, tri);
            break;
        }
        case BadgeState::Connected: fill = splash_color::OK; break;
        case BadgeState::Problem:
            if ((t_ms / 400) % 2 == 0) fill = splash_color::ERROR;
            break;
    }
    gfx::roundRect(s, x, y, x + SPLASH_BADGE_W - 1, y + SPLASH_BADGE_H - 1, 5, ring);
    gfx::roundRect(s, x + 2, y + 2, x + SPLASH_BADGE_W - 3, y + SPLASH_BADGE_H - 3, 3, fill);
    rune(s, x + SPLASH_BADGE_W / 2 - 1, y + (SPLASH_BADGE_H - 12) / 2, runeColor);
}

}  // namespace

int arcsLit(uint32_t t_ms) { return static_cast<int>((t_ms / ARC_STEP_MS) % 4); }

void drawLinkRow(const gfx::Surface& s, BadgeState b1, BadgeState b2, uint32_t t_ms) {
    gfx::fill(s, splash_color::BACKGROUND);

    const int digitW = 5 * SPLASH_DIGIT_SCALE, digitH = 7 * SPLASH_DIGIT_SCALE;
    const int totalW = digitW + 3 + SPLASH_BADGE_W + SPLASH_ARCS_W + SPLASH_BADGE_W + 3 + digitW;
    const int x0 = (s.w - totalW) / 2;
    const int by = (s.h - SPLASH_BADGE_H) / 2;
    const int dy = (s.h - digitH) / 2;

    const int b1x = x0 + digitW + 3;
    const int b2x = b1x + SPLASH_BADGE_W + SPLASH_ARCS_W;

    gfx::glyph(s, x0, dy, '1', SPLASH_DIGIT_SCALE, splash_color::TEXT);
    gfx::glyph(s, b2x + SPLASH_BADGE_W + 3, dy, '2', SPLASH_DIGIT_SCALE, splash_color::TEXT);
    badge(s, b1x, by, b1, splash_color::LAUNCHER, t_ms);
    badge(s, b2x, by, b2, splash_color::SWITCHBOARD, t_ms);

    // signal arcs ")))" between the badges
    const bool bothOk = b1 == BadgeState::Connected && b2 == BadgeState::Connected;
    const bool problem = b1 == BadgeState::Problem || b2 == BadgeState::Problem;
    const int lit = bothOk ? 3 : (problem ? 0 : arcsLit(t_ms));
    const int cx = b1x + SPLASH_BADGE_W + SPLASH_ARC_CX, cy = s.h / 2;
    static const int radii[3] = {6, 10, 14};
    for (int i = 0; i < 3; i++) {
        const uint32_t c = i < lit ? (bothOk ? splash_color::OK : splash_color::ARC_ON) : splash_color::ARC_OFF;
        gfx::arc(s, cx, cy, radii[i], -36, 36, 2, c);
    }
}

}  // namespace splash_sprites
