#include "control_logic.h"

#include <string.h>

namespace {

inline uint32_t elapsed(uint32_t now, uint32_t since) { return static_cast<uint32_t>(now - since); }

// A problem message is never hidden by an informational one in the same step.
void set_message(ControlOutputs& out, Msg m) {
    if (out.message == Msg::None || msg_is_problem(m)) out.message = m;
}

constexpr uint8_t DEV_LAUNCHER = 0;
constexpr uint8_t DEV_SWITCHBOARD = 1;

}  // namespace

const char* msg_text(Msg m) {
    switch (m) {
        case Msg::LaunchSent: return "LAUNCHED";
        case Msg::LaunchOffline: return "launcher offline";
        case Msg::LaunchStateUnknown: return "launcher state ?";
        case Msg::LaunchNotPrepared: return "not prepared";
        case Msg::LaunchBusy: return "still preparing";
        case Msg::LaunchError: return "launcher error";
        case Msg::LaunchLockout: return "just launched";
        case Msg::LaunchFailed: return "LAUNCH FAILED";
        case Msg::PrepareSent: return "preparing...";
        case Msg::PrepareOffline: return "launcher offline";
        case Msg::PrepareStateUnknown: return "launcher state ?";
        case Msg::PrepareBusy: return "already preparing";
        case Msg::PrepareLockout: return "just launched";
        case Msg::PrepareRefused: return "prepare refused";
        case Msg::PrepareTimeout: return "prepare timeout";
        case Msg::PrepareFailed: return "PREPARE FAILED";
        case Msg::None: break;
    }
    return "";
}

bool msg_is_problem(Msg m) { return m != Msg::None && m != Msg::LaunchSent && m != Msg::PrepareSent; }

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

bool ControlLogic::lockedOut(uint32_t now) const { return launched_ && elapsed(now, launchedAt_) < LAUNCH_LOCKOUT_MS; }

// ------------------------------------------------------------------ launcher
void ControlLogic::trackLauncher(const ControlInputs& in, ControlOutputs& out) {
    const uint32_t now = in.now;

    if (!in.launcherConnected) {
        // Everything known about the launcher dies with the connection.
        prepared_ = false;
        prepRunning_ = false;
        prepTimedOut_ = false;
        prepareRequested_ = false;
        prevKnown_ = false;
    } else if (!in.prepValid) {
        prepared_ = false;
        prepRunning_ = false;
        prevKnown_ = false;
    } else {
        const uint8_t p = in.prep;
        const bool running = (p >= 1 && p <= 3);
        if (running) {
            if (!prepRunning_) {
                prepRunning_ = true;
                prepRunningSince_ = now;
            }
            prepared_ = false;
            prepareRequested_ = false;  // it started, so it was not refused
            if (!prepTimedOut_ && elapsed(now, prepRunningSince_) > PREPARE_TIMEOUT_MS) {
                prepTimedOut_ = true;
                set_message(out, Msg::PrepareTimeout);
            }
        } else {
            // Ready only if WE saw a preparation finish normally during this connection.
            if (prepRunning_ && prevKnown_ && p == 0) prepared_ = true;
            prepRunning_ = false;
            prepTimedOut_ = false;
            if (p == 4) {
                prepared_ = false;
                prepareRequested_ = false;
            }
        }
        prevKnown_ = true;
    }

    if (prepareRequested_ && elapsed(now, prepareRequestedAt_) > PREPARE_START_TIMEOUT_MS) {
        prepareRequested_ = false;  // written, but the launcher never started: it refused
        set_message(out, Msg::PrepareRefused);
        rejectFlash_ = true;
        rejectFlashAt_ = now;
    }
}

void ControlLogic::handleLaunch(const ControlInputs& in, ControlOutputs& out) {
    if (in.launchResult == Result::Failed) {
        set_message(out, Msg::LaunchFailed);
        rejectFlash_ = true;
        rejectFlashAt_ = in.now;
    }
    if (!in.launchPressed) return;

    Msg reject = Msg::None;
    if (!in.launcherConnected) reject = Msg::LaunchOffline;
    else if (!in.prepValid) reject = Msg::LaunchStateUnknown;
    else if (in.prep == 4 || prepTimedOut_) reject = Msg::LaunchError;
    else if (in.prep != 0) reject = Msg::LaunchBusy;
    else if (lockedOut(in.now)) reject = Msg::LaunchLockout;
    else if (!prepared_) reject = Msg::LaunchNotPrepared;

    if (reject != Msg::None) {
        set_message(out, reject);
        rejectFlash_ = true;
        rejectFlashAt_ = in.now;
        return;  // nothing is sent
    }
    addCommand(out, DEV_LAUNCHER, LCH_LAUNCH, 1, 0, 1, CmdTag::Launch);
    prepared_ = false;  // the arm is released: it must be prepared again
    launched_ = true;
    launchedAt_ = in.now;
    set_message(out, Msg::LaunchSent);
}

void ControlLogic::handlePrepare(const ControlInputs& in, ControlOutputs& out) {
    if (in.prepareResult == Result::Failed) {
        prepareRequested_ = false;
        set_message(out, Msg::PrepareFailed);
        rejectFlash_ = true;
        rejectFlashAt_ = in.now;
    }
    if (!in.prepareRequested) return;

    Msg reject = Msg::None;
    if (!in.launcherConnected) reject = Msg::PrepareOffline;
    else if (!in.prepValid) reject = Msg::PrepareStateUnknown;
    else if (in.prep >= 1 && in.prep <= 3) reject = Msg::PrepareBusy;
    else if (prepareRequested_) reject = Msg::PrepareBusy;
    else if (lockedOut(in.now)) reject = Msg::PrepareLockout;

    if (reject != Msg::None) {
        set_message(out, reject);
        rejectFlash_ = true;
        rejectFlashAt_ = in.now;
        return;
    }
    addCommand(out, DEV_LAUNCHER, LCH_PREPARE, 1, 0, 1, CmdTag::Prepare);
    prepareRequested_ = true;
    prepareRequestedAt_ = in.now;
    prepared_ = false;
    prepTimedOut_ = false;
    set_message(out, Msg::PrepareSent);
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

// ---------------------------------------------------------------------- step
void ControlLogic::step(const ControlInputs& in, ControlOutputs& out) {
    out.cmdCount = 0;
    out.message = Msg::None;
    for (uint8_t i = 0; i < 8; i++) out.led[i] = LED_OFF;

    trackLauncher(in, out);
    handleLaunch(in, out);
    handlePrepare(in, out);

    const bool sw_ok = in.switchesValid;
    out.led[3] = reconcileRelay(in, out, RELAY_DECORATIVE_LIGHT, sw_ok && in.sw[CSW_DECOR]);   // L4
    out.led[6] = reconcileRelay(in, out, RELAY_FLASHING_LIGHT, sw_ok && in.sw[CSW_FLASH]);     // L7
    reconcileRelay(in, out, RELAY_SIREN, sw_ok && in.sw[CSW_SIREN]);                           // no LED
    out.led[7] = handleFog(in, out);                                                           // L8

    // Launcher status and L2 (ready to launch).
    const bool known = in.launcherConnected && in.prepValid;
    const bool running = known && in.prep >= 1 && in.prep <= 3;
    if (!known) out.launcher = LauncherStatus::Offline;
    else if (in.prep == 4 || prepTimedOut_) out.launcher = LauncherStatus::Error;
    else if (running) out.launcher = LauncherStatus::Preparing;
    else if (lockedOut(in.now)) out.launcher = LauncherStatus::Launched;
    else if (prepared_) out.launcher = LauncherStatus::Ready;
    else out.launcher = LauncherStatus::Idle;

    if (rejectFlash_ && elapsed(in.now, rejectFlashAt_) >= REJECT_FLASH_MS) rejectFlash_ = false;
    switch (out.launcher) {
        case LauncherStatus::Ready: out.led[1] = LED_ON; break;
        case LauncherStatus::Preparing: out.led[1] = LED_PULSE; break;
        case LauncherStatus::Error: out.led[1] = LED_FLASH_FAST; break;
        default: out.led[1] = LED_OFF; break;
    }
    if (rejectFlash_) out.led[1] = LED_FLASH_FAST;
}
