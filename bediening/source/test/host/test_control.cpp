// Host tests for the pure control logic (relays, fog, panic, automatic launcher).
//   g++ -std=c++11 -Wall -Wextra -Iinclude test/host/test_control.cpp src/control_logic.cpp src/ble_devices.cpp -o /tmp/test_control && /tmp/test_control

#include <cstdio>
#include <cstring>

#include "control_logic.h"

static int failures = 0;
#define CHECK(cond)                                                     \
    do {                                                                \
        if (!(cond)) {                                                  \
            std::printf("FAIL line %d: %s\n", __LINE__, #cond);         \
            failures++;                                                 \
        }                                                               \
    } while (0)

// Small simulator: edit `in`, call run(dt) to advance time and step the logic once.
struct Sim {
    ControlLogic logic;
    ControlInputs in;
    ControlOutputs out;
    uint32_t rnd = 12345;
    Sim() {
        in.now = 10000;
        in.switchesValid = true;
        in.launcherConnected = true;
        in.prepValid = true;
        in.prep = 0;
        in.switchboardConnected = true;
        in.relaysValid = true;
        in.fogStateValid = true;
        in.fogState = 2;
        in.fogControlValid = true;
        in.fogControl = 0;
    }
    void run(uint32_t dt = 0) {
        in.now += dt;
        rnd = rnd * 1664525u + 1013904223u;
        in.random = rnd;
        logic.step(in, out);
        in.launchPressed = false;   // one-shot inputs are consumed like the real controller does
        in.checkRequested = false;
        in.answer = Answer::None;
        in.launchResult = Result::None;
        in.prepareResult = Result::None;
        in.releaseResult = Result::None;
        in.dispenseResult = Result::None;
        in.releaseRequested = false;
        in.durationSave = false;
        in.durationReadRequest = false;
        in.durationWriteResult = Result::None;
        in.durationReadResult = Result::None;
    }
    int count(uint8_t device, uint8_t ci) const {
        int n = 0;
        for (uint8_t i = 0; i < out.cmdCount; i++)
            if (out.cmds[i].device == device && out.cmds[i].charIndex == ci) n++;
        return n;
    }
    const Command* find(uint8_t device, uint8_t ci) const {
        for (uint8_t i = 0; i < out.cmdCount; i++)
            if (out.cmds[i].device == device && out.cmds[i].charIndex == ci) return &out.cmds[i];
        return nullptr;
    }
    // The launcher walks through a complete preparation that the unit sees.
    void walk_preparation() {
        in.prepareResult = Result::Ok;
        in.prep = 1; run(100);
        in.prep = 2; run(100);
        in.prep = 3; run(100);
        in.prep = 0; run(100);
    }
    // Power-up check answered "Empty, bucket up" and the automatic preparation completed.
    void power_up_and_arm() {
        run();                                   // first look: the driver is asked
        in.answer = Answer::Empty;
        run(100);                                // prepare is sent by itself
        walk_preparation();
    }
    // A step that pretends the switchboard applied every relay command it was sent.
    void apply_relay_commands() {
        for (uint8_t i = 0; i < out.cmdCount; i++)
            if (out.cmds[i].device == 1 && out.cmds[i].charIndex == SCH_RELAYS)
                in.relays[out.cmds[i].data[0] - 1] = out.cmds[i].data[1];
    }
};

// ------------------------------------------------------------------ relays
static void test_flash_light_relay() {
    Sim s;
    s.in.sw[CSW_FLASH] = true;
    s.run();
    const Command* c = s.find(1, SCH_RELAYS);
    CHECK(c && c->len == 2 && c->data[0] == RELAY_FLASHING_LIGHT && c->data[1] == 1);
    CHECK(s.out.led[6] == LED_FLASH);              // wanted but not confirmed yet

    s.run(100);                                    // too soon: no repeat
    CHECK(s.count(1, SCH_RELAYS) == 0);
    s.run(500);                                    // still unconfirmed: sent again
    CHECK(s.count(1, SCH_RELAYS) == 1);

    s.in.relays[RELAY_FLASHING_LIGHT - 1] = 1;     // switchboard confirms
    s.run(100);
    CHECK(s.count(1, SCH_RELAYS) == 0 && s.out.led[6] == LED_ON);

    s.in.sw[CSW_FLASH] = false;                    // switch off
    s.run(100);
    c = s.find(1, SCH_RELAYS);
    CHECK(c && c->data[0] == RELAY_FLASHING_LIGHT && c->data[1] == 0);
    s.in.relays[RELAY_FLASHING_LIGHT - 1] = 0;
    s.run(100);
    CHECK(s.count(1, SCH_RELAYS) == 0 && s.out.led[6] == LED_OFF);
}

static void test_relays_independent_of_launcher_and_siren_momentary() {
    Sim s;
    s.in.launcherConnected = false;                // the launcher being away changes nothing here
    s.in.prepValid = false;
    s.in.sw[CSW_DECOR] = true;
    s.in.sw[CSW_SIREN] = true;
    s.run();
    int decor = 0, siren = 0, flash = 0;
    for (uint8_t i = 0; i < s.out.cmdCount; i++) {
        if (s.out.cmds[i].data[0] == RELAY_DECORATIVE_LIGHT) decor++;
        if (s.out.cmds[i].data[0] == RELAY_SIREN) siren++;
        if (s.out.cmds[i].data[0] == RELAY_FLASHING_LIGHT) flash++;
    }
    CHECK(decor == 1 && siren == 1 && flash == 0);

    s.in.relays[RELAY_DECORATIVE_LIGHT - 1] = 1;
    s.in.relays[RELAY_SIREN - 1] = 1;
    s.run(100);
    CHECK(s.out.cmdCount == 0);

    s.in.sw[CSW_SIREN] = false;                    // button released: relay follows at once
    s.run(100);
    const Command* c = s.find(1, SCH_RELAYS);
    CHECK(c && c->data[0] == RELAY_SIREN && c->data[1] == 0);
}

static void test_relays_reapplied_after_reconnect() {
    Sim s;
    s.in.sw[CSW_FLASH] = true;
    s.in.relays[RELAY_FLASHING_LIGHT - 1] = 1;
    s.run();
    CHECK(s.out.cmdCount == 0 && s.out.led[6] == LED_ON);

    s.in.switchboardConnected = false;             // link lost: nothing can be sent, LED says "waiting"
    s.in.relaysValid = false;
    s.run(100);
    CHECK(s.out.cmdCount == 0 && s.out.led[6] == LED_FLASH);
    s.run(4000);
    CHECK(s.out.led[6] == LED_FLASH_FAST);         // still unconfirmed after the pending timeout

    s.in.switchboardConnected = true;              // reconnected: the failsafe turned everything off
    s.in.relaysValid = true;
    memset(s.in.relays, 0, sizeof(s.in.relays));
    s.run(100);
    const Command* c = s.find(1, SCH_RELAYS);
    CHECK(c && c->data[0] == RELAY_FLASHING_LIGHT && c->data[1] == 1);
}

static void test_unknown_switch_state_changes_nothing_and_stops_fog() {
    Sim s;
    s.in.switchesValid = false;
    s.in.sw[CSW_FLASH] = true;                     // stale value must be ignored
    s.run();
    CHECK(s.count(1, SCH_RELAYS) == 0);

    s.in.fogState = 3;
    s.in.fogControl = 1;                           // fogging while we lost the switches
    s.run(100);
    const Command* c = s.find(1, SCH_FOG_CONTROL);
    CHECK(c && c->data[0] == 0);                   // stop
}

// --------------------------------------------------------------------- fog
static void test_fog_keeps_fogging_while_switch_on() {
    Sim s;
    s.in.sw[CSW_FOG] = true;
    s.run();
    const Command* c = s.find(1, SCH_FOG_CONTROL);
    CHECK(c && c->len == 1 && c->data[0] == 1);    // ready + wanted -> start
    CHECK(s.out.led[7] == LED_ON);

    s.run(300);                                    // request rate limit
    CHECK(s.count(1, SCH_FOG_CONTROL) == 0);
    s.run(800);                                    // rejected / ignored, still ready -> ask again
    CHECK(s.count(1, SCH_FOG_CONTROL) == 1);

    s.in.fogState = 3;
    s.in.fogControl = 1;                           // fogging: nothing to do
    s.run(100);
    CHECK(s.count(1, SCH_FOG_CONTROL) == 0 && s.out.led[7] == LED_PULSE);

    s.in.fogState = 2;
    s.in.fogControl = 0;                           // burst ended, still wanted: start again
    s.run(2000);
    CHECK(s.count(1, SCH_FOG_CONTROL) == 1);
}

static void test_fog_waits_while_heating_and_stops_on_switch_off() {
    Sim s;
    s.in.sw[CSW_FOG] = true;
    s.in.fogState = 1;                             // heating
    s.run();
    CHECK(s.count(1, SCH_FOG_CONTROL) == 0 && s.out.led[7] == LED_FLASH);
    s.in.fogState = 0;                             // machine not connected but fog wanted
    s.run(100);
    CHECK(s.count(1, SCH_FOG_CONTROL) == 0 && s.out.led[7] == LED_FLASH_FAST);

    s.in.fogState = 3;
    s.in.fogControl = 1;
    s.in.sw[CSW_FOG] = false;
    s.run(100);
    const Command* c = s.find(1, SCH_FOG_CONTROL);
    CHECK(c && c->data[0] == 0);
    s.run(100);                                    // stop is re-sent until the machine reports it
    CHECK(s.count(1, SCH_FOG_CONTROL) == 0);
    s.run(500);
    CHECK(s.count(1, SCH_FOG_CONTROL) == 1);
    s.in.fogState = 2;
    s.in.fogControl = 0;
    s.run(100);
    CHECK(s.count(1, SCH_FOG_CONTROL) == 0 && s.out.led[7] == LED_ON);
}

// ------------------------------------------------------------------- panic
static bool is_panic_relay(uint8_t r) {
    for (uint8_t k = 0; k < PANIC_RELAY_COUNT; k++)
        if (PANIC_RELAYS[k] == r) return true;
    return false;
}

static void test_panic_fires_relays_in_random_order_and_stops() {
    // the panic relays must not collide with the other relays
    for (uint8_t k = 0; k < PANIC_RELAY_COUNT; k++) {
        CHECK(PANIC_RELAYS[k] >= 1 && PANIC_RELAYS[k] <= 8);
        CHECK(PANIC_RELAYS[k] != RELAY_FLASHING_LIGHT && PANIC_RELAYS[k] != RELAY_DECORATIVE_LIGHT &&
              PANIC_RELAYS[k] != RELAY_SIREN);
    }

    Sim s;
    s.in.sw[CSW_PANIC] = true;
    int on_events = 0, max_on_at_once = 0, other_relays = 0;
    bool used[9] = {false};
    uint8_t last_on = 0, seen_changes = 0;
    for (int i = 0; i < 200; i++) {                // 20 s in 100 ms steps, switchboard applies commands
        s.run(100);
        for (uint8_t c = 0; c < s.out.cmdCount; c++) {
            if (s.out.cmds[c].device != 1) continue;
            const uint8_t r = s.out.cmds[c].data[0];
            if (!is_panic_relay(r)) other_relays++;
            if (s.out.cmds[c].data[1] == 1) { on_events++; used[r] = true; if (r == last_on) seen_changes = 255; last_on = r; }
        }
        s.apply_relay_commands();
        int on_now = 0;
        for (uint8_t r = 0; r < 8; r++) if (s.in.relays[r] == 1) on_now++;
        if (on_now > max_on_at_once) max_on_at_once = on_now;
    }
    int distinct = 0;
    for (int r = 1; r <= 8; r++) if (used[r]) distinct++;
    CHECK(on_events >= 8);                         // keeps firing
    CHECK(max_on_at_once == 1);                    // one at a time
    CHECK(distinct >= 3);                          // random order over several relays
    CHECK(other_relays == 0);                      // never touches any other relay
    CHECK(seen_changes != 255);                    // never the same relay twice in a row
    CHECK(s.out.led[5] == LED_ON);                 // L6: panic active

    s.in.sw[CSW_PANIC] = false;                    // switched off: everything off at once
    s.run(100);
    s.apply_relay_commands();
    s.run(100);
    int still_on = 0;
    for (uint8_t r = 0; r < 8; r++) if (s.in.relays[r] == 1) still_on++;
    CHECK(still_on == 0 && s.out.led[5] == LED_OFF);

    s.run(3000);                                   // after the clean-up time those relays are left alone
    s.in.relays[PANIC_RELAYS[0] - 1] = 1;          // e.g. used by something else
    s.run(100);
    CHECK(s.out.cmdCount == 0);
}

static void test_panic_waits_for_switchboard_and_sw5_does_nothing() {
    Sim s;
    s.in.sw[CSW_PANIC] = true;
    s.in.switchboardConnected = false;
    s.in.relaysValid = false;
    s.run(500);
    CHECK(s.out.cmdCount == 0 && s.out.led[5] == LED_FLASH_FAST);   // cannot reach the bells

    Sim t;
    t.in.sw[CSW_AUTO] = true;                      // SW5 automatic mode: not defined, does nothing
    t.run(100);
    CHECK(t.out.cmdCount == 0 && t.out.led[0] == LED_OFF);
}

// ----------------------------------------------------- launcher: power-up check
static void test_power_up_asks_the_driver_and_moves_nothing() {
    Sim s;
    s.run();
    CHECK(s.out.needsConfirm && s.out.reason == CheckReason::Startup);
    CHECK(s.out.launcher == LauncherStatus::NeedsCheck && s.out.led[1] == LED_FLASH);
    CHECK(s.out.cmdCount == 0);

    s.run(5000);                                   // however long it takes: still nothing moves
    CHECK(s.out.cmdCount == 0 && s.out.needsConfirm);

    s.in.launchPressed = true;                     // and launching is refused
    s.run(100);
    CHECK(s.out.cmdCount == 0 && s.out.message == Msg::LaunchCheckFirst);

    s.in.answer = Answer::NotSure;                 // "Not sure": keep asking, nothing moves
    s.run(100);
    CHECK(s.out.needsConfirm && s.out.cmdCount == 0);
}

static void test_answer_empty_prepares_automatically() {
    Sim s;
    s.run();
    s.in.answer = Answer::Empty;
    s.run(100);
    const Command* c = s.find(0, LCH_PREPARE);
    CHECK(c && c->tag == CmdTag::Prepare && c->len == 1 && c->data[0] == 1);
    CHECK(!s.out.needsConfirm && s.out.launcher == LauncherStatus::Preparing);

    s.in.prepareResult = Result::Ok;               // idle for a while: still only ONE prepare per attempt
    s.run(500);
    s.run(1000);
    CHECK(s.out.cmdCount == 0);

    s.in.prep = 1; s.run(100);
    s.in.prep = 2; s.run(100);
    s.in.prep = 3; s.run(100);
    CHECK(s.out.launcher == LauncherStatus::Preparing && s.out.led[1] == LED_PULSE);
    CHECK(s.out.led[8] == LED_PULSE);                // L9: busy preparing
    s.in.prep = 0; s.run(100);
    CHECK(s.out.launcher == LauncherStatus::Ready && s.out.led[1] == LED_ON);
    CHECK(s.out.led[8] == LED_OFF);                  // L9: off when ready to launch
    CHECK(s.out.known == Known::Armed && !s.out.needsConfirm);
    s.run(1000);
    CHECK(s.out.cmdCount == 0);                    // ready: nothing more is sent
}

static void test_answer_full_prepares_without_the_dispenser() {
    Sim s;
    s.run();
    s.in.answer = Answer::Full;                    // the load is unknown to the unit: the driver says what is in the bucket
    s.run(100);
    const Command* c = s.find(0, LCH_PREPARE);
    CHECK(c && c->len == 1 && c->data[0] == 2 && c->tag == CmdTag::Prepare);   // 0x02: everything except the dispense cycle
    CHECK(s.out.launcher != LauncherStatus::Ready && !s.out.needsConfirm);      // never ready without a preparation
    s.run(1000);
    CHECK(s.count(0, LCH_PREPARE) == 0);
    s.in.prepareResult = Result::Ok;
    s.run(10);
    s.walk_preparation();
    CHECK(s.out.launcher == LauncherStatus::Ready);
    s.in.launchPressed = true;
    s.run(100);
    CHECK(s.count(0, LCH_LAUNCH) == 1);

    // after the launch the bucket is empty: the automatic preparation fills it again (0x01)
    s.in.launchResult = Result::Ok;
    s.run(REARM_DELAY_MS + 100);
    c = s.find(0, LCH_PREPARE);
    CHECK(c && c->data[0] == 1);

    // "Empty" is 0x01
    Sim e;
    e.run();
    e.in.answer = Answer::Empty;
    e.run(100);
    CHECK(e.find(0, LCH_PREPARE) && e.find(0, LCH_PREPARE)->data[0] == 1);

    // a failed write is retried with the SAME choice
    Sim r;
    r.run();
    r.in.answer = Answer::Full;
    r.run(100);
    r.in.prepareResult = Result::Failed;
    int retried_with = -1;
    for (uint32_t t = 0; t < PREPARE_RETRY_MS + 500; t += 10) {
        r.run(10);
        if (const Command* rc = r.find(0, LCH_PREPARE)) retried_with = rc->data[0];
    }
    CHECK(retried_with == 2);

    // also with an error on the launcher: it is prepared again (0x02), not ignored
    Sim x;
    x.in.prep = 4;
    x.run();
    CHECK(x.out.needsConfirm && x.out.reason == CheckReason::Error);
    x.in.answer = Answer::Full;
    x.run(100);
    CHECK(x.find(0, LCH_PREPARE) && x.find(0, LCH_PREPARE)->data[0] == 2 && !x.out.needsConfirm);

    // the old shortcut is gone: "Not sure" and a missing answer never move anything
    Sim n;
    n.run();
    n.in.answer = Answer::NotSure;
    n.run(5000);
    CHECK(n.out.needsConfirm && n.out.cmdCount == 0 && n.count(0, LCH_PREPARE) == 0);
}

// ------------------------------------------------------- launcher: launching
static void test_launch_rearms_automatically_after_the_delay() {
    Sim s;
    s.power_up_and_arm();
    s.run(2000);                                   // let the rejection / state settle
    s.in.launchPressed = true;
    s.run(100);
    const Command* c = s.find(0, LCH_LAUNCH);
    CHECK(c && c->tag == CmdTag::Launch && c->len == 1 && c->data[0] == 1);
    CHECK(s.out.message == Msg::LaunchSent);
    CHECK(s.out.launcher == LauncherStatus::Rearming && s.out.rearmInMs == REARM_DELAY_MS);

    s.run(1000);
    CHECK(s.out.cmdCount == 0 && s.out.rearmInMs == REARM_DELAY_MS - 1000);

    s.run(REARM_DELAY_MS - 1000);                  // delay is over, but the launch write is not confirmed yet
    CHECK(s.out.cmdCount == 0);
    s.in.launchResult = Result::Ok;                // confirmed: now (and only now) the next preparation starts
    s.run(100);
    const Command* p = s.find(0, LCH_PREPARE);
    CHECK(p && p->tag == CmdTag::Prepare);
    CHECK(s.count(0, LCH_LAUNCH) == 0);            // the launch is never sent again

    s.in.prepareResult = Result::Ok;
    s.run(100);
    CHECK(s.out.cmdCount == 0);                    // and prepare is not repeated either
    s.walk_preparation();
    CHECK(s.out.launcher == LauncherStatus::Ready);
}

static void test_launch_press_while_not_ready_does_nothing() {
    Sim s;
    s.power_up_and_arm();
    s.run(2000);
    s.in.launchPressed = true;
    s.run(100);
    s.in.launchResult = Result::Ok;
    s.in.launchPressed = true;                     // second press during the countdown
    s.run(100);
    CHECK(s.count(0, LCH_LAUNCH) == 0);
    CHECK(s.out.message == Msg::LaunchBusy || s.out.message == Msg::LaunchNotReady);
}

static void test_failed_launch_write_asks_the_driver_and_never_retries() {
    Sim s;
    s.power_up_and_arm();
    s.in.launchPressed = true;
    s.run(100);
    s.in.launchResult = Result::Failed;
    s.run(100);
    CHECK(s.out.message == Msg::LaunchFailed);
    CHECK(s.out.needsConfirm && s.out.reason == CheckReason::Error && s.out.launcher == LauncherStatus::Error);
    CHECK(s.out.led[1] == LED_FLASH_FAST);
    s.run(10000);
    CHECK(s.out.cmdCount == 0);                    // no new launch, and no preparation before the driver confirms
}

static void test_launch_rejections() {
    Sim s;
    s.power_up_and_arm();
    s.run(2000);

    s.in.prep = 2;                                 // preparing
    s.in.launchPressed = true; s.run(100);
    CHECK(s.out.cmdCount == 0 && s.out.message == Msg::LaunchBusy);
    s.in.prep = 0; s.run(100);

    s.in.prepValid = false;                        // state not current
    s.in.launchPressed = true; s.run(100);
    CHECK(s.out.cmdCount == 0 && s.out.message == Msg::LaunchStateUnknown);
    s.in.launcherConnected = false;
    s.in.launchPressed = true; s.run(100);
    CHECK(s.out.cmdCount == 0 && s.out.message == Msg::LaunchOffline && s.out.launcher == LauncherStatus::Offline);
    CHECK(s.out.led[1] == LED_FLASH_FAST);         // attention flash after a rejected launch
}

// ------------------------------------------------- launcher: trust and recovery
static void test_dropout_while_ready_keeps_the_belief() {
    Sim s;
    s.power_up_and_arm();
    CHECK(s.out.launcher == LauncherStatus::Ready);

    s.in.launcherConnected = false;                // a bump on the road: link lost for a moment
    s.in.prepValid = false;
    s.run(5000);
    CHECK(s.out.launcher == LauncherStatus::Offline && !s.out.needsConfirm);
    s.in.launcherConnected = true;                 // back, idle: still armed, driver is not bothered
    s.in.prepValid = true;
    s.in.prep = 0;
    s.run(100);
    CHECK(s.out.launcher == LauncherStatus::Ready && !s.out.needsConfirm && s.out.cmdCount == 0);
}

static void test_dropout_while_preparing_asks_the_driver() {
    Sim s;
    s.run();
    s.in.answer = Answer::Empty;
    s.run(100);
    s.in.prepareResult = Result::Ok;
    s.in.prep = 2; s.run(100);

    s.in.launcherConnected = false;                // lost mid-preparation: the outcome cannot be known
    s.in.prepValid = false;
    s.run(100);
    s.in.launcherConnected = true;
    s.in.prepValid = true;
    s.in.prep = 0;
    s.run(100);
    CHECK(s.out.needsConfirm && s.out.reason == CheckReason::Startup);
    CHECK(s.out.cmdCount == 0 && s.out.launcher == LauncherStatus::NeedsCheck);
}

static void test_a_preparation_seen_to_finish_is_trusted_without_asking() {
    Sim s;
    s.in.prep = 3;                                 // already running when we connect
    s.run(100);
    CHECK(!s.out.needsConfirm && s.out.launcher == LauncherStatus::Preparing && s.out.cmdCount == 0);
    s.in.prep = 0;
    s.run(100);
    CHECK(s.out.launcher == LauncherStatus::Ready && !s.out.needsConfirm);
}

static void test_launcher_error_is_confirmed_by_the_driver() {
    Sim s;
    s.in.prep = 4;                                 // error state on first sight
    s.run();
    CHECK(s.out.needsConfirm && s.out.reason == CheckReason::Error);
    CHECK(s.out.launcher == LauncherStatus::Error && s.out.led[1] == LED_FLASH_FAST);
    CHECK(s.out.cmdCount == 0);

    s.in.answer = Answer::NotSure;               // keep asking, nothing moves
    s.run(100);
    CHECK(s.out.needsConfirm && s.out.cmdCount == 0);

    s.in.answer = Answer::Empty;                  // fixed by hand: confirmed, prepare is tried again
    s.run(100);
    CHECK(s.count(0, LCH_PREPARE) == 1 && !s.out.needsConfirm);
    s.run(100);                                    // the launcher still reports 4 for a moment: no new question
    CHECK(!s.out.needsConfirm && s.out.cmdCount == 0);
    s.in.prepareResult = Result::Ok;
    s.in.prep = 1; s.run(100);
    CHECK(!s.out.needsConfirm && s.out.launcher == LauncherStatus::Preparing);
}

static void test_refused_preparation_asks_the_driver() {
    Sim s;
    s.run();
    s.in.answer = Answer::Empty;
    s.run(100);
    CHECK(s.count(0, LCH_PREPARE) == 1);
    s.in.prepareResult = Result::Ok;
    s.run(2000);
    CHECK(s.out.message == Msg::None);
    s.run(1500);                                   // 3.6 s, the launcher never left idle
    CHECK(s.out.message == Msg::PrepareRefused);
    CHECK(s.out.needsConfirm && s.out.reason == CheckReason::Error && s.out.led[1] == LED_FLASH_FAST);
    s.run(5000);
    CHECK(s.out.cmdCount == 0);                    // not retried blindly
}

static void test_preparation_timeout_asks_the_driver() {
    Sim s;
    s.in.prep = 2;                                 // a preparation that never ends
    s.run(100);
    s.run(80000);
    CHECK(s.out.message == Msg::None && s.out.launcher == LauncherStatus::Preparing);
    s.run(11000);
    CHECK(s.out.message == Msg::PrepareTimeout);
    s.in.prep = 0;                                 // finally idle: not trusted, the driver is asked
    s.run(100);
    CHECK(s.out.needsConfirm && s.out.reason == CheckReason::Error && s.out.known == Known::Unknown);
}

static void test_failed_prepare_writes_are_retried_a_few_times() {
    Sim s;
    s.run();
    s.in.answer = Answer::Empty;
    s.run(100);
    CHECK(s.count(0, LCH_PREPARE) == 1);

    s.in.prepareResult = Result::Failed;
    s.run(100);
    CHECK(s.out.message == Msg::PrepareFailed && !s.out.needsConfirm);
    s.run(2000);
    CHECK(s.count(0, LCH_PREPARE) == 0);           // waits before trying again
    s.run(1500);
    CHECK(s.count(0, LCH_PREPARE) == 1);
    s.in.prepareResult = Result::Failed;
    s.run(100);
    s.run(3100);
    CHECK(s.count(0, LCH_PREPARE) == 1);           // third attempt
    s.in.prepareResult = Result::Failed;
    s.run(100);
    CHECK(s.out.needsConfirm && s.out.reason == CheckReason::Error);   // then the driver is asked
    s.run(5000);
    CHECK(s.out.cmdCount == 0);
}

static void test_check_launcher_from_the_menu() {
    Sim s;
    s.power_up_and_arm();
    CHECK(s.out.launcher == LauncherStatus::Ready);

    s.in.checkRequested = true;                    // driver wants to re-sync: automation stops
    s.run(100);
    CHECK(s.out.needsConfirm && s.out.reason == CheckReason::Requested && s.out.launcher == LauncherStatus::NeedsCheck);
    s.in.launchPressed = true;
    s.run(100);
    CHECK(s.out.cmdCount == 0 && s.out.message == Msg::LaunchCheckFirst);

    s.in.answer = Answer::Full;
    s.run(100);
    CHECK(s.find(0, LCH_PREPARE) && s.find(0, LCH_PREPARE)->data[0] == 2 && !s.out.needsConfirm);
    s.in.prepareResult = Result::Ok;
    s.run(10);
    s.walk_preparation();
    CHECK(s.out.launcher == LauncherStatus::Ready && !s.out.needsConfirm);
}

static void test_messages() {
    const Msg all[] = {Msg::LaunchSent, Msg::LaunchOffline, Msg::LaunchStateUnknown, Msg::LaunchNotReady,
                       Msg::LaunchBusy, Msg::LaunchError, Msg::LaunchCheckFirst, Msg::LaunchFailed,
                       Msg::PrepareRefused, Msg::PrepareTimeout, Msg::PrepareFailed};
    for (Msg m : all) CHECK(strlen(msg_text(m)) > 0 && strlen(msg_text(m)) <= 20);   // fits one display row
    CHECK(!msg_is_problem(Msg::LaunchSent) && !msg_is_problem(Msg::None));
    CHECK(msg_is_problem(Msg::LaunchNotReady) && msg_is_problem(Msg::PrepareFailed));
}

static void test_display_outputs_launch_pending_confirmed_and_siren() {
    Sim s;
    s.power_up_and_arm();
    s.run(2000);
    CHECK(!s.out.launchPending && s.out.launchOkCount == 0);
    s.in.launchPressed = true;
    s.run(100);
    CHECK(s.out.launchPending && s.out.launchOkCount == 0);  // sent, not confirmed: the display must not show a launch yet
    s.run(500);
    CHECK(s.out.launchPending && s.out.launchOkCount == 0);
    s.in.launchResult = Result::Ok;
    s.run(100);
    CHECK(!s.out.launchPending && s.out.launchOkCount == 1);
    s.run(100);
    CHECK(s.out.launchOkCount == 1);                          // counted once, not per step

    // a failed write is never counted as a launch
    Sim f;
    f.power_up_and_arm();
    f.run(2000);
    f.in.launchPressed = true;
    f.run(100);
    f.in.launchResult = Result::Failed;
    f.run(100);
    CHECK(!f.out.launchPending && f.out.launchOkCount == 0 && f.out.launcher == LauncherStatus::Error);

    // siren: no LED, but its state is reported for the display
    Sim r;
    r.in.sw[CSW_SIREN] = true;
    r.run(100);
    CHECK(r.out.sirenState == LED_FLASH);                     // wanted, not confirmed
    r.in.relays[RELAY_SIREN - 1] = 1;
    r.run(100);
    CHECK(r.out.sirenState == LED_ON);
}

// ------------------------------------------------------------------ maintenance
static void test_release_bucket() {
    Sim s;
    s.power_up_and_arm();
    s.in.maintenance = true;
    s.in.releaseRequested = true;
    s.run(100);
    const Command* c = s.find(0, LCH_LAUNCH);
    CHECK(c && c->len == 1 && c->data[0] == 1 && c->tag == CmdTag::Release);
    s.run(1000);
    CHECK(s.count(0, LCH_LAUNCH) == 0);                     // one-shot: never repeated
    s.in.releaseResult = Result::Ok;
    s.run(100);
    CHECK(s.out.message == Msg::ReleaseOk && !msg_is_problem(Msg::ReleaseOk));
    s.in.releaseResult = Result::Failed;
    s.run(100);
    CHECK(s.out.message == Msg::ReleaseFailed);

    // not allowed: outside maintenance, offline, state unknown, launcher moving
    Sim r;
    r.power_up_and_arm();
    r.in.releaseRequested = true;
    r.run(100);
    CHECK(r.out.cmdCount == 0);                             // the UI only asks from the maintenance screens
    r.in.maintenance = true;
    r.in.launcherConnected = false;
    r.in.releaseRequested = true;
    r.run(100);
    CHECK(r.out.cmdCount == 0 && r.out.message == Msg::LaunchOffline);
    r.in.launcherConnected = true;
    r.in.prep = 2;
    r.in.releaseRequested = true;
    r.run(100);
    CHECK(r.out.cmdCount == 0 && r.out.message == Msg::MaintBusy);
}

static void test_maintenance_pauses_automation() {
    // SW8 is blocked, and nothing is sent
    Sim s;
    s.power_up_and_arm();
    s.in.maintenance = true;
    s.in.launchPressed = true;
    s.run(100);
    CHECK(s.count(0, LCH_LAUNCH) == 0 && s.out.message == Msg::MaintBlocksLaunch && s.out.led[1] == LED_FLASH_FAST);
    s.in.maintenance = false;                               // nothing was done: the belief is kept, the launch works again
    s.run(2000);
    CHECK(s.out.launcher == LauncherStatus::Ready && !s.out.needsConfirm);
    s.in.launchPressed = true;
    s.run(100);
    CHECK(s.count(0, LCH_LAUNCH) == 1);

    // no automatic prepare while in maintenance; it resumes afterwards
    Sim a;
    a.power_up_and_arm();
    a.in.launchPressed = true;
    a.run(100);
    a.in.launchResult = Result::Ok;
    a.run(100);
    a.in.maintenance = true;
    a.run(REARM_DELAY_MS + 1000);
    CHECK(a.count(0, LCH_PREPARE) == 0);
    a.run(1000);
    CHECK(a.count(0, LCH_PREPARE) == 0);
    a.in.maintenance = false;
    a.run(100);
    CHECK(a.count(0, LCH_PREPARE) == 1);
}

static void test_check_is_held_back_and_asked_after_a_release_or_dispense() {
    Sim s;
    s.run();                                                // power-up: the check is pending
    CHECK(s.out.needsConfirm);
    s.in.maintenance = true;
    s.run(100);
    CHECK(!s.out.needsConfirm);                             // the overlay must not pop up inside maintenance
    s.in.releaseRequested = true;
    s.run(100);
    s.in.maintenance = false;
    s.run(100);
    CHECK(s.out.needsConfirm && s.out.launcher == LauncherStatus::NeedsCheck);
    s.run(5000);
    CHECK(s.count(0, LCH_PREPARE) == 0);                    // nothing moves before the driver answers

    // armed machine, driver releases the bucket: the old belief is gone
    Sim a;
    a.power_up_and_arm();
    CHECK(a.out.launcher == LauncherStatus::Ready);
    a.in.maintenance = true;
    a.in.releaseRequested = true;
    a.run(100);
    a.in.maintenance = false;
    a.run(100);
    CHECK(a.out.needsConfirm && a.out.known == Known::Unknown);
    a.in.answer = Answer::Empty;
    a.run(100);
    CHECK(a.count(0, LCH_PREPARE) == 1);                    // the normal flow continues from the answer

    // entering and leaving without doing anything does not bother the driver
    Sim n;
    n.power_up_and_arm();
    n.in.maintenance = true;
    n.run(100);
    n.in.maintenance = false;
    n.run(100);
    CHECK(!n.out.needsConfirm && n.out.launcher == LauncherStatus::Ready);
}

// Dispenser fake: reports `value` back.
static void test_dispense_hold_and_stop() {
    Sim s;
    s.power_up_and_arm();
    s.in.maintenance = true;
    s.in.dispenserValid = true;
    s.in.dispenser = 0;
    s.run(100);
    s.in.dispenseHold = 1;
    s.run(10);
    const Command* c = s.find(0, LCH_DISPENSER_CONTROL);
    CHECK(c && c->len == 1 && c->data[0] == 2 && c->tag == CmdTag::DispenseStart);
    CHECK(s.out.dispense == RunState::Starting);
    s.run(1000);
    CHECK(s.count(0, LCH_DISPENSER_CONTROL) == 0);          // started once, not repeated
    s.in.dispenser = 1;
    s.run(100);
    CHECK(s.out.dispense == RunState::Running);
    s.run(30000);
    CHECK(s.out.dispense == RunState::Running && s.count(0, LCH_DISPENSER_CONTROL) == 0);

    s.in.dispenseHold = 0;                              // knob released
    s.run(10);
    c = s.find(0, LCH_DISPENSER_CONTROL);
    CHECK(c && c->data[0] == 0 && c->tag == CmdTag::None);  // stop at once
    CHECK(s.out.dispense == RunState::Stopping);
    s.run(DISPENSE_STOP_RESEND_MS - 10);
    CHECK(s.count(0, LCH_DISPENSER_CONTROL) == 0);
    s.run(10);
    CHECK(s.count(0, LCH_DISPENSER_CONTROL) == 1);          // repeated while it still reports running
    s.run(DISPENSE_STOP_RESEND_MS);
    CHECK(s.count(0, LCH_DISPENSER_CONTROL) == 1);

    s.in.dispenser = 0;                                     // finally reports stopped
    s.run(10);
    CHECK(s.out.dispense == RunState::Idle);
    s.run(5000);
    CHECK(s.count(0, LCH_DISPENSER_CONTROL) == 0);
}

static void test_dispense_stop_is_always_sent_twice() {
    // released right after the start: the dispenser has not reported anything yet (value still 0)
    Sim s;
    s.power_up_and_arm();
    s.in.maintenance = true;
    s.in.dispenserValid = true;
    s.in.dispenser = 0;
    s.run(100);
    s.in.dispenseHold = 1;
    s.run(10);
    s.in.dispenseHold = 0;
    s.run(10);
    CHECK(s.count(0, LCH_DISPENSER_CONTROL) == 1 && s.find(0, LCH_DISPENSER_CONTROL)->data[0] == 0);
    CHECK(s.out.dispense == RunState::Stopping);       // a stale "0" is not trusted yet
    s.run(DISPENSE_STOP_RESEND_MS);
    CHECK(s.count(0, LCH_DISPENSER_CONTROL) == 1);
    s.run(10);
    CHECK(s.out.dispense == RunState::Idle);

    // pressed again while it is still stopping: it runs again
    Sim r;
    r.power_up_and_arm();
    r.in.maintenance = true;
    r.in.dispenserValid = true;
    r.in.dispenseHold = 1;
    r.run(10);
    r.in.dispenser = 1;
    r.run(100);
    r.in.dispenseHold = 0;
    r.run(10);
    r.in.dispenseHold = 1;
    r.run(10);
    const Command* c = r.find(0, LCH_DISPENSER_CONTROL);
    CHECK(c && c->data[0] == 2);
    r.run(3000);
    CHECK(r.out.dispense == RunState::Running && r.count(0, LCH_DISPENSER_CONTROL) == 0);
}

static void test_dispense_stops_for_every_reason() {
    // 1. leaving maintenance while the knob is still down
    Sim s;
    s.power_up_and_arm();
    s.in.maintenance = true;
    s.in.dispenserValid = true;
    s.in.dispenseHold = 1;
    s.run(10);
    s.in.dispenser = 1;
    s.run(100);
    s.in.maintenance = false;
    s.run(10);
    CHECK(s.find(0, LCH_DISPENSER_CONTROL) && s.find(0, LCH_DISPENSER_CONTROL)->data[0] == 0);
    // ... and the driver is asked what state the machine is in
    s.in.dispenser = 0;
    s.run(DISPENSE_STOP_RESEND_MS + 10);
    s.run(10);
    CHECK(s.out.needsConfirm);

    // 2. the launcher starts to move
    Sim m;
    m.power_up_and_arm();
    m.in.maintenance = true;
    m.in.dispenserValid = true;
    m.in.dispenseHold = 1;
    m.run(10);
    m.in.dispenser = 1;
    m.run(100);
    m.in.prep = 1;
    m.run(10);
    CHECK(m.find(0, LCH_DISPENSER_CONTROL) && m.find(0, LCH_DISPENSER_CONTROL)->data[0] == 0);

    // 3. the link is lost: nothing can be sent, but the stop follows the reconnection
    Sim l;
    l.power_up_and_arm();
    l.in.maintenance = true;
    l.in.dispenserValid = true;
    l.in.dispenseHold = 1;
    l.run(10);
    l.in.dispenser = 1;
    l.run(100);
    l.in.launcherConnected = false;
    l.in.dispenseHold = 0;
    l.run(10);
    CHECK(l.out.cmdCount == 0 && l.out.dispense == RunState::Stopping);
    l.run(5000);
    CHECK(l.out.cmdCount == 0);
    l.in.launcherConnected = true;
    l.run(10);
    CHECK(l.find(0, LCH_DISPENSER_CONTROL) && l.find(0, LCH_DISPENSER_CONTROL)->data[0] == 0);

    // 4. the link drops while the knob is STILL held: after the reconnection it stops, and only a new press runs it again
    Sim h;
    h.power_up_and_arm();
    h.in.maintenance = true;
    h.in.dispenserValid = true;
    h.in.dispenseHold = 1;
    h.run(10);
    h.in.dispenser = 1;
    h.run(100);
    h.in.launcherConnected = false;
    h.run(10);
    CHECK(h.out.dispense == RunState::Stopping);
    h.in.launcherConnected = true;
    h.run(10);
    CHECK(h.out.dispense == RunState::Stopping);
    CHECK(h.find(0, LCH_DISPENSER_CONTROL) && h.find(0, LCH_DISPENSER_CONTROL)->data[0] == 0);
    h.in.dispenser = 0;
    h.run(DISPENSE_STOP_RESEND_MS + 10);
    h.run(10);
    CHECK(h.out.dispense == RunState::Idle && h.count(0, LCH_DISPENSER_CONTROL) == 0);
    h.in.dispenseHold = 0;
    h.run(10);
    h.in.dispenseHold = 1;                                // a new press starts it again
    h.run(10);
    CHECK(h.find(0, LCH_DISPENSER_CONTROL) && h.find(0, LCH_DISPENSER_CONTROL)->data[0] == 2);
}

static void test_dispense_problems_are_reported() {
    // not accepted: no "running" within the timeout
    Sim s;
    s.power_up_and_arm();
    s.in.maintenance = true;
    s.in.dispenserValid = true;
    s.in.dispenseHold = 1;
    s.run(10);
    s.run(DISPENSE_START_TIMEOUT_MS);
    CHECK(s.out.message != Msg::DispenseFailed);
    s.run(10);
    CHECK(s.out.message == Msg::DispenseFailed);
    s.run(100);
    CHECK(s.out.message == Msg::None);                      // reported once

    // a failed start write
    Sim f;
    f.power_up_and_arm();
    f.in.maintenance = true;
    f.in.dispenseHold = 1;
    f.run(10);
    f.in.dispenseResult = Result::Failed;
    f.run(10);
    CHECK(f.out.message == Msg::DispenseFailed);

    // stuck: the stop is never confirmed
    Sim k;
    k.power_up_and_arm();
    k.in.maintenance = true;
    k.in.dispenserValid = true;
    k.in.dispenseHold = 1;
    k.run(10);
    k.in.dispenser = 1;
    k.run(100);
    k.in.dispenseHold = 0;
    k.run(10);
    k.run(DISPENSE_STUCK_MS);
    k.run(10);
    CHECK(k.out.message == Msg::DispenseStuck);
    CHECK(k.out.dispense == RunState::Stopping);       // and it keeps trying
    k.run(DISPENSE_STOP_RESEND_MS);
    CHECK(k.count(0, LCH_DISPENSER_CONTROL) == 1);

    // refused: offline, busy launcher, dispenser already running for the automatic cycle
    Sim r;
    r.power_up_and_arm();
    r.in.maintenance = true;
    r.in.dispenserValid = true;
    r.in.dispenser = 1;
    r.in.dispenseHold = 1;
    r.run(10);
    CHECK(r.out.cmdCount == 0 && r.out.message == Msg::MaintBusy && r.out.dispense == RunState::Idle);
    r.in.dispenseHold = 0;
    r.run(10);
    r.in.dispenser = 0;
    r.in.prep = 3;
    r.in.dispenseHold = 1;
    r.run(10);
    CHECK(r.out.cmdCount == 0 && r.out.message == Msg::MaintBusy);
    r.in.dispenseHold = 0;
    r.run(10);
    r.in.prep = 0;
    r.in.launcherConnected = false;
    r.in.dispenseHold = 1;
    r.run(10);
    CHECK(r.out.cmdCount == 0 && r.out.message == Msg::LaunchOffline);

    // outside maintenance a held knob does nothing at all
    Sim o;
    o.power_up_and_arm();
    o.in.dispenserValid = true;
    o.in.dispenseHold = 1;
    o.run(10);
    CHECK(o.out.cmdCount == 0 && o.out.dispense == RunState::Idle);
}

// ------------------------------------------------------------------ pull motor (hold to run, same safe-stop rules)
static const Command* find_motor(const Sim& s) { return s.find(0, LCH_PULL_MOTOR); }

static void test_motor_start_stop_payloads() {
    Sim s;
    s.power_up_and_arm();
    s.in.maintenance = true;
    s.in.motorValid = true;
    s.in.motor = 0;
    s.run(100);
    s.in.motorHold = +1;
    s.run(10);
    const Command* c = find_motor(s);
    CHECK(c && c->len == 4 && c->data[0] == 0x00 && c->data[1] == 0x00 && c->data[2] == 0x02 && c->data[3] == 0x00);  // +512
    CHECK(c->tag == CmdTag::MotorStart && s.out.motor == RunState::Starting && s.out.dispense == RunState::Idle);
    s.run(1000);
    CHECK(s.count(0, LCH_PULL_MOTOR) == 0);                  // started once
    s.in.motor = 512;
    s.run(100);
    CHECK(s.out.motor == RunState::Running);

    s.in.motorHold = 0;                                      // released: stop at once, then repeated until it reports 0
    s.run(10);
    c = find_motor(s);
    CHECK(c && c->len == 4 && c->data[0] == 0 && c->data[1] == 0 && c->data[2] == 0 && c->data[3] == 0 && c->tag == CmdTag::None);
    CHECK(s.out.motor == RunState::Stopping);
    s.run(DISPENSE_STOP_RESEND_MS);
    CHECK(s.count(0, LCH_PULL_MOTOR) == 1);
    s.in.motor = 0;
    s.run(10);
    CHECK(s.out.motor == RunState::Idle);
    s.run(3000);
    CHECK(s.count(0, LCH_PULL_MOTOR) == 0);

    Sim d;                                                   // -512 = FF FF FE 00
    d.power_up_and_arm();
    d.in.maintenance = true;
    d.in.motorValid = true;
    d.in.motorHold = -1;
    d.run(10);
    c = find_motor(d);
    CHECK(c && c->len == 4 && c->data[0] == 0xFF && c->data[1] == 0xFF && c->data[2] == 0xFE && c->data[3] == 0x00);
}

static void test_motor_never_reverses_on_the_fly() {
    Sim s;
    s.power_up_and_arm();
    s.in.maintenance = true;
    s.in.motorValid = true;
    s.in.motorHold = +1;
    s.run(10);
    s.in.motor = 512;
    s.run(100);
    s.in.motorHold = 0;
    s.run(10);                                               // stopping
    s.in.motorHold = -1;                                     // the other direction pressed at once
    s.run(10);
    CHECK(s.out.message == Msg::StillStopping);
    for (int i = 0; i < s.out.cmdCount; i++) CHECK(!(s.out.cmds[i].charIndex == LCH_PULL_MOTOR && s.out.cmds[i].data[0] == 0xFF));
    for (int t = 0; t < 300; t++) {                          // still held, but it was aborted: nothing reverses by itself
        s.run(10);
        for (int i = 0; i < s.out.cmdCount; i++) CHECK(s.out.cmds[i].data[0] != 0xFF);
    }
    s.in.motor = 0;
    s.run(DISPENSE_STOP_RESEND_MS + 10);
    s.run(10);
    CHECK(s.out.motor == RunState::Idle);
    s.in.motorHold = 0;                                      // released and pressed again: now it starts
    s.run(10);
    s.in.motorHold = -1;
    s.run(10);
    CHECK(find_motor(s) && find_motor(s)->data[0] == 0xFF);
}

static void test_motor_stops_for_every_reason_and_reports_problems() {
    // leaving maintenance with the knob down
    Sim a;
    a.power_up_and_arm();
    a.in.maintenance = true;
    a.in.motorValid = true;
    a.in.motorHold = +1;
    a.run(10);
    a.in.motor = 512;
    a.run(100);
    a.in.maintenance = false;
    a.run(10);
    CHECK(find_motor(a) && find_motor(a)->len == 4 && find_motor(a)->data[2] == 0);

    // the launcher starts moving by itself
    Sim m;
    m.power_up_and_arm();
    m.in.maintenance = true;
    m.in.motorValid = true;
    m.in.motorHold = -1;
    m.run(10);
    m.in.motor = -512;
    m.run(100);
    m.in.prep = 1;
    m.run(10);
    CHECK(find_motor(m) && find_motor(m)->data[2] == 0 && m.out.motor == RunState::Stopping);

    // link drop while held: no resume after the reconnection
    Sim l;
    l.power_up_and_arm();
    l.in.maintenance = true;
    l.in.motorValid = true;
    l.in.motorHold = +1;
    l.run(10);
    l.in.motor = 512;
    l.run(100);
    l.in.launcherConnected = false;
    l.run(10);
    CHECK(l.out.motor == RunState::Stopping && l.out.cmdCount == 0);
    l.in.launcherConnected = true;
    l.run(10);
    CHECK(find_motor(l) && find_motor(l)->data[2] == 0);
    l.in.motor = 0;
    l.run(DISPENSE_STOP_RESEND_MS + 10);
    l.run(10);
    CHECK(l.out.motor == RunState::Idle && l.count(0, LCH_PULL_MOTOR) == 0);

    // not accepted / failed start / stuck stop
    Sim n;
    n.power_up_and_arm();
    n.in.maintenance = true;
    n.in.motorValid = true;
    n.in.motorHold = +1;
    n.run(10);
    n.run(DISPENSE_START_TIMEOUT_MS + 10);
    CHECK(n.out.message == Msg::MotorFailed);
    Sim f;
    f.power_up_and_arm();
    f.in.maintenance = true;
    f.in.motorHold = +1;
    f.run(10);
    f.in.motorResult = Result::Failed;
    f.run(10);
    CHECK(f.out.message == Msg::MotorFailed);
    Sim k;
    k.power_up_and_arm();
    k.in.maintenance = true;
    k.in.motorValid = true;
    k.in.motorHold = +1;
    k.run(10);
    k.in.motor = 512;
    k.run(100);
    k.in.motorHold = 0;
    k.run(10);
    k.run(DISPENSE_STUCK_MS + 10);
    CHECK(k.out.message == Msg::MotorStuck && k.out.motor == RunState::Stopping);

    // refused: preparing, already moving for the automatic cycle, outside maintenance, dispenser busy
    Sim r;
    r.power_up_and_arm();
    r.in.maintenance = true;
    r.in.motorValid = true;
    r.in.prep = 2;
    r.in.motorHold = +1;
    r.run(10);
    CHECK(r.out.cmdCount == 0 && r.out.message == Msg::MaintBusy);
    Sim o;
    o.power_up_and_arm();
    o.in.motorValid = true;
    o.in.motorHold = +1;
    o.run(10);
    CHECK(o.out.cmdCount == 0 && o.out.motor == RunState::Idle);
    Sim x;
    x.power_up_and_arm();
    x.in.maintenance = true;
    x.in.motorValid = true;
    x.in.dispenserValid = true;
    x.in.dispenseHold = 1;
    x.run(10);                                               // dispenser owns the launcher
    x.in.motorHold = +1;
    x.run(10);
    CHECK(x.out.message == Msg::MaintBusy && x.count(0, LCH_PULL_MOTOR) == 0);
}

// ------------------------------------------------------------------ dispenser reverse (un-jam)
static void test_dispense_reverse_and_direction_change() {
    Sim s;
    s.power_up_and_arm();
    s.in.maintenance = true;
    s.in.dispenserValid = true;
    s.in.dispenseHold = -1;                                  // "Hold to reverse"
    s.run(10);
    const Command* c = s.find(0, LCH_DISPENSER_CONTROL);
    CHECK(c && c->len == 1 && c->data[0] == 3 && c->tag == CmdTag::DispenseStart);   // 0x03 = reverse, until 0x00
    CHECK(s.out.dispense == RunState::Starting);
    s.in.dispenser = 1;
    s.run(100);
    CHECK(s.out.dispense == RunState::Running);
    s.in.dispenseHold = 0;                                   // released: the usual stop (0x00), repeated until reported
    s.run(10);
    c = s.find(0, LCH_DISPENSER_CONTROL);
    CHECK(c && c->len == 1 && c->data[0] == 0 && c->tag == CmdTag::None);
    CHECK(s.out.dispense == RunState::Stopping);

    s.in.dispenseHold = 1;                                   // the OTHER direction at once: refused until the stop is confirmed
    s.run(10);
    CHECK(s.out.message == Msg::StillStopping);
    for (int t = 0; t < 100; t++) {
        s.run(10);
        for (int i = 0; i < s.out.cmdCount; i++) CHECK(s.out.cmds[i].data[0] != 2 && s.out.cmds[i].data[0] != 3);
    }
    s.in.dispenseHold = 0;
    s.in.dispenser = 0;                                      // stop confirmed
    s.run(DISPENSE_STOP_RESEND_MS + 10);
    s.run(10);
    CHECK(s.out.dispense == RunState::Idle);
    s.in.dispenseHold = 1;                                   // now forward is fine
    s.run(10);
    CHECK(s.find(0, LCH_DISPENSER_CONTROL) && s.find(0, LCH_DISPENSER_CONTROL)->data[0] == 2);

    // the same direction pressed again while it stops simply runs again (like before)
    Sim r;
    r.power_up_and_arm();
    r.in.maintenance = true;
    r.in.dispenserValid = true;
    r.in.dispenseHold = -1;
    r.run(10);
    r.in.dispenser = 1;
    r.run(100);
    r.in.dispenseHold = 0;
    r.run(10);
    r.in.dispenseHold = -1;
    r.run(10);
    CHECK(r.find(0, LCH_DISPENSER_CONTROL) && r.find(0, LCH_DISPENSER_CONTROL)->data[0] == 3);
}

// ------------------------------------------------------------------ dispenser time (write, read back, compare)
static void test_dispenser_time_read_and_save() {
    Sim s;
    s.power_up_and_arm();
    s.in.maintenance = true;
    s.in.durationReadRequest = true;                         // the screen opened
    s.run(10);
    const Command* c = s.find(0, LCH_DISPENSER_DURATION);
    CHECK(c && c->read && c->len == 0 && c->tag == CmdTag::DurationRead);
    s.in.durationReadRequest = false;
    s.run(100);
    CHECK(s.out.cmdCount == 0 && !s.out.durationBusy);

    s.in.durationSave = true;
    s.in.durationValue = 12;
    s.run(10);
    c = s.find(0, LCH_DISPENSER_DURATION);
    CHECK(c && !c->read && c->len == 1 && c->data[0] == 12 && c->tag == CmdTag::DurationWrite);
    CHECK(s.out.durationBusy);
    s.in.durationSave = false;
    s.run(100);
    CHECK(s.out.cmdCount == 0 && s.out.durationBusy);        // waiting for the write result, nothing sent twice
    s.in.durationWriteResult = Result::Ok;
    s.run(10);
    c = s.find(0, LCH_DISPENSER_DURATION);
    CHECK(c && c->read && c->tag == CmdTag::DurationVerify); // the write alone proves nothing: read it back
    CHECK(s.out.message != Msg::DurationSaved);
    s.in.durationValid = true;
    s.in.duration = 12;
    s.in.durationReadResult = Result::Ok;
    s.run(10);
    CHECK(s.out.message == Msg::DurationSaved && !s.out.durationBusy && !msg_is_problem(Msg::DurationSaved));
    CHECK(s.out.launcher == LauncherStatus::Ready);          // nothing physical changed
    s.in.maintenance = false;
    s.run(100);
    CHECK(!s.out.needsConfirm && s.out.launcher == LauncherStatus::Ready);   // no check question afterwards
}

static void test_dispenser_time_failures_and_limits() {
    // read-back differs: NOT saved
    Sim a;
    a.power_up_and_arm();
    a.in.maintenance = true;
    a.in.durationSave = true;
    a.in.durationValue = 20;
    a.run(10);
    a.in.durationSave = false;
    a.in.durationWriteResult = Result::Ok;
    a.run(10);
    a.in.durationValid = true;
    a.in.duration = 7;                                       // the launcher kept the old / another value
    a.in.durationReadResult = Result::Ok;
    a.run(10);
    CHECK(a.out.message == Msg::DurationFailed && !a.out.durationBusy);

    // the write fails
    Sim w;
    w.power_up_and_arm();
    w.in.maintenance = true;
    w.in.durationSave = true;
    w.in.durationValue = 20;
    w.run(10);
    w.in.durationSave = false;
    w.in.durationWriteResult = Result::Failed;
    w.run(10);
    CHECK(w.out.message == Msg::DurationFailed && !w.out.durationBusy && w.count(0, LCH_DISPENSER_DURATION) == 0);

    // no read-back in time
    Sim t;
    t.power_up_and_arm();
    t.in.maintenance = true;
    t.in.durationSave = true;
    t.in.durationValue = 20;
    t.run(10);
    t.in.durationSave = false;
    t.in.durationWriteResult = Result::Ok;
    t.run(10);
    t.run(DURATION_VERIFY_TIMEOUT_MS - 100);
    CHECK(t.out.durationBusy);
    t.run(200);
    CHECK(t.out.message == Msg::DurationFailed && !t.out.durationBusy);

    // limits 1..60
    Sim l;
    l.power_up_and_arm();
    l.in.maintenance = true;
    l.in.durationSave = true;
    l.in.durationValue = 0;
    l.run(10);
    CHECK(l.find(0, LCH_DISPENSER_DURATION) && l.find(0, LCH_DISPENSER_DURATION)->data[0] == DURATION_MIN_S);
    Sim h;
    h.power_up_and_arm();
    h.in.maintenance = true;
    h.in.durationSave = true;
    h.in.durationValue = 200;
    h.run(10);
    CHECK(h.find(0, LCH_DISPENSER_DURATION) && h.find(0, LCH_DISPENSER_DURATION)->data[0] == DURATION_MAX_S);

    // refused: outside maintenance, offline, preparing
    Sim o;
    o.power_up_and_arm();
    o.in.durationSave = true;
    o.in.durationValue = 10;
    o.run(10);
    CHECK(o.out.cmdCount == 0 && !o.out.durationBusy);
    Sim f;
    f.power_up_and_arm();
    f.in.maintenance = true;
    f.in.launcherConnected = false;
    f.in.durationSave = true;
    f.in.durationValue = 10;
    f.run(10);
    CHECK(f.out.cmdCount == 0 && f.out.message == Msg::LaunchOffline);
    Sim p;
    p.power_up_and_arm();
    p.in.maintenance = true;
    p.in.prep = 2;
    p.in.durationSave = true;
    p.in.durationValue = 10;
    p.run(10);
    CHECK(p.out.cmdCount == 0 && p.out.message == Msg::MaintBusy);

    // one save at a time
    Sim d;
    d.power_up_and_arm();
    d.in.maintenance = true;
    d.in.durationSave = true;
    d.in.durationValue = 10;
    d.run(10);
    d.in.durationValue = 11;                                 // pressed again while the first one is in progress
    d.run(10);
    CHECK(d.out.cmdCount == 0);
}

// ------------------------------------------------------------------ L9: busy preparing
static void test_l9_follows_the_preparation() {
    Sim s;
    s.run();                                                // power-up: the driver is asked, nothing moves
    CHECK(s.out.launcher == LauncherStatus::NeedsCheck && s.out.led[8] == LED_OFF);
    s.in.answer = Answer::Empty;
    s.run(100);
    CHECK(s.out.launcher == LauncherStatus::Preparing && s.out.led[8] == LED_PULSE);   // pulses from the start of the preparation
    s.in.prepareResult = Result::Ok;
    s.in.prep = 1;
    s.run(500);
    CHECK(s.out.led[8] == LED_PULSE);                       // moving down
    s.in.prep = 2;
    s.run(500);
    CHECK(s.out.led[8] == LED_PULSE);                       // unwinding
    s.in.prep = 3;
    s.run(500);
    CHECK(s.out.led[8] == LED_PULSE);
    s.in.prep = 0;
    s.run(500);
    CHECK(s.out.launcher == LauncherStatus::Ready && s.out.led[8] == LED_OFF && s.out.led[1] == LED_ON);   // ready: L9 off, L2 on

    // a launch: the re-arm countdown is not "preparing" (L9 off), the next preparation pulses again
    s.in.launchPressed = true;
    s.run(100);
    CHECK(s.out.led[8] == LED_OFF && s.out.led[1] != LED_OFF);
    s.in.launchResult = Result::Ok;
    s.run(REARM_DELAY_MS / 2);
    CHECK(s.out.launcher == LauncherStatus::Rearming && s.out.led[8] == LED_OFF);
    s.run(REARM_DELAY_MS);
    CHECK(s.out.launcher == LauncherStatus::Preparing && s.out.led[8] == LED_PULSE);

    // an error behaves like L2 (fast flash), also when the check overlay is up; a rejected launch is L2 only
    Sim e;
    e.in.prep = 4;
    e.run();
    CHECK(e.out.launcher == LauncherStatus::Error && e.out.led[1] == LED_FLASH_FAST && e.out.led[8] == LED_FLASH_FAST);
    Sim r;
    r.power_up_and_arm();
    r.in.launchPressed = true;
    r.in.maintenance = true;                                // rejected (maintenance blocks it)
    r.run(100);
    CHECK(r.out.led[1] == LED_FLASH_FAST && r.out.led[8] == LED_OFF);

    // offline / unknown: off; the other LEDs are not touched
    Sim o;
    o.in.launcherConnected = false;
    o.run();
    CHECK(o.out.launcher == LauncherStatus::Offline && o.out.led[8] == LED_OFF);
    for (int i = 9; i < 12; i++) CHECK(o.out.led[i] == LED_OFF);
}

int main() {
    test_flash_light_relay();
    test_relays_independent_of_launcher_and_siren_momentary();
    test_relays_reapplied_after_reconnect();
    test_unknown_switch_state_changes_nothing_and_stops_fog();
    test_fog_keeps_fogging_while_switch_on();
    test_fog_waits_while_heating_and_stops_on_switch_off();
    test_panic_fires_relays_in_random_order_and_stops();
    test_panic_waits_for_switchboard_and_sw5_does_nothing();
    test_power_up_asks_the_driver_and_moves_nothing();
    test_answer_empty_prepares_automatically();
    test_answer_full_prepares_without_the_dispenser();
    test_launch_rearms_automatically_after_the_delay();
    test_launch_press_while_not_ready_does_nothing();
    test_failed_launch_write_asks_the_driver_and_never_retries();
    test_launch_rejections();
    test_dropout_while_ready_keeps_the_belief();
    test_dropout_while_preparing_asks_the_driver();
    test_a_preparation_seen_to_finish_is_trusted_without_asking();
    test_launcher_error_is_confirmed_by_the_driver();
    test_refused_preparation_asks_the_driver();
    test_preparation_timeout_asks_the_driver();
    test_failed_prepare_writes_are_retried_a_few_times();
    test_check_launcher_from_the_menu();
    test_messages();
    test_display_outputs_launch_pending_confirmed_and_siren();
    test_release_bucket();
    test_maintenance_pauses_automation();
    test_check_is_held_back_and_asked_after_a_release_or_dispense();
    test_dispense_hold_and_stop();
    test_dispense_stop_is_always_sent_twice();
    test_dispense_stops_for_every_reason();
    test_dispense_problems_are_reported();
    test_dispense_reverse_and_direction_change();
    test_dispenser_time_read_and_save();
    test_l9_follows_the_preparation();
    test_dispenser_time_failures_and_limits();
    test_motor_start_stop_payloads();
    test_motor_never_reverses_on_the_fly();
    test_motor_stops_for_every_reason_and_reports_problems();

    if (failures == 0) {
        std::printf("ALL CONTROL TESTS PASSED (39 tests)\n");
        return 0;
    }
    std::printf("%d CHECK(S) FAILED\n", failures);
    return 1;
}
