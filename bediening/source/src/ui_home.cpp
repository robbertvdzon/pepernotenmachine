#include "ui_home.h"

#include <string.h>

#include "fonts.h"
#include "gfx.h"
#include "home_config.h"
#include "home_sprites.h"
#include "ui_compat.h"
#include "ui_strings.h"
#include "ui_theme.h"

// LVGL may pad canvas rows to LV_DRAW_BUF_STRIDE_ALIGN bytes (1 by default); our pixel code writes packed rows.
#define HOME_STRIDE_OK(w) ((((w) * 2) % (LV_DRAW_BUF_STRIDE_ALIGN > 0 ? LV_DRAW_BUF_STRIDE_ALIGN : 1)) == 0)
static_assert(HOME_STRIDE_OK(HOME_BAND_W) && HOME_STRIDE_OK(HOME_TILES_W) && HOME_STRIDE_OK(HOME_TOAST_W),
              "canvas row size must be a multiple of LV_DRAW_BUF_STRIDE_ALIGN (see home_config.h)");

namespace {

// Canvas pixel buffers (RGB565), static so they cost no LVGL heap.
alignas(64) uint16_t band_buf[HOME_BAND_W * HOME_BAND_H];
alignas(64) uint16_t tiles_buf[HOME_TILES_W * HOME_TILES_H];
alignas(64) uint16_t toast_buf[HOME_TOAST_W * HOME_TOAST_H];

const gfx::Surface band_surf = {band_buf, HOME_BAND_W, HOME_BAND_H};
const gfx::Surface tiles_surf = {tiles_buf, HOME_TILES_W, HOME_TILES_H};
const gfx::Surface toast_surf = {toast_buf, HOME_TOAST_W, HOME_TOAST_H};

HomeModel model;
HomeView view;

lv_obj_t* band_cv = nullptr;
lv_obj_t* tiles_cv = nullptr;
lv_obj_t* toast_box = nullptr;
lv_obj_t* toast_cv = nullptr;
lv_obj_t* word_big = nullptr;
lv_obj_t* word_small = nullptr;
lv_obj_t* toast_label = nullptr;

// What is on screen now, so that nothing is redrawn (and re-sent to the panel) without a reason.
struct Label {
    char text[24] = {0};
    uint32_t color = 0;
    bool init = false;
};
Label big_cache, small_cache, toast_cache;

bool was_visible = false;
bool band_valid = false;
BandState drawn_band = BandState::Offline;
uint32_t drawn_band_frame = 0;
uint8_t drawn_secs = 0;
uint16_t drawn_progress = 0;

bool tiles_valid = false;
TileState drawn_tiles[TILE_COUNT];
uint32_t drawn_tiles_frame = 0;

bool toast_valid = false;
ToastLevel drawn_toast_level = ToastLevel::None;
int drawn_toast_offset = -1;

lv_obj_t* make_canvas(lv_obj_t* parent, uint16_t* buf, int w, int h, int x, int y) {
    lv_obj_t* c = lv_canvas_create(parent);
    lv_canvas_set_buffer(c, buf, w, h, LV_COLOR_FORMAT_RGB565);
    lv_obj_set_pos(c, x, y);
    return c;
}

lv_obj_t* make_label(lv_obj_t* parent, int x, int y, int width) {
    lv_obj_t* l = lv_label_create(parent);
    lv_obj_set_style_text_font(l, UI_FONT_NORMAL, LV_PART_MAIN);
    lv_label_set_long_mode(l, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(l, width);
    lv_obj_set_pos(l, x, y);
    lv_label_set_text(l, "");
    return l;
}

void set_label(lv_obj_t* l, Label& c, const char* text, uint32_t color) {
    if (c.init && c.color == color && strcmp(c.text, text) == 0) return;
    c.init = true;
    c.color = color;
    strncpy(c.text, text, sizeof(c.text) - 1);
    lv_label_set_text(l, text);
    lv_obj_set_style_text_color(l, lv_color_hex(color), LV_PART_MAIN);
}

struct Words {
    const char* big;
    const char* small;
};

Words words_for(BandState b) {
    switch (b) {
        case BandState::Offline: return {STR_BAND_OFFLINE, STR_BAND_SEARCHING};
        case BandState::Check: return {STR_BAND_CHECK, STR_BAND_ANSWER};
        case BandState::Error: return {STR_BAND_ERROR, STR_BAND_FIX};
        case BandState::Starting: return {STR_BAND_PREPARING, STR_BAND_STARTING};
        case BandState::Down: return {STR_BAND_PREPARING, STR_BAND_LOWERING};
        case BandState::Lock: return {STR_BAND_PREPARING, STR_BAND_LOCKING};
        case BandState::Up: return {STR_BAND_PREPARING, STR_BAND_UNWINDING};
        case BandState::Ready: return {STR_BAND_READY, STR_BAND_TO_LAUNCH};
        case BandState::Launched: return {STR_BAND_LAUNCHED, ""};
        case BandState::Rearm: return {STR_BAND_REARM, STR_BAND_STAND_CLEAR};
    }
    return {"", ""};
}

void draw_band(uint32_t now) {
    const uint32_t frame = (now - view.bandSince) / HOME_FRAME_MS;
    if (band_valid && view.band == drawn_band && frame == drawn_band_frame && view.secsLeft == drawn_secs &&
        view.progress == drawn_progress) {
        return;
    }
    band_valid = true;
    drawn_band = view.band;
    drawn_band_frame = frame;
    drawn_secs = view.secsLeft;
    drawn_progress = view.progress;

    const uint32_t t = frame * HOME_FRAME_MS;
    sprites::drawBand(band_surf, view.band, t, view.progress, view.secsLeft);
    lv_obj_invalidate(band_cv);

    const Words w = words_for(view.band);
    const uint32_t accent = sprites::bandAccent(view.band, t);
    const bool lit = view.band == BandState::Ready || view.band == BandState::Error || view.band == BandState::Rearm;
    set_label(word_big, big_cache, w.big, view.band == BandState::Offline ? ui_color::TEXT_MUTED : ui_color::TEXT);
    set_label(word_small, small_cache, w.small, lit ? accent : ui_color::TEXT_MUTED);
}

void draw_tiles(uint32_t now) {
    const uint32_t frame = now / HOME_FRAME_MS;
    bool animated = false;
    for (int i = 0; i < TILE_COUNT; i++) animated |= sprites::tileAnimates(view.tile[i]);
    const bool changed = !tiles_valid || memcmp(drawn_tiles, view.tile, sizeof(drawn_tiles)) != 0;
    if (!changed && !(animated && frame != drawn_tiles_frame)) return;
    tiles_valid = true;
    memcpy(drawn_tiles, view.tile, sizeof(drawn_tiles));
    drawn_tiles_frame = frame;
    sprites::drawTiles(tiles_surf, view.tile, frame * HOME_FRAME_MS);
    lv_obj_invalidate(tiles_cv);
}

void draw_toast(uint32_t now) {
    if (view.toastLevel == ToastLevel::None) {
        if (!ui_is_hidden(toast_box)) ui_set_hidden(toast_box, true);
        toast_valid = false;
        return;
    }
    if (ui_is_hidden(toast_box)) ui_set_hidden(toast_box, false);

    if (!toast_valid || view.toastLevel != drawn_toast_level) {
        toast_valid = true;
        drawn_toast_level = view.toastLevel;
        sprites::drawToast(toast_surf, view.toastLevel);
        lv_obj_invalidate(toast_cv);
    }
    set_label(toast_label, toast_cache, view.toast, ui_color::TEXT);

    // Slides in from below: the box stays where it is (never partly off-screen: that makes a screen scrollable and
    // LVGL draws a scrollbar), its children move inside it and are clipped by it.
    const uint32_t el = now - view.toastSince;
    const int offset = el >= TOAST_SLIDE_MS ? 0 : static_cast<int>((HOME_TOAST_H + 2) * (TOAST_SLIDE_MS - el) / TOAST_SLIDE_MS);
    if (offset != drawn_toast_offset) {
        drawn_toast_offset = offset;
        lv_obj_set_y(toast_cv, offset);
        lv_obj_set_y(toast_label, offset + HOME_TOAST_TEXT_DY);
    }
}

}  // namespace

void ui_home_build(lv_obj_t* parent) {
    ui_set_scrollable(parent, false);  // nothing on the home screen ever scrolls (and never shows a scrollbar)
    gfx::fill(band_surf, ui_color::BACKGROUND);
    gfx::fill(tiles_surf, ui_color::BACKGROUND);
    gfx::fill(toast_surf, ui_color::BACKGROUND);

    band_cv = make_canvas(parent, band_buf, HOME_BAND_W, HOME_BAND_H, HOME_BAND_X, HOME_BAND_Y);
    tiles_cv = make_canvas(parent, tiles_buf, HOME_TILES_W, HOME_TILES_H, 0, HOME_TILES_Y);
    word_big = make_label(parent, HOME_TEXT_X, HOME_WORD_Y, HOME_BAND_W - HOME_TEXT_X);
    word_small = make_label(parent, HOME_TEXT_X, HOME_SMALL_Y, HOME_BAND_W - HOME_TEXT_X);

    toast_box = lv_obj_create(parent);  // a plain container so the canvas and its text slide together
    ui_set_scrollable(toast_box, false);
    lv_obj_set_size(toast_box, HOME_TOAST_W, HOME_TOAST_H);
    lv_obj_set_pos(toast_box, 0, HOME_TOAST_Y);
    lv_obj_set_style_bg_opa(toast_box, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(toast_box, 0, LV_PART_MAIN);
    lv_obj_set_style_outline_width(toast_box, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(toast_box, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(toast_box, 0, LV_PART_MAIN);
    toast_cv = make_canvas(toast_box, toast_buf, HOME_TOAST_W, HOME_TOAST_H, 0, 0);
    toast_label = make_label(toast_box, HOME_TOAST_MARGIN + 8, HOME_TOAST_TEXT_DY, HOME_TOAST_W - 2 * HOME_TOAST_MARGIN - 10);
    ui_set_hidden(toast_box, true);
}

void ui_home_update(const HomeInputs& in, bool visible) {
    if (!band_cv) return;
    model.update(in, view);
    if (!visible) {
        was_visible = false;
        return;
    }
    if (!was_visible) {  // coming back from another screen: everything is redrawn once
        was_visible = true;
        band_valid = tiles_valid = toast_valid = false;
        drawn_toast_offset = -1;
        lv_obj_invalidate(band_cv);
        lv_obj_invalidate(tiles_cv);
    }
    draw_band(in.now);
    draw_tiles(in.now);
    draw_toast(in.now);
}
