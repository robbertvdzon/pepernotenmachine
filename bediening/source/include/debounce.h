#pragma once

// Pure, hardware-independent time-based debouncer (host-testable).
// A new level is accepted only after it has been continuously observed for
// `debounce_ms`. Unsigned subtraction keeps it correct across millis() wrap.

#include <stdint.h>

class Debouncer {
   public:
    void reset(bool state, uint32_t now) {
        stable_ = state;
        candidate_ = state;
        since_ = now;
    }

    // Feed the latest raw sample. Returns true exactly once per debounced change.
    bool update(bool raw, uint32_t now, uint32_t debounce_ms) {
        if (raw != candidate_) {
            candidate_ = raw;
            since_ = now;
        }
        if (candidate_ != stable_ && static_cast<uint32_t>(now - since_) >= debounce_ms) {
            stable_ = candidate_;
            return true;
        }
        return false;
    }

    bool state() const { return stable_; }

   private:
    bool stable_ = false;
    bool candidate_ = false;
    uint32_t since_ = 0;
};
