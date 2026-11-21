#include "control_logic.h"

#include <string.h>

#include "ui_strings.h"

namespace {

inline uint32_t elapsed(uint32_t now, uint32_t since) { return static_cast<uint32_t>(now - since); }
inline bool due(uint32_t now, uint32_t at) { return static_cast<int32_t>(now - at) >= 0; }

// A problem message is never hidden by an informational one in the same step.
void set_message(ControlOutputs& out, Msg m) {
    if (out.message == Msg::None || msg_is_problem(m)) out.message = m;
}

constexpr uint8_t DEV_LAUNCHER = 0;
constexpr uint8_t DEV_SWITCHBOARD = 1;

}  // namespace

const char* msg_text(Msg m) {
    switch (m) {
        case Msg::LaunchSent: return STR_MSG_LAUNCHED;
        case Msg::LaunchOffline: return STR_MSG_LAUNCHER_OFFLINE;
        case Msg::LaunchStateUnknown: return STR_MSG_LAUNCHER_STATE_UNKNOWN;
        case Msg::LaunchNotReady: return STR_MSG_NOT_READY;
        case Msg::LaunchBusy: return STR_MSG_STILL_PREPARING;
        case Msg::LaunchError: return STR_MSG_LAUNCHER_ERROR;
        case Msg::LaunchCheckFirst: return STR_MSG_CHECK_FIRST;
        case Msg::LaunchFailed: return STR_MSG_LAUNCH_FAILED;
        case Msg::PrepareRefused: return STR_MSG_PREPARE_REFUSED;
        case Msg::PrepareTimeout: return STR_MSG_PREPARE_TIMEOUT;
        case Msg::PrepareFailed: return STR_MSG_PREPARE_FAILED;
        case Msg::MaintBlocksLaunch: return STR_MSG_MAINT_BLOCKS_LAUNCH;
        case Msg::MaintBusy: return STR_MSG_MAINT_BUSY;
        case Msg::ReleaseOk: return STR_MSG_RELEASE_OK;
        case Msg::ReleaseFailed: return STR_MSG_RELEASE_FAILED;
        case Msg::DispenseFailed: return STR_MSG_DISPENSE_FAILED;
        case Msg::DispenseStuck: return STR_MSG_DISPENSE_STUCK;
        case Msg::MotorFailed: return STR_MSG_MOTOR_FAILED;
        case Msg::MotorStuck: return STR_MSG_MOTOR_STUCK;
        case Msg::StillStopping: return STR_MSG_STILL_STOPPING;
        case Msg::DurationSaved: return STR_MSG_DURATION_SAVED;
        case Msg::DurationFailed: return STR_MSG_DURATION_FAILED;
        case Msg::None: break;
    }
    return "";
}

bool msg_is_problem(Msg m) { return m != Msg::None && m != Msg::LaunchSent && m != Msg::ReleaseOk && m != Msg::DurationSaved; }

ControlLogic::ControlLogic() {
    memset(relaySent_, 0, sizeof(relaySent_));
    memset(lastRelaySend_, 0, sizeof(lastRelaySend_));
    memset(pending_, 0, sizeof(pending_));
    memset(pendingSince_, 0, sizeof(pendingSince_));
    memset(relaySentWant_, 0, sizeof(relaySentWant_));
    for (uint8_t i = 0; i < DEVICE_COUNT; i++) source_[i] = Source::None;
}

void ControlLogic::addCommand(ControlOutputs& out, uint8_t device, uint8_t ci, uint8_t b0, uint8_t b1, uint8_t len,
                              CmdTag tag) {
    if (out.cmdCount >= sizeof(out.cmds) / sizeof(out.cmds[0])) return;
    Command& c = out.cmds[out.cmdCount++];
    c.device = device;
    c.charIndex = ci;
    c.len = len;
    c.data[0] = b0;
    c.data[1] = b1;
    c.data[2] = 0;
    c.data[3] = 0;
    c.tag = tag;
    c.read = false;
}

void ControlLogic::addCommandBytes(ControlOutputs& out, uint8_t device, uint8_t ci, const uint8_t* data, uint8_t len, CmdTag tag) {
    if (out.cmdCount >= sizeof(out.cmds) / sizeof(out.cmds[0])) return;
    Command& c = out.cmds[out.cmdCount++];
    c.device = device;
    c.charIndex = ci;
    c.len = len > 4 ? 4 : len;
    for (uint8_t i = 0; i < 4; i++) c.data[i] = i < c.len ? data[i] : 0;
    c.tag = tag;
    c.read = false;
}

void ControlLogic::addRead(ControlOutputs& out, uint8_t device, uint8_t ci, CmdTag tag) {
    if (out.cmdCount >= sizeof(out.cmds) / sizeof(out.cmds[0])) return;
    Command& c = out.cmds[out.cmdCount++];
    c.device = device;
    c.charIndex = ci;
    c.len = 0;
    for (uint8_t i = 0; i < 4; i++) c.data[i] = 0;
    c.tag = tag;
    c.read = true;
}

// What we believed about the launcher can no longer be trusted: stop acting on it, ask the driver.
void ControlLogic::askDriver(CheckReason why) {
    known_ = Known::Unknown;
    needConfirm_ = true;
    reason_ = why;
    rearmValid_ = false;
    prepareRequested_ = false;
}

// ----------------------------------------------------------------- one-shot results
void ControlLogic::results(const ControlInputs& in, ControlOutputs& out) {
    if (in.launchResult != Result::None) launchPending_ = false;
    if (in.launchResult == Result::Ok) launchOkCount_++;
    if (in.launchResult == Result::Failed) {
        // The write failed, so it is unknown whether the launcher fired. Never retried.
        set_message(out, Msg::LaunchFailed);
        rejectFlash_ = true;
        rejectFlashAt_ = in.now;
        askDriver(CheckReason::Error);
    }

    if (in.releaseResult == Result::Ok) set_message(out, Msg::ReleaseOk);
    if (in.releaseResult == Result::Failed) set_message(out, Msg::ReleaseFailed);  // never retried
    if (in.dispenseResult == Result::Failed) set_message(out, Msg::DispenseFailed);
    if (in.motorResult == Result::Failed) set_message(out, Msg::MotorFailed);

    if (in.prepareResult == Result::Ok) prepFails_ = 0;
    if (in.prepareResult == Result::Failed) {
        prepareRequested_ = false;
        set_message(out, Msg::PrepareFailed);
        if (++prepFails_ >= PREPARE_MAX_FAILS) {
            prepFails_ = 0;
            askDriver(CheckReason::Error);
        } else {
            rearmValid_ = true;  // try again shortly
            rearmAt_ = in.now + PREPARE_RETRY_MS;
        }
    }
}

// ------------------------------------------------------------------- launcher state
void ControlLogic::trackLauncher(const ControlInputs& in, ControlOutputs& out) {
    const uint32_t now = in.now;

    if (!in.launcherConnected) {
        // Lost while preparing: how it ended cannot be known. Lost while armed or at rest: the
        // belief is kept (a dropout on a bumpy tractor must not bother the driver).
        if ((prepareRequested_ || prepRunning_) && known_ != Known::Unknown) askDriver(CheckReason::Startup);
        prepRunning_ = false;
        prepareRequested_ = false;
        prepTimedOut_ = false;
        return;
    }
    if (!in.prepValid) {
        prepRunning_ = false;
        return;
    }

    const uint8_t p = in.prep;
    const bool running = (p >= 1 && p <= 3);
    if (running) {
        if (!prepRunning_) {
            prepRunning_ = true;
            prepRunningSince_ = now;
        }
        prepareRequested_ = false;  // it started, so it was not refused
        errorAcked_ = false;
        if (!prepTimedOut_ && elapsed(now, prepRunningSince_) > PREPARE_TIMEOUT_MS) {
            prepTimedOut_ = true;
            set_message(out, Msg::PrepareTimeout);
            askDriver(CheckReason::Error);
        }
    } else {
        if (prepRunning_) {
            prepRunning_ = false;
            // Armed only if WE watched a preparation finish normally.
            if (p == 0 && !prepTimedOut_) {
                known_ = Known::Armed;
                needConfirm_ = false;
                reason_ = CheckReason::None;
                prepFails_ = 0;
            }
            prepTimedOut_ = false;
        }
        if (p == 4) {
            if (!errorAcked_ && !(needConfirm_ && reason_ == CheckReason::Error)) askDriver(CheckReason::Error);
        } else {
            errorAcked_ = false;
        }
        // First look at an idle launcher whose state we do not know: ask the driver.
        if (p == 0 && known_ == Known::Unknown && !needConfirm_) {
            needConfirm_ = true;
            reason_ = CheckReason::Startup;
        }
    }

    // The prepare command was accepted but the launcher never left idle: refused. Do not retry, ask the driver.
    if (prepareRequested_ && (p == 0 || p == 4) && elapsed(now, prepareRequestedAt_) > PREPARE_START_TIMEOUT_MS) {
        set_message(out, Msg::PrepareRefused);
        rejectFlash_ = true;
        rejectFlashAt_ = now;
        askDriver(CheckReason::Error);
    }
}

void ControlLogic::handleAnswer(const ControlInputs& in) {
    if (in.checkRequested && !needConfirm_) askDriver(CheckReason::Requested);

    if (in.answer == Answer::None || !needConfirm_) return;
    if (!in.launcherConnected || !in.prepValid || prepRunning_) return;

    if (in.answer != Answer::Empty && in.answer != Answer::Full) return;  // "Not sure": keep asking, nothing moves

    // Either way the launcher is prepared from scratch (the preparation always starts with "going down", also
    // when the bucket already sits in the lock); only the load is unknown, hence whether to fill the bucket.
    const bool error_state = (in.prep == 4);
    known_ = Known::AtRest;
    prepareWithDispenser_ = (in.answer == Answer::Empty);
    rearmValid_ = true;  // start preparing right away
    rearmAt_ = in.now;
    errorAcked_ = error_state;  // the launcher keeps reporting its error until it is prepared again
    needConfirm_ = false;
    reason_ = CheckReason::None;
    prepFails_ = 0;
    prepareRequested_ = false;
}

void ControlLogic::handleLaunch(const ControlInputs& in, ControlOutputs& out) {
    if (!in.launchPressed) return;

    Msg reject = Msg::None;
    if (in.maintenance) reject = Msg::MaintBlocksLaunch;
    else if (!in.launcherConnected) reject = Msg::LaunchOffline;
    else if (!in.prepValid) reject = Msg::LaunchStateUnknown;
    else if (needConfirm_) reject = Msg::LaunchCheckFirst;
    else if (in.prep == 4) reject = Msg::LaunchError;
    else if (in.prep != 0 || prepareRequested_) reject = Msg::LaunchBusy;
    else if (known_ != Known::Armed || launchPending_) reject = Msg::LaunchNotReady;

    if (reject != Msg::None) {
        set_message(out, reject);
        rejectFlash_ = true;
        rejectFlashAt_ = in.now;
        return;  // nothing is sent
    }
    addCommand(out, DEV_LAUNCHER, LCH_LAUNCH, 1, 0, 1, CmdTag::Launch);
    known_ = Known::AtRest;  // the bucket is gone: it has to be prepared (and filled) again
    prepareWithDispenser_ = true;
    launchPending_ = true;
    rearmValid_ = true;
    rearmAt_ = in.now + REARM_DELAY_MS;
    set_message(out, Msg::LaunchSent);
}

// Keeps the launcher sharp: prepares it whenever it is known to be at rest.
void ControlLogic::autoPrepare(const ControlInputs& in, ControlOutputs& out) {
    if (in.maintenance) return;  // automation is paused
    const bool idle = in.launcherConnected && in.prepValid && (in.prep == 0 || in.prep == 4);
    if (known_ != Known::AtRest || needConfirm_ || !idle) return;
    if (prepRunning_ || prepareRequested_ || launchPending_) return;
    if (rearmValid_ && !due(in.now, rearmAt_)) return;

    addCommand(out, DEV_LAUNCHER, LCH_PREPARE, prepareWithDispenser_ ? 1 : 2, 0, 1, CmdTag::Prepare);
    prepareRequested_ = true;
    prepareRequestedAt_ = in.now;
    rearmValid_ = false;
    prepTimedOut_ = false;
}

// -------------------------------------------------------------- maintenance
void ControlLogic::handleMaintenance(const ControlInputs& in, ControlOutputs& out) {

    // Leaving maintenance after a release / dispense: the machine is in a state the unit cannot know.
    if (maintPrev_ && !in.maintenance && maintDirty_) {
        askDriver(CheckReason::Requested);
        maintDirty_ = false;
    }
    maintPrev_ = in.maintenance;

    const bool link = in.launcherConnected && in.prepValid;
    const bool moving = link && in.prep >= 1 && in.prep <= 3;
    const bool busy = moving || prepareRequested_;

    // ---- release the bucket (a real launch: one-shot, never retried)
    if (in.releaseRequested && in.maintenance) {
        Msg reject = Msg::None;
        if (!in.launcherConnected) reject = Msg::LaunchOffline;
        else if (!in.prepValid) reject = Msg::LaunchStateUnknown;
        else if (busy) reject = Msg::MaintBusy;
        if (reject != Msg::None) {
            set_message(out, reject);
        } else {
            addCommand(out, DEV_LAUNCHER, LCH_LAUNCH, 1, 0, 1, CmdTag::Release);
            maintDirty_ = true;
        }
    }

    // ---- hold to run: the dispenser (write 2 = until 0) and the pull motor (speed +512 / -512, stop = 0)
    static const HoldSpec DISPENSER = {LCH_DISPENSER_CONTROL, CmdTag::DispenseStart, Msg::DispenseFailed, Msg::DispenseStuck, true};
    static const HoldSpec MOTOR = {LCH_PULL_MOTOR, CmdTag::MotorStart, Msg::MotorFailed, Msg::MotorStuck, false};
    out.dispense = runHold(dispenseHold_, DISPENSER, in.dispenseHold, in.dispenserValid, in.dispenser != 0,
                           motorHold_.owned, in, out);
    out.motor = runHold(motorHold_, MOTOR, in.motorHold, in.motorValid, in.motor != 0, dispenseHold_.owned, in, out);
}

// Runs something while the knob is held. The STOP is the safe direction (see the header): it is sent at once
// when the knob is released, the screen is left, maintenance ends, the launcher starts moving or the link was
// interrupted; always at least twice, repeated until it reports "stopped".
RunState ControlLogic::runHold(HoldState& h, const HoldSpec& sp, int8_t dir, bool valid, bool running, bool otherOwned,
                               const ControlInputs& in, ControlOutputs& out) {
    const uint32_t now = in.now;
    const bool link = in.launcherConnected && in.prepValid;
    const bool moving = link && in.prep >= 1 && in.prep <= 3;
    const bool busy = moving || prepareRequested_;
    const bool held = in.maintenance && dir != 0;
    const bool others_running = valid && running && !h.owned;

    if (held && !h.prev) {  // pressed
        Msg reject = Msg::None;
        if (!in.launcherConnected) reject = Msg::LaunchOffline;
        else if (!in.prepValid) reject = Msg::LaunchStateUnknown;
        else if (busy || others_running || otherOwned) reject = Msg::MaintBusy;
        else if (h.owned && (!sp.restartWhileStopping || dir != h.dir)) reject = Msg::StillStopping;  // never reverse on the fly
        if (reject != Msg::None) {
            set_message(out, reject);
        } else {
            uint8_t start[4] = {0, 0, 0, 0};
            uint8_t start_len = 1;
            if (sp.chr == LCH_PULL_MOTOR) {  // signed 32-bit big-endian: +512 = 00 00 02 00, -512 = FF FF FE 00
                start_len = 4;
                start[0] = dir > 0 ? 0x00 : 0xFF;
                start[1] = dir > 0 ? 0x00 : 0xFF;
                start[2] = dir > 0 ? 0x02 : 0xFE;
            } else {
                start[0] = dir > 0 ? 2 : 3;  // 0x02 dispense, 0x03 reverse (both until 0x00)
            }
            addCommandBytes(out, DEV_LAUNCHER, sp.chr, start, start_len, sp.tag);
            h.owned = true;
            h.started = false;
            h.aborted = false;
            h.dir = dir;
            h.startAt = now;
            h.startWarned = false;
            h.stopSends = 0;
            h.stopWarned = false;
            maintDirty_ = true;
        }
    }
    h.prev = held;

    // Keep running only while everything is still fine; anything else is a stop.
    const bool run = held && link && !moving && h.owned && !h.aborted;
    RunState state = RunState::Idle;
    if (h.owned) {
        if (run) {
            h.stopSends = 0;
            h.stopSince = 0;
            if (valid && running) h.started = true;
            if (!h.started && !h.startWarned && elapsed(now, h.startAt) > DISPENSE_START_TIMEOUT_MS) {
                h.startWarned = true;  // reported once; the knob has to be released and pressed again
                set_message(out, sp.failed);
            }
            state = h.started ? RunState::Running : RunState::Starting;
        } else {
            h.aborted = true;  // whatever the reason: it has to be pressed again
            state = RunState::Stopping;
            if (h.stopSends == 0) h.stopSince = now;
            if (in.launcherConnected) {
                const bool stopped = valid && !running;
                if (stopped && h.stopSends >= DISPENSE_STOP_MIN_SENDS) {
                    h.owned = false;
                    state = RunState::Idle;
                } else if (h.stopSends == 0 || elapsed(now, h.lastStopSend) >= DISPENSE_STOP_RESEND_MS) {
                    const uint8_t zero[4] = {0, 0, 0, 0};
                    addCommandBytes(out, DEV_LAUNCHER, sp.chr, zero, sp.chr == LCH_PULL_MOTOR ? 4 : 1, CmdTag::None);
                    h.stopSends = h.stopSends < 250 ? h.stopSends + 1 : h.stopSends;
                    h.lastStopSend = now;
                }
            }
            if (h.owned && !h.stopWarned && elapsed(now, h.stopSince) > DISPENSE_STUCK_MS) {
                h.stopWarned = true;
                set_message(out, sp.stuck);
            }
        }
    }
    return state;
}

// ------------------------------------------------------------ dispenser time
void ControlLogic::handleDuration(const ControlInputs& in, ControlOutputs& out) {
    const uint32_t now = in.now;
    // 1. the outcome of what was sent
    if (durPhase_ == DurPhase::Writing) {
        if (in.durationWriteResult == Result::Ok) {
            addRead(out, DEV_LAUNCHER, LCH_DISPENSER_DURATION, CmdTag::DurationVerify);  // never trust the write alone
            durPhase_ = DurPhase::Verifying;
            durAt_ = now;
        } else if (in.durationWriteResult == Result::Failed) {
            set_message(out, Msg::DurationFailed);
            durPhase_ = DurPhase::Idle;
        }
    } else if (durPhase_ == DurPhase::Verifying) {
        if (in.durationReadResult == Result::Ok) {
            set_message(out, (in.durationValid && in.duration == durWant_) ? Msg::DurationSaved : Msg::DurationFailed);
            durPhase_ = DurPhase::Idle;
        } else if (in.durationReadResult == Result::Failed || elapsed(now, durAt_) > DURATION_VERIFY_TIMEOUT_MS) {
            set_message(out, Msg::DurationFailed);
            durPhase_ = DurPhase::Idle;
        }
    }

    // 2. new requests (only from the maintenance screens, one at a time)
    if (durPhase_ == DurPhase::Idle && in.maintenance) {
        const bool link = in.launcherConnected && in.prepValid;
        const bool busy = (link && in.prep >= 1 && in.prep <= 3) || prepareRequested_;
        if (in.durationSave) {
            Msg reject = Msg::None;
            if (!in.launcherConnected) reject = Msg::LaunchOffline;
            else if (!in.prepValid) reject = Msg::LaunchStateUnknown;
            else if (busy) reject = Msg::MaintBusy;
            if (reject != Msg::None) {
                set_message(out, reject);
            } else {
                uint8_t v = in.durationValue;
                if (v < DURATION_MIN_S) v = DURATION_MIN_S;
                if (v > DURATION_MAX_S) v = DURATION_MAX_S;
                addCommand(out, DEV_LAUNCHER, LCH_DISPENSER_DURATION, v, 0, 1, CmdTag::DurationWrite);
                durWant_ = v;
                durPhase_ = DurPhase::Writing;
                durAt_ = now;
            }
        } else if (in.durationReadRequest && in.launcherConnected) {
            addRead(out, DEV_LAUNCHER, LCH_DISPENSER_DURATION, CmdTag::DurationRead);
        }
    }
    out.durationBusy = durPhase_ != DurPhase::Idle;
}

// -------------------------------------------------------------------- relays
// The switch position is the truth. The relay is re-sent until the switchboard reports the
// wanted state. Returns the LED state for it: on/off when confirmed, flashing while unconfirmed.
// Makes one switchboard relay match the wish: sent at once when the wish changes, repeated until the relay reports it.
// The status is only "Pending" after LED_PENDING_SHOW_MS (a normal round trip is not worth a blinking LED) and "Failing"
// after LED_PENDING_ERROR_MS.
ControlLogic::RelayStatus ControlLogic::reconcileRelay(const ControlInputs& in, ControlOutputs& out, uint8_t relay, bool want_on) {
    if (relay < 1 || relay > 8) return RelayStatus::Synced;  // not wired
    const uint8_t i = relay - 1;
    if (!in.switchesValid) {  // the wish is unknown: change nothing
        pending_[i] = false;
        return RelayStatus::Synced;
    }

    const bool can_know = in.switchboardConnected && in.relaysValid;
    const bool matches = can_know && in.relays[i] == (want_on ? 1 : 0);
    if (matches) {
        pending_[i] = false;
        relaySent_[i] = false;
        return RelayStatus::Synced;
    }
    if (!want_on && !can_know) {  // nothing wanted and nothing known: stay quiet
        pending_[i] = false;
        return RelayStatus::Synced;
    }

    if (!pending_[i]) {
        pending_[i] = true;
        pendingSince_[i] = in.now;
    }
    if (can_know && (!relaySent_[i] || relaySentWant_[i] != want_on || elapsed(in.now, lastRelaySend_[i]) >= RELAY_RESEND_MS)) {
        addCommand(out, DEV_SWITCHBOARD, SCH_RELAYS, relay, want_on ? 1 : 0, 2, CmdTag::None);
        relaySent_[i] = true;
        relaySentWant_[i] = want_on;
        lastRelaySend_[i] = in.now;
    }
    const uint32_t waited = elapsed(in.now, pendingSince_[i]);
    return waited > LED_PENDING_ERROR_MS ? RelayStatus::Failing : (waited > LED_PENDING_SHOW_MS ? RelayStatus::Pending : RelayStatus::Synced);
}

// ------------------------------------------------------------------- devices
// Every device of control_patterns.h is decided each step: its manual switch (steady on) beats panic mode (its panic
// behavior) beats automatic mode (its auto behavior). All wired relays are always driven, so leaving a mode switches
// the devices off again by itself. Also sets L1 (auto), L4 / L7 (illumination / flashlights: their manual switch or
// a mode makes them active; on = all relays follow, flash / fast flash = a relay does not follow), L6 (panic),
// L8 (fog) and the airhorn state.
void ControlLogic::handleDevices(const ControlInputs& in, ControlOutputs& out) {
    const bool sw_ok = in.switchesValid;
    const bool panic = sw_ok && in.sw[CSW_PANIC];
    const bool auto_on = sw_ok && in.sw[CSW_AUTO];
    const bool board_ok = in.switchboardConnected && in.relaysValid;
    rng_ ^= in.random;  // fresh randomness every step; the generator keeps it varied

    bool want[DEVICE_COUNT];
    bool enabled[DEVICE_COUNT];
    RelayStatus status[DEVICE_COUNT];
    for (uint8_t i = 0; i < DEVICE_COUNT; i++) {
        const DeviceCfg& d = DEVICES[i];
        Source src = Source::None;
        Behaviour b = BEH_OFF;
        if (sw_ok && d.manualSwitch != CSW_NONE && in.sw[d.manualSwitch]) {
            src = Source::Manual;
            b = BEH_ON;
        } else if (panic) {
            src = Source::Panic;
            b = d.panicMode;
        } else if (auto_on) {
            src = Source::Auto;
            b = d.autoMode;
        }
        if (src != source_[i]) {  // another source takes over: its pattern starts from its first phase
            source_[i] = src;
            runner_[i].reset();
        }
        enabled[i] = b.mode != Mode::Off;
        if (b.mode == Mode::Pattern && b.pattern) want[i] = runner_[i].step(*b.pattern, in.now, rng_);
        else {
            want[i] = b.mode == Mode::On;
            runner_[i].reset();
        }
        status[i] = d.relay ? reconcileRelay(in, out, d.relay, want[i]) : RelayStatus::Synced;
    }

    // L4 / L7 / airhorn: one LED per manual switch, covering every device that switch belongs to
    auto group = [&](int8_t manual) {
        bool any = false;
        RelayStatus worst = RelayStatus::Synced;
        for (uint8_t i = 0; i < DEVICE_COUNT; i++) {
            if (DEVICES[i].manualSwitch != manual || !DEVICES[i].relay) continue;
            any = any || enabled[i];
            if (status[i] == RelayStatus::Failing) worst = RelayStatus::Failing;
            else if (status[i] == RelayStatus::Pending && worst == RelayStatus::Synced) worst = RelayStatus::Pending;
        }
        if (!any) return LED_OFF;
        return worst == RelayStatus::Failing ? LED_FLASH_FAST : (worst == RelayStatus::Pending ? LED_FLASH : LED_ON);
    };
    out.led[3] = group(CSW_DECOR);   // L4
    out.led[6] = group(CSW_FLASH);   // L7
    out.hornState = group(CSW_HORN);

    out.led[0] = !auto_on ? LED_OFF : (board_ok ? LED_ON : LED_FLASH_FAST);    // L1 automatic mode
    out.led[5] = !panic ? LED_OFF : (board_ok ? LED_ON : LED_FLASH_FAST);      // L6 panic
    out.led[7] = handleFog(in, out, want[DEV_FOG], enabled[DEV_FOG]);          // L8
}

// ----------------------------------------------------------------------- fog
// `want` = fogging is wanted right now (manual switch, panic, or the on-phase of the auto pattern): start whenever the
// machine is ready and idle. Not wanted (or switch state unknown) = stop. `enabled` = a source wants fog at all (it is
// only then that an unavailable machine is worth a fast flash).
led_state_t ControlLogic::handleFog(const ControlInputs& in, ControlOutputs& out, bool want, bool enabled) {
    if (in.switchboardConnected && in.fogStateValid && in.fogControlValid) {
        const bool fogging = in.fogControl == 1 || in.fogState == 3;
        if (want && in.fogState == 2 && !fogging) {
            if (!fogStartSent_ || elapsed(in.now, lastFogStart_) >= FOG_START_INTERVAL_MS) {
                addCommand(out, DEV_SWITCHBOARD, SCH_FOG_CONTROL, 1, 0, 1, CmdTag::None);
                fogStartSent_ = true;
                lastFogStart_ = in.now;
            }
        }
        if (!want && fogging) {
            if (!fogStopSent_ || elapsed(in.now, lastFogStop_) >= FOG_STOP_RESEND_MS) {
                addCommand(out, DEV_SWITCHBOARD, SCH_FOG_CONTROL, 0, 0, 1, CmdTag::None);
                fogStopSent_ = true;
                lastFogStop_ = in.now;
            }
        } else {
            fogStopSent_ = false;
        }
    }

    // L8: off = machine not connected, flash = heating, on = ready, pulse = fogging,
    // fast flash = fogging is wanted but impossible (machine or switchboard not available).
    if (!in.switchboardConnected || !in.fogStateValid) return enabled ? LED_FLASH_FAST : LED_OFF;
    switch (in.fogState) {
        case 0: return enabled ? LED_FLASH_FAST : LED_OFF;
        case 1: return LED_FLASH;
        case 2: return LED_ON;
        case 3: return LED_PULSE;
        default: return LED_FLASH_FAST;
    }
}

// ---------------------------------------------------------------------- step
void ControlLogic::step(const ControlInputs& in, ControlOutputs& out) {
    out.cmdCount = 0;
    out.message = Msg::None;
    for (uint8_t i = 0; i < 12; i++) out.led[i] = LED_OFF;

    results(in, out);
    trackLauncher(in, out);
    handleAnswer(in);
    handleLaunch(in, out);
    handleMaintenance(in, out);
    handleDuration(in, out);
    autoPrepare(in, out);

    handleDevices(in, out);

    // Launcher status and L2 (ready to launch).
    const bool known = in.launcherConnected && in.prepValid;
    const bool running = known && in.prep >= 1 && in.prep <= 3;
    out.rearmInMs = (known_ == Known::AtRest && rearmValid_ && !due(in.now, rearmAt_)) ? rearmAt_ - in.now : 0;
    if (!known) out.launcher = LauncherStatus::Offline;
    else if (needConfirm_ && reason_ == CheckReason::Error) out.launcher = LauncherStatus::Error;
    else if (needConfirm_) out.launcher = LauncherStatus::NeedsCheck;
    else if (running || prepareRequested_) out.launcher = LauncherStatus::Preparing;
    else if (known_ == Known::Armed && in.prep == 0) out.launcher = LauncherStatus::Ready;
    else if (out.rearmInMs > 0) out.launcher = LauncherStatus::Rearming;
    else out.launcher = LauncherStatus::Preparing;  // at rest, about to start

    // The check overlay is only shown while it can be answered (idle launcher, state known).
    out.needsConfirm = needConfirm_ && known && !running && !prepRunning_ && !in.maintenance;  // held back while in maintenance
    out.reason = out.needsConfirm ? reason_ : CheckReason::None;
    out.known = known_;
    out.launchPending = launchPending_;
    out.launchOkCount = launchOkCount_;

    if (rejectFlash_ && elapsed(in.now, rejectFlashAt_) >= REJECT_FLASH_MS) rejectFlash_ = false;
    switch (out.launcher) {
        case LauncherStatus::Ready: out.led[1] = LED_ON; break;
        case LauncherStatus::Preparing: out.led[1] = LED_PULSE; break;
        case LauncherStatus::Rearming: out.led[1] = LED_FLASH; break;
        case LauncherStatus::NeedsCheck: out.led[1] = LED_FLASH; break;
        case LauncherStatus::Error: out.led[1] = LED_FLASH_FAST; break;
        default: out.led[1] = LED_OFF; break;
    }
    if (rejectFlash_) out.led[1] = LED_FLASH_FAST;

    // L9: the launcher is busy preparing (pulses), off once it is ready to launch; an error behaves like L2.
    // (Waiting for the driver, the re-arm countdown and a rejected launch are L2 matters: L9 stays off.)
    switch (out.launcher) {
        case LauncherStatus::Preparing: out.led[8] = LED_PULSE; break;
        case LauncherStatus::Error: out.led[8] = LED_FLASH_FAST; break;
        default: out.led[8] = LED_OFF; break;
    }
}
