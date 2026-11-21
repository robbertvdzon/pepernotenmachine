/*******************************************************************************
 * Size: 17 px
 * Bpp: 1
 * Opts: --font /tmp/lvc/BarlowCondensed-Bold.ttf --size 17 --bpp 1 --no-compress --no-prefilter --format lvgl --lv-include lvgl.h --lv-font-name font_splash_sub -r 0x20-0x7E -r 0xA0-0xFF -o tools/../src/fonts/font_splash_sub.c
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl.h"
#endif

#ifndef FONT_SPLASH_SUB
#define FONT_SPLASH_SUB 1
#endif

#if FONT_SPLASH_SUB

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */
    0x0,

    /* U+0021 "!" */
    0xff, 0xff, 0xf,

    /* U+0022 "\"" */
    0xde, 0xf7, 0xb0,

    /* U+0023 "#" */
    0x33, 0x99, 0xcc, 0xff, 0xff, 0xfb, 0xb9, 0xdd,
    0xff, 0xff, 0xf3, 0x39, 0x9c, 0xc0,

    /* U+0024 "$" */
    0x30, 0xef, 0xf3, 0xcf, 0x8f, 0x1e, 0x3c, 0x7c,
    0xf3, 0xfd, 0xe3, 0x0,

    /* U+0025 "%" */
    0x78, 0x8f, 0xd8, 0xcd, 0x8c, 0xf0, 0x7b, 0x0,
    0x60, 0x7, 0xe0, 0xff, 0xf, 0x31, 0xb3, 0x1b,
    0x33, 0x1e,

    /* U+0026 "&" */
    0x3c, 0x3f, 0x19, 0x8c, 0xc7, 0xe1, 0xe1, 0xe1,
    0xfb, 0xcf, 0xe3, 0xbf, 0xcf, 0xf0,

    /* U+0027 "'" */
    0xff,

    /* U+0028 "(" */
    0x66, 0x6c, 0xcc, 0xcc, 0xcc, 0xc6, 0x66,

    /* U+0029 ")" */
    0x66, 0x63, 0x33, 0x33, 0x33, 0x36, 0x66,

    /* U+002A "*" */
    0x32, 0xdf, 0xde, 0xfe, 0xd3, 0x0,

    /* U+002B "+" */
    0x30, 0x63, 0xff, 0xf3, 0x6, 0x0,

    /* U+002C "," */
    0x6d, 0x20,

    /* U+002D "-" */
    0xff, 0xc0,

    /* U+002E "." */
    0xf0,

    /* U+002F "/" */
    0xc, 0x31, 0xc6, 0x18, 0xe3, 0xc, 0x71, 0x86,
    0x38,

    /* U+0030 "0" */
    0x7b, 0xfc, 0xf3, 0xcf, 0x3c, 0xf3, 0xcf, 0x3f,
    0xde,

    /* U+0031 "1" */
    0x7f, 0x33, 0x33, 0x33, 0x33, 0x33,

    /* U+0032 "2" */
    0x7b, 0xfc, 0xf3, 0xc, 0x71, 0x8c, 0x73, 0x8f,
    0xff,

    /* U+0033 "3" */
    0xff, 0xf1, 0xce, 0x70, 0xe0, 0xc3, 0xcf, 0x3f,
    0xde,

    /* U+0034 "4" */
    0x38, 0x38, 0x38, 0x70, 0x70, 0x76, 0xe6, 0xff,
    0xff, 0x6, 0x6, 0x6,

    /* U+0035 "5" */
    0xff, 0xfc, 0x30, 0xd3, 0xff, 0xc3, 0xf, 0x3f,
    0xde,

    /* U+0036 "6" */
    0x7b, 0xfc, 0xf0, 0xfb, 0xfc, 0xf3, 0xcf, 0x3f,
    0xde,

    /* U+0037 "7" */
    0xff, 0xff, 0x30, 0x61, 0xc3, 0x86, 0x1c, 0x38,
    0x60, 0xc3, 0x80,

    /* U+0038 "8" */
    0x7b, 0xfc, 0xf3, 0xcd, 0xef, 0xb3, 0xcf, 0x3f,
    0xde,

    /* U+0039 "9" */
    0x7b, 0xfc, 0xf3, 0xcf, 0x3f, 0xdf, 0xf, 0x3f,
    0xde,

    /* U+003A ":" */
    0xf0, 0xf,

    /* U+003B ";" */
    0xff, 0x80, 0x36, 0xd0,

    /* U+003C "<" */
    0x6, 0x3d, 0xf7, 0xe, 0xf, 0x87, 0x83,

    /* U+003D "=" */
    0xff, 0xfc, 0x7, 0xff, 0xe0,

    /* U+003E ">" */
    0x81, 0xc1, 0xf0, 0xf1, 0xef, 0xbc, 0x40,

    /* U+003F "?" */
    0x3c, 0xfb, 0x1e, 0x30, 0xe3, 0x8e, 0x18, 0x30,
    0x0, 0xc1, 0x80,

    /* U+0040 "@" */
    0xf, 0x81, 0xfe, 0x38, 0xe6, 0xf7, 0x7f, 0x3d,
    0xb3, 0xdb, 0x3d, 0x37, 0xdf, 0xee, 0xdc, 0x71,
    0x7, 0xf0, 0x1e, 0x0,

    /* U+0041 "A" */
    0x1c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3e, 0x66, 0x66,
    0x7e, 0x7e, 0xe7, 0xe7,

    /* U+0042 "B" */
    0xfb, 0xfc, 0xf3, 0xcf, 0xef, 0xb3, 0xcf, 0x3f,
    0xfe,

    /* U+0043 "C" */
    0x7d, 0xff, 0x1e, 0x3c, 0x18, 0x30, 0x60, 0xc7,
    0x8f, 0xfb, 0xc0,

    /* U+0044 "D" */
    0xfb, 0xfc, 0xf3, 0xcf, 0x3c, 0xf3, 0xcf, 0x3f,
    0xfe,

    /* U+0045 "E" */
    0xff, 0xfc, 0x30, 0xc3, 0xef, 0xb0, 0xc3, 0xf,
    0xff,

    /* U+0046 "F" */
    0xff, 0xfc, 0x30, 0xc3, 0xef, 0xb0, 0xc3, 0xc,
    0x30,

    /* U+0047 "G" */
    0x7d, 0xff, 0x1e, 0x3c, 0x19, 0xf3, 0xe3, 0xc7,
    0x8f, 0xfb, 0xc0,

    /* U+0048 "H" */
    0xcf, 0x3c, 0xf3, 0xcf, 0xff, 0xf3, 0xcf, 0x3c,
    0xf3,

    /* U+0049 "I" */
    0xff, 0xff, 0xff,

    /* U+004A "J" */
    0x6, 0xc, 0x18, 0x30, 0x60, 0xc1, 0xe3, 0xc7,
    0x8f, 0xfb, 0xe0,

    /* U+004B "K" */
    0xcf, 0x9b, 0x76, 0xcf, 0x9e, 0x3e, 0x6c, 0xdd,
    0xbb, 0x36, 0x70,

    /* U+004C "L" */
    0xc1, 0x83, 0x6, 0xc, 0x18, 0x30, 0x60, 0xc1,
    0x83, 0xff, 0xf0,

    /* U+004D "M" */
    0xe7, 0xe7, 0xff, 0xff, 0xff, 0xdb, 0xc3, 0xc3,
    0xc3, 0xc3, 0xc3, 0xc3,

    /* U+004E "N" */
    0xe7, 0xcf, 0xdf, 0xbf, 0x7f, 0xff, 0xef, 0xdf,
    0xbf, 0x3e, 0x70,

    /* U+004F "O" */
    0x7d, 0xff, 0x1e, 0x3c, 0x78, 0xf1, 0xe3, 0xc7,
    0x8f, 0xfb, 0xc0,

    /* U+0050 "P" */
    0xfd, 0xfb, 0x1e, 0x3c, 0x7f, 0xff, 0x60, 0xc1,
    0x83, 0x6, 0x0,

    /* U+0051 "Q" */
    0x7b, 0xfc, 0xf3, 0xcf, 0x3c, 0xf3, 0xcf, 0x37,
    0x8c, 0x30,

    /* U+0052 "R" */
    0xfd, 0xfb, 0x1e, 0x3c, 0x7f, 0xff, 0x6e, 0xcd,
    0x9f, 0x3e, 0x30,

    /* U+0053 "S" */
    0x7b, 0xfc, 0xf3, 0xe1, 0xc3, 0x87, 0xcf, 0x3f,
    0xde,

    /* U+0054 "T" */
    0xff, 0xfc, 0x60, 0xc1, 0x83, 0x6, 0xc, 0x18,
    0x30, 0x60, 0xc0,

    /* U+0055 "U" */
    0xc7, 0x8f, 0x1e, 0x3c, 0x78, 0xf1, 0xe3, 0xc7,
    0x8d, 0xf1, 0xc0,

    /* U+0056 "V" */
    0xe7, 0x67, 0x66, 0x66, 0x76, 0x76, 0x3e, 0x3c,
    0x3c, 0x3c, 0x1c, 0x1c,

    /* U+0057 "W" */
    0xe6, 0x6c, 0xcd, 0xb9, 0xb7, 0x76, 0xfe, 0xff,
    0x8f, 0xf1, 0xfe, 0x3b, 0xc7, 0x38, 0xe6, 0xc,
    0xc0,

    /* U+0058 "X" */
    0x67, 0x66, 0x7e, 0x3c, 0x3c, 0x1c, 0x1c, 0x3c,
    0x3c, 0x7e, 0x66, 0x67,

    /* U+0059 "Y" */
    0xcf, 0x3f, 0xde, 0x79, 0xe3, 0xc, 0x30, 0xc3,
    0xc,

    /* U+005A "Z" */
    0xff, 0xf1, 0xc6, 0x38, 0xc3, 0x1c, 0x63, 0x8f,
    0xff,

    /* U+005B "[" */
    0xff, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xff,

    /* U+005C "\\" */
    0xc3, 0xe, 0x18, 0x61, 0xc3, 0xc, 0x38, 0x61,
    0x87,

    /* U+005D "]" */
    0xff, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0xff,

    /* U+005E "^" */
    0x38, 0x70, 0xe3, 0x66, 0xcd, 0x99, 0x80,

    /* U+005F "_" */
    0xff, 0xfc,

    /* U+0060 "`" */
    0x6c,

    /* U+0061 "a" */
    0x7b, 0xfe, 0xc3, 0x7f, 0xfc, 0xff, 0x7c,

    /* U+0062 "b" */
    0xc3, 0xc, 0x3e, 0xff, 0x3c, 0xf3, 0xcf, 0x3f,
    0xfe,

    /* U+0063 "c" */
    0x7b, 0xfc, 0xf0, 0xc3, 0xc, 0xff, 0x78,

    /* U+0064 "d" */
    0xc, 0x30, 0xdf, 0xff, 0x3c, 0xf3, 0xcf, 0x3f,
    0xdf,

    /* U+0065 "e" */
    0x7b, 0xfc, 0xff, 0xff, 0xd, 0xff, 0x78,

    /* U+0066 "f" */
    0x37, 0x6f, 0xf6, 0x66, 0x66, 0x66,

    /* U+0067 "g" */
    0x7f, 0xfc, 0xf3, 0xcf, 0x3c, 0xff, 0x7c, 0x37,
    0x9e,

    /* U+0068 "h" */
    0xc3, 0xc, 0x3e, 0xff, 0x3c, 0xf3, 0xcf, 0x3c,
    0xf3,

    /* U+0069 "i" */
    0x30, 0xff, 0xff, 0xc0,

    /* U+006A "j" */
    0x3, 0x0, 0x33, 0x33, 0x33, 0x33, 0x33, 0xfe,

    /* U+006B "k" */
    0xc3, 0xc, 0x37, 0xdb, 0xef, 0x3e, 0xdb, 0x6d,
    0xf3,

    /* U+006C "l" */
    0xff, 0xff, 0xff,

    /* U+006D "m" */
    0xfb, 0xbf, 0xfc, 0xcf, 0x33, 0xcc, 0xf3, 0x3c,
    0xcf, 0x33, 0xcc, 0xc0,

    /* U+006E "n" */
    0xfb, 0xfc, 0xf3, 0xcf, 0x3c, 0xf3, 0xcc,

    /* U+006F "o" */
    0x7b, 0xfc, 0xf3, 0xcf, 0x3c, 0xff, 0x78,

    /* U+0070 "p" */
    0xfb, 0xfc, 0xf3, 0xcf, 0x3c, 0xff, 0xfb, 0xc,
    0x30,

    /* U+0071 "q" */
    0x7f, 0xfc, 0xf3, 0xcf, 0x3c, 0xff, 0x7c, 0x30,
    0xc3,

    /* U+0072 "r" */
    0xff, 0xcc, 0xcc, 0xcc, 0xc0,

    /* U+0073 "s" */
    0x7b, 0xfc, 0xf8, 0x78, 0x7c, 0xff, 0x78,

    /* U+0074 "t" */
    0x66, 0xff, 0x66, 0x66, 0x67, 0x70,

    /* U+0075 "u" */
    0xcf, 0x3c, 0xf3, 0xcf, 0x3c, 0xff, 0x7c,

    /* U+0076 "v" */
    0xef, 0xdd, 0xb3, 0x66, 0xcf, 0x8e, 0x1c, 0x38,

    /* U+0077 "w" */
    0xcc, 0xf3, 0x7e, 0xd9, 0xfe, 0x7f, 0x9f, 0xe7,
    0xf8, 0xdc, 0x33, 0x0,

    /* U+0078 "x" */
    0xee, 0xdd, 0xf1, 0xe3, 0x87, 0x9f, 0x37, 0xee,

    /* U+0079 "y" */
    0xef, 0xdd, 0xb3, 0x66, 0xcf, 0x9e, 0x1c, 0x38,
    0x63, 0xc7, 0x0,

    /* U+007A "z" */
    0xff, 0xf1, 0xce, 0x79, 0xce, 0x3f, 0xfc,

    /* U+007B "{" */
    0x3b, 0xd8, 0xc6, 0x31, 0x9c, 0xe3, 0x18, 0xc6,
    0x31, 0xe7,

    /* U+007C "|" */
    0xff, 0xff, 0xff, 0xf0,

    /* U+007D "}" */
    0xe7, 0x8c, 0x63, 0x18, 0xc7, 0x39, 0x8c, 0x63,
    0x1b, 0xdc,

    /* U+007E "~" */
    0x72, 0x7f, 0x4e,

    /* U+00A0 " " */
    0x0,

    /* U+00A1 "¡" */
    0xf0, 0xff, 0xff,

    /* U+00A2 "¢" */
    0x30, 0xef, 0xf3, 0xcf, 0xc, 0x33, 0xcf, 0xf3,
    0x8c,

    /* U+00A3 "£" */
    0x1c, 0x7e, 0x67, 0x60, 0x60, 0xfc, 0xfc, 0x60,
    0x60, 0x60, 0x7f, 0x7f,

    /* U+00A4 "¤" */
    0x41, 0x7f, 0xdf, 0xcc, 0x66, 0x33, 0x19, 0xfd,
    0xff, 0x41, 0x0,

    /* U+00A5 "¥" */
    0xcf, 0x9f, 0xb3, 0xe7, 0x87, 0x3f, 0xff, 0xff,
    0xfc, 0xc1, 0x80,

    /* U+00A6 "¦" */
    0xff, 0xf0, 0xff, 0xf0,

    /* U+00A7 "§" */
    0x7b, 0xfc, 0xf0, 0x71, 0xef, 0xf3, 0xfd, 0xe3,
    0x83, 0xcf, 0xf7, 0x80,

    /* U+00A8 "¨" */
    0xde, 0xc0,

    /* U+00A9 "©" */
    0xf, 0x81, 0xff, 0x1c, 0x1c, 0xc0, 0x6c, 0x71,
    0xe6, 0x8f, 0x30, 0x79, 0x83, 0xcd, 0x1b, 0x39,
    0x9c, 0x1c, 0x7f, 0xc0, 0xf8, 0x0,

    /* U+00AA "ª" */
    0x76, 0x87, 0xfd, 0xfc, 0x1f,

    /* U+00AB "«" */
    0x6f, 0xed, 0xb6, 0xf9, 0xb0,

    /* U+00AC "¬" */
    0xff, 0xfc, 0x18, 0x30,

    /* U+00AD "­" */
    0xff, 0xc0,

    /* U+00AE "®" */
    0x38, 0x8a, 0x6c, 0xd9, 0xa8, 0x8e, 0x0,

    /* U+00AF "¯" */
    0xff,

    /* U+00B0 "°" */
    0x7b, 0xfc, 0xf3, 0xfd, 0xe0,

    /* U+00B1 "±" */
    0x30, 0x60, 0xc7, 0xff, 0xe6, 0x0, 0x0, 0xff,
    0xfc,

    /* U+00B2 "²" */
    0x6f, 0x33, 0x6c, 0xf0,

    /* U+00B3 "³" */
    0xf3, 0x67, 0x3f, 0x60,

    /* U+00B4 "´" */
    0x68,

    /* U+00B5 "µ" */
    0xcf, 0x3c, 0xf3, 0xcf, 0x3c, 0xff, 0xff, 0xc,
    0x30,

    /* U+00B6 "¶" */
    0x3f, 0x7f, 0x7b, 0xfb, 0x7b, 0x7b, 0x7b, 0x1b,
    0x1b, 0x1b, 0x1b, 0x1b, 0x1b, 0x1b, 0x1b,

    /* U+00B7 "·" */
    0xf0,

    /* U+00B8 "¸" */
    0xfc,

    /* U+00B9 "¹" */
    0xed, 0xb6, 0xd8,

    /* U+00BA "º" */
    0x6f, 0xff, 0xff, 0xf,

    /* U+00BB "»" */
    0xd9, 0xe6, 0xdb, 0x7b, 0x60,

    /* U+00BC "¼" */
    0xe0, 0x86, 0x18, 0x61, 0x6, 0x30, 0x62, 0x6,
    0x6c, 0x6c, 0xc0, 0x98, 0x19, 0xf1, 0x1f, 0x30,
    0x32, 0x3,

    /* U+00BD "½" */
    0xe0, 0xc6, 0x18, 0x61, 0x6, 0x30, 0x62, 0x66,
    0x6f, 0x6c, 0xb0, 0xc3, 0x18, 0x61, 0x4, 0x30,
    0xf2, 0xf,

    /* U+00BE "¾" */
    0xf0, 0x47, 0x86, 0x8, 0x60, 0xe3, 0x1, 0x30,
    0x79, 0x33, 0xd9, 0x8d, 0x88, 0xc, 0xf8, 0xc7,
    0xc4, 0x6, 0x60, 0x30,

    /* U+00BF "¿" */
    0x18, 0x30, 0x0, 0xc1, 0x87, 0x1c, 0x70, 0xe7,
    0xcf, 0xf3, 0xc0,

    /* U+00C0 "À" */
    0x30, 0x18, 0x0, 0x1c, 0x3c, 0x3c, 0x3c, 0x3c,
    0x3e, 0x66, 0x66, 0x7e, 0x7e, 0xe7, 0xe7,

    /* U+00C1 "Á" */
    0xc, 0x18, 0x0, 0x1c, 0x3c, 0x3c, 0x3c, 0x3c,
    0x3e, 0x66, 0x66, 0x7e, 0x7e, 0xe7, 0xe7,

    /* U+00C2 "Â" */
    0x3c, 0x3c, 0x0, 0x1c, 0x3c, 0x3c, 0x3c, 0x3c,
    0x3e, 0x66, 0x66, 0x7e, 0x7e, 0xe7, 0xe7,

    /* U+00C3 "Ã" */
    0x30, 0x3e, 0xc, 0x0, 0x1c, 0x3c, 0x3c, 0x3c,
    0x3c, 0x3e, 0x66, 0x66, 0x7e, 0x7e, 0xe7, 0xe7,

    /* U+00C4 "Ä" */
    0x6c, 0xd8, 0x1, 0xc3, 0x87, 0x1b, 0x36, 0x6c,
    0xd9, 0xbf, 0xff, 0xf8, 0xf1, 0x80,

    /* U+00C5 "Å" */
    0x30, 0xf1, 0xa3, 0xc3, 0x0, 0xe, 0x1c, 0x38,
    0x71, 0xa3, 0x66, 0xcd, 0xbf, 0x7e, 0xcf, 0x9c,

    /* U+00C6 "Æ" */
    0x7, 0xf0, 0xff, 0xf, 0x1, 0xf0, 0x1b, 0x1,
    0xbc, 0x3b, 0xc3, 0x30, 0x7f, 0x7, 0xf0, 0x63,
    0xfe, 0x3f,

    /* U+00C7 "Ç" */
    0x7d, 0xff, 0x1e, 0x3c, 0x18, 0x30, 0x60, 0xc7,
    0x8f, 0xfb, 0xe1, 0x83, 0xe, 0x0,

    /* U+00C8 "È" */
    0x60, 0x80, 0x3f, 0xff, 0xc, 0x30, 0xfb, 0xec,
    0x30, 0xc3, 0xff, 0xc0,

    /* U+00C9 "É" */
    0x30, 0xc0, 0x3f, 0xff, 0xc, 0x30, 0xfb, 0xec,
    0x30, 0xc3, 0xff, 0xc0,

    /* U+00CA "Ê" */
    0x71, 0x60, 0x3f, 0xff, 0xc, 0x30, 0xfb, 0xec,
    0x30, 0xc3, 0xff, 0xc0,

    /* U+00CB "Ë" */
    0xff, 0xf0, 0x3f, 0xff, 0xc, 0x30, 0xf3, 0xcc,
    0x30, 0xc3, 0xff, 0xc0,

    /* U+00CC "Ì" */
    0xcc, 0x36, 0xdb, 0x6d, 0xb6, 0xd8,

    /* U+00CD "Í" */
    0x78, 0x6d, 0xb6, 0xdb, 0x6d, 0xb0,

    /* U+00CE "Î" */
    0x6f, 0x6, 0x66, 0x66, 0x66, 0x66, 0x66, 0x60,

    /* U+00CF "Ï" */
    0xde, 0xc0, 0x63, 0x18, 0xc6, 0x31, 0x8c, 0x63,
    0x18, 0xc0,

    /* U+00D0 "Ð" */
    0x7c, 0xfd, 0x9b, 0x36, 0x7e, 0xfd, 0xb3, 0x66,
    0xcd, 0xfb, 0xe0,

    /* U+00D1 "Ñ" */
    0x30, 0xf8, 0x30, 0xe, 0x7c, 0xfd, 0xfb, 0xf7,
    0xff, 0xfe, 0xfd, 0xfb, 0xf3, 0xe7,

    /* U+00D2 "Ò" */
    0x60, 0x60, 0x3, 0xef, 0xf8, 0xf1, 0xe3, 0xc7,
    0x8f, 0x1e, 0x3c, 0x7f, 0xde, 0x0,

    /* U+00D3 "Ó" */
    0x18, 0x60, 0x3, 0xef, 0xf8, 0xf1, 0xe3, 0xc7,
    0x8f, 0x1e, 0x3c, 0x7f, 0xde, 0x0,

    /* U+00D4 "Ô" */
    0x38, 0xd8, 0x3, 0xef, 0xf8, 0xf1, 0xe3, 0xc7,
    0x8f, 0x1e, 0x3c, 0x7f, 0xde, 0x0,

    /* U+00D5 "Õ" */
    0x30, 0xf8, 0x30, 0x7, 0xdf, 0xf1, 0xe3, 0xc7,
    0x8f, 0x1e, 0x3c, 0x78, 0xff, 0xbc,

    /* U+00D6 "Ö" */
    0x6c, 0xd8, 0x0, 0x7, 0xdf, 0xf1, 0xe3, 0xc7,
    0x8f, 0x1e, 0x3c, 0x78, 0xff, 0xbc,

    /* U+00D7 "×" */
    0x6d, 0xf3, 0x9f, 0x6d, 0x0,

    /* U+00D8 "Ø" */
    0x0, 0x7f, 0xfe, 0x3c, 0x78, 0xf1, 0xe3, 0xc7,
    0x8f, 0x1f, 0xef, 0x80, 0x0,

    /* U+00D9 "Ù" */
    0x60, 0x60, 0x6, 0x3c, 0x78, 0xf1, 0xe3, 0xc7,
    0x8f, 0x1e, 0x3c, 0x6f, 0x8e, 0x0,

    /* U+00DA "Ú" */
    0x18, 0x60, 0x6, 0x3c, 0x78, 0xf1, 0xe3, 0xc7,
    0x8f, 0x1e, 0x3c, 0x6f, 0x8e, 0x0,

    /* U+00DB "Û" */
    0x38, 0xd8, 0x6, 0x3c, 0x78, 0xf1, 0xe3, 0xc7,
    0x8f, 0x1e, 0x3c, 0x6f, 0x8e, 0x0,

    /* U+00DC "Ü" */
    0x6c, 0xd8, 0x0, 0xc, 0x78, 0xf1, 0xe3, 0xc7,
    0x8f, 0x1e, 0x3c, 0x78, 0xdf, 0x1c,

    /* U+00DD "Ý" */
    0x18, 0xc0, 0x33, 0xcf, 0xf7, 0x9e, 0x78, 0xc3,
    0xc, 0x30, 0xc3, 0x0,

    /* U+00DE "Þ" */
    0xc1, 0x83, 0xe7, 0xec, 0xf8, 0xf1, 0xe7, 0xfd,
    0xf3, 0x6, 0x0,

    /* U+00DF "ß" */
    0x7b, 0xfc, 0xf3, 0xcf, 0xef, 0xf3, 0xcf, 0x3f,
    0xfe,

    /* U+00E0 "à" */
    0x60, 0xc0, 0x1e, 0xff, 0xb0, 0xdf, 0xff, 0x3f,
    0xdf,

    /* U+00E1 "á" */
    0x18, 0xc0, 0x1e, 0xff, 0xb0, 0xdf, 0xff, 0x3f,
    0xdf,

    /* U+00E2 "â" */
    0x71, 0xe0, 0x1e, 0xff, 0xb0, 0xdf, 0xff, 0x3f,
    0xdf,

    /* U+00E3 "ã" */
    0x1, 0xf7, 0x80, 0x7b, 0xfe, 0xc3, 0x7f, 0xfc,
    0xff, 0x7c,

    /* U+00E4 "ä" */
    0x1, 0xb0, 0x0, 0x7b, 0xfe, 0xc3, 0x7f, 0xfc,
    0xff, 0x7c,

    /* U+00E5 "å" */
    0x31, 0xe4, 0x9e, 0x31, 0xef, 0xfb, 0xd, 0xff,
    0xf3, 0xfd, 0xf0,

    /* U+00E6 "æ" */
    0x7b, 0xef, 0xff, 0xe6, 0x37, 0xff, 0xff, 0xfc,
    0x60, 0xc6, 0x6f, 0xfe, 0x7b, 0xc0,

    /* U+00E7 "ç" */
    0x7b, 0xfc, 0xf0, 0xc3, 0xc, 0xff, 0x78, 0x41,
    0x8c,

    /* U+00E8 "è" */
    0x60, 0x80, 0x1e, 0xff, 0x3f, 0xff, 0xc3, 0x7f,
    0xde,

    /* U+00E9 "é" */
    0x18, 0xc0, 0x1e, 0xff, 0x3f, 0xff, 0xc3, 0x7f,
    0xde,

    /* U+00EA "ê" */
    0x71, 0xe0, 0x1e, 0xff, 0x3f, 0xff, 0xc3, 0x7f,
    0xde,

    /* U+00EB "ë" */
    0x1, 0xb0, 0x0, 0x7b, 0xfc, 0xff, 0xff, 0xc,
    0xff, 0x78,

    /* U+00EC "ì" */
    0xcc, 0x36, 0xdb, 0x6d, 0xb0,

    /* U+00ED "í" */
    0x78, 0x6d, 0xb6, 0xdb, 0x60,

    /* U+00EE "î" */
    0x6f, 0x6, 0x66, 0x66, 0x66, 0x66,

    /* U+00EF "ï" */
    0x6, 0xc0, 0x3, 0x18, 0xc6, 0x31, 0x8c, 0x63,
    0x0,

    /* U+00F0 "ð" */
    0x30, 0xf1, 0x9f, 0xff, 0x3c, 0xf3, 0xcf, 0x3f,
    0xde,

    /* U+00F1 "ñ" */
    0x1, 0xe7, 0x80, 0xfb, 0xfc, 0xf3, 0xcf, 0x3c,
    0xf3, 0xcc,

    /* U+00F2 "ò" */
    0x60, 0x80, 0x1e, 0xff, 0x3c, 0xf3, 0xcf, 0x3f,
    0xde,

    /* U+00F3 "ó" */
    0x18, 0xc0, 0x1e, 0xff, 0x3c, 0xf3, 0xcf, 0x3f,
    0xde,

    /* U+00F4 "ô" */
    0x71, 0xe0, 0x1e, 0xff, 0x3c, 0xf3, 0xcf, 0x3f,
    0xde,

    /* U+00F5 "õ" */
    0x1, 0xf7, 0x80, 0x7b, 0xfc, 0xf3, 0xcf, 0x3c,
    0xff, 0x78,

    /* U+00F6 "ö" */
    0x1, 0xb0, 0x0, 0x7b, 0xfc, 0xf3, 0xcf, 0x3c,
    0xff, 0x78,

    /* U+00F7 "÷" */
    0x30, 0x60, 0x7, 0xff, 0xe0, 0xc, 0x18,

    /* U+00F8 "ø" */
    0x1, 0xff, 0xf3, 0xcf, 0x3c, 0xf3, 0xff, 0xe0,
    0x0,

    /* U+00F9 "ù" */
    0x60, 0x80, 0x33, 0xcf, 0x3c, 0xf3, 0xcf, 0x3f,
    0xdf,

    /* U+00FA "ú" */
    0x30, 0xc0, 0x33, 0xcf, 0x3c, 0xf3, 0xcf, 0x3f,
    0xdf,

    /* U+00FB "û" */
    0x79, 0xe0, 0x33, 0xcf, 0x3c, 0xf3, 0xcf, 0x3f,
    0xdf,

    /* U+00FC "ü" */
    0x1, 0xb0, 0x0, 0xcf, 0x3c, 0xf3, 0xcf, 0x3c,
    0xff, 0x7c,

    /* U+00FD "ý" */
    0x18, 0x60, 0x7, 0x7e, 0xed, 0x9b, 0x36, 0x7c,
    0xf0, 0xe1, 0xc3, 0x1e, 0x38, 0x0,

    /* U+00FE "þ" */
    0xc3, 0xc, 0x3e, 0xff, 0x3c, 0xf3, 0xcf, 0x3f,
    0xfe, 0xc3, 0xc, 0x0,

    /* U+00FF "ÿ" */
    0x20, 0xec, 0x80, 0x6, 0x6c, 0xdd, 0xbf, 0x3c,
    0x78, 0xf0, 0xe1, 0x83, 0x1e, 0x38
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 54, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1, .adv_w = 76, .box_w = 2, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 4, .adv_w = 94, .box_w = 5, .box_h = 4, .ofs_x = 1, .ofs_y = 8},
    {.bitmap_index = 7, .adv_w = 168, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 21, .adv_w = 121, .box_w = 6, .box_h = 15, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 33, .adv_w = 208, .box_w = 12, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 51, .adv_w = 164, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 65, .adv_w = 46, .box_w = 2, .box_h = 4, .ofs_x = 1, .ofs_y = 8},
    {.bitmap_index = 66, .adv_w = 85, .box_w = 4, .box_h = 14, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 73, .adv_w = 85, .box_w = 4, .box_h = 14, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 80, .adv_w = 98, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 6},
    {.bitmap_index = 86, .adv_w = 119, .box_w = 7, .box_h = 6, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 92, .adv_w = 57, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 94, .adv_w = 90, .box_w = 5, .box_h = 2, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 96, .adv_w = 61, .box_w = 2, .box_h = 2, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 97, .adv_w = 113, .box_w = 6, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 106, .adv_w = 123, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 115, .adv_w = 77, .box_w = 4, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 121, .adv_w = 119, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 130, .adv_w = 119, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 139, .adv_w = 132, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 151, .adv_w = 119, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 160, .adv_w = 120, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 169, .adv_w = 111, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 180, .adv_w = 120, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 189, .adv_w = 118, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 198, .adv_w = 75, .box_w = 2, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 200, .adv_w = 63, .box_w = 3, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 204, .adv_w = 119, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 211, .adv_w = 119, .box_w = 7, .box_h = 5, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 216, .adv_w = 119, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 223, .adv_w = 119, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 234, .adv_w = 206, .box_w = 12, .box_h = 13, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 254, .adv_w = 131, .box_w = 8, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 266, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 275, .adv_w = 126, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 286, .adv_w = 129, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 295, .adv_w = 119, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 304, .adv_w = 115, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 313, .adv_w = 127, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 324, .adv_w = 131, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 333, .adv_w = 63, .box_w = 2, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 336, .adv_w = 123, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 347, .adv_w = 134, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 358, .adv_w = 116, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 369, .adv_w = 149, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 381, .adv_w = 140, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 392, .adv_w = 129, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 403, .adv_w = 126, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 414, .adv_w = 125, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 424, .adv_w = 128, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 435, .adv_w = 121, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 444, .adv_w = 127, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 455, .adv_w = 130, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 466, .adv_w = 133, .box_w = 8, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 478, .adv_w = 187, .box_w = 11, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 495, .adv_w = 129, .box_w = 8, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 507, .adv_w = 129, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 516, .adv_w = 112, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 525, .adv_w = 94, .box_w = 4, .box_h = 16, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 533, .adv_w = 113, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 542, .adv_w = 94, .box_w = 4, .box_h = 16, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 550, .adv_w = 115, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 5},
    {.bitmap_index = 557, .adv_w = 115, .box_w = 7, .box_h = 2, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 559, .adv_w = 60, .box_w = 3, .box_h = 2, .ofs_x = 0, .ofs_y = 10},
    {.bitmap_index = 560, .adv_w = 121, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 567, .adv_w = 121, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 576, .adv_w = 118, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 583, .adv_w = 121, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 592, .adv_w = 119, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 599, .adv_w = 81, .box_w = 4, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 605, .adv_w = 120, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 614, .adv_w = 122, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 623, .adv_w = 60, .box_w = 2, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 627, .adv_w = 59, .box_w = 4, .box_h = 16, .ofs_x = -1, .ofs_y = -3},
    {.bitmap_index = 635, .adv_w = 122, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 644, .adv_w = 57, .box_w = 2, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 647, .adv_w = 184, .box_w = 10, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 659, .adv_w = 122, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 666, .adv_w = 120, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 673, .adv_w = 122, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 682, .adv_w = 122, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 691, .adv_w = 88, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 696, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 703, .adv_w = 79, .box_w = 4, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 709, .adv_w = 122, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 716, .adv_w = 119, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 724, .adv_w = 167, .box_w = 10, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 736, .adv_w = 119, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 744, .adv_w = 115, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 755, .adv_w = 101, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 762, .adv_w = 94, .box_w = 5, .box_h = 16, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 772, .adv_w = 51, .box_w = 2, .box_h = 14, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 776, .adv_w = 94, .box_w = 5, .box_h = 16, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 786, .adv_w = 133, .box_w = 8, .box_h = 3, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 789, .adv_w = 54, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 790, .adv_w = 76, .box_w = 2, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 793, .adv_w = 118, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 802, .adv_w = 141, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 814, .adv_w = 154, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 825, .adv_w = 144, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 836, .adv_w = 51, .box_w = 2, .box_h = 14, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 840, .adv_w = 119, .box_w = 6, .box_h = 15, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 852, .adv_w = 100, .box_w = 5, .box_h = 2, .ofs_x = 1, .ofs_y = 10},
    {.bitmap_index = 854, .adv_w = 215, .box_w = 13, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 876, .adv_w = 79, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 881, .adv_w = 125, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 886, .adv_w = 119, .box_w = 7, .box_h = 4, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 890, .adv_w = 90, .box_w = 5, .box_h = 2, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 892, .adv_w = 118, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 899, .adv_w = 91, .box_w = 4, .box_h = 2, .ofs_x = 1, .ofs_y = 10},
    {.bitmap_index = 900, .adv_w = 98, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 8},
    {.bitmap_index = 905, .adv_w = 119, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 914, .adv_w = 72, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 7},
    {.bitmap_index = 918, .adv_w = 77, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 7},
    {.bitmap_index = 922, .adv_w = 60, .box_w = 3, .box_h = 2, .ofs_x = 0, .ofs_y = 10},
    {.bitmap_index = 923, .adv_w = 122, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 932, .adv_w = 140, .box_w = 8, .box_h = 15, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 947, .adv_w = 61, .box_w = 2, .box_h = 2, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 948, .adv_w = 41, .box_w = 2, .box_h = 3, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 949, .adv_w = 43, .box_w = 3, .box_h = 7, .ofs_x = 0, .ofs_y = 7},
    {.bitmap_index = 952, .adv_w = 80, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 956, .adv_w = 125, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 961, .adv_w = 195, .box_w = 12, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 979, .adv_w = 188, .box_w = 12, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 997, .adv_w = 216, .box_w = 13, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1017, .adv_w = 119, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1028, .adv_w = 131, .box_w = 8, .box_h = 15, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1043, .adv_w = 131, .box_w = 8, .box_h = 15, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1058, .adv_w = 131, .box_w = 8, .box_h = 15, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1073, .adv_w = 131, .box_w = 8, .box_h = 16, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1089, .adv_w = 131, .box_w = 7, .box_h = 15, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1103, .adv_w = 131, .box_w = 7, .box_h = 18, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1119, .adv_w = 205, .box_w = 12, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1137, .adv_w = 126, .box_w = 7, .box_h = 15, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1151, .adv_w = 119, .box_w = 6, .box_h = 15, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1163, .adv_w = 119, .box_w = 6, .box_h = 15, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1175, .adv_w = 119, .box_w = 6, .box_h = 15, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1187, .adv_w = 119, .box_w = 6, .box_h = 15, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1199, .adv_w = 63, .box_w = 3, .box_h = 15, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1205, .adv_w = 63, .box_w = 3, .box_h = 15, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1211, .adv_w = 63, .box_w = 4, .box_h = 15, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1219, .adv_w = 63, .box_w = 5, .box_h = 15, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 1229, .adv_w = 138, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1240, .adv_w = 140, .box_w = 7, .box_h = 16, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1254, .adv_w = 129, .box_w = 7, .box_h = 15, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1268, .adv_w = 129, .box_w = 7, .box_h = 15, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1282, .adv_w = 129, .box_w = 7, .box_h = 15, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1296, .adv_w = 129, .box_w = 7, .box_h = 16, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1310, .adv_w = 129, .box_w = 7, .box_h = 16, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1324, .adv_w = 109, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 1329, .adv_w = 151, .box_w = 7, .box_h = 14, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1342, .adv_w = 130, .box_w = 7, .box_h = 15, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1356, .adv_w = 130, .box_w = 7, .box_h = 15, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1370, .adv_w = 130, .box_w = 7, .box_h = 15, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1384, .adv_w = 130, .box_w = 7, .box_h = 16, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1398, .adv_w = 129, .box_w = 6, .box_h = 15, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1410, .adv_w = 126, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1421, .adv_w = 125, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1430, .adv_w = 121, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1439, .adv_w = 121, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1448, .adv_w = 121, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1457, .adv_w = 121, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1467, .adv_w = 121, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1477, .adv_w = 121, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1488, .adv_w = 187, .box_w = 12, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1502, .adv_w = 118, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1511, .adv_w = 119, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1520, .adv_w = 119, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1529, .adv_w = 119, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1538, .adv_w = 119, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1548, .adv_w = 52, .box_w = 3, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1553, .adv_w = 52, .box_w = 3, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1558, .adv_w = 52, .box_w = 4, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1564, .adv_w = 52, .box_w = 5, .box_h = 13, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 1573, .adv_w = 122, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1582, .adv_w = 122, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1592, .adv_w = 120, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1601, .adv_w = 120, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1610, .adv_w = 120, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1619, .adv_w = 120, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1629, .adv_w = 120, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1639, .adv_w = 119, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 1646, .adv_w = 124, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1655, .adv_w = 122, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1664, .adv_w = 122, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1673, .adv_w = 122, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1682, .adv_w = 122, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1692, .adv_w = 115, .box_w = 7, .box_h = 15, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1706, .adv_w = 120, .box_w = 6, .box_h = 15, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1718, .adv_w = 115, .box_w = 7, .box_h = 16, .ofs_x = 0, .ofs_y = -3}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/



/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 32, .range_length = 95, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 160, .range_length = 96, .glyph_id_start = 96,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    }
};

/*-----------------
 *    KERNING
 *----------------*/


/*Map glyph_ids to kern left classes*/
static const uint8_t kern_left_class_mapping[] =
{
    0, 0, 0, 1, 0, 0, 0, 0,
    1, 0, 0, 0, 2, 3, 4, 3,
    5, 0, 6, 7, 8, 9, 10, 11,
    12, 7, 7, 0, 0, 0, 0, 0,
    0, 0, 13, 14, 15, 14, 16, 17,
    15, 18, 0, 19, 20, 21, 0, 0,
    15, 22, 23, 24, 25, 26, 19, 27,
    27, 28, 29, 0, 0, 0, 0, 0,
    0, 0, 30, 31, 32, 0, 33, 34,
    35, 31, 36, 0, 37, 0, 31, 31,
    4, 31, 35, 38, 39, 40, 0, 41,
    41, 42, 41, 43, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 44,
    13, 13, 13, 13, 13, 13, 0, 15,
    0, 0, 0, 0, 0, 0, 0, 0,
    14, 0, 15, 15, 15, 15, 15, 0,
    0, 19, 19, 19, 19, 0, 14, 14,
    31, 31, 31, 31, 31, 31, 33, 32,
    33, 33, 33, 33, 0, 0, 0, 0,
    30, 0, 4, 4, 4, 4, 4, 0,
    4, 0, 0, 0, 0, 41, 31, 41
};

/*Map glyph_ids to kern right classes*/
static const uint8_t kern_right_class_mapping[] =
{
    0, 0, 0, 1, 0, 0, 0, 0,
    1, 0, 0, 0, 0, 2, 3, 2,
    4, 5, 6, 7, 0, 0, 0, 0,
    8, 0, 0, 0, 0, 0, 0, 0,
    9, 0, 10, 11, 12, 11, 11, 11,
    12, 0, 0, 13, 14, 15, 0, 11,
    12, 11, 12, 16, 17, 18, 0, 19,
    19, 20, 21, 0, 0, 0, 0, 0,
    0, 0, 22, 0, 3, 3, 3, 23,
    24, 0, 25, 26, 0, 0, 27, 27,
    3, 27, 24, 27, 28, 23, 29, 30,
    30, 31, 30, 32, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    10, 10, 10, 10, 10, 10, 10, 12,
    11, 11, 11, 11, 0, 0, 0, 0,
    0, 11, 12, 12, 12, 12, 12, 0,
    0, 0, 0, 0, 0, 21, 0, 11,
    22, 22, 22, 22, 22, 22, 22, 3,
    3, 3, 3, 3, 0, 0, 0, 0,
    3, 0, 3, 3, 3, 3, 3, 0,
    3, 29, 29, 29, 29, 30, 27, 30
};

/*Kern values between classes*/
static const int8_t kern_class_values[] =
{
    0, 0, -6, 0, 0, 0, 0, 0,
    0, -13, 0, 0, -18, 0, 0, 0,
    0, 0, 0, 0, 0, -4, 1, 0,
    0, 0, -1, -5, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -4, -10,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -12, 0, 0, -9,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -2, -1, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -4, -5, 0,
    0, 0, -16, 0, 0, 0, 0, 0,
    0, -21, -2, -8, 0, -2, -3, -2,
    -2, 0, 0, 0, 0, -18, 0, -5,
    0, 0, -4, -10, -6, -8, 0, -6,
    0, 0, 0, 0, 0, -2, 1, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -4, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -4, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -15, 0, -15,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -2, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -3, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -13, 0, -23, 0, -1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -13, 0, -1, 2, 0, 0, 0, 0,
    0, 4, 0, -5, 0, 0, 0, 0,
    -7, -16, -21, 1, -15, 0, -2, 0,
    0, 0, 0, -1, -4, -11, 2, 0,
    0, -2, 0, -1, 0, 0, 0, 0,
    0, -5, -1, 0, 0, -1, 0, 0,
    0, 0, -7, -8, -9, 0, 0, 0,
    0, 0, 0, -2, 0, -2, 0, 0,
    0, -1, -1, 0, 0, 0, 0, 0,
    0, -3, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -4, -4, 0, 0, 0,
    0, 0, 0, 0, 0, -2, 0, 0,
    0, 0, -1, 0, 0, 0, 0, 0,
    0, 4, 0, 0, 2, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -1, 0, 0, 0, 0,
    0, 0, -9, -15, 0, 0, 0, 0,
    0, -11, 0, 0, -2, 0, 0, 0,
    0, 0, 0, 0, 0, -8, -2, -9,
    0, 0, -9, -7, -5, -6, -6, -6,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -1, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -2, 0, -4, 0, 0, 0, 0,
    0, -4, -2, -1, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 2, 0, 2, 0, 0, 0, 0,
    0, 2, 0, -6, 0, 0, 0, 0,
    -6, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 2, 0, 2, 0, 0, 0, 0,
    -4, 5, 0, -1, 0, 0, 0, 0,
    -1, -22, 0, 0, -9, 0, -4, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -5, 0, -18, 0, 0, 0, 0,
    0, -9, 0, 0, -16, 0, 0, 0,
    0, 0, -4, -6, -6, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -1, -1, 0, 0, 0, 0, 0,
    4, -3, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -4, -4, 0, 0, 0,
    0, 0, 0, 0, 0, -2, 0, 0,
    0, 0, 0, 2, 0, 0, 0, 0,
    0, 1, 0, 0, 0, 0, 0, 0,
    1, 0, -1, 0, -8, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -1, 0, 0, 0, 0, 0, 0,
    0, 0, -1, 0, 0, 0, 0, 0,
    0, -2, -4, -8, -7, 0, 0, 0,
    0, 0, 0, 0, 0, -2, 0, 0,
    0, -12, -19, -2, 0, 0, 0, 0,
    8, -15, 0, -5, 0, 0, -1, 0,
    0, 0, 0, 0, 0, -14, -4, -18,
    0, 0, -8, -17, -5, -8, -2, -8,
    0, -9, -13, -16, 0, 0, 0, 0,
    0, -20, 0, -5, -10, 0, 0, -1,
    -4, 0, 0, 0, 0, -8, -4, -9,
    0, 0, -6, -12, -2, -4, -6, -4,
    0, 1, -4, 0, 0, 0, 0, 0,
    0, 1, 0, -4, -2, 0, 0, 0,
    -8, 0, 0, 0, 0, 0, -4, -4,
    0, 0, 0, -4, -4, -13, 0, 0,
    0, -20, -16, -16, 0, 0, 0, 0,
    0, -19, -2, -6, -16, 0, -1, -2,
    -7, 0, 0, 0, 0, 0, -8, -15,
    0, 0, -9, -1, -10, -12, -6, -6,
    -2, -1, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -4, 0, 0,
    -1, -2, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -2, -4, 0,
    0, -1, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -2, 0,
    0, 0, 0, 0, 0, -6, -4, 0,
    -3, -1, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -2, -1, 0,
    0, 0, 0, 0, 0, -4, 0, 0,
    1, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -2,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    1, 0, 0, 0, 0, -2, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -2, 0, 0, 0, 0, 0, 0,
    0, 0, -6, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -4, -6,
    0, 0, 0, -6, 0, 0, 0, 0,
    1, 0, -2, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -1, 0, -3,
    0, 0, 0, 0, 0, 1, 0, 0,
    -4, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -3, 0, -6, -4, 0,
    1, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -6, -4, -8, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -4, 0, -6,
    0, 0, 0, -4, 0, 2, 0, 0,
    0, 0, -6, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 4, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -1, 0, 0, 0, 0, 0,
    0, 0, 0, -6, -4, 0, 6, 0,
    -6, -13, -17, 0, -18, 0, 0, -2,
    0, 0, 0, 0, 0, 0, 0, 0
};


/*Collect the kern class' data in one place*/
static const lv_font_fmt_txt_kern_classes_t kern_classes =
{
    .class_pair_values   = kern_class_values,
    .left_class_mapping  = kern_left_class_mapping,
    .right_class_mapping = kern_right_class_mapping,
    .left_class_cnt      = 44,
    .right_class_cnt     = 32,
};

/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LVGL_VERSION_MAJOR == 8
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
#endif

#if LVGL_VERSION_MAJOR >= 8
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = &kern_classes,
    .kern_scale = 16,
    .cmap_num = 2,
    .bpp = 1,
    .kern_classes = 1,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif
};



/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t font_splash_sub = {
#else
lv_font_t font_splash_sub = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 21,          /*The maximum line height required by the font*/
    .base_line = 3,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -1,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if FONT_SPLASH_SUB*/

