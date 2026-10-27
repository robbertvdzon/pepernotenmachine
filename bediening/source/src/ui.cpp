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
#include "link_state.h"
#include "ui_compat.h"
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

// home cards
constexpr int CARD_H = 46;
constexpr int CARD_GAP = 4;
constexpr int CARD1_Y = TITLE_Y + UI_FONT_NORMAL_H + 3;
constexpr int CARD2_Y = CARD1_Y + CARD_H + CARD_GAP;
constexpr int TEXT_X = BAR_W + 8;
constexpr int NAME_Y = 3;
constexpr int STATUS_Y = NAME_Y + UI_FONT_NORMAL_H + 3;

// menu rows
constexpr int ROW_H = 24;
constexpr int ROW_GAP = 4;
constexpr int ROW_TEXT_X = BAR_W + 6;
constexpr int INFO_Y = TITLE_Y + UI_FONT_NORMAL_H + 2;  // small info line under the title
constexpr int MENU_Y = INFO_Y + UI_FONT_SMALL_H + 5;    // first menu row
constexpr int SCAN_ROW_H = 28;                          // two-line device row
constexpr int SCAN_LIST_TOP = INFO_Y + UI_FONT_SMALL_H + 3;
constexpr int SCAN_LIST_HEIGHT = DISPLAY_HEIGHT - SAFE_BOTTOM - SCAN_LIST_TOP;

constexpr int text_width(int chars) { return chars * UI_FONT_NORMAL_W; }
constexpr int small_width(int chars) { return chars * UI_FONT_SMALL_W; }

static_assert(text_width(sizeof("Control unit") - 1) <= DISPLAY_WIDTH, "title too wide");
static_assert(text_width(sizeof("Forget switchboard?") - 1) <= DISPLAY_WIDTH, "title too wide");
static_assert(text_width(sizeof("Scan switchboard") - 1) <= DISPLAY_WIDTH, "title too wide");
static_assert(text_width(20) <= DISPLAY_WIDTH, "home message too wide");        // messages are <= 20 characters
static_assert(ROW_TEXT_X + text_width(sizeof("Prepare launcher") - 1) <= CONTENT_W, "actions row too wide");
static_assert(small_width(17) <= DISPLAY_WIDTH, "address line too wide");        // "AA:BB:CC:DD:EE:FF"
static_assert(small_width(17) <= CONTENT_W - ROW_TEXT_X, "device detail line too wide");  // "-127dBm pub UUID" = 16
static_assert(TEXT_X + text_width(17) <= CONTENT_W, "card status (max 17 characters) too wide");
static_assert(ROW_TEXT_X + text_width(17) <= CONTENT_W, "device address row too wide");  // "AA:BB:CC:DD:EE:FF"
static_assert(ROW_TEXT_X + text_width(sizeof("Switchboard") - 1) + small_width(5) + 12 <= CONTENT_W,
              "setup row too wide");  // "Switchboard" + "saved"
static_assert(STATUS_Y + UI_FONT_NORMAL_H <= CARD_H, "card text taller than the card");
static_assert(CARD2_Y + CARD_H <= DISPLAY_HEIGHT - SAFE_BOTTOM, "cards do not fit the screen height");
static_assert(MENU_Y + 3 * ROW_H + 2 * ROW_GAP <= DISPLAY_HEIGHT - SAFE_BOTTOM, "three menu rows do not fit");
static_assert(SCAN_LIST_HEIGHT >= SCAN_ROW_H * 2, "scan list too short for two device rows");

// ================================================================== state
enum class Screen : uint8_t { Home, Setup, Device, Scan, Confirm, Actions };

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
    ACT_PREPARE,
    ACT_SCAN_PICK = 100  // + scanner entry index
};
intptr_t pending_action = ACT_NONE;

// input
int32_t pending_diff = 0;
bool push_down = false;
bool push_swallow = false;  // the press that opened setup must not click inside it
bool long_fired = false;
uint32_t push_start = 0;
uint32_t last_now = 0;

// home cards
struct CardUi {
    lv_obj_t* status = nullptr;
    LinkState shown = LinkState::NotConfigured;
    char shownDetail[20] = {0};
    bool initialised = false;
};
CardUi cards[2];  // 0 = launcher, 1 = switchboard

// live text on the home title and the actions info line (only touched when it changes)
struct TextCache {
    char text[24] = {0};
    uint32_t color = 0;
    bool init = false;
};
lv_obj_t* home_title = nullptr;
lv_obj_t* actions_info = nullptr;
TextCache title_cache;
TextCache actions_cache;

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
const char* name_for(DeviceKind k) { return k == DeviceKind::Switchboard ? "Switchboard" : "Launcher"; }
uint8_t idx_for(DeviceKind k) { return k == DeviceKind::Switchboard ? 1 : 0; }

const char* addr_type_short(uint8_t t) {
    switch (t) {
        case 0: return "pub";
        case 1: return "rnd";
        case 2: return "pid";
        case 3: return "rid";
        default: return "?";
    }
}

void row_clicked(lv_event_t* e) { pending_action = reinterpret_cast<intptr_t>(lv_event_get_user_data(e)); }

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

// ================================================================== home
lv_obj_t* make_card(lv_obj_t* parent, int32_t y, uint32_t accent, const char* title) {
    lv_obj_t* card = lv_obj_create(parent);
    ui_set_scrollable(card, false);
    lv_obj_set_size(card, CONTENT_W, CARD_H);
    lv_obj_set_pos(card, MARGIN, y);
    lv_obj_set_style_bg_color(card, lv_color_hex(ui_color::CARD), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(card, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(card, 4, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card, 0, LV_PART_MAIN);

    lv_obj_t* bar = lv_obj_create(card);  // accent bar on the left edge
    ui_set_scrollable(bar, false);
    lv_obj_set_size(bar, BAR_W, CARD_H);
    lv_obj_set_pos(bar, 0, 0);
    lv_obj_set_style_bg_color(bar, lv_color_hex(accent), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(bar, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(bar, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(bar, 0, LV_PART_MAIN);

    lv_obj_t* name = lv_label_create(card);
    lv_label_set_text(name, title);
    lv_obj_set_style_text_color(name, lv_color_hex(accent), LV_PART_MAIN);
    lv_obj_set_pos(name, TEXT_X, NAME_Y);

    lv_obj_t* st = lv_label_create(card);
    lv_label_set_text(st, "");
    lv_obj_set_pos(st, TEXT_X, STATUS_Y);
    return st;  // the status label; kept for live updates
}

uint32_t status_color_for(LinkState s) {
    switch (s) {
        case LinkState::Connected: return ui_color::OK;
        case LinkState::Error: return ui_color::ERROR;
        default: return ui_color::WARNING;  // not set / searching / connecting
    }
}

void refresh_card(CardUi& c, DeviceKind kind) {
    const LinkState s = ble_link_state(kind);
    char detail[20];
    ble_link_detail(kind, detail, sizeof(detail));
    if (c.initialised && s == c.shown && strcmp(detail, c.shownDetail) == 0) return;

    c.initialised = true;
    c.shown = s;
    strncpy(c.shownDetail, detail, sizeof(c.shownDetail) - 1);
    lv_label_set_text(c.status, (s == LinkState::Error && detail[0]) ? detail : link_state_text(s));
    lv_obj_set_style_text_color(c.status, lv_color_hex(status_color_for(s)), LV_PART_MAIN);
}

// ============================================================ screen builders
lv_obj_t* build_setup() {
    lv_obj_t* scr = make_screen("Setup", ui_color::ACCENT);
    make_info(scr, "select a device");
    StoredDevice d;
    const bool l = device_store_load(DeviceKind::Launcher, d);
    const bool s = device_store_load(DeviceKind::Switchboard, d);
    // right side: "saved" (green) or "none" (muted)
    make_menu_row(scr, MENU_Y, "Launcher", l ? "saved" : "none", l ? ui_color::OK : ui_color::TEXT_DIM,
                  ui_color::LAUNCHER, ACT_SETUP_LAUNCHER);
    make_menu_row(scr, MENU_Y + ROW_H + ROW_GAP, "Switchboard", s ? "saved" : "none",
                  s ? ui_color::OK : ui_color::TEXT_DIM, ui_color::SWITCHBOARD, ACT_SETUP_SWITCHBOARD);
    make_menu_row(scr, MENU_Y + 2 * (ROW_H + ROW_GAP), "Back", nullptr, 0, ui_color::TEXT_MUTED, ACT_BACK);
    return scr;
}

lv_obj_t* build_device(DeviceKind kind) {
    lv_obj_t* scr = make_screen(name_for(kind), accent_for(kind));
    StoredDevice d;
    const bool saved = device_store_load(kind, d);
    char addr[18] = "not set";
    if (saved) ble_address_format(d.addr, addr, sizeof(addr));
    make_info(scr, addr);

    int y = MENU_Y;
    make_menu_row(scr, y, "Scan & select", nullptr, 0, accent_for(kind), ACT_DEVICE_SCAN);
    y += ROW_H + ROW_GAP;
    if (saved) {
        make_menu_row(scr, y, "Forget", nullptr, 0, ui_color::ERROR, ACT_DEVICE_FORGET);
        y += ROW_H + ROW_GAP;
    }
    make_menu_row(scr, y, "Back", nullptr, 0, ui_color::TEXT_MUTED, ACT_BACK);
    return scr;
}

lv_obj_t* build_confirm(DeviceKind kind) {
    lv_obj_t* scr = make_screen(kind == DeviceKind::Switchboard ? "Forget switchboard?" : "Forget launcher?",
                                ui_color::ERROR);
    StoredDevice d;
    char addr[18] = "";
    if (device_store_load(kind, d)) ble_address_format(d.addr, addr, sizeof(addr));
    make_info(scr, addr);
    make_menu_row(scr, MENU_Y, "Cancel", nullptr, 0, ui_color::TEXT_MUTED, ACT_BACK);  // safe choice first
    make_menu_row(scr, MENU_Y + ROW_H + ROW_GAP, "Yes, forget", nullptr, 0, ui_color::ERROR, ACT_CONFIRM_YES);
    return scr;
}

lv_obj_t* build_scan(DeviceKind kind) {
    lv_obj_t* scr = make_screen(kind == DeviceKind::Switchboard ? "Scan switchboard" : "Scan launcher",
                                accent_for(kind));
    scan = ScanCtx();
    scan.kind = kind;
    scan.status = make_info(scr, "starting...");
    scan.phase = ScanPhase::Starting;
    scan.nextTryMs = 0;
    scan.giveUpMs = last_now + 10000;

    lv_obj_t* list = lv_obj_create(scr);
    lv_obj_set_size(list, CONTENT_W, SCAN_LIST_HEIGHT);
    lv_obj_set_pos(list, MARGIN, SCAN_LIST_TOP);
    lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(list, 0, LV_PART_MAIN);
    lv_obj_set_style_outline_width(list, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(list, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_row(list, 3, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(list, LV_DIR_VER);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    scan.list = list;

    // Fixed rows below the devices that will appear above them.
    lv_obj_t* again = make_row(list, -1, ROW_H, accent_for(kind), ACT_SCAN_AGAIN);
    ui_set_hidden(again, true);  // shown when the scan has finished
    scan.again = again;
    lv_obj_t* al = lv_label_create(again);
    lv_label_set_text(al, "Scan again");
    lv_obj_set_pos(al, ROW_TEXT_X, (ROW_H - UI_FONT_NORMAL_H) / 2);
    lv_obj_t* bk = make_row(list, -1, ROW_H, ui_color::TEXT_MUTED, ACT_BACK);
    lv_obj_t* bl = lv_label_create(bk);
    lv_label_set_text(bl, "Back");
    lv_obj_set_pos(bl, ROW_TEXT_X, (ROW_H - UI_FONT_NORMAL_H) / 2);
    return scr;
}

void update_text(lv_obj_t* label, const char* text, uint32_t color, TextCache& cache) {
    if (cache.init && cache.color == color && strcmp(cache.text, text) == 0) return;
    cache.init = true;
    cache.color = color;
    strncpy(cache.text, text, sizeof(cache.text) - 1);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, lv_color_hex(color), LV_PART_MAIN);
}

lv_obj_t* build_actions() {
    lv_obj_t* scr = make_screen("Actions", ui_color::ACCENT);
    actions_info = make_info(scr, "");
    actions_cache = TextCache();
    make_menu_row(scr, MENU_Y, "Prepare launcher", nullptr, 0, ui_color::LAUNCHER, ACT_PREPARE);
    make_menu_row(scr, MENU_Y + ROW_H + ROW_GAP, "Back", nullptr, 0, ui_color::TEXT_MUTED, ACT_BACK);
    return scr;
}

// Launcher status or the latest message, shown on the actions screen and as the home title.
void refresh_live_text() {
    char msg[24];
    bool problem = false;
    const bool has_msg = controller_message(msg, sizeof(msg), &problem);

    if (cur_id == Screen::Home && home_title) {
        if (has_msg) update_text(home_title, msg, problem ? ui_color::WARNING : ui_color::OK, title_cache);
        else update_text(home_title, "Control unit", ui_color::TEXT_MUTED, title_cache);
    }
    if (cur_id == Screen::Actions && actions_info) {
        if (has_msg) {
            update_text(actions_info, msg, problem ? ui_color::WARNING : ui_color::OK, actions_cache);
        } else {
            char text[24];
            controller_launcher_text(text, sizeof(text));
            const LauncherStatus st = controller_launcher_status();
            const uint32_t color = st == LauncherStatus::Ready ? ui_color::OK
                                   : st == LauncherStatus::Error ? ui_color::ERROR
                                   : st == LauncherStatus::Preparing ? ui_color::TEXT
                                                                     : ui_color::TEXT_MUTED;
            update_text(actions_info, text, color, actions_cache);
        }
    }
}

// ============================================================== navigation
void enter(Screen s, DeviceKind k) {
    if (cur_id == Screen::Scan && s != Screen::Scan) ble_scanner_stop();

    lv_group_remove_all_objs(group);  // old rows must leave the group before their screen is deleted
    actions_info = nullptr;
    lv_obj_t* scr = nullptr;
    switch (s) {
        case Screen::Home: scr = home_obj; break;
        case Screen::Setup: scr = build_setup(); break;
        case Screen::Device: scr = build_device(k); break;
        case Screen::Confirm: scr = build_confirm(k); break;
        case Screen::Scan: scr = build_scan(k); break;
        case Screen::Actions: scr = build_actions(); break;
    }
    lv_obj_t* old = cur_obj;
    cur_id = s;
    cur_kind = k;
    cur_obj = scr;
    lv_screen_load(scr);
    if (old && old != home_obj && old != scr) lv_obj_delete(old);

    if (s == Screen::Scan) regroup(scan.list);
    else if (s != Screen::Home) regroup(scr);
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
        snprintf(text, sizeof(text), "%ddBm %s %s", (int)e.rssi, addr_type_short(e.addrType), e.strong ? "UUID" : "name");
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
            scan_set_status("starting...");
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
            snprintf(text, sizeof(text), "scanning %us", (unsigned)left);
            scan_set_status(text);
            if (!ble_scanner_is_scanning()) {
                scan_sync_rows();
                scan.phase = ScanPhase::Done;
            }
            break;
        }
        case ScanPhase::Done:
            if (scan.rowCount == 0) {
                scan_set_status("nothing found");
            } else {
                snprintf(text, sizeof(text), "found %u", (unsigned)scan.rowCount);
                scan_set_status(text);
            }
            break;
        case ScanPhase::Failed:
            scan_set_status("scan failed");
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
        scan_set_status("save failed");
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
        case ACT_PREPARE: controller_request_prepare(); break;
        default:
            if (a >= ACT_SCAN_PICK && cur_id == Screen::Scan) scan_pick(static_cast<uint8_t>(a - ACT_SCAN_PICK));
            break;
    }
}

void encoder_read(lv_indev_t*, lv_indev_data_t* data) {
    data->enc_diff = static_cast<int16_t>(pending_diff);
    pending_diff = 0;
    // The home screen has nothing to click, so LVGL never sees presses there. Otherwise the press
    // that is still being held when a long press opens the setup would arrive as a RELEASE on the
    // setup's first row and select it immediately.
    const bool forward = push_down && !push_swallow && cur_id != Screen::Home;
    data->state = forward ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
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

    home_title = lv_label_create(home_obj);
    lv_label_set_text(home_title, "Control unit");
    lv_obj_set_style_text_color(home_title, lv_color_hex(ui_color::TEXT_MUTED), LV_PART_MAIN);
    lv_obj_set_width(home_title, DISPLAY_WIDTH);
    lv_obj_set_style_text_align(home_title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_pos(home_title, 0, TITLE_Y);

    cards[0].status = make_card(home_obj, CARD1_Y, ui_color::LAUNCHER, "Launcher");
    cards[1].status = make_card(home_obj, CARD2_Y, ui_color::SWITCHBOARD, "Switchboard");
    ui_update(0);

    // Setup incomplete: go straight to the setup. Back leads to the home screen.
    StoredDevice d;
    if (!device_store_load(DeviceKind::Launcher, d) || !device_store_load(DeviceKind::Switchboard, d)) {
        go(Screen::Setup, DeviceKind::Unknown);
    }
}

void ui_update(uint32_t now) {
    last_now = now;
    if (!cards[0].status) return;

    // 1. an action recorded by a row click
    if (pending_action != ACT_NONE) {
        const intptr_t a = pending_action;
        pending_action = ACT_NONE;
        run_action(a);
    }

    // 2. long press on the home screen opens the setup
    if (cur_id == Screen::Home && push_down && !long_fired && static_cast<uint32_t>(now - push_start) >= UI_LONG_PRESS_MS) {
        long_fired = true;
        push_swallow = true;  // LVGL must not see this press, or it would click inside the setup
        go(Screen::Setup, DeviceKind::Unknown);
    }

    // 3. live content
    refresh_card(cards[0], DeviceKind::Launcher);
    refresh_card(cards[1], DeviceKind::Switchboard);
    refresh_live_text();
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
    if (pressed) {
        push_start = last_now;
        long_fired = false;
        push_swallow = false;
    } else if (cur_id == Screen::Home && !long_fired && static_cast<uint32_t>(last_now - push_start) < UI_LONG_PRESS_MS) {
        go(Screen::Actions, DeviceKind::Unknown);  // short press on the home screen
    }
}

void ui_on_back() {
    if (cur_id != Screen::Home) go_back();
}
