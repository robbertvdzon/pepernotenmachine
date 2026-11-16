#pragma once

// Runs one Pattern (see control_patterns.h): tells whether the device is on or off right now. Pure, host-testable.
// The random durations come from a caller-owned generator state, so the tests are reproducible.

#include <stdint.h>

#include "control_patterns.h"

uint32_t pattern_random(uint32_t& state);  // advances the generator, returns 24 random bits

class PatternRunner {
   public:
    void reset() { active_ = false; }
    bool active() const { return active_; }
    // `now` in ms. The first call after reset() starts the pattern at its first phase.
    bool step(const Pattern& p, uint32_t now, uint32_t& rng);

   private:
    uint32_t duration(const Pattern& p, uint8_t phase, uint32_t start, uint32_t& rng) const;

    bool active_ = false;
    uint8_t phase_ = 0;
    uint32_t phaseEnd_ = 0;
    uint32_t cycleStart_ = 0;
};
