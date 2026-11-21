#!/bin/sh
# Regenerates the two splash-screen fonts (1-bit, no anti-aliasing, like the other UI fonts):
#   src/fonts/font_splash_title.c   Passion One Bold 26 px   (title "nut-o-matic" / "noot-o-maat")
#   src/fonts/font_splash_sub.c     Barlow Condensed Bold 17 px   (subtitle, caps)
# Both fonts are SIL Open Font License 1.1 (free to embed). Fetch them first, e.g.:
#   B=https://raw.githubusercontent.com/google/fonts/main/ofl
#   curl -LO $B/passionone/PassionOne-Bold.ttf
#   curl -LO $B/barlowcondensed/BarlowCondensed-Bold.ttf
# Requires: npm install lv_font_conv
# Usage: tools/make_splash_fonts.sh [dir with the two .ttf files, default .]
set -e
DIR="${1:-.}"
OUT="$(dirname "$0")/../src/fonts"
# ASCII + all of Latin-1, so the titles can be changed or translated without regenerating.
RANGES="-r 0x20-0x7E -r 0xA0-0xFF"
npx lv_font_conv --font "$DIR/PassionOne-Bold.ttf" --size 26 --bpp 1 --no-compress --no-prefilter \
  --format lvgl --lv-include lvgl.h --lv-font-name font_splash_title $RANGES -o "$OUT/font_splash_title.c"
npx lv_font_conv --font "$DIR/BarlowCondensed-Bold.ttf" --size 17 --bpp 1 --no-compress --no-prefilter \
  --format lvgl --lv-include lvgl.h --lv-font-name font_splash_sub $RANGES -o "$OUT/font_splash_sub.c"
