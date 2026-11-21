// Host-side tests for the pure logic headers (no hardware, no Arduino).
// Plain asserts, not Unity. Build & run:
//   g++ -std=c++17 -Wall -Wextra -Iinclude test/host/test_logic.cpp -o /tmp/test_logic && /tmp/test_logic

#include <array>
#include <set>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "ble_address.h"
#include "ble_devices.h"
#include "ble_identity.h"
#include "link_state.h"
#include "debounce.h"
#include "device_record.h"
#include "led_pattern.h"
#include "encoder_decoder.h"
#include "event_queue.h"

static int failures = 0;
#define CHECK(cond)                                                        \
    do {                                                                   \
        if (!(cond)) {                                                     \
            std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);    \
            failures++;                                                    \
        }                                                                  \
    } while (0)

// ------------------------------------------------------------- Debouncer
static void test_debounce_clean_change() {
    Debouncer d;
    d.reset(false, 0);
    CHECK(!d.update(true, 100, 25));   // first sight of new level
    CHECK(!d.update(true, 120, 25));   // 20 ms: not yet
    CHECK(d.update(true, 125, 25));    // 25 ms: accepted, exactly once
    CHECK(d.state());
    CHECK(!d.update(true, 200, 25));   // no repeat event
}

static void test_debounce_rejects_bounce() {
    Debouncer d;
    d.reset(false, 0);
    // Contact bounce: toggles faster than the debounce time.
    CHECK(!d.update(true, 100, 25));
    CHECK(!d.update(false, 105, 25));
    CHECK(!d.update(true, 110, 25));
    CHECK(!d.update(false, 115, 25));
    CHECK(!d.update(true, 120, 25));
    CHECK(!d.update(true, 140, 25));   // only 20 ms stable since 120
    CHECK(!d.state());
    CHECK(d.update(true, 145, 25));    // 25 ms stable since 120
    CHECK(d.state());
}

static void test_debounce_glitch_does_not_change_state() {
    Debouncer d;
    d.reset(true, 0);
    CHECK(!d.update(false, 1000, 25));  // short glitch
    CHECK(!d.update(true, 1010, 25));   // back to stable level
    CHECK(!d.update(true, 1100, 25));
    CHECK(d.state());
}

static void test_debounce_release_reports_once() {
    Debouncer d;
    d.reset(true, 0);
    int events = 0;
    for (uint32_t t = 100; t < 300; t += 5) events += d.update(false, t, 25) ? 1 : 0;
    CHECK(events == 1);
    CHECK(!d.state());
}

static void test_debounce_millis_wraparound() {
    Debouncer d;
    const uint32_t start = 0xFFFFFFF0u;  // 16 ms before wrap
    d.reset(false, start);
    CHECK(!d.update(true, start, 25));
    CHECK(!d.update(true, start + 10, 25));
    CHECK(d.update(true, start + 30, 25));  // crosses the wrap
    CHECK(d.state());
}

// ---------------------------------------------------- QuadratureDecoder
// One clockwise detent in the sequence the original test project decodes
// as +1: 00 -> 01 -> 11 -> 10 -> 00 produces table sums +1 each... verified
// below against the actual table rather than assumed.
static int feed(QuadratureDecoder& q, const uint8_t* seq, int n) {
    int total = 0;
    for (int i = 0; i < n; i++) total += q.step(seq[i]);
    return total;
}

static void test_decoder_full_detent_each_direction() {
    // Both directions must give exactly one detent of opposite sign.
    const uint8_t a[] = {0b01, 0b11, 0b10, 0b00};  // 00 -> 01 -> 11 -> 10 -> 00
    const uint8_t b[] = {0b10, 0b11, 0b01, 0b00};  // 00 -> 10 -> 11 -> 01 -> 00
    QuadratureDecoder q1, q2;
    q1.reset(0b00);
    q2.reset(0b00);
    const int ra = feed(q1, a, 4);
    const int rb = feed(q2, b, 4);
    CHECK(ra == 1 || ra == -1);
    CHECK(rb == -ra);
}

static void test_decoder_repeats_and_partial() {
    const uint8_t a[] = {0b01, 0b11, 0b10, 0b00};
    QuadratureDecoder q;
    q.reset(0b00);
    const int one = feed(q, a, 4);
    CHECK(one != 0);
    // 3 detents in a row
    int total = one;
    total += feed(q, a, 4);
    total += feed(q, a, 4);
    CHECK(total == 3 * one);

    // A partial turn (3 of 4 transitions) and back must produce no detent.
    QuadratureDecoder p;
    p.reset(0b00);
    int r = 0;
    r += p.step(0b01);
    r += p.step(0b11);
    r += p.step(0b01);  // reverse
    r += p.step(0b00);
    CHECK(r == 0);
}

static void test_decoder_ignores_invalid_transition() {
    QuadratureDecoder q;
    q.reset(0b00);
    CHECK(q.step(0b11) == 0);  // 00 -> 11 skips a state: invalid, contributes 0
    // Decoder recovers: a clean full turn from the current state still yields one detent.
    const uint8_t a[] = {0b10, 0b00, 0b01, 0b11};  // 11 -> 10 -> 00 -> 01 -> 11
    int r = feed(q, a, 4);
    CHECK(r == 1 || r == -1);
}

// ------------------------------------------------------------ EventQueue
static void test_queue_fifo_and_overflow() {
    EventQueue<int, 3> q;
    int v = 0;
    CHECK(!q.pop(v));
    CHECK(q.push(1));
    CHECK(q.push(2));
    CHECK(q.push(3));
    CHECK(!q.push(4));          // full: dropped and counted
    CHECK(q.dropped() == 1);
    CHECK(q.size() == 3);
    CHECK(q.pop(v) && v == 1);
    CHECK(q.push(5));           // wraps around
    CHECK(q.pop(v) && v == 2);
    CHECK(q.pop(v) && v == 3);
    CHECK(q.pop(v) && v == 5);
    CHECK(!q.pop(v));
}


// ------------------------------------------------------- BLE identity
static void test_identity() {
    // Strong: service UUID, regardless of name.
    Identity a = classify_advertisement("whatever", true, false);
    CHECK(a.kind == DeviceKind::Launcher && a.strong && !a.conflict);
    Identity b = classify_advertisement(nullptr, false, true);
    CHECK(b.kind == DeviceKind::Switchboard && b.strong);
    // Weak: name only.
    Identity c = classify_advertisement("Peppernut launcher", false, false);
    CHECK(c.kind == DeviceKind::Launcher && !c.strong && !c.conflict);
    Identity d = classify_advertisement("IO Switchboard", false, false);
    CHECK(d.kind == DeviceKind::Switchboard && !d.strong);
    // Lookalike names are NOT matched.
    CHECK(classify_advertisement("Peppernut launcher 2", false, false).kind == DeviceKind::Unknown);
    CHECK(classify_advertisement("peppernut launcher", false, false).kind == DeviceKind::Unknown);
    CHECK(classify_advertisement("", false, false).kind == DeviceKind::Unknown);
    // Agreeing name + UUID is strong.
    Identity e = classify_advertisement("Peppernut launcher", true, false);
    CHECK(e.kind == DeviceKind::Launcher && e.strong && !e.conflict);
    // Disagreement is a conflict and is not classified.
    Identity f = classify_advertisement("IO Switchboard", true, false);
    CHECK(f.kind == DeviceKind::Unknown && f.conflict);
    Identity g = classify_advertisement("x", true, true);
    CHECK(g.kind == DeviceKind::Unknown && g.conflict);
}

// ------------------------------------------------------- BLE address
static void test_address() {
    // NimBLE native order is little-endian: 06:C4:E6:F9:D1:08 reversed.
    const uint8_t native[6] = {0x06, 0xC4, 0xE6, 0xF9, 0xD1, 0x08};
    uint8_t disp[6];
    ble_address_from_native(native, disp);
    char text[18];
    CHECK(ble_address_format(disp, text, sizeof(text)));
    CHECK(strcmp(text, "08:D1:F9:E6:C4:06") == 0);

    uint8_t parsed[6];
    CHECK(ble_address_parse("08:D1:F9:E6:C4:06", parsed));
    CHECK(ble_address_equal(parsed, disp));
    CHECK(ble_address_parse("ec:da:3b:55:2b:69", parsed));  // lower case accepted
    CHECK(parsed[0] == 0xEC && parsed[5] == 0x69);

    CHECK(!ble_address_parse("08:D1:F9:E6:C4", parsed));      // too short
    CHECK(!ble_address_parse("08:D1:F9:E6:C4:06:00", parsed)); // too long
    CHECK(!ble_address_parse("08-D1-F9-E6-C4-06", parsed));   // wrong separator
    CHECK(!ble_address_parse("0G:D1:F9:E6:C4:06", parsed));   // not hex
    CHECK(!ble_address_parse(nullptr, parsed));
    CHECK(!ble_address_format(disp, text, 10));                // buffer too small
    CHECK(ble_address_is_zero(std::array<uint8_t,6>{}.data()));
}

// ------------------------------------------------- persistence records
static StoredDevice make_device(const char* name) {
    StoredDevice d;
    d.valid = true;
    const uint8_t addr[6] = {0x08, 0xD1, 0xF9, 0xE6, 0xC4, 0x06};
    memcpy(d.addr, addr, 6);
    d.addrType = 0;
    strncpy(d.name, name, DEVICE_NAME_MAX);
    return d;
}

static void test_record_roundtrip() {
    StoredDevice in = make_device("Peppernut launcher");
    uint8_t buf[DEVICE_RECORD_MAX];
    const size_t n = device_record_encode(in, buf, sizeof(buf));
    CHECK(n == DEVICE_RECORD_HEADER + strlen("Peppernut launcher"));
    StoredDevice out;
    CHECK(device_record_decode(buf, n, out));
    CHECK(out.valid && out.addrType == 0);
    CHECK(ble_address_equal(in.addr, out.addr));
    CHECK(strcmp(out.name, "Peppernut launcher") == 0);

    // Empty name and random address type are fine.
    StoredDevice e = make_device("");
    e.addrType = 1;
    const size_t n2 = device_record_encode(e, buf, sizeof(buf));
    CHECK(n2 == DEVICE_RECORD_HEADER);
    CHECK(device_record_decode(buf, n2, out) && out.addrType == 1 && out.name[0] == 0);
}

static void test_record_defaults_and_corruption() {
    StoredDevice out;
    uint8_t buf[DEVICE_RECORD_MAX];

    // Default / invalid devices are never encoded.
    StoredDevice none;
    CHECK(device_record_encode(none, buf, sizeof(buf)) == 0);
    // Buffer too small.
    StoredDevice d = make_device("Peppernut launcher");
    CHECK(device_record_encode(d, buf, 5) == 0);

    const size_t n = device_record_encode(d, buf, sizeof(buf));
    CHECK(n > 0);

    // Empty / short / truncated / extended data decodes to "no device".
    CHECK(!device_record_decode(buf, 0, out) && !out.valid);
    CHECK(!device_record_decode(buf, 4, out) && !out.valid);
    CHECK(!device_record_decode(buf, n - 1, out));
    uint8_t longer[DEVICE_RECORD_MAX + 4];
    memcpy(longer, buf, n);
    CHECK(!device_record_decode(longer, n + 1, out));
    CHECK(!device_record_decode(nullptr, n, out));

    // Bad version, bad address type, bad name length, all-zero address.
    uint8_t bad[DEVICE_RECORD_MAX];
    memcpy(bad, buf, n);
    bad[0] = 99;
    CHECK(!device_record_decode(bad, n, out));
    memcpy(bad, buf, n);
    bad[1] = 7;
    CHECK(!device_record_decode(bad, n, out));
    memcpy(bad, buf, n);
    bad[8] = 200;
    CHECK(!device_record_decode(bad, n, out));
    memcpy(bad, buf, n);
    memset(bad + 2, 0, 6);
    CHECK(!device_record_decode(bad, n, out));
}

// ------------------------------------------------------- LED patterns
static void test_led_static_states() {
    for (uint32_t t : {0u, 1u, 499u, 500u, 12345u, 0xFFFFFFFFu}) {
        CHECK(led_pattern_brightness(LED_OFF, t) == 0);
        CHECK(led_pattern_brightness(LED_ON, t) == 255);
    }
}

static void test_led_flash() {
    // 1 Hz, 50 % duty, starts ON.
    CHECK(led_pattern_brightness(LED_FLASH, 0) == 255);
    CHECK(led_pattern_brightness(LED_FLASH, 499) == 255);
    CHECK(led_pattern_brightness(LED_FLASH, 500) == 0);
    CHECK(led_pattern_brightness(LED_FLASH, 999) == 0);
    CHECK(led_pattern_brightness(LED_FLASH, 1000) == 255);  // next period
    // Fast flash: 5 Hz, clearly different from the slow flash.
    CHECK(led_pattern_brightness(LED_FLASH_FAST, 0) == 255);
    CHECK(led_pattern_brightness(LED_FLASH_FAST, 99) == 255);
    CHECK(led_pattern_brightness(LED_FLASH_FAST, 100) == 0);
    CHECK(led_pattern_brightness(LED_FLASH_FAST, 199) == 0);
    CHECK(led_pattern_brightness(LED_FLASH_FAST, 200) == 255);
    // Count transitions over 2 s: slow = 2 periods, fast = 10 periods.
    int slow_on = 0, fast_on = 0;
    for (uint32_t t = 0; t < 2000; t++) {
        slow_on += led_pattern_brightness(LED_FLASH, t) ? 1 : 0;
        fast_on += led_pattern_brightness(LED_FLASH_FAST, t) ? 1 : 0;
    }
    CHECK(slow_on == 1000 && fast_on == 1000);  // both 50 % duty
}

static void test_led_pulse() {
    CHECK(led_pattern_brightness(LED_PULSE, 0) == 0);
    CHECK(led_pattern_brightness(LED_PULSE, LED_PULSE_PERIOD_MS / 2) == 255);
    CHECK(led_pattern_brightness(LED_PULSE, LED_PULSE_PERIOD_MS) == 0);  // wraps
    // Symmetric triangle: b(t) == b(P - t).
    for (uint32_t t = 1; t < LED_PULSE_PERIOD_MS; t += 7)
        CHECK(led_pattern_brightness(LED_PULSE, t) == led_pattern_brightness(LED_PULSE, LED_PULSE_PERIOD_MS - t));
    // Rising monotonic in the first half, falling in the second.
    uint8_t prev = 0;
    for (uint32_t t = 0; t <= LED_PULSE_PERIOD_MS / 2; t++) {
        const uint8_t b = led_pattern_brightness(LED_PULSE, t);
        CHECK(b >= prev);
        prev = b;
    }
    for (uint32_t t = LED_PULSE_PERIOD_MS / 2; t < LED_PULSE_PERIOD_MS; t++) {
        const uint8_t b = led_pattern_brightness(LED_PULSE, t);
        CHECK(b <= prev);
        prev = b;
    }
}

static void test_led_ramps() {
    CHECK(led_pattern_brightness(LED_RAMP_UP, 0) == 0);
    CHECK(led_pattern_brightness(LED_RAMP_UP, LED_RAMP_PERIOD_MS - 1) == 255);
    CHECK(led_pattern_brightness(LED_RAMP_UP, LED_RAMP_PERIOD_MS) == 0);  // jumps back
    CHECK(led_pattern_brightness(LED_RAMP_DOWN, 0) == 255);
    CHECK(led_pattern_brightness(LED_RAMP_DOWN, LED_RAMP_PERIOD_MS - 1) == 0);
    uint8_t prev = 0;
    for (uint32_t t = 0; t < LED_RAMP_PERIOD_MS; t++) {
        const uint8_t up = led_pattern_brightness(LED_RAMP_UP, t);
        const uint8_t down = led_pattern_brightness(LED_RAMP_DOWN, t);
        CHECK(up >= prev);
        CHECK(up + down == 255);  // exact mirror images
        prev = up;
    }
}

static void test_led_levels_and_fading_flag() {
    CHECK(led_brightness_to_level(0) == 0);
    CHECK(led_brightness_to_level(255) == LED_PWM_LEVELS);
    uint8_t prev = 0;
    for (int b = 0; b <= 255; b++) {
        const uint8_t l = led_brightness_to_level(static_cast<uint8_t>(b));
        CHECK(l >= prev && l <= LED_PWM_LEVELS);
        prev = l;
    }
    // Steady states map only to 0 or full, so no PWM traffic is needed for them.
    CHECK(led_brightness_to_level(led_pattern_brightness(LED_ON, 5)) == LED_PWM_LEVELS);
    CHECK(led_brightness_to_level(led_pattern_brightness(LED_OFF, 5)) == 0);
    CHECK(!led_state_is_fading(LED_OFF) && !led_state_is_fading(LED_ON));
    CHECK(!led_state_is_fading(LED_FLASH) && !led_state_is_fading(LED_FLASH_FAST));
    CHECK(led_state_is_fading(LED_PULSE) && led_state_is_fading(LED_RAMP_UP) && led_state_is_fading(LED_RAMP_DOWN));
    CHECK(led_state_is_valid(LED_RAMP_DOWN) && !led_state_is_valid(7) && !led_state_is_valid(-1));
}

// ------------------------------------------------------- link state
static void test_link_state_mapping() {
    CHECK(link_state_led(LinkState::NotConfigured) == LED_OFF);
    CHECK(link_state_led(LinkState::Searching) == LED_FLASH);
    CHECK(link_state_led(LinkState::Connecting) == LED_FLASH);
    CHECK(link_state_led(LinkState::Connected) == LED_ON);
    CHECK(link_state_led(LinkState::Error) == LED_FLASH_FAST);
    // All texts and error details must fit a card on the 160 px display (17 chars at 8 px).
    const LinkState all[] = {LinkState::NotConfigured, LinkState::Searching, LinkState::Connecting,
                             LinkState::Connected, LinkState::Error};
    for (LinkState st : all) CHECK(strlen(link_state_text(st)) > 0 && strlen(link_state_text(st)) <= 17);
    const char* errs[] = {LINK_ERR_SERVICE_MISSING, LINK_ERR_CHAR_MISSING, LINK_ERR_NOT_WRITABLE,
                          LINK_ERR_SUBSCRIBE, LINK_ERR_READ, LINK_ERR_NO_CLIENT};
    for (const char* e : errs) CHECK(strlen(e) > 0 && strlen(e) <= 17);
}

// ------------------------------------------------- device characteristic tables
static bool uuid_shape_ok(const char* u) {
    if (strlen(u) != 36) return false;
    for (int i = 0; i < 36; i++) {
        const bool dash = (i == 8 || i == 13 || i == 18 || i == 23);
        if (dash) {
            if (u[i] != '-') return false;
        } else if (ble_hex_value(u[i]) < 0) {
            return false;
        }
    }
    return true;
}

static void test_device_tables() {
    std::set<std::string> seen;
    const DeviceKind kinds[2] = {DeviceKind::Launcher, DeviceKind::Switchboard};
    for (DeviceKind k : kinds) {
        uint8_t n = 0;
        const CharSpec* specs = ble_char_specs(k, n);
        CHECK(specs != nullptr && n > 0 && n <= LINK_MAX_CHARS);
        CHECK(uuid_shape_ok(ble_service_uuid(k)));
        bool any_required = false;
        for (uint8_t i = 0; i < n; i++) {
            CHECK(specs[i].name && specs[i].name[0]);
            CHECK(uuid_shape_ok(specs[i].uuid));
            CHECK(seen.insert(specs[i].uuid).second);          // unique across both devices
            CHECK(strcmp(specs[i].uuid, ble_service_uuid(k)) != 0);
            // a characteristic we neither read, subscribe to nor write has no purpose
            CHECK(specs[i].read || specs[i].subscribe || specs[i].write);
            // subscribing only makes sense for readable state
            if (specs[i].subscribe) CHECK(specs[i].read);
            any_required = any_required || specs[i].required;
        }
        CHECK(any_required);
    }
    uint8_t n = 99;
    CHECK(ble_char_specs(DeviceKind::Unknown, n) == nullptr && n == 0);
    // Spot-check against the device sources: launcher PREPARATION_STATE ends in 26b1.
    CHECK(strcmp(LAUNCHER_CHARS[LCH_PREPARATION_STATE].uuid, "beb5483e-36e1-4688-b7f5-ea07361b26b1") == 0);
    CHECK(strcmp(LAUNCHER_CHARS[LCH_LAUNCH].uuid, "beb5483e-36e1-4688-b7f5-ea07361b26b2") == 0);
    CHECK(strcmp(SWITCHBOARD_CHARS[SCH_FOG_STATE].uuid, "3dc93703-33d3-4883-ae7b-877c14fd3eca") == 0);
}

int main() {
    test_debounce_clean_change();
    test_debounce_rejects_bounce();
    test_debounce_glitch_does_not_change_state();
    test_debounce_release_reports_once();
    test_debounce_millis_wraparound();
    test_decoder_full_detent_each_direction();
    test_decoder_repeats_and_partial();
    test_decoder_ignores_invalid_transition();
    test_queue_fifo_and_overflow();
    test_identity();
    test_address();
    test_record_roundtrip();
    test_record_defaults_and_corruption();
    test_led_static_states();
    test_led_flash();
    test_led_pulse();
    test_led_ramps();
    test_led_levels_and_fading_flag();
    test_link_state_mapping();
    test_device_tables();

    if (failures == 0) {
        std::printf("ALL HOST TESTS PASSED (20 tests)\n");
        return 0;
    }
    std::printf("%d CHECK(S) FAILED\n", failures);
    return 1;
}
