// Host tests for the home screen model (what is shown) and the pixel art (stays inside its surface).
//   g++ -std=c++11 -Wall -Wextra -Iinclude test/host/test_home.cpp src/home_model.cpp src/home_sprites.cpp src/gfx.cpp src/control_logic.cpp src/ble_devices.cpp -o /tmp/test_home && /tmp/test_home

#include <cstdio>
#include <cstring>
#include <vector>

#include "gfx.h"
#include "home_config.h"
#include "home_model.h"
#include "home_sprites.h"

static int failures = 0;
#define CHECK(cond)                                             \
    do {                                                        \
        if (!(cond)) {                                          \
            std::printf("FAIL line %d: %s\n", __LINE__, #cond); \
            failures++;                                         \
        }                                                       \
    } while (0)

struct H {
    HomeModel m;
    HomeInputs in;
    HomeView v;
    H() {
        in.now = 10000;
        in.launcher = LauncherStatus::Preparing;
        in.prepValid = true;
        in.switchboardConnected = true;
    }
    void run(uint32_t dt = 0) {
        in.now += dt;
        m.update(in, v);
    }
};

static void test_down_then_lock_then_up_with_progress() {
    H h;
    h.in.prep = 0;
    h.run();
    CHECK(h.v.band == BandState::Starting && h.v.progress == 0);

    h.in.prep = 1;
    h.run(100);
    CHECK(h.v.band == BandState::Down && h.v.progress < 10);
    h.run(PREP_DOWN_MS / 2);
    CHECK(h.v.band == BandState::Down && h.v.progress == 500);
    h.run(60000);
    CHECK(h.v.band == BandState::Down && h.v.progress == 1000);  // clamped, the real state decides when it ends

    h.in.prep = 2;  // seen switching from down to up: the lock plays first, the bar restarts
    h.run();
    CHECK(h.v.band == BandState::Lock && h.v.progress == 0);
    h.run(LOCK_ANIM_MS - 1);
    CHECK(h.v.band == BandState::Lock);
    h.run(1);
    CHECK(h.v.band == BandState::Up && h.v.progress == 0);
    h.run((PREP_UP_MS - LOCK_ANIM_MS) / 2);
    CHECK(h.v.band == BandState::Up && h.v.progress >= 499 && h.v.progress <= 501);
    h.in.prep = 3;  // slow -> fast is one phase: no second lock, the bar keeps going
    h.run(100);
    CHECK(h.v.band == BandState::Up && h.v.progress > 500);
    h.run(100000);
    CHECK(h.v.band == BandState::Up && h.v.progress == 1000);

    h.in.prep = 0;
    h.in.launcher = LauncherStatus::Ready;
    h.run();
    CHECK(h.v.band == BandState::Ready);
}

static void test_lock_time_is_subtracted_from_the_up_bar() {
    // the bar reaches 100 % PREP_UP_MS after the switch from down to up, lock animation included
    H h;
    h.in.prep = 1;
    h.run(100);
    h.in.prep = 2;
    h.run();
    h.run(PREP_UP_MS - 1);
    CHECK(h.v.progress >= 999 && h.v.progress < 1000);
    h.run(1);
    CHECK(h.v.progress == 1000);
}

static void test_going_up_seen_first_has_no_lock() {
    H h;
    h.in.prep = 2;  // e.g. connected in the middle of a preparation
    h.run(100);
    CHECK(h.v.band == BandState::Up);
    h.run(PREP_UP_MS / 2);
    CHECK(h.v.progress == 500);
}

static void test_phase_resets_when_the_preparation_ends() {
    H h;
    h.in.prep = 1;
    h.run(100);
    h.in.prep = 0;
    h.in.launcher = LauncherStatus::Error;
    h.run(1000);
    CHECK(h.v.band == BandState::Error);
    h.in.launcher = LauncherStatus::Preparing;
    h.in.prep = 2;  // a new preparation first seen going up: no lock from the old down phase
    h.run(100);
    CHECK(h.v.band == BandState::Up);
}

static void test_rocket_flies_only_after_the_launch_is_confirmed() {
    H h;
    h.in.launcher = LauncherStatus::Ready;
    h.run();
    CHECK(h.v.band == BandState::Ready);

    h.in.launcher = LauncherStatus::Rearming;  // button pressed, write not confirmed yet
    h.in.launchPending = true;
    h.in.rearmInMs = 4000;
    h.run(300);
    CHECK(h.v.band == BandState::Ready);

    h.in.launchPending = false;  // confirmed
    h.in.launchOkCount = 1;
    h.in.rearmInMs = 3500;
    h.run(100);
    CHECK(h.v.band == BandState::Launched);
    h.in.rearmInMs = 3000;
    h.run(LAUNCHED_ANIM_MS - 1);
    CHECK(h.v.band == BandState::Launched);
    h.in.rearmInMs = 2001;
    h.run(1);
    CHECK(h.v.band == BandState::Rearm && h.v.secsLeft == 3);
    h.in.rearmInMs = 1;
    h.run(10);
    CHECK(h.v.secsLeft == 1);
}

static void test_failed_launch_shows_no_rocket() {
    H h;
    h.in.launcher = LauncherStatus::Ready;
    h.run();
    h.in.launcher = LauncherStatus::Rearming;
    h.in.launchPending = true;
    h.run(100);
    h.in.launcher = LauncherStatus::Error;  // write failed: the driver is asked
    h.in.launchPending = false;
    h.run(100);
    CHECK(h.v.band == BandState::Error);
    h.run(5000);
    CHECK(h.v.band == BandState::Error);
}

static void test_late_confirmation_and_offline() {
    H h;
    h.in.launcher = LauncherStatus::Preparing;  // confirmation arrived after the countdown was over
    h.in.prep = 0;
    h.in.launchOkCount = 1;
    h.run(10);
    CHECK(h.v.band == BandState::Launched);
    h.run(LAUNCHED_ANIM_MS);
    CHECK(h.v.band == BandState::Starting);

    H o;
    o.in.launcher = LauncherStatus::Offline;
    o.in.launchOkCount = 1;
    o.run(10);
    CHECK(o.v.band == BandState::Offline);  // the link is gone: show that, not a rocket
}

static void test_other_band_states() {
    H h;
    h.in.launcher = LauncherStatus::NeedsCheck;
    h.run();
    CHECK(h.v.band == BandState::Check);
    h.in.launcher = LauncherStatus::Error;
    h.run();
    CHECK(h.v.band == BandState::Error);
    h.in.launcher = LauncherStatus::Offline;
    h.run();
    CHECK(h.v.band == BandState::Offline);
    h.in.rearmInMs = 20000;
    h.in.launcher = LauncherStatus::Rearming;
    h.run();
    CHECK(h.v.secsLeft == 9);
}

static void test_band_since_changes_only_on_a_new_state() {
    H h;
    h.in.launcher = LauncherStatus::Ready;
    h.run();
    const uint32_t since = h.v.bandSince;
    h.run(500);
    CHECK(h.v.bandSince == since);
    h.in.launcher = LauncherStatus::Offline;
    h.run(500);
    CHECK(h.v.bandSince == h.in.now);
}

static void test_tiles_follow_the_led_states() {
    H h;
    h.in.launcher = LauncherStatus::Ready;
    h.run();
    for (int i = 0; i < TILE_COUNT; i++) CHECK(h.v.tile[i] == TileState::Off);

    h.in.fogLed = LED_FLASH;
    h.in.panicLed = LED_ON;
    h.in.flashLed = LED_ON;
    h.in.lampLed = LED_FLASH_FAST;
    h.in.sirenLed = LED_FLASH;
    h.run();
    CHECK(h.v.tile[TILE_FOG] == TileState::Pending);   // heating
    CHECK(h.v.tile[TILE_PANIC] == TileState::Active);
    CHECK(h.v.tile[TILE_FLASH] == TileState::Active);  // the beacon is animated
    CHECK(h.v.tile[TILE_LAMP] == TileState::Problem);
    CHECK(h.v.tile[TILE_SIREN] == TileState::Pending);
    h.in.fogLed = LED_ON;
    h.run();
    CHECK(h.v.tile[TILE_FOG] == TileState::On);        // ready: steady
    h.in.fogLed = LED_PULSE;
    h.run();
    CHECK(h.v.tile[TILE_FOG] == TileState::Active);    // fogging: animated

    CHECK(h.v.tile[TILE_DISPENSER] == TileState::Off);
    h.in.dispenserValid = true;
    h.in.dispenserOn = true;
    h.run();
    CHECK(h.v.tile[TILE_DISPENSER] == TileState::Active);
    h.in.dispenserValid = false;  // characteristic not available: never shown as dispensing
    h.run();
    CHECK(h.v.tile[TILE_DISPENSER] == TileState::Off);
    h.in.switchboardConnected = true;
    h.in.fogLed = LED_ON;
    h.run();
    CHECK(h.v.tile[TILE_AUTO] == TileState::Off);       // SW5 is not defined yet
}

static void test_toast_events_and_conditions() {
    H h;
    h.in.launcher = LauncherStatus::Ready;
    h.run();
    CHECK(h.v.toastLevel == ToastLevel::None && h.v.toast[0] == 0);

    h.in.message = Msg::LaunchSent;  // the rocket says it, no toast
    h.in.messageAt = h.in.now;
    h.run(100);
    CHECK(h.v.toastLevel == ToastLevel::None);

    h.in.message = Msg::LaunchNotReady;
    h.in.messageAt = h.in.now;
    h.run(100);
    CHECK(h.v.toastLevel == ToastLevel::Warn && std::strcmp(h.v.toast, "not ready yet") == 0);
    const uint32_t since = h.v.toastSince;
    h.run(1000);
    CHECK(h.v.toastSince == since);                       // same toast: the slide-in does not restart
    h.run(MESSAGE_SHOW_MS);
    CHECK(h.v.toastLevel == ToastLevel::None);            // events expire

    h.in.message = Msg::LaunchFailed;
    h.in.messageAt = h.in.now;
    h.run(10);
    CHECK(h.v.toastLevel == ToastLevel::Error);
}

static void test_toast_conditions() {
    H h;
    h.in.launcher = LauncherStatus::Ready;
    h.in.switchboardConnected = false;
    h.run();
    h.run(BOARD_OFFLINE_TOAST_MS - 100);
    CHECK(h.v.toastLevel == ToastLevel::None);            // not for a short dropout / start-up
    h.run(100);
    CHECK(h.v.toastLevel == ToastLevel::Error && std::strcmp(h.v.toast, "board offline") == 0);
    h.in.flashLed = LED_FLASH_FAST;                       // a consequence: not counted
    h.run(10);
    CHECK(std::strcmp(h.v.toast, "board offline") == 0);

    h.in.switchboardConnected = true;
    h.in.flashLed = LED_OFF;
    h.in.fogLed = LED_FLASH_FAST;
    h.run(10);
    CHECK(std::strcmp(h.v.toast, "fog problem") == 0 && h.v.toastLevel == ToastLevel::Warn);
    h.in.sirenLed = LED_FLASH_FAST;
    h.run(10);
    CHECK(std::strcmp(h.v.toast, "fog problem +1") == 0);  // the most important one plus a count
    h.in.fogLed = LED_OFF;
    h.run(10);
    CHECK(std::strcmp(h.v.toast, "relay problem") == 0);
    h.in.sirenLed = LED_OFF;
    h.run(10);
    CHECK(h.v.toastLevel == ToastLevel::None);
}

// ---------------------------------------------------------------- pixel art
struct Guarded {
    std::vector<uint16_t> mem;
    gfx::Surface s;
    static const int G = 64;
    Guarded(int w, int h) : mem(w * h + 2 * G, 0xA5A5) {
        s.px = mem.data() + G;
        s.w = w;
        s.h = h;
    }
    bool guardsIntact() const {
        for (int i = 0; i < G; i++)
            if (mem[i] != 0xA5A5 || mem[mem.size() - 1 - i] != 0xA5A5) return false;
        return true;
    }
    size_t hash() const {
        size_t h = 1469598103u;
        for (int i = 0; i < s.w * s.h; i++) h = (h ^ s.px[i]) * 16777619u;
        return h;
    }
};

static void test_bands_draw_inside_their_surface_and_animate() {
    for (int st = 0; st <= static_cast<int>(BandState::Rearm); st++) {
        Guarded g(HOME_BAND_W, HOME_BAND_H);
        std::vector<size_t> seen;
        for (uint32_t t = 0; t < 4000; t += 41) {
            for (int p = 0; p <= 1000; p += 500) {
                sprites::drawBand(g.s, static_cast<BandState>(st), t, p, static_cast<int>(t / 1000));
                CHECK(g.guardsIntact());
            }
            sprites::drawBand(g.s, static_cast<BandState>(st), t, 0, 3);
            seen.push_back(g.hash());
        }
        bool varies = false;
        for (size_t i = 1; i < seen.size(); i++) varies |= seen[i] != seen[0];
        CHECK(varies);  // every band state is animated (the countdown changes with secsLeft below)
    }
    Guarded a(HOME_BAND_W, HOME_BAND_H), b(HOME_BAND_W, HOME_BAND_H);
    sprites::drawBand(a.s, BandState::Rearm, 0, 0, 3);
    sprites::drawBand(b.s, BandState::Rearm, 0, 0, 2);
    CHECK(a.hash() != b.hash());
    sprites::drawBand(a.s, BandState::Down, 0, 0, 0);
    sprites::drawBand(b.s, BandState::Down, 0, 600, 0);
    CHECK(a.hash() != b.hash());  // the progress bar moves
}

static void test_tiles_draw_inside_and_every_state_looks_different() {
    for (int id = 0; id < TILE_COUNT; id++) {
        Guarded off(HOME_TILES_W, HOME_TILES_H), g(HOME_TILES_W, HOME_TILES_H);
        TileState t[TILE_COUNT];
        for (int i = 0; i < TILE_COUNT; i++) t[i] = TileState::Off;
        sprites::drawTiles(off.s, t, 0);
        const size_t offHash = off.hash();
        for (int st = 1; st <= static_cast<int>(TileState::Problem); st++) {
            t[id] = static_cast<TileState>(st);
            for (uint32_t now = 0; now < 3000; now += 37) {
                sprites::drawTiles(g.s, t, now);
                CHECK(g.guardsIntact());
            }
            sprites::drawTiles(g.s, t, 0);
            if (id != TILE_AUTO || st != 0) CHECK(g.hash() != offHash);
        }
    }
    CHECK(!sprites::tileAnimates(TileState::Off) && !sprites::tileAnimates(TileState::On) && sprites::tileAnimates(TileState::Active));
    Guarded toast(HOME_TOAST_W, HOME_TOAST_H);
    sprites::drawToast(toast.s, ToastLevel::Warn);
    CHECK(toast.guardsIntact());
}

static void test_gfx_clips_everything() {
    Guarded g(20, 20);
    gfx::fill(g.s, 0);
    gfx::rect(g.s, -50, -50, 200, 200, 0xFFFFFF);
    gfx::line(g.s, -100, -100, 300, 300, 0xFF0000);
    gfx::ellipse(g.s, -30, -30, 60, 60, 0x00FF00);
    gfx::arc(g.s, 10, 10, 200, 0, 360, 3, 0x0000FF);
    const gfx::Pt tri[] = {{-40, -40}, {80, 10}, {10, 90}};
    gfx::polygon(g.s, tri, 3, 0x123456);
    gfx::polyOutline(g.s, tri, 3, 0x654321);
    gfx::glyph(g.s, 15, 15, '8', 4, 0xFFFFFF);
    gfx::roundCorners(g.s, 6, 0);
    CHECK(g.guardsIntact());
    CHECK(gfx::mix(0x000000, 0xFFFFFF, 0) == 0x000000 && gfx::mix(0x000000, 0xFFFFFF, 256) == 0xFFFFFF);
}

int main() {
    test_down_then_lock_then_up_with_progress();
    test_lock_time_is_subtracted_from_the_up_bar();
    test_going_up_seen_first_has_no_lock();
    test_phase_resets_when_the_preparation_ends();
    test_rocket_flies_only_after_the_launch_is_confirmed();
    test_failed_launch_shows_no_rocket();
    test_late_confirmation_and_offline();
    test_other_band_states();
    test_band_since_changes_only_on_a_new_state();
    test_tiles_follow_the_led_states();
    test_toast_events_and_conditions();
    test_toast_conditions();
    test_bands_draw_inside_their_surface_and_animate();
    test_tiles_draw_inside_and_every_state_looks_different();
    test_gfx_clips_everything();

    if (failures == 0) {
        std::printf("ALL HOME TESTS PASSED (15 tests)\n");
        return 0;
    }
    std::printf("%d CHECK(S) FAILED\n", failures);
    return 1;
}
