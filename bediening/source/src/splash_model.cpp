#include "splash_model.h"

#include "splash_config.h"

BadgeState splash_badge_for(LinkState s) {
    switch (s) {
        case LinkState::Connected: return BadgeState::Connected;
        case LinkState::Error: return BadgeState::Problem;
        case LinkState::NotConfigured:
        case LinkState::Searching:
        case LinkState::Connecting: return BadgeState::Waiting;
    }
    return BadgeState::Waiting;
}

void SplashModel::update(const SplashInputs& in, SplashView& out) {
    if (!started_) {
        started_ = true;
        startAt_ = in.now;
    }
    const uint32_t elapsed = in.now - startAt_;

    out.badge[SPLASH_BADGE_LAUNCHER] = splash_badge_for(in.launcher);
    out.badge[SPLASH_BADGE_SWITCHBOARD] = splash_badge_for(in.switchboard);
    out.elapsed = elapsed;

    if (!done_) {
        const bool both = in.launcher == LinkState::Connected && in.switchboard == LinkState::Connected;
        if (both && !both_) bothSince_ = in.now;
        both_ = both;

        // The offer is made from the clock alone; a push only counts once it is on offer.
        const bool offer = elapsed >= SPLASH_CONTINUE_AFTER_MS;
        if (offer && in.pushed) {
            done_ = true;
            skipped_ = true;
        } else if (both_ && elapsed >= SPLASH_MIN_MS && (in.now - bothSince_) >= SPLASH_CONNECTED_HOLD_MS) {
            done_ = true;
        }
    }

    out.done = done_;
    out.skipped = skipped_;
    out.showContinue = !done_ && elapsed >= SPLASH_CONTINUE_AFTER_MS;
}
