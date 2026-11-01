// Flush consistency check of the home screen on a host build of LVGL. Plays a long session (a toast sliding in and out,
// messages, tile changes), applies every flush to an emulated panel exactly like display.cpp does, and verifies:
//   1. every flush area lies inside the screen,
//   2. the emulated panel always equals LVGL's frame buffer (nothing is flushed wrongly or forgotten),
//   3. the incremental frame always equals a full redraw (no ghost pixels, no scrollbar),
//   4. the screen can never scroll.
//   see test/offtarget/run.sh
#include <lvgl.h>
#include <stdio.h>
#include <string.h>

#include "home_config.h"
#include "ui_home.h"

uint32_t stub_millis = 0;
alignas(64) static uint8_t fb[160 * 4 * 128];
static uint32_t stride = 0;
static uint16_t panel[128][160];
static int bad_area = 0, flushes = 0;

static void flush_cb(lv_display_t* d, const lv_area_t* a, uint8_t* px) {
    flushes++;
    if (a->x1 < 0 || a->y1 < 0 || a->x2 > 159 || a->y2 > 127 || a->x2 < a->x1 || a->y2 < a->y1) {
        bad_area++;
        printf("BAD AREA %d,%d - %d,%d at t=%u\n", (int)a->x1, (int)a->y1, (int)a->x2, (int)a->y2, stub_millis);
    } else {
        for (int y = a->y1; y <= a->y2; y++) {
            const uint16_t* row = reinterpret_cast<const uint16_t*>(px + y * stride);
            for (int x = a->x1; x <= a->x2; x++) panel[y][x] = row[x];
        }
    }
    lv_display_flush_ready(d);
}

int main() {
    lv_init();
    lv_tick_set_cb([]() -> uint32_t { return stub_millis; });
    lv_display_t* disp = lv_display_create(160, 128);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
    stride = lv_draw_buf_width_to_stride(160, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(disp, fb, NULL, sizeof(fb), LV_DISPLAY_RENDER_MODE_DIRECT);
    lv_display_set_flush_cb(disp, flush_cb);
    lv_obj_t* scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);
    ui_home_build(scr);

    HomeInputs in;
    in.launcher = LauncherStatus::Offline;
    int ghost_steps = 0, panel_steps = 0, scroll_steps = 0;
    static uint16_t snap[128 * 160];
    for (uint32_t t = 0; t < 20000; t += 10) {
        stub_millis = t;
        in.now = t;
        in.switchboardConnected = t >= 12000;                                  // toast "board offline" for 3..12 s
        in.fogLed = (t > 7000 && t < 9000) ? LED_FLASH_FAST : LED_OFF;         // toast "fog problem"
        in.flashLed = t > 4000 ? LED_ON : LED_OFF;
        if (t == 5000 || t == 14000) { in.message = Msg::LaunchNotReady; in.messageAt = t; }
        if (t == 16000) { in.message = Msg::LaunchFailed; in.messageAt = t; }
        in.launcher = t < 1000 ? LauncherStatus::Offline : (t < 15000 ? LauncherStatus::Preparing : LauncherStatus::Ready);
        in.prepValid = t >= 1000;
        in.prep = t < 6000 ? 1 : 3;
        ui_home_update(in, true);
        lv_timer_handler();

        if (lv_obj_get_scroll_bottom(scr) > 0 || lv_obj_get_scroll_top(scr) > 0 || lv_obj_get_scroll_y(scr) != 0) scroll_steps++;
        lv_refr_now(NULL);  // render what is pending (LVGL refreshes every 33 ms)
        int panel_diff = 0;
        for (int y = 0; y < 128; y++)
            for (int x = 0; x < 160; x++)
                if (panel[y][x] != reinterpret_cast<const uint16_t*>(fb + y * stride)[x]) panel_diff++;
        if (panel_diff && t > 100) panel_steps++;
        for (int y = 0; y < 128; y++) memcpy(&snap[y * 160], fb + y * stride, 320);
        lv_obj_invalidate(scr);
        lv_refr_now(NULL);
        int ghost = 0;
        for (int y = 0; y < 128; y++)
            for (int x = 0; x < 160; x++)
                if (snap[y * 160 + x] != reinterpret_cast<const uint16_t*>(fb + y * stride)[x]) ghost++;
        if (ghost) {
            if (ghost_steps < 5) printf("t=%u: incremental frame differs from a full redraw in %d pixels\n", t, ghost);
            ghost_steps++;
        }
    }
    const bool ok = bad_area == 0 && panel_steps == 0 && ghost_steps == 0 && scroll_steps == 0;
    printf("%s: flushes=%d bad_areas=%d panel_mismatch_steps=%d ghost_steps=%d scrollable_steps=%d\n", ok ? "panel check ok" : "PANEL CHECK FAILED",
           flushes, bad_area, panel_steps, ghost_steps, scroll_steps);
    return ok ? 0 : 1;
}
