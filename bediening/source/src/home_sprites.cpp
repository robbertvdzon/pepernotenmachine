#include "home_sprites.h"

#include <math.h>

#include "home_config.h"
#include "ui_theme.h"

using namespace gfx;

namespace sprites {

namespace {

constexpr uint32_t BG = ui_color::BACKGROUND;
constexpr uint32_t CARD = ui_color::CARD;
constexpr uint32_t TEXT = ui_color::TEXT;
constexpr uint32_t MUTED = ui_color::TEXT_MUTED;
constexpr uint32_t DIM = ui_color::TEXT_DIM;
constexpr uint32_t ORANGE = ui_color::LAUNCHER;
constexpr uint32_t TEAL = ui_color::SWITCHBOARD;
constexpr uint32_t OKC = ui_color::OK;
constexpr uint32_t WARN = ui_color::WARNING;
constexpr uint32_t ERRC = ui_color::ERROR;
constexpr uint32_t OFFGRAY = 0x5A6070;
constexpr uint32_t SMOKE = 0xE1E6F0;

constexpr int CX = 38;  // horizontal centre of the band animation

inline int ri(float v) { return static_cast<int>(floorf(v + 0.5f)); }
inline int f256(float f) { return static_cast<int>(f * 256.0f); }

void puff(const Surface& s, float x, float y, float r, float alpha) {
    if (alpha <= 0.02f) return;
    const uint32_t c = mix(CARD, SMOKE, f256(alpha));
    ellipse(s, ri(x - r), ri(y - r), ri(x + r), ri(y + r), c);
}

// ------------------------------------------------------------------ band pieces
void rocket(const Surface& s, int cx, int top) {
    const uint32_t body = 0xE8ECF4, red = 0xE74C3C, win = 0x3C8CDC;
    const Pt hull[] = {{(int16_t)cx, (int16_t)top}, {(int16_t)(cx + 8), (int16_t)(top + 12)}, {(int16_t)(cx + 9), (int16_t)(top + 30)},
                       {(int16_t)(cx - 9), (int16_t)(top + 30)}, {(int16_t)(cx - 8), (int16_t)(top + 12)}};
    polygon(s, hull, 5, body);
    const Pt nose[] = {{(int16_t)cx, (int16_t)top}, {(int16_t)(cx + 6), (int16_t)(top + 9)}, {(int16_t)(cx - 6), (int16_t)(top + 9)}};
    polygon(s, nose, 3, red);
    const Pt finL[] = {{(int16_t)(cx - 9), (int16_t)(top + 22)}, {(int16_t)(cx - 16), (int16_t)(top + 34)}, {(int16_t)(cx - 9), (int16_t)(top + 32)}};
    const Pt finR[] = {{(int16_t)(cx + 9), (int16_t)(top + 22)}, {(int16_t)(cx + 16), (int16_t)(top + 34)}, {(int16_t)(cx + 9), (int16_t)(top + 32)}};
    polygon(s, finL, 3, red);
    polygon(s, finR, 3, red);
    ellipse(s, cx - 4, top + 12, cx + 4, top + 20, 0x96AAC8);
    ellipse(s, cx - 3, top + 13, cx + 3, top + 19, win);
    rect(s, cx - 4, top + 30, cx + 4, top + 34, 0x6E737D);
    plot(s, cx - 3, top + 14, 0xBEE1FF);
    plot(s, cx - 2, top + 15, 0xBEE1FF);
}

void pad(const Surface& s, int cx, int y) {
    rect(s, cx - 20, y, cx + 20, y + 4, 0x78828A);
    rect(s, cx - 14, y + 5, cx - 11, y + 8, 0x50555F);
    rect(s, cx + 11, y + 5, cx + 14, y + 8, 0x50555F);
}

void chevrons(const Surface& s, float t, int dir, uint32_t color) {
    const int spacing = 22;
    const float off = fmodf(t * 34.0f, static_cast<float>(spacing));
    for (int k = -1; k < 4; k++) {
        const float y0 = 4 + k * spacing + (dir > 0 ? off : -off) + (dir > 0 ? 0 : spacing);
        float ys[6] = {0, 18, 0, 8, 26, 8};
        const int xs[6] = {CX - 18, CX, CX + 18, CX + 18, CX, CX - 18};
        const float f = 1.0f - fabsf(y0 + 13 - 39) / 36.0f;
        if (f <= 0.05f) continue;
        Pt p[6];
        for (int i = 0; i < 6; i++) {
            const float y = dir < 0 ? 26 - ys[i] : ys[i];
            p[i].x = static_cast<int16_t>(xs[i]);
            p[i].y = static_cast<int16_t>(ri(y + y0));
        }
        polygon(s, p, 6, mix(CARD, color, f256(f)));
    }
}

void lockAnim(const Surface& s, float t, uint32_t color) {
    const float u = fmodf(t, 1.7f);
    float lift = 0;
    if (u < 0.5f) lift = 9;
    else if (u < 0.85f) lift = 9 * (1 - (u - 0.5f) / 0.35f);
    const bool click = u >= 0.85f && u < 1.15f;
    const uint32_t body = click ? mix(color, TEXT, 128) : color;
    const int top = ri(14 - lift);
    rect(s, CX - 9, top, CX - 5, 40, DIM);
    rect(s, CX + 5, top, CX + 9, ri(40 - (lift > 0 ? lift * 1.2f : 0)), DIM);
    rect(s, CX - 9, top, CX + 9, top + 4, DIM);
    rect(s, CX - 14, 38, CX + 14, 62, body);
    rect(s, CX - 2, 46, CX + 2, 51, CARD);
    rect(s, CX - 1, 51, CX + 1, 56, CARD);
    if (click) {
        for (int ang = 0; ang < 360; ang += 45) {
            const float a = ang * 0.01745329f, r1 = 26 + (u - 0.85f) * 12;
            line(s, CX + ri(20 * cosf(a)), 50 + ri(20 * sinf(a)), CX + ri(r1 * cosf(a)), 50 + ri(r1 * sinf(a)), WARN);
        }
    }
}

void spinner(const Surface& s, float t, uint32_t color) {
    for (int i = 0; i < 8; i++) {
        const float a = (i * 45 - 90) * 0.01745329f;
        const int x = CX + ri(18 * cosf(a)), y = 39 + ri(18 * sinf(a));
        const float f = static_cast<float>((((int)(t * 10) - i) % 8 + 8) % 8) / 8.0f;
        rect(s, x - 3, y - 3, x + 3, y + 3, mix(CARD, color, f256(1 - f)));
    }
}

void offlineAnim(const Surface& s, float t) {
    rect(s, CX - 1, 30, CX + 1, 56, OFFGRAY);
    const Pt tri[] = {{(int16_t)(CX - 10), 60}, {(int16_t)CX, 36}, {(int16_t)(CX + 10), 60}};
    polyOutline(s, tri, 3, OFFGRAY);
    ellipse(s, CX - 4, 24, CX + 4, 32, OFFGRAY);
    for (int k = 0; k < 3; k++) {
        const float p = fmodf(t / 1.5f + k / 3.0f, 1.0f);
        arc(s, CX, 28, ri(8 + p * 22), 200, 340, 2, mix(CARD, MUTED, f256(1 - p)));
    }
}

void errorAnim(const Surface& s, float t) {
    const uint32_t col = (static_cast<int>(t * 3) % 2 == 0) ? ERRC : mix(CARD, ERRC, 90);
    for (int k = 0; k < 3; k++) {
        const Pt tri[] = {{(int16_t)CX, (int16_t)(8 + 5 * k)}, {(int16_t)(CX + 28 - 4 * k), (int16_t)(58 - 3 * k)}, {(int16_t)(CX - 28 + 4 * k), (int16_t)(58 - 3 * k)}};
        polyOutline(s, tri, 3, col);
    }
    rect(s, CX - 2, 26, CX + 2, 42, col);
    rect(s, CX - 2, 46, CX + 2, 50, col);
}

void readyAnim(const Surface& s, float t) {
    pad(s, CX, 58);
    rocket(s, CX, 16);
    for (int i = 0; i < 8; i++) {
        const float p = fmodf(t / 1.9f + i / 8.0f, 1.0f);
        const int side = (i % 2) ? -1 : 1;
        puff(s, CX + side * (3 + p * 20 + (i * 5) % 4), 56 - p * 15 - (i * 3) % 3, 2 + p * 4, powf(1 - p, 0.6f));
    }
}

void launchedAnim(const Surface& s, float t) {
    const float a = 170.0f;
    const float top = 16 - 0.5f * a * t * t;
    pad(s, CX, 58);
    for (int i = 0; i < 6; i++) {
        const int side = (i % 2) ? -1 : 1;
        const float p = t / 1.4f > 1 ? 1 : t / 1.4f;
        puff(s, CX + side * (4 + p * 24 + i * 2), 56 - (i % 3) * 3, 3 + p * 5, powf(1 - p, 0.6f));
    }
    for (int k = 1; k < 10; k++) {
        const float tk = t - 0.055f * k;
        if (tk < 0) continue;
        const float yk = 16 - 0.5f * a * tk * tk + 36;
        puff(s, CX + (k % 3 - 1) * 2, yk, 2 + k * 0.7f, 0.85f - k * 0.08f);
    }
    if (top + 34 > -4) {
        const int fl = 8 + (static_cast<int>(t * 24) % 3) * 3;
        const int ty = ri(top) + 34;
        const Pt outer[] = {{(int16_t)(CX - 5), (int16_t)ty}, {(int16_t)(CX + 5), (int16_t)ty}, {(int16_t)CX, (int16_t)(ty + fl + 6)}};
        const Pt inner[] = {{(int16_t)(CX - 3), (int16_t)ty}, {(int16_t)(CX + 3), (int16_t)ty}, {(int16_t)CX, (int16_t)(ty + fl)}};
        polygon(s, outer, 3, 0xE74C3C);
        polygon(s, inner, 3, 0xFAD250);
        rocket(s, CX, ri(top));
    }
}

void rearmAnim(const Surface& s, int secsLeft) {
    pad(s, CX, 58);
    for (int i = 0; i < 4; i++) {
        rect(s, CX - 17 + i * 10, 69, CX - 11 + i * 10, 73, i < secsLeft ? WARN : mix(CARD, WARN, 51));
    }
    glyph(s, CX - 12, 15, static_cast<char>('0' + (secsLeft < 0 ? 0 : secsLeft > 9 ? 9 : secsLeft)), 5, WARN);
}

void checkAnim(const Surface& s, float t) {
    const float f = 0.55f + 0.45f * (0.5f + 0.5f * sinf(t * 5));
    glyph(s, CX - 12, 14, '?', 5, mix(CARD, WARN, f256(f)));
}

}  // namespace

uint32_t bandAccent(BandState st, uint32_t t_ms) {
    switch (st) {
        case BandState::Offline: return OFFGRAY;
        case BandState::Error: return ERRC;
        case BandState::Ready:
        case BandState::Launched: return OKC;
        case BandState::Rearm: return ((t_ms / 250) % 2 == 0) ? WARN : ERRC;  // stand clear: alternate with red
        default: return WARN;
    }
}

void drawBand(const Surface& s, BandState st, uint32_t t_ms, int progress, int secsLeft) {
    fill(s, BG);
    roundRect(s, 0, 0, s.w - 1, s.h - 1, 6, CARD);
    const float t = t_ms / 1000.0f;
    const uint32_t accent = bandAccent(st, t_ms);
    switch (st) {
        case BandState::Offline: offlineAnim(s, t); break;
        case BandState::Check: checkAnim(s, t); break;
        case BandState::Error: errorAnim(s, t); break;
        case BandState::Starting: spinner(s, t, accent); break;
        case BandState::Down: chevrons(s, t, +1, accent); break;
        case BandState::Up: chevrons(s, t, -1, accent); break;
        case BandState::Lock: lockAnim(s, t, accent); break;
        case BandState::Ready: readyAnim(s, t); break;
        case BandState::Launched: launchedAnim(s, t); break;
        case BandState::Rearm: rearmAnim(s, secsLeft); break;
    }
    rect(s, 0, 0, 3, s.h - 1, accent);  // left stripe, like the other cards
    if (st == BandState::Starting || st == BandState::Down || st == BandState::Lock || st == BandState::Up) {
        rectOutline(s, HOME_TEXT_X, 60, 150, 64, MUTED);
        const int w = progress <= 0 ? 0 : (progress >= 1000 ? 78 : progress * 78 / 1000);
        if (w > 0) rect(s, HOME_TEXT_X + 1, 61, HOME_TEXT_X + w, 63, accent);
    }
    roundCorners(s, 6, BG);
}

// ------------------------------------------------------------------ tiles
namespace {

struct IconMode {
    uint32_t color;
    bool anim;
};

// The universal "fog" pictogram: a cloud with dashes below it that drift sideways while fogging.
void iconFog(const Surface& s, float t, const IconMode& m) {
    const uint32_t c = m.color;
    ellipse(s, 2, 3, 7, 8, c);
    ellipse(s, 5, 1, 11, 8, c);
    ellipse(s, 9, 3, 14, 8, c);
    rect(s, 4, 5, 12, 8, c);
    const int off = m.anim ? static_cast<int>(t * 6) % 6 : 0;  // drift: whole pattern shifts, wrapping around
    const int xs[3] = {1, 6, 3}, ws[3] = {8, 8, 8};
    for (int row = 0; row < 3; row++) {
        const int y = 10 + row * 2;
        const int x0 = ((xs[row] + (row % 2 ? -off : off)) % 16 + 16) % 16;
        for (int k = 0; k < ws[row]; k++) plot(s, (x0 + k) % 16, y, c);
    }
}

void iconPanic(const Surface& s, float t, const IconMode& m) {
    const uint32_t c = m.color;
    uint16_t buf[16 * 16], out[16 * 16];
    const Surface bell = {buf, 16, 16}, rot = {out, 16, 16};
    fill(bell, CARD);
    rect(bell, 7, 0, 8, 1, c);
    ellipseRows(bell, 3, 1, 12, 15, 1, 8, c);
    rect(bell, 3, 8, 12, 10, c);
    rect(bell, 1, 10, 14, 12, c);
    rect(bell, 7, 13, 8, 14, c);
    const float ang = m.anim ? sinf(t * 9) * 22.0f : 0.0f;
    rotate(bell, rot, ang, 8, 1, CARD);
    blit(s, 0, 0, rot);
}

void iconFlash(const Surface& s, float t, const IconMode& m) {
    uint32_t c = m.color;
    const bool alt = m.anim && (static_cast<int>(t * 4) % 2 == 1);
    const uint32_t dome = alt ? mix(c, TEXT, 110) : c;  // the beacon pulses
    rect(s, 2, 12, 13, 14, c);
    ellipseRows(s, 3, 3, 12, 21, 3, 12, dome);
    line(s, 6, 6, 6, 10, CARD);
    if (m.anim) {
        if (alt) {
            line(s, 0, 8, 1, 8, c); line(s, 14, 8, 15, 8, c); line(s, 2, 3, 3, 4, c); line(s, 12, 3, 13, 4, c);
        } else {
            line(s, 0, 5, 1, 6, c); line(s, 14, 5, 15, 6, c); line(s, 7, 0, 7, 1, c);
        }
    }
}

void iconLamp(const Surface& s, float t, const IconMode& m) {
    const uint32_t c = m.color;
    ellipse(s, 3, 2, 12, 11, c);
    rect(s, 6, 11, 9, 14, m.anim ? DIM : c);
    if (m.anim && static_cast<int>(t * 3) % 2 == 0) {
        plot(s, 0, 3, c); plot(s, 15, 3, c); plot(s, 1, 10, c); plot(s, 14, 10, c);
    }
}

void iconSiren(const Surface& s, float t, const IconMode& m) {
    const uint32_t c = m.color;
    const Pt horn[] = {{1, 6}, {5, 6}, {10, 2}, {10, 14}, {5, 10}, {1, 10}};
    polygon(s, horn, 6, c);
    if (m.anim) {
        const int n = static_cast<int>(t * 6) % 3;
        for (int k = 0; k <= n; k++) arc(s, 8 + k * 2 + 3, 8, 3 + k * 2, 300, 420, 1, c);
    }
}

void iconAuto(const Surface& s, float, const IconMode& m) {
    ellipse(s, 1, 1, 14, 14, m.color);
    ellipse(s, 2, 2, 13, 13, CARD);
    glyph(s, 5, 4, 'A', 1, m.color);
}

void iconDispenser(const Surface& s, float t, const IconMode& m) {
    const uint32_t c = m.color;
    const Pt funnel[] = {{2, 1}, {14, 1}, {9, 8}, {9, 10}, {7, 10}, {7, 8}};
    polygon(s, funnel, 6, c);
    if (m.anim) {
        for (int i = 0; i < 2; i++) {
            const float p = fmodf(t / 0.6f + i / 2.0f, 1.0f);
            const int y = 11 + ri(p * 5);
            rect(s, 7, y, 8, y + 1, mix(CARD, c, f256(1 - p * 0.6f)));
        }
    }
}

typedef void (*IconFn)(const Surface&, float, const IconMode&);
const IconFn ICONS[TILE_COUNT] = {iconFog, iconPanic, iconFlash, iconLamp, iconSiren, iconAuto, iconDispenser};

uint32_t tileOnColor(int id) { return id == TILE_DISPENSER ? ORANGE : TEAL; }

IconMode tileMode(int id, TileState st, uint32_t now_ms) {
    switch (st) {
        case TileState::Off: return {OFFGRAY, false};
        case TileState::Pending: return {((now_ms / 500) % 2 == 0) ? WARN : mix(CARD, WARN, 77), false};
        case TileState::Problem: return {((now_ms / 200) % 2 == 0) ? ERRC : mix(CARD, ERRC, 100), false};
        case TileState::Active: return {tileOnColor(id), true};
        default: return {tileOnColor(id), false};
    }
}

}  // namespace

bool tileAnimates(TileState st) { return st == TileState::Active || st == TileState::Pending || st == TileState::Problem; }

void drawTiles(const Surface& s, const TileState* tiles, uint32_t now_ms) {
    fill(s, BG);
    for (int i = 0; i < TILE_COUNT; i++) {
        const int x = HOME_TILE_X0 + i * (HOME_TILE_W + HOME_TILE_GAP);
        roundRect(s, x, 0, x + HOME_TILE_W - 1, s.h - 1, 4, CARD);
        uint16_t buf[16 * 16];
        const Surface ic = {buf, 16, 16};
        fill(ic, CARD);
        const IconMode m = tileMode(i, tiles[i], now_ms);
        ICONS[i](ic, now_ms / 1000.0f, m);
        blit(s, x + 2, 4, ic);
        if (tiles[i] == TileState::Pending || tiles[i] == TileState::Problem) {
            rect(s, x + 4, s.h - 3, x + HOME_TILE_W - 5, s.h - 2, m.color);
        }
    }
}

void drawToast(const Surface& s, ToastLevel level) {
    const uint32_t col = level == ToastLevel::Error ? ERRC : WARN;
    fill(s, BG);
    const int m = HOME_TOAST_MARGIN;
    roundRect(s, m, 0, s.w - 1 - m, s.h - 1, 4, mix(CARD, col, 90));
    rect(s, m, 3, m + 2, s.h - 4, col);
}

}  // namespace sprites
