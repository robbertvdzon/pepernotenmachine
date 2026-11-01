#include "ui.h"

#include <lvgl.h>
#include <stdio.h>
#include <string.h>

#include "ble_address.h"
#include "ble_link.h"
#include "ble_scanner.h"
#include "config.h"
#include "controller.h"
#include "device_store.h"
#include "fonts.h"
#include "home_model.h"
#include "link_state.h"
#include "ui_compat.h"
#include "ui_home.h"
#include "ui_strings.h"
#include "ui_theme.h"

namespace {

// ============================================================= layout numbers
// Everything derives from the real font metrics (see fonts.h) and is
// static_assert-checked, so a layout that does not fit does not build.
constexpr int MARGIN = 4;
constexpr int SAFE_BOTTOM = 8;  // keep clear of the bottom edge
constexpr int TITLE_Y = 2;
constexpr int CONTENT_W = DISPLAY_WIDTH - 2 * MARGIN;
constexpr int BAR_W = 4;

// menu rows
constexpr int ROW_H = 24;
constexpr int ROW_GAP = 4;
constexpr int ROW_TEXT_X = BAR_W + 6;
constexpr int INFO_Y = TITLE_Y + UI_FONT_NORMAL_H + 2;  // small info line under the title
constexpr int MENU_Y = INFO_Y + UI_FONT_SMALL_H + 5;    // first menu row
constexpr int MENU_LIST_TOP = TITLE_Y + UI_FONT_NORMAL_H + 5;  // scrolling menu list (no info line)
constexpr int MENU_LIST_HEIGHT = DISPLAY_HEIGHT - SAFE_BOTTOM - MENU_LIST_TOP;
constexpr int MAINT_STATUS_Y = 92;  // status text on the dispense screen
constexpr int SCAN_ROW_H = 28;                          // two-line device row
constexpr int SCAN_LIST_TOP = INFO_Y + UI_FONT_SMALL_H + 3;
constexpr int SCAN_LIST_HEIGHT = DISPLAY_HEIGHT - SAFE_BOTTOM - SCAN_LIST_TOP;

// launcher check overlay: title, one question line, three answers
constexpr int CHECK_QUESTION_Y = TITLE_Y + UI_FONT_NORMAL_H + 2;
constexpr int CHECK_ROW_GAP = 3;
constexpr int CHECK_ROW_Y = CHECK_QUESTION_Y + UI_FONT_NORMAL_H + 1;
constexpr int HELP_LINE_Y = TITLE_Y + UI_FONT_NORMAL_H + 5;
constexpr int HELP_OK_Y = HELP_LINE_Y + 3 * UI_FONT_NORMAL_H + 6;

constexpr int text_width(int chars) { return chars * UI_FONT_NORMAL_W; }
constexpr int small_width(int chars) { return chars * UI_FONT_SMALL_W; }

// (the length of every text is checked in ui_strings.h)
static_assert(text_width(20) <= DISPLAY_WIDTH, "a 20 character message does not fit the display");
static_assert(small_width(17) <= DISPLAY_WIDTH, "address line too wide");        // "AA:BB:CC:DD:EE:FF"
static_assert(small_width(17) <= CONTENT_W - ROW_TEXT_X, "device detail line too wide");  // "-127dBm pub UUID" = 16
static_assert(ROW_TEXT_X + text_width(17) <= CONTENT_W, "device address row too wide");  // "AA:BB:CC:DD:EE:FF"
static_assert(ROW_TEXT_X + text_width(ui_chars(STR_SWITCHBOARD)) + small_width(5) + 12 <= CONTENT_W,
              "STR_SWITCHBOARD is too long for the setup row (device name + saved/none)");
static_assert(ROW_TEXT_X + text_width(ui_chars(STR_LAUNCHER)) + small_width(5) + 12 <= CONTENT_W,
              "STR_LAUNCHER is too long for the setup row (device name + saved/none)");
static_assert(CHECK_ROW_Y + 3 * ROW_H + 2 * CHECK_ROW_GAP <= DISPLAY_HEIGHT - SAFE_BOTTOM, "check overlay does not fit");
static_assert(HELP_OK_Y + ROW_H <= DISPLAY_HEIGHT - SAFE_BOTTOM, "check help page does not fit");
static_assert(MENU_Y + 3 * ROW_H + 2 * ROW_GAP <= DISPLAY_HEIGHT - SAFE_BOTTOM, "three setup / device rows do not fit");
static_assert(MENU_LIST_HEIGHT >= 3 * ROW_H + 2 * ROW_GAP, "the scrolling menu must show at least three rows");
static_assert(MENU_Y + 2 * (ROW_H + ROW_GAP) + ROW_H <= MAINT_STATUS_Y + 2 * ROW_H, "dispense screen layout");
static_assert(SCAN_LIST_HEIGHT >= SCAN_ROW_H * 2, "scan list too short for two device rows");

// ================================================================== state
enum class Screen : uint8_t { Home, Menu, Setup, Device, Scan, Confirm, Check, CheckHelp, Maintenance, ConfirmRelease, Dispense, Motor };

struct NavEntry {
    Screen screen;
    DeviceKind kind;
};
constexpr int NAV_MAX = 6;
NavEntry nav[NAV_MAX];
int nav_depth = 0;  // screens BELOW the current one
Screen cur_id = Screen::Home;
DeviceKind cur_kind = DeviceKind::Unknown;
lv_obj_t* home_obj = nullptr;
lv_obj_t* cur_obj = nullptr;
lv_obj_t* cur_list = nullptr;  // scrolling list of the current screen (rows live inside it), if any
lv_group_t* group = nullptr;
lv_indev_t* indev = nullptr;

// Row actions. A row's click only RECORDS the action; it is executed from
// ui_update(), because screens must not be deleted inside their own event.
enum Action : intptr_t {
    ACT_NONE = 0,
    ACT_SETUP_LAUNCHER,
    ACT_SETUP_SWITCHBOARD,
    ACT_BACK,
    ACT_DEVICE_SCAN,
    ACT_DEVICE_FORGET,
    ACT_CONFIRM_YES,
    ACT_SCAN_AGAIN,
    ACT_MENU_CHECK,
    ACT_MENU_SETUP,
    ACT_MENU_MAINT,
    ACT_MAINT_RELEASE,
    ACT_MAINT_DISPENSE,
    ACT_MAINT_MOTOR,
    ACT_RELEASE_YES,
    ACT_ANS_EMPTY,
    ACT_ANS_FULL,
    ACT_ANS_UNSURE,
    ACT_SCAN_PICK = 100  // + scanner entry index
};
intptr_t pending_action = ACT_NONE;

// input
int32_t pending_diff = 0;
bool push_down = false;
uint32_t last_now = 0;
uint32_t screen_since = 0;  // when the current screen opened
uint32_t screen_epoch = 0;  // counts screen changes
uint32_t press_epoch = 0;   // screen_epoch when the knob went down

// launcher check overlay
CheckReason check_reason = CheckReason::Startup;
lv_obj_t* check_rows[3] = {nullptr, nullptr, nullptr};

// scan screen
enum class ScanPhase : uint8_t { Starting, Scanning, Done, Failed };
struct ScanCtx {
    DeviceKind kind = DeviceKind::Unknown;
    ScanPhase phase = ScanPhase::Starting;
    uint32_t phaseStartMs = 0;
    uint32_t nextTryMs = 0;
    uint32_t giveUpMs = 0;
    size_t seen = 0;  // scanner entries already looked at
    lv_obj_t* list = nullptr;
    lv_obj_t* status = nullptr;
    lv_obj_t* again = nullptr;  // "Scan again": only visible while no scan is running
    uint8_t rowCount = 0;
    lv_obj_t* rows[SCAN_MAX_ENTRIES] = {nullptr};
    lv_obj_t* detail[SCAN_MAX_ENTRIES] = {nullptr};
    uint8_t entryIdx[SCAN_MAX_ENTRIES] = {0};
    bool userMoved = false;
    char shownStatus[24] = {0};
};
ScanCtx scan;

// ============================================================ small helpers
uint32_t accent_for(DeviceKind k) {
    return k == DeviceKind::Switchboard ? ui_color::SWITCHBOARD : ui_color::LAUNCHER;
}
const char* name_for(DeviceKind k) { return k == DeviceKind::Switchboard ? STR_SWITCHBOARD : STR_LAUNCHER; }

const char* addr_type_short(uint8_t t) {
    switch (t) {
        case 0: return "pub";
        case 1: return "rnd";
        case 2: return "pid";
        case 3: return "rid";
        default: return "?";
    }
}

// A click only counts if the knob went down on this very screen: when the screen changed while it was held (e.g. the
// Back button), letting go must not activate whatever row is focused on the new screen.
void row_clicked(lv_event_t* e) {
    if (press_epoch != screen_epoch) return;
    pending_action = reinterpret_cast<intptr_t>(lv_event_get_user_data(e));
}

lv_obj_t* make_screen(const char* title, uint32_t title_color) {
    lv_obj_t* scr = lv_obj_create(nullptr);
    ui_set_scrollable(scr, false);
    lv_obj_set_style_bg_color(scr, lv_color_hex(ui_color::BACKGROUND), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_text_font(scr, UI_FONT_NORMAL, LV_PART_MAIN);  // inherited by all children
    lv_obj_set_style_text_color(scr, lv_color_hex(ui_color::TEXT), LV_PART_MAIN);

    lv_obj_t* t = lv_label_create(scr);
    lv_label_set_text(t, title);
    lv_obj_set_style_text_color(t, lv_color_hex(title_color), LV_PART_MAIN);
    lv_obj_align(t, LV_ALIGN_TOP_MID, 0, TITLE_Y);
    return scr;
}

lv_obj_t* make_info(lv_obj_t* scr, const char* text) {
    lv_obj_t* l = lv_label_create(scr);
    lv_obj_set_style_text_font(l, UI_FONT_SMALL, LV_PART_MAIN);
    lv_obj_set_style_text_color(l, lv_color_hex(ui_color::TEXT_MUTED), LV_PART_MAIN);
    lv_label_set_text(l, text);
    lv_obj_align(l, LV_ALIGN_TOP_MID, 0, INFO_Y);
    return l;
}

// A focusable row: dark card with a colored bar on the left; the focused row
// turns accent colored. `y < 0` = position is left to a flex parent.
lv_obj_t* make_row(lv_obj_t* parent, int y, int h, uint32_t bar_color, intptr_t action) {
    lv_obj_t* row = lv_obj_create(parent);
    ui_set_scrollable(row, false);
    ui_set_scroll_on_focus(row, true);
    lv_obj_set_size(row, CONTENT_W, h);
    if (y >= 0) lv_obj_set_pos(row, MARGIN, y);
    lv_obj_set_style_bg_color(row, lv_color_hex(ui_color::CARD), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(row, 0, LV_PART_MAIN);
    lv_obj_set_style_outline_width(row, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(row, 4, LV_PART_MAIN);
    lv_obj_set_style_pad_all(row, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(row, 0, LV_PART_MAIN);
    // The default theme gives lv_obj a dark text color; labels inherit from the row, so set it here.
    lv_obj_set_style_text_color(row, lv_color_hex(ui_color::TEXT), LV_PART_MAIN);
    // focus: encoder focus sets FOCUSED (and FOCUS_KEY); the default theme would add an outline
    lv_obj_set_style_bg_color(row, lv_color_hex(ui_color::ACCENT), LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(row, 0, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(row, 0, LV_PART_MAIN | LV_STATE_FOCUS_KEY);

    lv_obj_t* bar = lv_obj_create(row);
    ui_set_scrollable(bar, false);
    ui_set_clickable(bar, false);
    lv_obj_set_size(bar, BAR_W, h);
    lv_obj_set_pos(bar, 0, 0);
    lv_obj_set_style_bg_color(bar, lv_color_hex(bar_color), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(bar, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(bar, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(bar, 0, LV_PART_MAIN);

    lv_obj_set_user_data(row, reinterpret_cast<void*>(1));  // marks a focusable row
    // RELEASED (not CLICKED): CLICKED is not sent when the button was held longer than LVGL's
    // long-press time, so a slow tap would be ignored. Also keeps this free of version-specific API.
    lv_obj_add_event_cb(row, row_clicked, LV_EVENT_RELEASED, reinterpret_cast<void*>(action));
    return row;
}

lv_obj_t* make_menu_row(lv_obj_t* scr, int y, const char* text, const char* right, uint32_t right_color,
                        uint32_t bar_color, intptr_t action) {
    lv_obj_t* row = make_row(scr, y, ROW_H, bar_color, action);
    lv_obj_t* l = lv_label_create(row);
    lv_label_set_text(l, text);
    lv_obj_set_pos(l, ROW_TEXT_X, (ROW_H - UI_FONT_NORMAL_H) / 2);
    if (right) {
        lv_obj_t* r = lv_label_create(row);
        lv_label_set_text(r, right);
        lv_obj_set_style_text_font(r, UI_FONT_SMALL, LV_PART_MAIN);
        lv_obj_set_style_text_color(r, lv_color_hex(right_color), LV_PART_MAIN);
        lv_obj_align(r, LV_ALIGN_RIGHT_MID, -6, 0);
    }
    return row;
}

// A scrolling column of rows below `top` (the encoder scrolls it by moving the focus). Rows are created
// inside it with y = -1.
lv_obj_t* make_list(lv_obj_t* scr, int top, int pad_row) {
    lv_obj_t* list = lv_obj_create(scr);
    lv_obj_set_size(list, CONTENT_W, DISPLAY_HEIGHT - SAFE_BOTTOM - top);
    lv_obj_set_pos(list, MARGIN, top);
    lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(list, 0, LV_PART_MAIN);
    lv_obj_set_style_outline_width(list, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(list, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_row(list, pad_row, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(list, LV_DIR_VER);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    return list;
}

// Puts every focusable row below `root` (in child order) into the encoder group.
void regroup(lv_obj_t* root) {
    lv_group_remove_all_objs(group);
    lv_obj_t* first = nullptr;
    const uint32_t n = lv_obj_get_child_count(root);
    for (uint32_t i = 0; i < n; i++) {
        lv_obj_t* c = lv_obj_get_child(root, i);
        if (lv_obj_get_user_data(c) && !ui_is_hidden(c)) {
            lv_group_add_obj(group, c);
            if (!first) first = c;
        }
    }
    if (first) lv_group_focus_obj(first);
}

// ============================================================ screen builders
lv_obj_t* build_setup() {
    lv_obj_t* scr = make_screen(STR_SETUP_TITLE, ui_color::ACCENT);
    make_info(scr, STR_SETUP_INFO);
    StoredDevice d;
    const bool l = device_store_load(DeviceKind::Launcher, d);
    const bool s = device_store_load(DeviceKind::Switchboard, d);
    // right side: "saved" (green) or "none" (muted)
    make_menu_row(scr, MENU_Y, STR_LAUNCHER, l ? STR_SAVED : STR_NONE, l ? ui_color::OK : ui_color::TEXT_DIM,
                  ui_color::LAUNCHER, ACT_SETUP_LAUNCHER);
    make_menu_row(scr, MENU_Y + ROW_H + ROW_GAP, STR_SWITCHBOARD, s ? STR_SAVED : STR_NONE,
                  s ? ui_color::OK : ui_color::TEXT_DIM, ui_color::SWITCHBOARD, ACT_SETUP_SWITCHBOARD);
    make_menu_row(scr, MENU_Y + 2 * (ROW_H + ROW_GAP), STR_BACK, nullptr, 0, ui_color::TEXT_MUTED, ACT_BACK);
    return scr;
}

lv_obj_t* build_device(DeviceKind kind) {
    lv_obj_t* scr = make_screen(name_for(kind), accent_for(kind));
    StoredDevice d;
    const bool saved = device_store_load(kind, d);
    char addr[18] = STR_NOT_SET;
    if (saved) ble_address_format(d.addr, addr, sizeof(addr));
    make_info(scr, addr);

    int y = MENU_Y;
    make_menu_row(scr, y, STR_SCAN_SELECT, nullptr, 0, accent_for(kind), ACT_DEVICE_SCAN);
    y += ROW_H + ROW_GAP;
    if (saved) {
        make_menu_row(scr, y, STR_FORGET, nullptr, 0, ui_color::ERROR, ACT_DEVICE_FORGET);
        y += ROW_H + ROW_GAP;
    }
    make_menu_row(scr, y, STR_BACK, nullptr, 0, ui_color::TEXT_MUTED, ACT_BACK);
    return scr;
}

lv_obj_t* build_confirm(DeviceKind kind) {
    lv_obj_t* scr = make_screen(kind == DeviceKind::Switchboard ? STR_FORGET_SWITCHBOARD_TITLE : STR_FORGET_LAUNCHER_TITLE,
                                ui_color::ERROR);
    StoredDevice d;
    char addr[18] = "";
    if (device_store_load(kind, d)) ble_address_format(d.addr, addr, sizeof(addr));
    make_info(scr, addr);
    make_menu_row(scr, MENU_Y, STR_CANCEL, nullptr, 0, ui_color::TEXT_MUTED, ACT_BACK);  // safe choice first
    make_menu_row(scr, MENU_Y + ROW_H + ROW_GAP, STR_YES_FORGET, nullptr, 0, ui_color::ERROR, ACT_CONFIRM_YES);
    return scr;
}

lv_obj_t* build_scan(DeviceKind kind) {
    lv_obj_t* scr = make_screen(kind == DeviceKind::Switchboard ? STR_SCAN_SWITCHBOARD_TITLE : STR_SCAN_LAUNCHER_TITLE,
                                accent_for(kind));
    scan = ScanCtx();
    scan.kind = kind;
    scan.status = make_info(scr, STR_SCAN_STARTING);
    scan.phase = ScanPhase::Starting;
    scan.nextTryMs = 0;
    scan.giveUpMs = last_now + 10000;

    lv_obj_t* list = make_list(scr, SCAN_LIST_TOP, 3);
    scan.list = list;

    // Fixed rows below the devices that will appear above them.
    lv_obj_t* again = make_row(list, -1, ROW_H, accent_for(kind), ACT_SCAN_AGAIN);
    ui_set_hidden(again, true);  // shown when the scan has finished
    scan.again = again;
    lv_obj_t* al = lv_label_create(again);
    lv_label_set_text(al, STR_SCAN_AGAIN);
    lv_obj_set_pos(al, ROW_TEXT_X, (ROW_H - UI_FONT_NORMAL_H) / 2);
    lv_obj_t* bk = make_row(list, -1, ROW_H, ui_color::TEXT_MUTED, ACT_BACK);
    lv_obj_t* bl = lv_label_create(bk);
    lv_label_set_text(bl, STR_BACK);
    lv_obj_set_pos(bl, ROW_TEXT_X, (ROW_H - UI_FONT_NORMAL_H) / 2);
    return scr;
}

lv_obj_t* build_menu() {
    lv_obj_t* scr = make_screen(STR_MENU_TITLE, ui_color::ACCENT);
    cur_list = make_list(scr, MENU_LIST_TOP, ROW_GAP);  // scrolls: more entries than fit
    make_menu_row(cur_list, -1, STR_MENU_CHECK, nullptr, 0, ui_color::LAUNCHER, ACT_MENU_CHECK);
    make_menu_row(cur_list, -1, STR_MENU_MAINTENANCE, nullptr, 0, ui_color::WARNING, ACT_MENU_MAINT);
    make_menu_row(cur_list, -1, STR_MENU_SETUP, nullptr, 0, ui_color::ACCENT, ACT_MENU_SETUP);
    make_menu_row(cur_list, -1, STR_BACK, nullptr, 0, ui_color::TEXT_MUTED, ACT_BACK);
    return scr;
}

// ---- maintenance: pauses all automation (the logic knows through controller_set_maintenance)
lv_obj_t* maint_info = nullptr;    // info line: hint, or the latest message
lv_obj_t* dispense_row = nullptr;  // the knob must be held on this row
lv_obj_t* dispense_status = nullptr;
lv_obj_t* motor_up_row = nullptr;    // hold-to-run rows of the motor screen
lv_obj_t* motor_down_row = nullptr;
lv_obj_t* hold_row = nullptr;        // the hold-to-run row the knob went down on (stays the same until it is released)
bool push_was_down = false;

lv_obj_t* build_maintenance() {
    lv_obj_t* scr = make_screen(STR_MAINT_TITLE, ui_color::WARNING);
    maint_info = make_info(scr, STR_MAINT_INFO);
    cur_list = make_list(scr, SCAN_LIST_TOP, ROW_GAP);
    make_menu_row(cur_list, -1, STR_MAINT_RELEASE, nullptr, 0, ui_color::ERROR, ACT_MAINT_RELEASE);
    make_menu_row(cur_list, -1, STR_MAINT_DISPENSE, nullptr, 0, ui_color::LAUNCHER, ACT_MAINT_DISPENSE);
    make_menu_row(cur_list, -1, STR_MAINT_MOTOR, nullptr, 0, ui_color::LAUNCHER, ACT_MAINT_MOTOR);
    make_menu_row(cur_list, -1, STR_BACK, nullptr, 0, ui_color::TEXT_MUTED, ACT_BACK);
    return scr;
}

lv_obj_t* build_confirm_release() {
    lv_obj_t* scr = make_screen(STR_RELEASE_TITLE, ui_color::ERROR);
    maint_info = make_info(scr, STR_RELEASE_INFO);
    make_menu_row(scr, MENU_Y, STR_CANCEL, nullptr, 0, ui_color::TEXT_MUTED, ACT_BACK);  // safe choice first
    make_menu_row(scr, MENU_Y + ROW_H + ROW_GAP, STR_YES_RELEASE, nullptr, 0, ui_color::ERROR, ACT_RELEASE_YES);
    return scr;
}

lv_obj_t* build_motor() {
    lv_obj_t* scr = make_screen(STR_MOTOR_TITLE, ui_color::LAUNCHER);
    maint_info = make_info(scr, STR_MOTOR_HINT);
    motor_up_row = make_menu_row(scr, MENU_Y, STR_MOTOR_UP, nullptr, 0, ui_color::LAUNCHER, ACT_NONE);
    motor_down_row = make_menu_row(scr, MENU_Y + ROW_H + ROW_GAP, STR_MOTOR_DOWN, nullptr, 0, ui_color::LAUNCHER, ACT_NONE);
    make_menu_row(scr, MENU_Y + 2 * (ROW_H + ROW_GAP), STR_BACK, nullptr, 0, ui_color::TEXT_MUTED, ACT_BACK);
    return scr;
}

lv_obj_t* build_dispense() {
    lv_obj_t* scr = make_screen(STR_DISPENSE_TITLE, ui_color::LAUNCHER);
    maint_info = make_info(scr, STR_DISPENSE_HINT);
    dispense_row = make_menu_row(scr, MENU_Y, STR_DISPENSE_ROW, nullptr, 0, ui_color::LAUNCHER, ACT_NONE);
    make_menu_row(scr, MENU_Y + ROW_H + ROW_GAP, STR_BACK, nullptr, 0, ui_color::TEXT_MUTED, ACT_BACK);
    dispense_status = lv_label_create(scr);
    lv_label_set_text(dispense_status, "");
    lv_obj_align(dispense_status, LV_ALIGN_TOP_MID, 0, MAINT_STATUS_Y);
    return scr;
}

// The launcher check: the unit cannot know the physical state of the machine, so the driver says.
// Two answers about the load (the position of the bucket does not matter) and "Not sure?", which explains how to
// check; the answer that matches what was believed at the last power-off is pre-selected.
lv_obj_t* build_check(CheckReason reason) {
    lv_obj_t* scr = make_screen(reason == CheckReason::Error ? STR_CHECK_ERROR_TITLE : STR_CHECK_TITLE,
                                reason == CheckReason::Error ? ui_color::ERROR : ui_color::WARNING);
    lv_obj_t* q = lv_label_create(scr);
    lv_label_set_text(q, STR_CHECK_QUESTION);
    lv_obj_align(q, LV_ALIGN_TOP_MID, 0, CHECK_QUESTION_Y);

    const int step = ROW_H + CHECK_ROW_GAP;
    check_rows[0] = make_menu_row(scr, CHECK_ROW_Y, STR_ANS_EMPTY, nullptr, 0, ui_color::LAUNCHER, ACT_ANS_EMPTY);
    check_rows[1] = make_menu_row(scr, CHECK_ROW_Y + step, STR_ANS_FULL, nullptr, 0, ui_color::OK, ACT_ANS_FULL);
    check_rows[2] = make_menu_row(scr, CHECK_ROW_Y + 2 * step, STR_ANS_UNSURE, nullptr, 0, ui_color::TEXT_MUTED, ACT_ANS_UNSURE);
    return scr;
}

lv_obj_t* build_check_help() {
    lv_obj_t* scr = make_screen(STR_HELP_TITLE, ui_color::WARNING);
    const char* lines[3] = {STR_HELP_1, STR_HELP_2, STR_HELP_3};
    for (int i = 0; i < 3; i++) {
        lv_obj_t* l = lv_label_create(scr);
        lv_label_set_text(l, lines[i]);
        lv_obj_align(l, LV_ALIGN_TOP_MID, 0, HELP_LINE_Y + i * UI_FONT_NORMAL_H);
    }
    make_menu_row(scr, HELP_OK_Y, STR_OK, nullptr, 0, ui_color::ACCENT, ACT_BACK);
    return scr;
}

// ============================================================== navigation
void enter(Screen s, DeviceKind k) {
    if (cur_id == Screen::Scan && s != Screen::Scan) ble_scanner_stop();

    lv_group_remove_all_objs(group);  // old rows must leave the group before their screen is deleted
    lv_obj_t* scr = nullptr;
    cur_list = nullptr;
    maint_info = nullptr;
    dispense_row = nullptr;
    dispense_status = nullptr;
    motor_up_row = nullptr;
    motor_down_row = nullptr;
    hold_row = nullptr;
    switch (s) {
        case Screen::Home: scr = home_obj; break;
        case Screen::Menu: scr = build_menu(); break;
        case Screen::Setup: scr = build_setup(); break;
        case Screen::Device: scr = build_device(k); break;
        case Screen::Confirm: scr = build_confirm(k); break;
        case Screen::Scan: scr = build_scan(k); break;
        case Screen::Check: scr = build_check(check_reason); break;
        case Screen::CheckHelp: scr = build_check_help(); break;
        case Screen::Maintenance: scr = build_maintenance(); break;
        case Screen::ConfirmRelease: scr = build_confirm_release(); break;
        case Screen::Dispense: scr = build_dispense(); break;
        case Screen::Motor: scr = build_motor(); break;
    }
    lv_obj_t* old = cur_obj;
    screen_since = last_now;
    screen_epoch++;
    cur_id = s;
    cur_kind = k;
    cur_obj = scr;
    lv_screen_load(scr);
    if (old && old != home_obj && old != scr) lv_obj_delete(old);

    if (s == Screen::Scan) regroup(scan.list);
    else if (s != Screen::Home) regroup(cur_list ? cur_list : scr);

    // The answer that matches the last known state is pre-selected; the driver still has to push.
    if (s == Screen::Check && controller_stored_state() == Known::Armed) lv_group_focus_obj(check_rows[1]);
}

// The check overlay replaces whatever idle screen is open; it cannot be dismissed without an answer.
void open_check(CheckReason reason) {
    check_reason = reason;
    nav_depth = 0;
    enter(Screen::Check, DeviceKind::Unknown);
}

void close_check() {
    nav_depth = 0;
    enter(Screen::Home, DeviceKind::Unknown);
}

void go(Screen s, DeviceKind k) {
    if (nav_depth < NAV_MAX) {
        nav[nav_depth].screen = cur_id;
        nav[nav_depth].kind = cur_kind;
        nav_depth++;
    }
    enter(s, k);
}

void go_back() {
    if (nav_depth == 0) return;
    nav_depth--;
    enter(nav[nav_depth].screen, nav[nav_depth].kind);
}

// ================================================================== scan
void scan_set_status(const char* text) {
    if (strcmp(scan.shownStatus, text) == 0) return;
    strncpy(scan.shownStatus, text, sizeof(scan.shownStatus) - 1);
    lv_label_set_text(scan.status, text);
}

void scan_add_device_row(uint8_t entry_idx, const ScanEntry& e) {
    if (scan.rowCount >= SCAN_MAX_ENTRIES) return;
    lv_obj_t* row = make_row(scan.list, -1, SCAN_ROW_H, accent_for(scan.kind), ACT_SCAN_PICK + entry_idx);
    lv_obj_move_to_index(row, scan.rowCount);  // devices above the fixed "Scan again" / "Back" rows

    char addr[18];
    ble_address_format(e.addr, addr, sizeof(addr));
    lv_obj_t* a = lv_label_create(row);
    lv_label_set_text(a, addr);
    lv_obj_set_pos(a, ROW_TEXT_X, 0);
    lv_obj_t* d = lv_label_create(row);
    lv_obj_set_style_text_font(d, UI_FONT_SMALL, LV_PART_MAIN);
    lv_label_set_text(d, "");
    lv_obj_set_pos(d, ROW_TEXT_X, UI_FONT_NORMAL_H + 1);

    scan.rows[scan.rowCount] = row;
    scan.detail[scan.rowCount] = d;
    scan.entryIdx[scan.rowCount] = entry_idx;
    scan.rowCount++;

    lv_obj_t* focused = lv_group_get_focused(group);
    regroup(scan.list);
    // The first device found takes the focus, unless the user already moved it.
    if (scan.userMoved && focused) lv_group_focus_obj(focused);
}

void scan_sync_rows() {
    const size_t n = ble_scanner_count();
    for (size_t i = scan.seen; i < n; i++) {
        const ScanEntry& e = ble_scanner_entry(i);
        if (e.kind == scan.kind && !e.conflict) scan_add_device_row(static_cast<uint8_t>(i), e);
    }
    scan.seen = n;

    for (uint8_t r = 0; r < scan.rowCount; r++) {  // keep the signal strength current
        const ScanEntry& e = ble_scanner_entry(scan.entryIdx[r]);
        char text[24];
        snprintf(text, sizeof(text), "%ddBm %s %s", (int)e.rssi, addr_type_short(e.addrType), e.strong ? STR_SRC_UUID : STR_SRC_NAME);
        if (strcmp(lv_label_get_text(scan.detail[r]), text) != 0) lv_label_set_text(scan.detail[r], text);
    }
}

// "Scan again" is only offered while no scan is running (or starting).
void scan_update_again_visibility() {
    const bool should_show = (scan.phase == ScanPhase::Done || scan.phase == ScanPhase::Failed);
    const bool is_shown = !ui_is_hidden(scan.again);
    if (should_show == is_shown) return;

    lv_obj_t* focused = lv_group_get_focused(group);
    ui_set_hidden(scan.again, !should_show);
    regroup(scan.list);
    if (focused && !ui_is_hidden(focused)) lv_group_focus_obj(focused);
    // Nothing found and the user did not touch the dial: put the focus on the obvious next step.
    if (should_show && scan.rowCount == 0 && !scan.userMoved) lv_group_focus_obj(scan.again);
}

void scan_restart() {
    if (scan.phase == ScanPhase::Scanning || scan.phase == ScanPhase::Starting) return;
    for (uint8_t r = 0; r < scan.rowCount; r++) lv_obj_delete(scan.rows[r]);
    scan.rowCount = 0;
    scan.seen = 0;
    scan.userMoved = false;
    scan.phase = ScanPhase::Starting;
    scan.nextTryMs = 0;
    scan.giveUpMs = last_now + 10000;
    regroup(scan.list);
}

void scan_update(uint32_t now) {
    char text[24];
    switch (scan.phase) {
        case ScanPhase::Starting:
            scan_set_status(STR_SCAN_STARTING);
            if (static_cast<int32_t>(now - scan.nextTryMs) >= 0) {
                if (ble_scanner_start(UI_SCAN_SECONDS)) {
                    scan.phase = ScanPhase::Scanning;
                    scan.phaseStartMs = now;
                    scan.seen = 0;
                } else if (static_cast<int32_t>(now - scan.giveUpMs) >= 0) {
                    scan.phase = ScanPhase::Failed;
                } else {
                    scan.nextTryMs = now + 500;  // a connection attempt may be in progress; try again
                }
            }
            break;
        case ScanPhase::Scanning: {
            scan_sync_rows();
            const uint32_t elapsed = (now - scan.phaseStartMs) / 1000;
            const uint32_t left = elapsed >= UI_SCAN_SECONDS ? 0 : UI_SCAN_SECONDS - elapsed;
            snprintf(text, sizeof(text), STR_SCAN_SCANNING, (unsigned)left);
            scan_set_status(text);
            if (!ble_scanner_is_scanning()) {
                scan_sync_rows();
                scan.phase = ScanPhase::Done;
            }
            break;
        }
        case ScanPhase::Done:
            if (scan.rowCount == 0) {
                scan_set_status(STR_SCAN_NOTHING);
            } else {
                snprintf(text, sizeof(text), STR_SCAN_FOUND, (unsigned)scan.rowCount);
                scan_set_status(text);
            }
            break;
        case ScanPhase::Failed:
            scan_set_status(STR_SCAN_FAILED);
            break;
    }
    scan_update_again_visibility();
}

void scan_pick(uint8_t entry_idx) {
    const ScanEntry& e = ble_scanner_entry(entry_idx);
    if (e.kind != scan.kind) return;
    StoredDevice d;
    d.valid = true;
    memcpy(d.addr, e.addr, 6);
    d.addrType = e.addrType;
    memcpy(d.name, e.name, sizeof(d.name));
    if (device_store_save(scan.kind, d)) {
        ble_scanner_stop();
        ble_link_reload();  // connect to the newly selected device right away
        go_back();          // back to the device screen, which now shows the new address
    } else {
        scan_set_status(STR_SAVE_FAILED);
    }
}

// ============================================================ action handling
void run_action(intptr_t a) {
    switch (a) {
        case ACT_SETUP_LAUNCHER: go(Screen::Device, DeviceKind::Launcher); break;
        case ACT_SETUP_SWITCHBOARD: go(Screen::Device, DeviceKind::Switchboard); break;
        case ACT_BACK: go_back(); break;
        case ACT_DEVICE_SCAN: go(Screen::Scan, cur_kind); break;
        case ACT_DEVICE_FORGET: go(Screen::Confirm, cur_kind); break;
        case ACT_CONFIRM_YES:
            device_store_erase(cur_kind);
            ble_link_reload();
            go_back();
            break;
        case ACT_SCAN_AGAIN: scan_restart(); break;
        case ACT_MENU_CHECK:
            controller_request_check();  // the overlay opens as soon as the logic has processed it
            go_back();
            break;
        case ACT_MENU_SETUP: go(Screen::Setup, DeviceKind::Unknown); break;
        case ACT_MENU_MAINT: go(Screen::Maintenance, DeviceKind::Unknown); break;
        case ACT_MAINT_RELEASE: go(Screen::ConfirmRelease, DeviceKind::Unknown); break;
        case ACT_MAINT_DISPENSE: go(Screen::Dispense, DeviceKind::Unknown); break;
        case ACT_MAINT_MOTOR: go(Screen::Motor, DeviceKind::Unknown); break;
        case ACT_RELEASE_YES:
            controller_request_release();  // the result shows on the maintenance screen
            go_back();
            break;
        case ACT_ANS_EMPTY: controller_answer(Answer::Empty); break;
        case ACT_ANS_FULL: controller_answer(Answer::Full); break;
        case ACT_ANS_UNSURE: go(Screen::CheckHelp, DeviceKind::Unknown); break;  // nothing moves, explain what to check
        default:
            if (a >= ACT_SCAN_PICK && cur_id == Screen::Scan) scan_pick(static_cast<uint8_t>(a - ACT_SCAN_PICK));
            break;
    }
}

// ============================================================ maintenance screens
bool in_maintenance() {
    return cur_id == Screen::Maintenance || cur_id == Screen::ConfirmRelease || cur_id == Screen::Dispense || cur_id == Screen::Motor;
}

void set_label_if_changed(lv_obj_t* l, const char* text, uint32_t color) {
    if (strcmp(lv_label_get_text(l), text) != 0) lv_label_set_text(l, text);
    lv_obj_set_style_text_color(l, lv_color_hex(color), LV_PART_MAIN);
}

// The info line shows the hint, or the latest message (a result or a problem) for a few seconds.
// The dispense screen also shows what the dispenser is doing.
void maint_update(uint32_t now) {
    if (maint_info) {
        const char* hint = cur_id == Screen::Maintenance ? STR_MAINT_INFO
                           : cur_id == Screen::ConfirmRelease ? STR_RELEASE_INFO
                           : cur_id == Screen::Motor ? STR_MOTOR_HINT : STR_DISPENSE_HINT;
        uint32_t at = 0;
        const Msg m = controller_last_message(&at);
        const bool fresh = m != Msg::None && static_cast<int32_t>(at - screen_since) >= 0 &&
                           static_cast<uint32_t>(now - at) < MESSAGE_SHOW_MS;
        const RunState motor = cur_id == Screen::Motor ? controller_motor_state() : RunState::Idle;
        if (!fresh && motor != RunState::Idle) {  // the motor screen has no room for a status line: it replaces the hint
            set_label_if_changed(maint_info, motor == RunState::Running ? STR_MOTOR_RUNNING
                                             : motor == RunState::Starting ? STR_DISPENSE_STARTING : STR_DISPENSE_STOPPING,
                                 motor == RunState::Running ? ui_color::OK : ui_color::WARNING);
        } else if (!fresh) {
            set_label_if_changed(maint_info, hint, ui_color::TEXT_MUTED);
        } else {
            const bool bad = m == Msg::ReleaseFailed || m == Msg::DispenseFailed || m == Msg::DispenseStuck ||
                             m == Msg::MotorFailed || m == Msg::MotorStuck;
            set_label_if_changed(maint_info, msg_text(m), !msg_is_problem(m) ? ui_color::OK : (bad ? ui_color::ERROR : ui_color::WARNING));
        }
    }
    if (dispense_status) {
        switch (controller_dispense_state()) {
            case RunState::Starting: set_label_if_changed(dispense_status, STR_DISPENSE_STARTING, ui_color::WARNING); break;
            case RunState::Running: set_label_if_changed(dispense_status, STR_DISPENSE_RUNNING, ui_color::OK); break;
            case RunState::Stopping: set_label_if_changed(dispense_status, STR_DISPENSE_STOPPING, ui_color::WARNING); break;
            default: set_label_if_changed(dispense_status, "", ui_color::TEXT); break;
        }
        lv_obj_align(dispense_status, LV_ALIGN_TOP_MID, 0, MAINT_STATUS_Y);
    }
}

void encoder_read(lv_indev_t*, lv_indev_data_t* data) {
    data->enc_diff = static_cast<int16_t>(pending_diff);
    pending_diff = 0;
    // The home screen has nothing to click and is static (turning the knob does nothing there);
    // a push opens the menu. LVGL therefore never sees presses on the home screen.
    data->state = (push_down && cur_id != Screen::Home) ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

}  // namespace

// ================================================================== public
void ui_init() {
    group = lv_group_create();
    indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_ENCODER);
    lv_indev_set_read_cb(indev, encoder_read);
    lv_indev_set_group(indev, group);

    home_obj = lv_screen_active();
    cur_obj = home_obj;
    ui_set_scrollable(home_obj, false);
    lv_obj_set_style_bg_color(home_obj, lv_color_hex(ui_color::BACKGROUND), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(home_obj, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_text_font(home_obj, UI_FONT_NORMAL, LV_PART_MAIN);

    ui_home_build(home_obj);
    ui_update(0);

    // Setup incomplete: go straight to the setup. Back leads to the home screen.
    StoredDevice d;
    if (!device_store_load(DeviceKind::Launcher, d) || !device_store_load(DeviceKind::Switchboard, d)) {
        go(Screen::Setup, DeviceKind::Unknown);
    }
}

void ui_update(uint32_t now) {
    last_now = now;
    if (!home_obj) return;

    // 1. an action recorded by a row click
    if (pending_action != ACT_NONE) {
        const intptr_t a = pending_action;
        pending_action = ACT_NONE;
        run_action(a);
    }

    // 2. the launcher check overlay appears on an idle screen when the logic needs the driver's answer
    CheckReason reason = CheckReason::None;
    const bool need = controller_needs_confirm(&reason);
    if (need && (cur_id == Screen::Home || cur_id == Screen::Menu)) open_check(reason);
    else if (!need && (cur_id == Screen::Check || cur_id == Screen::CheckHelp)) close_check();

    // 3. maintenance: all automation pauses while one of its screens is open; the dispenser runs only
    //    while the knob is held on its row (anything else, such as leaving the screen, is a stop)
    controller_set_maintenance(in_maintenance());
    // The row is taken when the knob goes down and kept until it is released: turning meanwhile never switches rows
    // (which would reverse the motor on the fly).
    if (!push_down) {
        hold_row = nullptr;
    } else if (!push_was_down) {
        lv_obj_t* f = lv_group_get_focused(group);
        hold_row = (f && (f == dispense_row || f == motor_up_row || f == motor_down_row)) ? f : nullptr;
    }
    push_was_down = push_down;
    controller_set_dispense_held(cur_id == Screen::Dispense && hold_row != nullptr && hold_row == dispense_row);
    controller_set_motor_hold(cur_id == Screen::Motor && hold_row != nullptr ? (hold_row == motor_up_row ? 1 : (hold_row == motor_down_row ? -1 : 0)) : 0);
    if (in_maintenance()) maint_update(now);

    // 4. live content
    HomeInputs home;
    controller_home_inputs(&home);
    home.now = now;
    ui_home_update(home, cur_id == Screen::Home);  // the model keeps running while another screen is open
    if (cur_id == Screen::Scan) scan_update(now);
}

void ui_on_encoder(int32_t delta) {
    if (delta == 0) return;
    pending_diff += ENCODER_DIRECTION_INVERT ? -delta : delta;
    if (cur_id == Screen::Scan) scan.userMoved = true;
}

void ui_on_push(bool pressed) {
    if (pressed == push_down) return;
    push_down = pressed;
    if (pressed) press_epoch = screen_epoch;
    if (!pressed && cur_id == Screen::Home) go(Screen::Menu, DeviceKind::Unknown);  // push on the home screen = menu
}

void ui_on_back() {
    if (cur_id == Screen::Home || cur_id == Screen::Check) return;  // the check must be answered
    go_back();
}
