#!/bin/sh
# Regenerates src/fonts/font_pixel_16.c: a true 8x16, 1-bit (no anti-aliasing)
# LVGL font from Unscii (public domain, https://github.com/viznut/unscii).
# Requires: npm install lv_font_conv   and   unscii-16.ttf from the unscii repo (fontfiles/).
# Glyphs: ASCII, degree, middle dot, multiply, arrows, bullet, circles/triangles, check mark, blocks.
set -e
UNSCII_TTF="${1:-unscii-16.ttf}"
npx lv_font_conv --font "$UNSCII_TTF" --size 16 --bpp 1 --no-compress --no-prefilter \
  --format lvgl --lv-include lvgl.h --lv-font-name font_pixel_16 \
  -r 0x20-0x7E -r 0xB0,0xB7,0xD7 -r 0x2190-0x2193 -r 0x2022 \
  -r 0x25CF,0x25CB,0x25B2,0x25BC,0x25B6,0x25C0 -r 0x2713 -r 0x2588,0x2591 \
  -o src/fonts/font_pixel_16.c
