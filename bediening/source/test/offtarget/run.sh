#!/bin/bash
# Off-target checks of the home screen and of the real ui.cpp on host builds of LVGL 9.1 and 9.6.
# LVGL is built with LV_DRAW_BUF_STRIDE_ALIGN=64, stricter than this project's default (1): a canvas whose row size does
# not match the padding would shear the picture, so this run also guards that. Needs git, gcc/g++, python3 with Pillow (for the sheets).
# The first run clones and compiles LVGL (several minutes per version); later runs reuse /tmp/lvgl*.
#   test/offtarget/run.sh [outdir]      writes PPM frames to <outdir>/<lvgl>/{home,ui}/ and prints the session results
set -e
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT=${1:-/tmp/offtarget}
DEFS="-DLV_CONF_SKIP -DLV_FONT_UNSCII_8=1 -DLV_DRAW_BUF_STRIDE_ALIGN=64"
EXCLUDE="^src/(drivers|libs)/|/(nema_gfx|vg_lite|opengles|eve|dma2d|renesas|nxp|stm32|ppa|pxp|g2d|sdl|arm2d|ambiq|espressif|opengl)|/blend/(neon|helium|riscv)|/(neon|helium|riscv)"
for v in 9.1.0 9.6.0; do
  n=${v%.*}; n=lvgl${n/./}
  [ -d /tmp/$n ] || git clone -q --depth 1 --branch v$v https://github.com/lvgl/lvgl.git /tmp/$n
  LIB=/tmp/lib${n}_s64.a
  if [ ! -f $LIB ]; then
    mkdir -p /tmp/obj_${n}_s64
    ( cd /tmp/$n
      { find src/libs/bin_decoder -name '*.c' 2>/dev/null; find src -name '*.c' | grep -Ev "$EXCLUDE"; } |
      while read f; do gcc -O0 -w $DEFS -I. -c $f -o /tmp/obj_${n}_s64/$(echo $f | tr / _).o; done )
    ar rcs $LIB /tmp/obj_${n}_s64/*.o
  fi
  mkdir -p $OUT/$n/home $OUT/$n/ui
  gcc -O0 -w $DEFS -I/tmp/$n -c $ROOT/src/fonts/font_pixel_16.c -o $OUT/$n/font.o
  COMMON="$ROOT/src/ui_home.cpp $ROOT/src/home_model.cpp $ROOT/src/home_sprites.cpp $ROOT/src/gfx.cpp $ROOT/src/control_logic.cpp $ROOT/src/ble_devices.cpp $OUT/$n/font.o $LIB -lm"
  FLAGS="-std=gnu++11 -O0 -Wall -Wextra $DEFS -I$ROOT/test/offtarget/stubs -I$ROOT/include -I/tmp/$n"
  echo "== $n: home screen"
  g++ $FLAGS $ROOT/test/offtarget/main.cpp $COMMON -o $OUT/$n/run_home
  $OUT/$n/run_home $OUT/$n/home
  echo "== $n: flush consistency (panel emulation)"
  g++ $FLAGS $ROOT/test/offtarget/panel_check.cpp $COMMON -o $OUT/$n/run_panel
  $OUT/$n/run_panel
  echo "== $n: real ui.cpp session"
  g++ $FLAGS -Wno-class-memaccess $ROOT/test/offtarget/ui_main.cpp $ROOT/src/ui.cpp $COMMON -o $OUT/$n/run_ui
  $OUT/$n/run_ui $OUT/$n/ui
done
