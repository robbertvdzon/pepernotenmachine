// Off-target run of the real home screen code (ui_home.cpp + model + sprites) on a host build of LVGL:
// a scripted launcher session is played, frames are written as PPM files for a visual check.
//   see test/offtarget/run.sh
#include <lvgl.h>
#include <stdio.h>
#include <string.h>

#include "home_config.h"
#include "ui_home.h"

uint32_t stub_millis = 0;

alignas(64) static uint8_t fb[160 * 4 * 128];
static uint32_t stride = 0;

static void flush_cb(lv_display_t* d, const lv_area_t*, uint8_t*) { lv_display_flush_ready(d); }

static void dump(const char* dir, const char* name) {
    char path[256];
    snprintf(path, sizeof(path), "%s/%s.ppm", dir, name);
    FILE* f = fopen(path, "wb");
    if (!f) return;
    fprintf(f, "P6\n160 128\n255\n");
    for (int y = 0; y < 128; y++) {
        const uint16_t* row = reinterpret_cast<const uint16_t*>(fb + y * stride);
        for (int x = 0; x < 160; x++) {
            const uint16_t c = row[x];
            const unsigned char rgb[3] = {static_cast<unsigned char>(((c >> 11) & 31) * 255 / 31),
                                          static_cast<unsigned char>(((c >> 5) & 63) * 255 / 63),
                                          static_cast<unsigned char>((c & 31) * 255 / 31)};
            fwrite(rgb, 1, 3, f);
        }
    }
    fclose(f);
}

int main(int argc, char** argv) {
    const char* dir = argc > 1 ? argv[1] : ".";
    lv_init();
    lv_tick_set_cb([]() -> uint32_t { return stub_millis; });
    lv_display_t* disp = lv_display_create(160, 128);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
    stride = lv_draw_buf_width_to_stride(160, LV_COLOR_FORMAT_RGB565);
    if (stride * 128 > sizeof(fb)) { printf("frame buffer too small\n"); return 1; }
    lv_display_set_buffers(disp, fb, NULL, sizeof(fb), LV_DISPLAY_RENDER_MODE_DIRECT);
    lv_display_set_flush_cb(disp, flush_cb);

    lv_obj_t* scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);
    ui_home_build(scr);

    struct Sample { uint32_t at; const char* name; };
    const Sample samples[] = {
        {500, "01_offline"}, {1500, "02_starting"}, {2600, "03_down_start"}, {10000, "04_toast_not_ready"},
        {14600, "05_down_half"}, {27400, "06_lock_open"}, {28300, "07_lock_click"}, {40000, "08_up"},
        {53000, "09_ready"}, {56200, "10_pending_hold_ready"}, {56700, "11_launched_a"}, {57000, "12_launched_b"},
        {57300, "13_launched_c"}, {57900, "14_launched_d"}, {58600, "15_rearm_3"}, {59400, "16_rearm_2"},
        {61000, "17_starting_again"}, {70000, "18_board_offline_toast"}};
    size_t next = 0;

    HomeInputs in;
    uint8_t okCount = 0;
    for (uint32_t t = 0; t <= 70100; t += 20) {
        stub_millis = t;
        in.now = t;
        in.switchboardConnected = !(t >= 62000);
        in.dispenserValid = true;
        in.dispenserOn = (t >= 29000 && t < 52000);
        in.flashLed = t >= 3000 ? LED_ON : LED_OFF;
        in.lampLed = t >= 3000 ? LED_ON : LED_OFF;
        in.fogLed = t < 6000 ? LED_FLASH : (t < 20000 ? LED_ON : LED_PULSE);
        in.panicLed = (t >= 40000 && t < 45000) ? LED_ON : LED_OFF;
        in.sirenLed = (t >= 12000 && t < 14000) ? LED_ON : LED_OFF;
        // launcher script
        if (t < 1000) { in.launcher = LauncherStatus::Offline; in.prepValid = false; }
        else if (t < 2000) { in.launcher = LauncherStatus::Preparing; in.prepValid = true; in.prep = 0; }
        else if (t < 27000) { in.launcher = LauncherStatus::Preparing; in.prep = 1; }
        else if (t < 52000) { in.launcher = LauncherStatus::Preparing; in.prep = t < 29000 ? 2 : 3; }
        else if (t < 56000) { in.launcher = LauncherStatus::Ready; in.prep = 0; }
        else if (t < 56500) { in.launcher = LauncherStatus::Rearming; in.launchPending = true; in.rearmInMs = 60000 - t; }
        else if (t < 60000) { in.launcher = LauncherStatus::Rearming; in.launchPending = false; in.rearmInMs = 60000 - t; if (okCount == 0) okCount = 1; }
        else { in.launcher = LauncherStatus::Preparing; in.prep = 0; in.rearmInMs = 0; }
        in.launchOkCount = okCount;
        if (t == 9000) { in.message = Msg::LaunchNotReady; in.messageAt = t; }
        ui_home_update(in, true);
        lv_timer_handler();
        if (next < sizeof(samples) / sizeof(samples[0]) && t >= samples[next].at) {
            dump(dir, samples[next].name);
            next++;
        }
    }
    printf("frames written: %zu\n", next);
    return 0;
}
