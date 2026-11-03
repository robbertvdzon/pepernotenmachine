// Off-target run of the REAL ui.cpp (menus, maintenance screens, home screen) on a host build of LVGL, with the
// controller and BLE replaced by stubs. A scripted encoder session is played; frames are written as PPM files and
// the stub records what the UI asked the controller to do (asserted at the end).
//   see test/offtarget/run.sh
#include <lvgl.h>
#include <stdio.h>
#include <string.h>

#include "ble_link.h"
#include "ble_scanner.h"
#include "controller.h"
#include "device_store.h"
#include "ui.h"

uint32_t stub_millis = 0;
SerialStub Serial;

// ---- what the stubbed controller records / is told to report ----
static bool s_maintenance = false;
static bool s_held = false;
static int s_release_count = 0;
static RunState s_dispense = RunState::Idle;
static Msg s_msg = Msg::None;
static uint32_t s_msg_at = 0;
static bool s_needs_confirm = false;
static int s_held_true_count = 0;
static int s_motor_dir = 0;
static int s_motor_up_presses = 0, s_motor_down_presses = 0;
static RunState s_motor = RunState::Idle;

void controller_set_maintenance(bool on) { s_maintenance = on; }
static int s_dispense_dir = 0, s_fwd_presses = 0, s_rev_presses = 0;
void controller_set_dispense_hold(int8_t dir) {
    if (dir > 0 && s_dispense_dir <= 0) s_fwd_presses++;
    if (dir < 0 && s_dispense_dir >= 0) s_rev_presses++;
    if (dir != 0 && !s_held) s_held_true_count++;
    s_dispense_dir = dir;
    s_held = dir != 0;
}
static bool s_dur_valid = true, s_dur_busy = false;
static uint8_t s_dur = 12, s_dur_saved_value = 0;
static int s_dur_reads = 0, s_dur_saves = 0;
void controller_request_duration_read() { s_dur_reads++; }
void controller_request_duration_save(uint8_t v) { s_dur_saves++; s_dur_saved_value = v; }
bool controller_duration(uint8_t* seconds) { if (seconds) *seconds = s_dur; return s_dur_valid; }
bool controller_duration_busy() { return s_dur_busy; }
static bool s_sw_valid = true, s_sw_pressed = false;
bool controller_bucket_switch(bool* pressed) { if (pressed) *pressed = s_sw_pressed; return s_sw_valid; }
void controller_request_release() { s_release_count++; }
RunState controller_dispense_state() { return s_dispense; }
void controller_set_motor_hold(int8_t dir) {
    if (dir > 0 && s_motor_dir <= 0) s_motor_up_presses++;
    if (dir < 0 && s_motor_dir >= 0) s_motor_down_presses++;
    s_motor_dir = dir;
}
RunState controller_motor_state() { return s_motor; }
Msg controller_last_message(uint32_t* at) { if (at) *at = s_msg_at; return s_msg; }
bool controller_needs_confirm(CheckReason* r) { if (r) *r = CheckReason::None; return s_needs_confirm; }
void controller_request_check() {}
static Answer s_answer = Answer::None;
void controller_answer(Answer a) { s_answer = a; s_needs_confirm = false; }
Known controller_stored_state() { return Known::Unknown; }
void controller_home_inputs(HomeInputs* h) { h->launcher = LauncherStatus::Ready; h->switchboardConnected = true; }

bool device_store_load(DeviceKind, StoredDevice& d) { memset(&d, 0, sizeof(d)); d.valid = true; return true; }
bool device_store_save(DeviceKind, const StoredDevice&) { return true; }
bool device_store_erase(DeviceKind) { return true; }
void ble_link_reload() {}
bool ble_scanner_start(uint32_t) { return false; }
void ble_scanner_stop() {}
bool ble_scanner_is_scanning() { return false; }
size_t ble_scanner_count() { return 0; }
const ScanEntry& ble_scanner_entry(size_t) { static ScanEntry e; return e; }

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

static int failures = 0;
#define EXPECT(cond)                                              \
    do {                                                          \
        if (!(cond)) {                                            \
            printf("EXPECT FAILED line %d: %s\n", __LINE__, #cond); \
            failures++;                                           \
        }                                                         \
    } while (0)

static uint32_t t = 0;
static void settle(uint32_t ms = 200) {  // runs the UI for a while (LVGL reads the encoder every 33 ms)
    for (uint32_t e = 0; e < ms; e += 10) {
        t += 10;
        stub_millis = t;
        ui_update(t);
        lv_timer_handler();
    }
}
static void turn(int n) { ui_on_encoder(n); settle(); }
static void push() { ui_on_push(true); settle(120); ui_on_push(false); settle(); }

int main(int argc, char** argv) {
    const char* dir = argc > 1 ? argv[1] : ".";
    lv_init();
    lv_tick_set_cb([]() -> uint32_t { return stub_millis; });
    lv_display_t* disp = lv_display_create(160, 128);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
    stride = lv_draw_buf_width_to_stride(160, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(disp, fb, NULL, sizeof(fb), LV_DISPLAY_RENDER_MODE_DIRECT);
    lv_display_set_flush_cb(disp, flush_cb);
    ui_init();
    settle(500);
    dump(dir, "01_home");

    EXPECT(!s_maintenance);
    push();                                  // home -> menu
    settle(300);
    dump(dir, "02_menu");
    turn(1);                                 // Maintenance
    dump(dir, "03_menu_maintenance_focused");
    turn(2);                                 // Back: the list has to scroll
    dump(dir, "04_menu_scrolled_to_back");
    turn(-2);                                // Maintenance again
    EXPECT(!s_maintenance);
    push();                                  // -> Maintenance screen
    settle(300);
    EXPECT(s_maintenance);
    dump(dir, "05_maintenance");

    push();                                  // Release bucket -> confirmation (Cancel is first)
    settle(300);
    EXPECT(s_maintenance && s_release_count == 0);
    dump(dir, "06_confirm_release");
    push();                                  // Cancel: nothing is released
    settle(300);
    EXPECT(s_release_count == 0);
    push();                                  // again -> confirmation
    settle(300);
    turn(1);                                 // "Yes, release"
    dump(dir, "07_confirm_release_yes_focused");
    s_msg = Msg::ReleaseOk; s_msg_at = t + 400;   // the controller will report the result a moment later
    push();                                  // -> released, back on the maintenance screen
    EXPECT(s_release_count == 1);
    settle(600);
    dump(dir, "08_maintenance_released_message");

    turn(1);                                 // Dispense
    push();
    settle(300);
    EXPECT(s_maintenance && !s_held);
    dump(dir, "09_dispense");
    ui_on_push(true);                        // knob down on the "Hold to dispense" row
    settle(150);
    EXPECT(s_held && s_held_true_count == 1);
    s_dispense = RunState::Running;
    settle(300);
    dump(dir, "10_dispense_running");
    ui_on_push(false);                       // released
    settle(150);
    EXPECT(!s_held);
    s_dispense = RunState::Stopping;
    settle(200);
    dump(dir, "11_dispense_stopping");
    s_dispense = RunState::Idle;

    turn(1);                                 // "Hold to reverse"
    dump(dir, "11a_dispense_reverse_focused");
    ui_on_push(true);
    settle(150);
    EXPECT(s_dispense_dir == -1 && s_rev_presses == 1 && s_fwd_presses == 1);
    ui_on_encoder(-1);                       // turning while held must not switch to "Hold to dispense"
    settle(200);
    EXPECT(s_dispense_dir == -1 && s_fwd_presses == 1);
    s_dispense = RunState::Running;
    settle(300);
    dump(dir, "11a2_dispense_reversing");
    ui_on_push(false);
    settle(150);
    EXPECT(s_dispense_dir == 0);
    s_dispense = RunState::Idle;
    settle(200);
    turn(1);                                 // Back row: pushing it must NOT dispense
    ui_on_push(true);
    settle(150);
    EXPECT(!s_held);
    ui_on_push(false);
    settle(300);                             // -> back on the maintenance screen
    EXPECT(s_maintenance);
    // ---- motor control (from the maintenance screen: Release, Dispense, Motor control, Back)
    turn(3);                                 // focus: Motor control (Release, Dispense, Dispenser time, Motor control)
    dump(dir, "11b_maintenance_motor_focused");
    push();
    settle(300);
    EXPECT(s_maintenance && s_motor_dir == 0);
    dump(dir, "11c_motor");
    ui_on_push(true);                        // knob down on "Hold: up"
    settle(150);
    EXPECT(s_motor_dir == 1 && s_motor_up_presses == 1);
    ui_on_encoder(1);                        // turning while it is held must NOT switch to "down"
    settle(200);
    EXPECT(s_motor_dir == 1 && s_motor_down_presses == 0);
    s_motor = RunState::Running;
    settle(300);
    dump(dir, "11d_motor_running_up");
    ui_on_push(false);
    settle(150);
    EXPECT(s_motor_dir == 0);
    s_motor = RunState::Stopping;
    settle(200);
    dump(dir, "11e_motor_stopping");
    s_motor = RunState::Idle;
    turn(1);                                 // focus "Hold: down"
    ui_on_push(true);                        // knob down on it
    settle(150);
    EXPECT(s_motor_dir == -1 && s_motor_down_presses == 1);
    ui_on_back();                            // leaving the screen while it is held: not held any more
    settle(300);
    EXPECT(s_motor_dir == 0 && s_maintenance);
    ui_on_push(false);                       // letting go on the new screen must not click its focused row
    settle(300);
    EXPECT(s_maintenance && s_release_count == 1);   // still the maintenance screen, nothing was activated

    // ---- the bottom switch (read only): menu -> Maintenance -> Bottom switch
    ui_on_back();                            // -> menu
    settle(300);
    turn(1);                                 // Maintenance
    push();
    settle(300);
    // ---- dispenser time: turn = change, push = save
    turn(2);                                 // Release, Dispense, Dispenser time
    push();
    settle(400);
    EXPECT(s_maintenance && s_dur_reads == 1);   // the reading is refreshed when the screen opens
    dump(dir, "11j_duration_open");
    turn(3);                                 // 12 -> 15, nothing is sent by turning
    EXPECT(s_dur_saves == 0);
    dump(dir, "11k_duration_unsaved");
    push();                                  // the only row: save
    settle(300);
    EXPECT(s_dur_saves == 1 && s_dur_saved_value == 15);
    s_dur_busy = true;
    settle(200);
    dump(dir, "11l_duration_saving");
    s_dur_busy = false;
    s_dur = 15;                              // the launcher reports the new value: saved
    settle(300);
    dump(dir, "11m_duration_saved");
    turn(-100);                              // limited to 1..60
    push();
    settle(300);
    EXPECT(s_dur_saves == 2 && s_dur_saved_value == 1);
    turn(500);
    push();
    settle(300);
    EXPECT(s_dur_saves == 3 && s_dur_saved_value == 60);
    s_dur_valid = false;                     // no reading: nothing to change, nothing to save
    settle(3500);
    dump(dir, "11n_duration_no_reading");
    turn(5);
    push();
    settle(300);
    EXPECT(s_dur_saves == 3);
    s_dur_valid = true;
    s_dur = 12;
    ui_on_back();                            // the Back button leaves without saving
    settle(300);
    EXPECT(s_maintenance && s_dur_saves == 3);
    turn(4);                                 // Release, Dispense, Dispenser time, Motor control, Bottom switch
    dump(dir, "11f_maintenance_switch_focused");
    push();
    settle(400);
    EXPECT(s_maintenance);
    dump(dir, "11g_switch_released");
    s_sw_pressed = true;
    settle(300);
    dump(dir, "11h_switch_pressed");
    s_sw_valid = false;
    settle(300);
    dump(dir, "11i_switch_no_reading");
    s_sw_valid = true;
    s_sw_pressed = false;
    ui_on_back();                            // -> Maintenance
    settle(300);
    EXPECT(s_maintenance);

    ui_on_back();                            // -> menu
    settle(300);
    EXPECT(!s_maintenance);
    s_msg = Msg::DispenseFailed; s_msg_at = t;
    ui_on_back();                            // -> home
    settle(500);
    dump(dir, "12_home_again");
    EXPECT(!s_maintenance && !s_held);

    // ---- the launcher check: Bucket empty / Bucket full / Not sure?
    s_needs_confirm = true;
    settle(500);
    dump(dir, "13_check_empty_focused");
    turn(1);
    dump(dir, "14_check_full_focused");
    turn(1);
    dump(dir, "15_check_unsure_focused");
    push();                                  // "Not sure?" explains how to check and answers nothing
    settle(300);
    dump(dir, "16_check_help");
    EXPECT(s_answer == Answer::None);
    push();                                  // OK: back to the question
    settle(500);
    EXPECT(s_answer == Answer::None && s_needs_confirm);
    turn(-1);
    turn(-1);                                // wraps around to the first row if needed
    for (int i = 0; i < 3 && s_answer == Answer::None; i++) { push(); settle(300); if (s_answer == Answer::None) turn(1); }
    EXPECT(s_answer == Answer::Empty || s_answer == Answer::Full);
    settle(400);

    printf("%s (%d failed checks)\n", failures ? "UI SESSION FAILED" : "UI session ok", failures);
    return failures ? 1 : 0;
}
