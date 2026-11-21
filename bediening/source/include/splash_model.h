#pragma once

// Pure model of the start-up splash screen: from the connection state of the two devices (and the knob) to WHAT to show:
// the two badges and the "push = continue" offer, and WHEN the splash is over.
// No LVGL, no Arduino.
//
// Rules:
//  * The splash is over when BOTH devices are Connected, it has been visible at least SPLASH_MIN_MS, and both badges have
//    shown "connected" for SPLASH_CONNECTED_HOLD_MS (so the driver sees it succeed). A device that drops out again
//    while this runs simply makes it wait again.
//  * After SPLASH_CONTINUE_AFTER_MS without both devices the splash offers "push = continue"; only then a push ends it.
//  * Once over, it stays over (a later dropout is the home screen's OFFLINE band, not the splash).

#include <stdint.h>

#include "link_state.h"

enum class BadgeState : uint8_t { Waiting, Connected, Problem };

enum SplashBadge : uint8_t { SPLASH_BADGE_LAUNCHER = 0, SPLASH_BADGE_SWITCHBOARD = 1, SPLASH_BADGE_COUNT };

struct SplashInputs {
    uint32_t now = 0;
    LinkState launcher = LinkState::NotConfigured;
    LinkState switchboard = LinkState::NotConfigured;
    bool pushed = false;  // the knob was pushed (and released) since the last update
};

struct SplashView {
    bool done = false;
    bool skipped = false;  // done because the driver pushed (not because both devices connected)
    BadgeState badge[SPLASH_BADGE_COUNT] = {BadgeState::Waiting, BadgeState::Waiting};
    bool showContinue = false;
    uint32_t elapsed = 0;  // ms since the splash started (animation clock)
};

class SplashModel {
   public:
    // The first call starts the clock.
    void update(const SplashInputs& in, SplashView& out);
    bool done() const { return done_; }

   private:
    bool started_ = false;
    uint32_t startAt_ = 0;
    bool done_ = false;
    bool skipped_ = false;
    bool both_ = false;
    uint32_t bothSince_ = 0;
};

BadgeState splash_badge_for(LinkState s);
