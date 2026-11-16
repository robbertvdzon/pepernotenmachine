#include "pattern_runner.h"

uint32_t pattern_random(uint32_t& state) {
    state = state * 1664525u + 1013904223u;
    return state >> 8;
}

uint32_t PatternRunner::duration(const Pattern& p, uint8_t phase, uint32_t start, uint32_t& rng) const {
    if (p.periodMs > 0 && phase == p.count - 1) {  // the last phase fills what is left of the period
        const uint32_t used = static_cast<uint32_t>(start - cycleStart_);
        return used >= p.periodMs ? 0 : p.periodMs - used;
    }
    const Phase& ph = p.phases[phase];
    if (ph.maxMs <= ph.minMs) return ph.minMs;
    return ph.minMs + pattern_random(rng) % (ph.maxMs - ph.minMs + 1);
}

bool PatternRunner::step(const Pattern& p, uint32_t now, uint32_t& rng) {
    if (p.count == 0 || p.phases == nullptr) return false;
    if (!active_) {
        active_ = true;
        phase_ = 0;
        cycleStart_ = now;
        phaseEnd_ = now + duration(p, 0, now, rng);
    }
    // Move on through every phase that is over. The guard keeps an all-zero pattern from spinning forever.
    for (uint8_t guard = 0; guard <= p.count && static_cast<int32_t>(now - phaseEnd_) >= 0; guard++) {
        const uint32_t end = phaseEnd_;
        phase_++;
        if (phase_ >= p.count) {  // a new cycle starts exactly where the old one ended (no drift)
            phase_ = 0;
            cycleStart_ = end;
        }
        phaseEnd_ = end + duration(p, phase_, end, rng);
    }
    return p.phases[phase_].on;
}
