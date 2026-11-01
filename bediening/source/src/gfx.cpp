#include "gfx.h"

#include <math.h>

namespace gfx {

namespace {

inline int imin(int a, int b) { return a < b ? a : b; }
inline int imax(int a, int b) { return a > b ? a : b; }

// 5x7 glyphs, one byte per row, bit 4 = leftmost pixel.
struct Glyph {
    char c;
    uint8_t rows[7];
};
const Glyph GLYPHS[] = {
    {'0', {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E}}, {'1', {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E}},
    {'2', {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F}}, {'3', {0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E}},
    {'4', {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02}}, {'5', {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E}},
    {'6', {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E}}, {'7', {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08}},
    {'8', {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E}}, {'9', {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C}},
    {'?', {0x0E, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04}}, {'A', {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}},
};

}  // namespace

uint16_t rgb565(uint32_t rgb) {
    const uint32_t r = (rgb >> 16) & 0xFF, g = (rgb >> 8) & 0xFF, b = rgb & 0xFF;
    return static_cast<uint16_t>(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}

uint32_t mix(uint32_t a, uint32_t b, int f256) {
    f256 = imax(0, imin(256, f256));
    uint32_t out = 0;
    for (int sh = 16; sh >= 0; sh -= 8) {
        const int ca = (a >> sh) & 0xFF, cb = (b >> sh) & 0xFF;
        out |= static_cast<uint32_t>(ca + ((cb - ca) * f256) / 256) << sh;
    }
    return out;
}

void fill(const Surface& s, uint32_t rgb) {
    const uint16_t c = rgb565(rgb);
    const int n = s.w * s.h;
    for (int i = 0; i < n; i++) s.px[i] = c;
}

void plot(const Surface& s, int x, int y, uint32_t rgb) {
    if (x < 0 || y < 0 || x >= s.w || y >= s.h) return;
    s.px[y * s.w + x] = rgb565(rgb);
}

void rect(const Surface& s, int x0, int y0, int x1, int y1, uint32_t rgb) {
    if (x0 > x1) { const int t = x0; x0 = x1; x1 = t; }
    if (y0 > y1) { const int t = y0; y0 = y1; y1 = t; }
    x0 = imax(x0, 0); y0 = imax(y0, 0); x1 = imin(x1, s.w - 1); y1 = imin(y1, s.h - 1);
    const uint16_t c = rgb565(rgb);
    for (int y = y0; y <= y1; y++) {
        uint16_t* row = s.px + y * s.w;
        for (int x = x0; x <= x1; x++) row[x] = c;
    }
}

void rectOutline(const Surface& s, int x0, int y0, int x1, int y1, uint32_t rgb) {
    rect(s, x0, y0, x1, y0, rgb);
    rect(s, x0, y1, x1, y1, rgb);
    rect(s, x0, y0, x0, y1, rgb);
    rect(s, x1, y0, x1, y1, rgb);
}

void roundRect(const Surface& s, int x0, int y0, int x1, int y1, int r, uint32_t rgb) {
    for (int y = y0; y <= y1; y++) {
        int inset = 0;
        const int dy = y < y0 + r ? (y0 + r - y) : (y > y1 - r ? (y - (y1 - r)) : 0);
        if (dy > 0) {
            const float dx = sqrtf(static_cast<float>(r * r) - static_cast<float>((dy - 1) * (dy - 1)) - 0.0f);
            inset = r - static_cast<int>(dx + 0.5f);
            if (inset < 0) inset = 0;
        }
        rect(s, x0 + inset, y, x1 - inset, y, rgb);
    }
}

void line(const Surface& s, int x0, int y0, int x1, int y1, uint32_t rgb) {
    const int dx = x1 > x0 ? x1 - x0 : x0 - x1, sx = x0 < x1 ? 1 : -1;
    const int dy = y1 > y0 ? y0 - y1 : y1 - y0, sy = y0 < y1 ? 1 : -1;  // dy is negative
    int err = dx + dy;
    for (int guard = 0; guard < 1000; guard++) {
        plot(s, x0, y0, rgb);
        if (x0 == x1 && y0 == y1) break;
        const int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void ellipseRows(const Surface& s, int x0, int y0, int x1, int y1, int ya, int yb, uint32_t rgb) {
    const float cx = (x0 + x1) / 2.0f, cy = (y0 + y1) / 2.0f;
    const float rx = (x1 - x0 + 1) / 2.0f, ry = (y1 - y0 + 1) / 2.0f;
    for (int y = imax(y0, ya); y <= imin(y1, yb); y++) {
        const float v = (y - cy) / ry;
        const float k = 1.0f - v * v;
        if (k < 0.0f) continue;
        const float half = rx * sqrtf(k);
        rect(s, static_cast<int>(floorf(cx - half + 0.5f)), y, static_cast<int>(ceilf(cx + half - 0.5f)), y, rgb);
    }
}

void ellipse(const Surface& s, int x0, int y0, int x1, int y1, uint32_t rgb) {
    ellipseRows(s, x0, y0, x1, y1, y0, y1, rgb);
}

void polygon(const Surface& s, const Pt* p, int n, uint32_t rgb) {
    if (n < 3 || n > 16) return;
    int ymin = p[0].y, ymax = p[0].y;
    for (int i = 1; i < n; i++) { ymin = imin(ymin, p[i].y); ymax = imax(ymax, p[i].y); }
    ymin = imax(ymin, 0);
    ymax = imin(ymax, s.h - 1);
    for (int y = ymin; y <= ymax; y++) {
        const float yc = y + 0.5f;
        float xs[16];
        int cnt = 0;
        for (int i = 0; i < n; i++) {
            const Pt a = p[i], b = p[(i + 1) % n];
            if (a.y == b.y) continue;
            const float ya = a.y, yb = b.y;
            if ((yc >= ya && yc < yb) || (yc >= yb && yc < ya)) {
                xs[cnt++] = a.x + (yc - ya) * (b.x - a.x) / (yb - ya);
            }
        }
        for (int i = 1; i < cnt; i++) {  // insertion sort, cnt is tiny
            const float v = xs[i];
            int j = i - 1;
            while (j >= 0 && xs[j] > v) { xs[j + 1] = xs[j]; j--; }
            xs[j + 1] = v;
        }
        for (int i = 0; i + 1 < cnt; i += 2) {
            rect(s, static_cast<int>(floorf(xs[i] + 0.5f)), y, static_cast<int>(floorf(xs[i + 1] + 0.5f)) - 1, y, rgb);
        }
    }
}

void polyOutline(const Surface& s, const Pt* p, int n, uint32_t rgb) {
    for (int i = 0; i < n; i++) line(s, p[i].x, p[i].y, p[(i + 1) % n].x, p[(i + 1) % n].y, rgb);
}

void arc(const Surface& s, int cx, int cy, int r, int a0, int a1, int thick, uint32_t rgb) {
    if (r <= 0) return;
    const float step = 0.5f / static_cast<float>(r);  // sub-pixel steps: no gaps
    const float f0 = a0 * 0.01745329f, f1 = a1 * 0.01745329f;
    for (float a = f0; a <= f1; a += step) {
        for (int k = 0; k < thick; k++) {
            const float rr = static_cast<float>(r - k);
            plot(s, cx + static_cast<int>(floorf(rr * cosf(a) + 0.5f)), cy + static_cast<int>(floorf(rr * sinf(a) + 0.5f)), rgb);
        }
    }
}

void blit(const Surface& dst, int x, int y, const Surface& src) {
    for (int sy = 0; sy < src.h; sy++) {
        const int dy = y + sy;
        if (dy < 0 || dy >= dst.h) continue;
        for (int sx = 0; sx < src.w; sx++) {
            const int dx = x + sx;
            if (dx < 0 || dx >= dst.w) continue;
            dst.px[dy * dst.w + dx] = src.px[sy * src.w + sx];
        }
    }
}

void rotate(const Surface& src, const Surface& dst, float deg, float cx, float cy, uint32_t bg) {
    const float a = deg * 0.01745329f, ca = cosf(a), sa = sinf(a);
    const uint16_t bgc = rgb565(bg);
    for (int y = 0; y < dst.h && y < src.h; y++) {
        for (int x = 0; x < dst.w && x < src.w; x++) {
            const float px = x + 0.5f - cx, py = y + 0.5f - cy;
            // inverse rotation (counter-clockwise on screen for positive deg, like image libraries)
            const int sx = static_cast<int>(floorf(cx + px * ca - py * sa));
            const int sy = static_cast<int>(floorf(cy + px * sa + py * ca));
            dst.px[y * dst.w + x] = (sx >= 0 && sy >= 0 && sx < src.w && sy < src.h) ? src.px[sy * src.w + sx] : bgc;
        }
    }
}

void roundCorners(const Surface& s, int r, uint32_t bg) {
    for (int i = 0; i < r; i++) {
        const float dx = sqrtf(static_cast<float>(r * r) - static_cast<float>((r - i - 1) * (r - i - 1)));
        const int cut = r - static_cast<int>(dx + 0.5f);
        if (cut <= 0) continue;
        rect(s, 0, i, cut - 1, i, bg);
        rect(s, s.w - cut, i, s.w - 1, i, bg);
        rect(s, 0, s.h - 1 - i, cut - 1, s.h - 1 - i, bg);
        rect(s, s.w - cut, s.h - 1 - i, s.w - 1, s.h - 1 - i, bg);
    }
}

void glyph(const Surface& s, int x, int y, char c, int scale, uint32_t rgb) {
    for (size_t g = 0; g < sizeof(GLYPHS) / sizeof(GLYPHS[0]); g++) {
        if (GLYPHS[g].c != c) continue;
        for (int row = 0; row < 7; row++) {
            for (int col = 0; col < 5; col++) {
                if (GLYPHS[g].rows[row] & (0x10 >> col)) {
                    rect(s, x + col * scale, y + row * scale, x + col * scale + scale - 1, y + row * scale + scale - 1, rgb);
                }
            }
        }
        return;
    }
}

}  // namespace gfx
