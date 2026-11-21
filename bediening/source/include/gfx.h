#pragma once

// Tiny software drawing library for the animated home screen (RGB565 surfaces, always clipped,
// no LVGL / Arduino dependency, so it is host-testable). Coordinates are inclusive pixel
// positions; colors are 0xRRGGBB. Angles follow the screen: 0 = 3 o'clock, growing clockwise.

#include <stdint.h>

namespace gfx {

struct Surface {
    uint16_t* px;
    int16_t w;
    int16_t h;
};

struct Pt {
    int16_t x;
    int16_t y;
};

uint16_t rgb565(uint32_t rgb);
// f256 = 0 -> a, 256 -> b
uint32_t mix(uint32_t a, uint32_t b, int f256);

void fill(const Surface& s, uint32_t rgb);
void plot(const Surface& s, int x, int y, uint32_t rgb);
void rect(const Surface& s, int x0, int y0, int x1, int y1, uint32_t rgb);
void rectOutline(const Surface& s, int x0, int y0, int x1, int y1, uint32_t rgb);
void roundRect(const Surface& s, int x0, int y0, int x1, int y1, int r, uint32_t rgb);
void line(const Surface& s, int x0, int y0, int x1, int y1, uint32_t rgb);
// Filled ellipse inside the bounding box; only rows ya..yb are drawn (use ellipse() for all rows).
void ellipseRows(const Surface& s, int x0, int y0, int x1, int y1, int ya, int yb, uint32_t rgb);
void ellipse(const Surface& s, int x0, int y0, int x1, int y1, uint32_t rgb);
void polygon(const Surface& s, const Pt* p, int n, uint32_t rgb);
void polyOutline(const Surface& s, const Pt* p, int n, uint32_t rgb);
// Arc of a circle, `thick` pixels wide (growing inwards), from a0 to a1 degrees.
void arc(const Surface& s, int cx, int cy, int r, int a0, int a1, int thick, uint32_t rgb);
void blit(const Surface& dst, int x, int y, const Surface& src);
// Rotates src into dst (same size) around (cx, cy), nearest neighbour; uncovered pixels get bg.
void rotate(const Surface& src, const Surface& dst, float deg, float cx, float cy, uint32_t bg);
// Pixels outside a circle of radius r in each corner are set to bg (rounded corners).
void roundCorners(const Surface& s, int r, uint32_t bg);
// 5x7 glyph (digits, '?', 'A'), each pixel drawn as scale x scale. Unknown characters are skipped.
void glyph(const Surface& s, int x, int y, char c, int scale, uint32_t rgb);

}  // namespace gfx
