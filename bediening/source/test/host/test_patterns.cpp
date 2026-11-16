// Host tests for the pattern engine (what a device does over time) and the pattern definitions.
//   g++ -std=c++11 -Wall -Wextra -Iinclude test/host/test_patterns.cpp src/pattern_runner.cpp -o /tmp/test_patterns && /tmp/test_patterns

#include <cstdio>
#include <vector>

#include "pattern_runner.h"

static int failures = 0;
#define CHECK(cond)                                             \
    do {                                                        \
        if (!(cond)) {                                          \
            std::printf("FAIL line %d: %s\n", __LINE__, #cond); \
            failures++;                                         \
        }                                                       \
    } while (0)

struct Seg {
    bool on;
    uint32_t ms;
};

// Runs a pattern with `step_ms` steps and returns the on/off segments (the last one is partial).
static std::vector<Seg> run(const Pattern& p, uint32_t total_ms, uint32_t step_ms, uint32_t seed = 1) {
    PatternRunner r;
    uint32_t rng = seed;
    std::vector<Seg> out;
    for (uint32_t t = 0; t < total_ms; t += step_ms) {
        const bool on = r.step(p, t, rng);
        if (out.empty() || out.back().on != on) out.push_back({on, 0});
        out.back().ms += step_ms;
    }
    return out;
}

static void test_fixed_pattern_is_exact_and_starts_with_its_first_phase() {
    const Phase ph[] = {{2000, 2000, true}, {2000, 2000, false}, {4000, 4000, true}, {2000, 2000, false}};
    const Pattern p = {ph, 4, 0};
    const std::vector<Seg> seg = run(p, 60000, 10);
    CHECK(seg.size() >= 12 && seg[0].on);
    const uint32_t expect[4] = {2000, 2000, 4000, 2000};
    for (size_t i = 0; i + 1 < seg.size(); i++) {
        CHECK(seg[i].ms == expect[i % 4]);
        CHECK(seg[i].on == (i % 4 == 0 || i % 4 == 2));
    }
}

static void test_random_phases_stay_in_range_and_vary() {
    const Phase ph[] = {{3000, 5000, true}, {1000, 3000, false}};
    const Pattern p = {ph, 2, 0};
    for (uint32_t seed = 1; seed < 6; seed++) {
        const std::vector<Seg> seg = run(p, 300000, 10, seed);
        bool vary_on = false, vary_off = false;
        for (size_t i = 0; i + 1 < seg.size(); i++) {
            if (seg[i].on) { CHECK(seg[i].ms >= 3000 && seg[i].ms <= 5000 + 10); vary_on |= seg[i].ms != seg[0].ms; }
            else { CHECK(seg[i].ms >= 1000 && seg[i].ms <= 3000 + 10); vary_off |= seg[i].ms != seg[1].ms; }
        }
        CHECK(vary_on && vary_off);
    }
    // the whole range is used (not just the minimum)
    const std::vector<Seg> seg = run(p, 3000000, 10, 7);
    uint32_t lo = 99999, hi = 0;
    for (size_t i = 0; i + 1 < seg.size(); i++) if (seg[i].on) { if (seg[i].ms < lo) lo = seg[i].ms; if (seg[i].ms > hi) hi = seg[i].ms; }
    CHECK(lo < 3300 && hi > 4700);
}

static void test_period_is_exact_with_a_filling_last_phase() {
    // briefly on once per 10 s: the cycle is exactly 10 s, however long the random on-phase was
    const Phase ph[] = {{400, 600, true}, {0, 0, false}};
    const Pattern p = {ph, 2, 10000};
    const std::vector<Seg> seg = run(p, 600000, 10, 3);
    uint32_t cycles = 0;
    for (size_t i = 0; i + 2 < seg.size(); i += 2) {
        CHECK(seg[i].on && seg[i].ms >= 400 && seg[i].ms <= 610);
        CHECK(seg[i].ms + seg[i + 1].ms == 10000);                       // no drift, ever
        cycles++;
    }
    CHECK(cycles >= 58);

    // a random moment once per minute, exactly one minute per cycle
    const Phase fog[] = {{0, 58000, false}, {2000, 2000, true}, {0, 0, false}};
    const Pattern f = {fog, 3, 60000};
    const std::vector<Seg> fs = run(f, 3600000, 20, 5);
    uint32_t on_count = 0, on_total = 0;
    for (const Seg& s : fs) if (s.on) { on_count++; on_total += s.ms; }
    CHECK(on_count >= 58 && on_count <= 61);
    CHECK(on_total >= on_count * 2000 - 40 && on_total <= on_count * 2000 + 40);
    // the moments are random: not always the same offset in the minute
    PatternRunner r;
    uint32_t rng = 9, t = 0;
    uint32_t first_on[6];
    int idx = 0;
    bool prev = false;
    for (t = 0; t < 360000 && idx < 6; t += 20) {
        const bool on = r.step(f, t, rng);
        if (on && !prev) first_on[idx++] = t % 60000;
        prev = on;
    }
    CHECK(idx == 6);
    bool different = false;
    for (int i = 1; i < 6; i++) different |= first_on[i] != first_on[0];
    CHECK(different);
}

static void test_period_does_not_drift_with_awkward_step_sizes() {
    // steps that do not divide the period: every cycle still starts where the previous one ended
    const Phase ph[] = {{400, 600, true}, {0, 0, false}};
    const Pattern p = {ph, 2, 10000};
    PatternRunner r;
    uint32_t rng = 1;
    std::vector<uint32_t> rises;
    bool prev = false;
    for (uint32_t t = 0; t < 1200000; t += 7) {    // 20 minutes in 7 ms steps
        const bool on = r.step(p, t, rng);
        if (on && !prev) rises.push_back(t);
        prev = on;
    }
    CHECK(rises.size() >= 119);
    for (size_t k = 0; k < rises.size(); k++) CHECK(rises[k] + 7 >= rises[0] + k * 10000 && rises[k] <= rises[0] + k * 10000 + 7);
}

static void test_both_ends_of_the_random_range_are_used() {
    const Phase ph[] = {{5, 7, true}, {5, 7, false}};
    const Pattern p = {ph, 2, 0};
    PatternRunner r;
    uint32_t rng = 3;
    bool seen[8] = {false};
    uint32_t last_change = 0;
    bool prev = false;
    for (uint32_t t = 0; t < 20000; t++) {
        const bool on = r.step(p, t, rng);
        if (on != prev) {
            if (t > 0 && t - last_change < 8) seen[t - last_change] = true;
            last_change = t;
            prev = on;
        }
    }
    CHECK(seen[5] && seen[6] && seen[7]);          // the minimum, the middle and the maximum all occur
    CHECK(!seen[4] && !seen[3]);
}

static void test_reset_restarts_at_the_first_phase() {
    const Phase ph[] = {{1000, 1000, false}, {3000, 3000, true}};
    const Pattern p = {ph, 2, 0};
    PatternRunner r;
    uint32_t rng = 1;
    bool on = false;
    for (uint32_t t = 0; t <= 2000; t += 10) on = r.step(p, t, rng);
    CHECK(on);                         // in the on-phase
    r.reset();
    CHECK(!r.step(p, 2010, rng));      // restarted: the first phase is "off" again
    CHECK(!r.step(p, 2900, rng));
    CHECK(r.step(p, 3100, rng));
}

static void test_long_gaps_and_degenerate_patterns_never_hang() {
    const Phase ph[] = {{100, 100, true}, {100, 100, false}};
    const Pattern p = {ph, 2, 0};
    PatternRunner r;
    uint32_t rng = 1;
    r.step(p, 0, rng);
    r.step(p, 1000000, rng);           // a huge jump: must return (catching up step by step)
    for (uint32_t t = 1000000; t < 1010000; t += 10) r.step(p, t, rng);
    CHECK(true);

    const Phase zero[] = {{0, 0, true}, {0, 0, false}};
    const Pattern z = {zero, 2, 0};
    PatternRunner rz;
    rz.step(z, 0, rng);
    rz.step(z, 100, rng);              // all-zero durations: no endless loop
    CHECK(true);

    const Pattern none = {nullptr, 0, 0};
    PatternRunner rn;
    CHECK(!rn.step(none, 0, rng));
}

static void test_the_clock_wraps_around_safely() {
    const Phase ph[] = {{1000, 1000, true}, {1000, 1000, false}};
    const Pattern p = {ph, 2, 0};
    PatternRunner r;
    uint32_t rng = 1;
    const uint32_t start = 0xFFFFFFFFu - 2500;   // millis() wraps in the middle of the pattern
    std::vector<bool> seq;
    for (uint32_t i = 0; i < 600; i++) seq.push_back(r.step(p, start + i * 10, rng));
    int changes = 0;
    for (size_t i = 1; i < seq.size(); i++) changes += seq[i] != seq[i - 1];
    CHECK(changes >= 5);               // 6 s = about 5 changes, and the wrap in between changed nothing
}

int main() {
    test_fixed_pattern_is_exact_and_starts_with_its_first_phase();
    test_random_phases_stay_in_range_and_vary();
    test_period_is_exact_with_a_filling_last_phase();
    test_period_does_not_drift_with_awkward_step_sizes();
    test_both_ends_of_the_random_range_are_used();
    test_reset_restarts_at_the_first_phase();
    test_long_gaps_and_degenerate_patterns_never_hang();
    test_the_clock_wraps_around_safely();
    if (failures == 0) {
        std::printf("ALL PATTERN TESTS PASSED (8 tests)\n");
        return 0;
    }
    std::printf("%d CHECK(S) FAILED\n", failures);
    return 1;
}
