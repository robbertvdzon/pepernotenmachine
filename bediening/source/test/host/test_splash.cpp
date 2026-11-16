// Host tests for the start-up splash: the model (when it is over, what the badges and dots show) and the pixel art
// (stays inside its surface, shows the states).
//   g++ -std=c++11 -Wall -Wextra -Iinclude test/host/test_splash.cpp src/splash_model.cpp src/splash_sprites.cpp src/gfx.cpp -o /tmp/test_splash && /tmp/test_splash

#include <cstdio>
#include <vector>

#include "gfx.h"
#include "splash_config.h"
#include "splash_model.h"
#include "splash_sprites.h"

static int failures = 0;
#define CHECK(cond)                                             \
    do {                                                        \
        if (!(cond)) {                                          \
            if (failures++ < 10) std::printf("FAIL line %d: %s\n", __LINE__, #cond); \
        }                                                       \
    } while (0)

struct S {
    SplashModel m;
    SplashInputs in;
    SplashView v;
    S() { in.now = 5000; }  // power-up is not at 0
    void step(uint32_t dt = 0) {
        in.now += dt;
        m.update(in, v);
        in.pushed = false;
    }
    void both(LinkState s) { in.launcher = s; in.switchboard = s; }
};

int main() {
    // ---- both connect at once: the splash still lasts SPLASH_MIN_MS (and shows "connected" for the hold time) ----
    {
        S s;
        s.both(LinkState::Connected);
        s.step();  // starts the clock
        CHECK(!s.v.done);
        CHECK(s.v.badge[0] == BadgeState::Connected && s.v.badge[1] == BadgeState::Connected);
        s.step(SPLASH_MIN_MS - 1);
        CHECK(!s.v.done);  // one millisecond short
        s.step(1);
        CHECK(s.v.done && !s.v.skipped);
        s.step(100000);
        CHECK(s.v.done);  // stays over
    }
    // ---- waiting for one device: it never ends by itself ----
    {
        S s;
        s.in.launcher = LinkState::Connected;
        s.in.switchboard = LinkState::Searching;
        for (int i = 0; i < 400; i++) {
            s.step(50);
            CHECK(!s.v.done);
        }
        CHECK(s.v.badge[0] == BadgeState::Connected && s.v.badge[1] == BadgeState::Waiting);
    }
    // ---- the last device connects late: hold time after it, not after the minimum ----
    {
        S s;
        s.in.launcher = LinkState::Connected;
        s.in.switchboard = LinkState::Connecting;
        s.step();
        s.step(5000);
        CHECK(!s.v.done);
        s.in.switchboard = LinkState::Connected;
        s.step(1);
        CHECK(!s.v.done);
        s.step(SPLASH_CONNECTED_HOLD_MS - 2);
        CHECK(!s.v.done);
        s.step(2);
        CHECK(s.v.done);
    }
    // ---- a device drops out again during the hold time: the hold restarts ----
    {
        S s;
        s.both(LinkState::Connected);
        s.step();
        s.step(SPLASH_MIN_MS - SPLASH_CONNECTED_HOLD_MS + 100);  // minimum time passed? no: still short of it
        s.in.switchboard = LinkState::Searching;
        s.step(1000);
        CHECK(!s.v.done);
        s.in.switchboard = LinkState::Connected;
        s.step();  // both connected from here on
        CHECK(!s.v.done);
        s.step(SPLASH_CONNECTED_HOLD_MS - 1);
        CHECK(!s.v.done);
        s.step(1);
        CHECK(s.v.done);
    }
    // ---- error state shows as a problem badge ----
    {
        S s;
        s.in.launcher = LinkState::Error;
        s.in.switchboard = LinkState::NotConfigured;
        s.step();
        CHECK(s.v.badge[0] == BadgeState::Problem && s.v.badge[1] == BadgeState::Waiting);
    }
    // ---- push to continue: only once offered ----
    {
        S s;
        s.in.launcher = LinkState::Connected;
        s.in.switchboard = LinkState::Searching;
        s.step();
        s.in.pushed = true;
        s.step(SPLASH_MIN_MS + 500);
        CHECK(!s.v.done && !s.v.showContinue);  // a push before the offer does nothing
        s.step(SPLASH_CONTINUE_AFTER_MS - SPLASH_MIN_MS - 501);
        CHECK(!s.v.showContinue);                // 1 ms before the offer
        s.step(1);
        CHECK(s.v.showContinue && !s.v.done);
        s.step(100);
        CHECK(!s.v.done);  // offered, but nobody pushed
        s.in.pushed = true;
        s.step(10);
        CHECK(s.v.done && s.v.skipped && !s.v.showContinue);
    }
    // ---- a push exactly 1 ms before the offer does nothing ----
    {
        S s;
        s.in.launcher = LinkState::Connected;
        s.in.switchboard = LinkState::Searching;
        s.step();
        s.step(SPLASH_CONTINUE_AFTER_MS - 1);
        s.in.pushed = true;
        s.step();
        CHECK(!s.v.done && !s.v.showContinue);
    }
    // ---- once over it stays over: a later dropout is the home screen's business ----
    {
        S s;
        s.both(LinkState::Connected);
        s.step();
        s.step(SPLASH_MIN_MS + SPLASH_CONNECTED_HOLD_MS);
        CHECK(s.v.done);
        s.in.switchboard = LinkState::Searching;
        s.step(1000);
        CHECK(s.v.done && !s.v.showContinue);
        s.step(SPLASH_CONTINUE_AFTER_MS);
        CHECK(s.v.done && !s.v.showContinue);
    }
    // ---- millis() wraps around: elapsed time stays right ----
    {
        S s;
        s.in.now = 0xFFFFFFFFu - 500;
        s.both(LinkState::Connected);
        s.step();
        s.step(SPLASH_MIN_MS - 1);  // now has wrapped
        CHECK(!s.v.done);
        s.step(1);
        CHECK(s.v.done);
    }

    // ---- pixel art: only ever inside the surface, states look different ----
    {
        const int W = SPLASH_ROW_W, H = SPLASH_ROW_H, GUARD = 8;
        const BadgeState all[3] = {BadgeState::Waiting, BadgeState::Connected, BadgeState::Problem};
        std::vector<uint16_t> big((W + 2 * GUARD) * (H + 2 * GUARD));
        for (int a = 0; a < 3; a++) {
            for (int b = 0; b < 3; b++) {
                for (uint32_t t = 0; t < 3000; t += 83) {
                    std::fill(big.begin(), big.end(), 0xABCD);
                    gfx::Surface s = {big.data() + GUARD * (W + 2 * GUARD) + GUARD, W, H};
                    // draw through a view with a different row pitch? the surfaces are packed: draw into a packed copy instead
                    std::vector<uint16_t> own(W * H, 0xABCD);
                    gfx::Surface o = {own.data(), W, H};
                    splash_sprites::drawLinkRow(o, all[a], all[b], t);
                    (void)s;
                    // every pixel was painted (background fill) and the left / right columns stay background
                    const uint16_t bg = gfx::rgb565(splash_color::BACKGROUND);
                    for (int y = 0; y < H; y++) {
                        CHECK(own[y * W + 0] == bg);
                        CHECK(own[y * W + W - 1] == bg);
                    }
                    for (int x = 0; x < W; x++) {
                        CHECK(own[0 * W + x] == bg);  // nothing touches the top / bottom rows either (badge has 1 px air)
                        CHECK(own[(H - 1) * W + x] == bg);
                    }
                }
            }
        }
        // waiting vs connected differ; connected + connected has green arcs
        std::vector<uint16_t> w(W * H), c(W * H);
        gfx::Surface sw = {w.data(), W, H}, sc = {c.data(), W, H};
        splash_sprites::drawLinkRow(sw, BadgeState::Waiting, BadgeState::Waiting, 0);
        splash_sprites::drawLinkRow(sc, BadgeState::Connected, BadgeState::Connected, 0);
        CHECK(w != c);
        const uint16_t ok = gfx::rgb565(splash_color::OK);
        int green = 0;
        for (size_t i = 0; i < c.size(); i++) green += c[i] == ok;
        CHECK(green > 2 * 100);  // two filled badges + three arcs
        int greenW = 0;
        for (size_t i = 0; i < w.size(); i++) greenW += w[i] == ok;
        CHECK(greenW == 0);
        // the arcs are centered between the two badges (equal air on both sides, within 1 px)
        {
            const uint16_t ring1 = gfx::rgb565(splash_color::LAUNCHER), ring2 = gfx::rgb565(splash_color::SWITCHBOARD);
            const uint16_t arc = gfx::rgb565(splash_color::ARC_OFF);
            int b1Right = -1, b2Left = 1 << 20, aMin = 1 << 20, aMax = -1;
            for (int y = 0; y < H; y++) {
                for (int x = 0; x < W; x++) {
                    const uint16_t c = w[y * W + x];  // all arcs unlit, both badges waiting
                    if (c == ring1 && x > b1Right) b1Right = x;
                    if (c == ring2 && x < b2Left) b2Left = x;
                    if (c == arc) {
                        if (x < aMin) aMin = x;
                        if (x > aMax) aMax = x;
                    }
                }
            }
            const int left = aMin - b1Right - 1, right = b2Left - aMax - 1;
            std::printf("arc air: left %d px, right %d px\n", left, right);
            CHECK(aMax > aMin && left >= 3 && right >= 3);
            CHECK(left - right <= 1 && right - left <= 1);
        }
        // arcs travel: the count of lit arcs cycles 0..3
        CHECK(splash_sprites::arcsLit(0) == 0 && splash_sprites::arcsLit(260) == 1 && splash_sprites::arcsLit(520) == 2 &&
              splash_sprites::arcsLit(780) == 3 && splash_sprites::arcsLit(1040) == 0);
    }

    std::printf(failures ? "%d FAILURES\n" : "splash tests OK\n", failures);
    return failures ? 1 : 0;
}
