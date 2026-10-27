// Host tests for the pure control logic (relays, fog, launcher interlocks).
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
    // One step; one-shot inputs are consumed like the real controller does.
    void run(uint32_t dt = 0) {
        in.now += dt;
        logic.step(in, out);
        in.launchPressed = false;
        in.prepareRequested = false;
        in.launchResult = Result::None;
        in.prepareResult = Result::None;
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
    // The launcher walks through a full preparation (the controller sees each state).
    void prepare_completely() {
        in.prepareRequested = true;
        run(100);
        in.prep = 1; run(100);
        in.prep = 2; run(100);
        in.prep = 3; run(100);
        in.prep = 0; run(100);
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
    CHECK(s.count(1, SCH_RELAYS) == 0);
    CHECK(s.out.led[6] == LED_ON);

    s.in.sw[CSW_FLASH] = false;                    // switch off
    s.run(100);
    c = s.find(1, SCH_RELAYS);
    CHECK(c && c->data[0] == RELAY_FLASHING_LIGHT && c->data[1] == 0);
    s.in.relays[RELAY_FLASHING_LIGHT - 1] = 0;
    s.run(100);
    CHECK(s.count(1, SCH_RELAYS) == 0 && s.out.led[6] == LED_OFF);
}

static void test_relays_are_independent_and_siren_is_momentary() {
    Sim s;
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

    // link lost: no state known, nothing can be sent, LED says "waiting"
    s.in.switchboardConnected = false;
    s.in.relaysValid = false;
    s.run(100);
    CHECK(s.out.cmdCount == 0 && s.out.led[6] == LED_FLASH);
    s.run(4000);
    CHECK(s.out.led[6] == LED_FLASH_FAST);         // still unconfirmed after the pending timeout

    // reconnected: the switchboard failsafe turned everything off, so the wish is applied again
    s.in.switchboardConnected = true;
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

// ----------------------------------------------------------------- launcher
static void test_launch_needs_a_completed_preparation() {
    Sim s;
    s.in.launchPressed = true;
    s.run();
    CHECK(s.out.cmdCount == 0);                    // nothing sent
    CHECK(s.out.message == Msg::LaunchNotPrepared);
    CHECK(s.out.launcher == LauncherStatus::Idle);

    s.prepare_completely();
    CHECK(s.out.launcher == LauncherStatus::Ready);
    CHECK(s.out.led[1] == LED_FLASH_FAST);         // the rejection flash is still running ...
    s.run(2000);
    CHECK(s.out.led[1] == LED_ON);                 // ... then L2 shows "ready"

    s.in.launchPressed = true;
    s.run(100);
    const Command* c = s.find(0, LCH_LAUNCH);
    CHECK(c && c->tag == CmdTag::Launch && c->len == 1 && c->data[0] == 1);
    CHECK(s.out.message == Msg::LaunchSent);
    CHECK(s.out.launcher == LauncherStatus::Launched);
}

static void test_launch_is_one_shot_with_lockout() {
    Sim s;
    s.prepare_completely();
    s.in.launchPressed = true;
    s.run(100);
    CHECK(s.count(0, LCH_LAUNCH) == 1);

    s.in.launchPressed = true;                     // second press while the latch re-locks
    s.run(1000);
    CHECK(s.count(0, LCH_LAUNCH) == 0 && s.out.message == Msg::LaunchLockout);

    s.in.launchPressed = true;                     // after the lockout: still not prepared again
    s.run(6000);
    CHECK(s.count(0, LCH_LAUNCH) == 0 && s.out.message == Msg::LaunchNotPrepared);
    s.run(100);
    CHECK(s.count(0, LCH_LAUNCH) == 0);            // and nothing is ever sent without a new press
}

static void test_launch_rejections() {
    Sim s;
    s.prepare_completely();

    s.in.prep = 2;                                 // preparing
    s.in.launchPressed = true; s.run(100);
    CHECK(s.out.cmdCount == 0 && s.out.message == Msg::LaunchBusy);
    s.in.prep = 4;                                 // error
    s.in.launchPressed = true; s.run(100);
    CHECK(s.out.cmdCount == 0 && s.out.message == Msg::LaunchError && s.out.launcher == LauncherStatus::Error);
    CHECK(s.out.led[1] == LED_FLASH_FAST);

    Sim t;
    t.prepare_completely();
    t.in.prepValid = false;                        // state not current
    t.in.launchPressed = true; t.run(100);
    CHECK(t.out.cmdCount == 0 && t.out.message == Msg::LaunchStateUnknown);
    t.in.launcherConnected = false;
    t.in.launchPressed = true; t.run(100);
    CHECK(t.out.cmdCount == 0 && t.out.message == Msg::LaunchOffline && t.out.launcher == LauncherStatus::Offline);
}

static void test_ready_is_lost_on_reconnect_and_needs_observed_completion() {
    Sim s;
    s.prepare_completely();
    CHECK(s.out.launcher == LauncherStatus::Ready);

    s.in.launcherConnected = false;                // link lost
    s.in.prepValid = false;
    s.run(100);
    CHECK(s.out.launcher == LauncherStatus::Offline);

    s.in.launcherConnected = true;                 // back: idle state read, but not known to be prepared
    s.in.prepValid = true;
    s.in.prep = 0;
    s.run(100);
    CHECK(s.out.launcher == LauncherStatus::Idle);
    s.in.launchPressed = true;
    s.run(100);
    CHECK(s.out.cmdCount == 0 && s.out.message == Msg::LaunchNotPrepared);

    // A preparation that was already running when we connected and finishes while we watch counts.
    Sim t;
    t.in.prep = 3;
    t.run(100);
    CHECK(t.out.launcher == LauncherStatus::Preparing);
    t.in.prep = 0;
    t.run(100);
    CHECK(t.out.launcher == LauncherStatus::Ready);
}

static void test_prepare_rules() {
    Sim s;
    s.in.prepareRequested = true;
    s.run();
    const Command* c = s.find(0, LCH_PREPARE);
    CHECK(c && c->tag == CmdTag::Prepare && c->data[0] == 1);
    CHECK(s.out.message == Msg::PrepareSent);

    s.in.prepareRequested = true;                  // again while the request is pending
    s.run(500);
    CHECK(s.out.cmdCount == 0 && s.out.message == Msg::PrepareBusy);

    s.in.prep = 1;                                 // launcher started: busy, not refused
    s.run(100);
    s.in.prepareRequested = true;
    s.run(100);
    CHECK(s.out.cmdCount == 0 && s.out.message == Msg::PrepareBusy);
    s.run(2000);                                   // let the rejection flash on L2 end
    CHECK(s.out.launcher == LauncherStatus::Preparing && s.out.led[1] == LED_PULSE);

    Sim t;
    t.in.launcherConnected = false;
    t.in.prepareRequested = true;
    t.run();
    CHECK(t.out.cmdCount == 0 && t.out.message == Msg::PrepareOffline);
}

static void test_prepare_refused_timeout_and_failures() {
    Sim s;
    s.in.prepareRequested = true;
    s.run();
    s.run(2000);
    CHECK(s.out.message == Msg::None);
    s.run(1500);                                   // 3.5 s, launcher never left idle
    CHECK(s.out.message == Msg::PrepareRefused);
    CHECK(s.out.led[1] == LED_FLASH_FAST);         // attention flash on L2

    Sim t;
    t.in.prep = 2;                                 // a preparation that never ends
    t.run(100);
    t.run(80000);
    CHECK(t.out.message == Msg::None && t.out.launcher == LauncherStatus::Preparing);
    t.run(11000);
    CHECK(t.out.message == Msg::PrepareTimeout && t.out.launcher == LauncherStatus::Error);

    Sim u;                                         // command results reported by the BLE layer
    u.in.launchResult = Result::Failed;
    u.run();
    CHECK(u.out.message == Msg::LaunchFailed);
    u.in.prepareResult = Result::Failed;
    u.run(100);
    CHECK(u.out.message == Msg::PrepareFailed);
}

static void test_prepare_after_error_is_allowed_but_not_during_lockout() {
    Sim s;
    s.in.prep = 4;
    s.in.prepareRequested = true;
    s.run();
    CHECK(s.count(0, LCH_PREPARE) == 1);           // retry from the error state is allowed

    Sim t;
    t.prepare_completely();
    t.in.launchPressed = true; t.run(100);
    t.in.prepareRequested = true; t.run(1000);
    CHECK(t.out.cmdCount == 0 && t.out.message == Msg::PrepareLockout);
}

static void test_panic_and_auto_do_nothing_yet() {
    Sim s;
    s.in.sw[CSW_PANIC] = true;
    s.in.sw[CSW_AUTO] = true;
    s.run();
    CHECK(s.out.cmdCount == 0);
    CHECK(s.out.led[0] == LED_OFF && s.out.led[5] == LED_OFF);   // L1 / L6 stay off
}

static void test_messages() {
    const Msg all[] = {Msg::LaunchSent, Msg::LaunchOffline, Msg::LaunchStateUnknown, Msg::LaunchNotPrepared,
                       Msg::LaunchBusy, Msg::LaunchError, Msg::LaunchLockout, Msg::LaunchFailed, Msg::PrepareSent,
                       Msg::PrepareOffline, Msg::PrepareStateUnknown, Msg::PrepareBusy, Msg::PrepareLockout,
                       Msg::PrepareRefused, Msg::PrepareTimeout, Msg::PrepareFailed};
    for (Msg m : all) CHECK(strlen(msg_text(m)) > 0 && strlen(msg_text(m)) <= 20);   // fits one display row
    CHECK(!msg_is_problem(Msg::LaunchSent) && !msg_is_problem(Msg::PrepareSent) && !msg_is_problem(Msg::None));
    CHECK(msg_is_problem(Msg::LaunchNotPrepared) && msg_is_problem(Msg::LaunchFailed));
}

int main() {
    test_flash_light_relay();
    test_relays_are_independent_and_siren_is_momentary();
    test_relays_reapplied_after_reconnect();
    test_unknown_switch_state_changes_nothing_and_stops_fog();
    test_fog_keeps_fogging_while_switch_on();
    test_fog_waits_while_heating_and_stops_on_switch_off();
    test_launch_needs_a_completed_preparation();
    test_launch_is_one_shot_with_lockout();
    test_launch_rejections();
    test_ready_is_lost_on_reconnect_and_needs_observed_completion();
    test_prepare_rules();
    test_prepare_refused_timeout_and_failures();
    test_prepare_after_error_is_allowed_but_not_during_lockout();
    test_panic_and_auto_do_nothing_yet();
    test_messages();

    if (failures == 0) {
        std::printf("ALL CONTROL TESTS PASSED (15 tests)\n");
        return 0;
    }
    std::printf("%d CHECK(S) FAILED\n", failures);
    return 1;
}
