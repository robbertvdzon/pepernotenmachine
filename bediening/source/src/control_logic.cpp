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
        case Msg::MotorStopping: return STR_MSG_MOTOR_STOPPING;
        case Msg::None: break;
    }
    return "";
}

bool msg_is_problem(Msg m) { return m != Msg::None && m != Msg::LaunchSent && m != Msg::ReleaseOk; }

ControlLogic::ControlLogic() {
    memset(relaySent_, 0, sizeof(relaySent_));
    memset(lastRelaySend_, 0, sizeof(lastRelaySend_));
    memset(pending_, 0, sizeof(pending_));
    memset(pendingSince_, 0, sizeof(pendingSince_));
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
}

void ControlLogic::addCommandBytes(ControlOutputs& out, uint8_t device, uint8_t ci, const uint8_t* data, uint8_t len, CmdTag tag) {
    if (out.cmdCount >= sizeof(out.cmds) / sizeof(out.cmds[0])) return;
    Command& c = out.cmds[out.cmdCount++];
    c.device = device;
    c.charIndex = ci;
    c.len = len > 4 ? 4 : len;
    for (uint8_t i = 0; i < 4; i++) c.data[i] = i < c.len ? data[i] : 0;
    c.tag = tag;
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
    out.dispense = runHold(dispenseHold_, DISPENSER, in.dispenseHeld ? 1 : 0, in.dispenserValid, in.dispenser != 0,
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
        else if (h.owned && !sp.restartWhileStopping) reject = Msg::MotorStopping;
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
                start[0] = 2;
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

// -------------------------------------------------------------------- relays
// The switch position is the truth. The relay is re-sent until the switchboard reports the
// wanted state. Returns the LED state for it: on/off when confirmed, flashing while unconfirmed.
led_state_t ControlLogic::reconcileRelay(const ControlInputs& in, ControlOutputs& out, uint8_t relay, bool want_on) {
    if (relay < 1 || relay > 8) return LED_OFF;  // not wired
    const uint8_t i = relay - 1;
    if (!in.switchesValid) {  // the wish is unknown: change nothing
        pending_[i] = false;
        return LED_OFF;
    }

    const bool can_know = in.switchboardConnected && in.relaysValid;
    const bool matches = can_know && in.relays[i] == (want_on ? 1 : 0);
    if (matches) {
        pending_[i] = false;
        relaySent_[i] = false;
        return want_on ? LED_ON : LED_OFF;
    }
    if (!want_on && !can_know) {  // nothing wanted and nothing known: stay quiet
        pending_[i] = false;
        return LED_OFF;
    }

    if (!pending_[i]) {
        pending_[i] = true;
        pendingSince_[i] = in.now;
    }
    if (can_know && (!relaySent_[i] || elapsed(in.now, lastRelaySend_[i]) >= RELAY_RESEND_MS)) {
        addCommand(out, DEV_SWITCHBOARD, SCH_RELAYS, relay, want_on ? 1 : 0, 2, CmdTag::None);
        relaySent_[i] = true;
        lastRelaySend_[i] = in.now;
    }
    return elapsed(in.now, pendingSince_[i]) > LED_PENDING_ERROR_MS ? LED_FLASH_FAST : LED_FLASH;
}

// ----------------------------------------------------------------------- fog
// Switch ON = keep fogging whenever possible: start whenever the machine is ready and idle.
// Switch OFF (or switch state unknown) = stop.
led_state_t ControlLogic::handleFog(const ControlInputs& in, ControlOutputs& out) {
    const bool want = in.switchesValid && in.sw[CSW_FOG];

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
    if (!in.switchboardConnected || !in.fogStateValid) return want ? LED_FLASH_FAST : LED_OFF;
    switch (in.fogState) {
        case 0: return want ? LED_FLASH_FAST : LED_OFF;
        case 1: return LED_FLASH;
        case 2: return LED_ON;
        case 3: return LED_PULSE;
        default: return LED_FLASH_FAST;
    }
}

// --------------------------------------------------------------------- panic
// SW2 ON: bells / horns (PANIC_RELAYS) go on one at a time, in random order, for random times
// with random pauses. SW2 OFF: they are all switched off (forced for PANIC_CLEANUP_MS, after
// that those relays are left alone again, they may be used for other things).
led_state_t ControlLogic::handlePanic(const ControlInputs& in, ControlOutputs& out) {
    const bool active = in.switchesValid && in.sw[CSW_PANIC];

    if (!active) {
        if (panicActive_) {
            panicActive_ = false;
            panicCurrent_ = 0;
            panicCleanup_ = true;
            panicCleanupUntil_ = in.now + PANIC_CLEANUP_MS;
        }
        if (panicCleanup_ && due(in.now, panicCleanupUntil_)) panicCleanup_ = false;
    } else {
        panicCleanup_ = false;
        if (!panicActive_) {
            panicActive_ = true;
            panicCurrent_ = 0;
            panicNextAt_ = in.now;
        }
        if (due(in.now, panicNextAt_)) {
            if (panicCurrent_ != 0) {  // pause
                panicCurrent_ = 0;
                panicNextAt_ = in.now + PANIC_GAP_MIN_MS + (in.random % PANIC_GAP_SPAN_MS);
            } else {  // next one: random, but not the same twice in a row
                uint8_t idx = (in.random >> 8) % PANIC_RELAY_COUNT;
                if (PANIC_RELAY_COUNT > 1 && PANIC_RELAYS[idx] == panicLast_) idx = (idx + 1) % PANIC_RELAY_COUNT;
                panicCurrent_ = PANIC_RELAYS[idx];
                panicLast_ = panicCurrent_;
                panicNextAt_ = in.now + PANIC_ON_MIN_MS + ((in.random >> 16) % PANIC_ON_SPAN_MS);
            }
        }
    }

    if (panicActive_ || panicCleanup_) {
        for (uint8_t k = 0; k < PANIC_RELAY_COUNT; k++) {
            reconcileRelay(in, out, PANIC_RELAYS[k], panicActive_ && panicCurrent_ == PANIC_RELAYS[k]);
        }
    }

    // L6: on = panic active; fast flash = active but the switchboard cannot be reached.
    if (!panicActive_) return LED_OFF;
    return (in.switchboardConnected && in.relaysValid) ? LED_ON : LED_FLASH_FAST;
}

// ---------------------------------------------------------------------- step
void ControlLogic::step(const ControlInputs& in, ControlOutputs& out) {
    out.cmdCount = 0;
    out.message = Msg::None;
    for (uint8_t i = 0; i < 8; i++) out.led[i] = LED_OFF;

    results(in, out);
    trackLauncher(in, out);
    handleAnswer(in);
    handleLaunch(in, out);
    handleMaintenance(in, out);
    autoPrepare(in, out);

    const bool sw_ok = in.switchesValid;
    out.led[3] = reconcileRelay(in, out, RELAY_DECORATIVE_LIGHT, sw_ok && in.sw[CSW_DECOR]);  // L4
    out.led[6] = reconcileRelay(in, out, RELAY_FLASHING_LIGHT, sw_ok && in.sw[CSW_FLASH]);    // L7
    out.sirenState = reconcileRelay(in, out, RELAY_SIREN, sw_ok && in.sw[CSW_SIREN]);         // no LED, display only
    out.led[7] = handleFog(in, out);                                                          // L8
    out.led[5] = handlePanic(in, out);                                                        // L6

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
}
