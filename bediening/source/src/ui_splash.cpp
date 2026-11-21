#include "ui_splash.h"

#include <stdio.h>
#include <string.h>

#include "fonts.h"
#include "gfx.h"
#include "splash_art.h"
#include "splash_config.h"
#include "splash_sprites.h"
#include "ui_compat.h"
#include "ui_strings.h"
#include "version.h"

// LVGL may pad canvas rows to LV_DRAW_BUF_STRIDE_ALIGN bytes (1 by default); our pixel code writes packed rows.
static_assert(((SPLASH_ROW_W * 2) % (LV_DRAW_BUF_STRIDE_ALIGN > 0 ? LV_DRAW_BUF_STRIDE_ALIGN : 1)) == 0,
              "canvas row size must be a multiple of LV_DRAW_BUF_STRIDE_ALIGN (see home_config.h)");

namespace {

alignas(64) uint16_t row_buf[SPLASH_ROW_W * SPLASH_ROW_H];  // static: costs no LVGL heap
const gfx::Surface row_surf = {row_buf, SPLASH_ROW_W, SPLASH_ROW_H};

lv_obj_t* row_cv = nullptr;
lv_obj_t* continue_label = nullptr;  // "push = continue", at the very top over the picture
lv_image_dsc_t art_dsc;              // describes the picture in flash (filled in at run time: no LVGL-version specific initializer)

bool row_valid = false;
uint32_t drawn_frame = 0;
BadgeState drawn_badge[SPLASH_BADGE_COUNT];
int drawn_continue = -1;

lv_obj_t* make_text(lv_obj_t* parent, const lv_font_t* font, uint32_t color, int x, int y, int w, lv_text_align_t align, const char* text, int letter_space = 0) {
    lv_obj_t* l = lv_label_create(parent);
    lv_obj_set_style_text_font(l, font, LV_PART_MAIN);
    lv_obj_set_style_text_color(l, lv_color_hex(color), LV_PART_MAIN);
    lv_obj_set_style_text_align(l, align, LV_PART_MAIN);
    lv_obj_set_style_text_letter_space(l, letter_space, LV_PART_MAIN);
    lv_label_set_long_mode(l, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(l, w);
    lv_obj_set_pos(l, x, y);
    lv_label_set_text(l, text);
    return l;
}

}  // namespace

lv_obj_t* ui_splash_build() {
    lv_obj_t* scr = lv_obj_create(nullptr);
    ui_set_scrollable(scr, false);
    lv_obj_set_style_bg_color(scr, lv_color_hex(splash_color::BACKGROUND), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_pad_all(scr, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(scr, 0, LV_PART_MAIN);

    // The artwork on top; everything else is drawn over / below it.
    memset(&art_dsc, 0, sizeof(art_dsc));
    art_dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
    art_dsc.header.cf = LV_COLOR_FORMAT_RGB565;
    art_dsc.header.w = SPLASH_ART_W;
    art_dsc.header.h = SPLASH_ART_H;
    art_dsc.header.stride = SPLASH_ART_W * 2;
    art_dsc.data_size = sizeof(splash_art_pixels);
    art_dsc.data = reinterpret_cast<const uint8_t*>(splash_art_pixels);
    lv_obj_t* art = lv_image_create(scr);
    lv_image_set_src(art, &art_dsc);
    lv_obj_set_pos(art, 0, 0);

    // Title: a one pixel outline made of the four diagonal copies, then the colored letters on top. All copies stay inside
    // the screen (labels are 158 px wide at x = 0, 1 or 2).
    static const int8_t dx[4] = {-1, 1, -1, 1}, dy[4] = {-1, -1, 1, 1};
    for (int i = 0; i < 4; i++) {
        make_text(scr, UI_FONT_SPLASH_TITLE, splash_color::TITLE_OUTLINE, SPLASH_LABEL_X + dx[i], SPLASH_TITLE_Y + dy[i],
                  SPLASH_LABEL_W, LV_TEXT_ALIGN_CENTER, STR_SPLASH_TITLE, SPLASH_TITLE_LETTER_SPACE);
    }
    make_text(scr, UI_FONT_SPLASH_TITLE, splash_color::TITLE, SPLASH_LABEL_X, SPLASH_TITLE_Y, SPLASH_LABEL_W, LV_TEXT_ALIGN_CENTER,
              STR_SPLASH_TITLE, SPLASH_TITLE_LETTER_SPACE);
    make_text(scr, UI_FONT_SPLASH_SUB, splash_color::TEXT, SPLASH_LABEL_X, SPLASH_SUB_Y, SPLASH_LABEL_W, LV_TEXT_ALIGN_CENTER,
              STR_SPLASH_SUBTITLE);

    row_cv = lv_canvas_create(scr);
    lv_canvas_set_buffer(row_cv, row_buf, SPLASH_ROW_W, SPLASH_ROW_H, LV_COLOR_FORMAT_RGB565);
    lv_obj_set_pos(row_cv, (SPLASH_W - SPLASH_ROW_W) / 2, SPLASH_ROW_Y);
    gfx::fill(row_surf, splash_color::BACKGROUND);

    char ver[16];
    snprintf(ver, sizeof(ver), "v%s", APP_VERSION);
    lv_obj_t* v = make_text(scr, UI_FONT_SMALL, splash_color::TEXT_MUTED, 0, 0, static_cast<int>(strlen(ver)) * UI_FONT_SMALL_W,
                            LV_TEXT_ALIGN_RIGHT, ver);
    lv_obj_align(v, LV_ALIGN_BOTTOM_RIGHT, -SPLASH_VERSION_MARGIN, -SPLASH_VERSION_MARGIN);

    // The offer to continue: over the picture at the very top, on a semi-transparent patch of the background color.
    // Created last so it is drawn on top of everything; hidden until the model asks for it.
    continue_label = lv_label_create(scr);
    lv_obj_set_style_text_font(continue_label, UI_FONT_NORMAL, LV_PART_MAIN);
    lv_obj_set_style_text_color(continue_label, lv_color_hex(splash_color::TEXT), LV_PART_MAIN);
    lv_obj_set_style_bg_color(continue_label, lv_color_hex(splash_color::BACKGROUND), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(continue_label, LV_OPA_80, LV_PART_MAIN);
    lv_obj_set_style_radius(continue_label, 3, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(continue_label, 3, LV_PART_MAIN);
    lv_label_set_text(continue_label, STR_SPLASH_CONTINUE);
    lv_obj_align(continue_label, LV_ALIGN_TOP_MID, 0, SPLASH_CONTINUE_Y);
    ui_set_hidden(continue_label, true);

    row_valid = false;
    drawn_continue = -1;
    return scr;
}

void ui_splash_update(const SplashView& view) {
    if (!row_cv) return;

    // link row: redrawn when a badge changes or the animation moves on to its next frame
    const uint32_t frame = view.elapsed / SPLASH_FRAME_MS;
    const bool changed = !row_valid || frame != drawn_frame || memcmp(drawn_badge, view.badge, sizeof(drawn_badge)) != 0;
    if (changed) {
        row_valid = true;
        drawn_frame = frame;
        memcpy(drawn_badge, view.badge, sizeof(drawn_badge));
        splash_sprites::drawLinkRow(row_surf, view.badge[SPLASH_BADGE_LAUNCHER], view.badge[SPLASH_BADGE_SWITCHBOARD],
                                    frame * SPLASH_FRAME_MS);
        lv_obj_invalidate(row_cv);
    }

    // the offer to continue
    const int cont = view.showContinue ? 1 : 0;
    if (cont != drawn_continue) {
        drawn_continue = cont;
        ui_set_hidden(continue_label, cont == 0);
    }
}
