#pragma once

// Pure control logic (no hardware, no BLE, no Arduino): turns the switch states, the
// device states and the operator's answers into device commands, LED states and
// messages. Called once per main-loop pass; all timing comes from `ControlInputs::now`.
//
// THE LAUNCHER (fully automatic, no switch involved):
//   The unit can never know the physical state of the launcher after a power-off. The position of the bucket
//   does not matter (a preparation always starts with "going down"); what nobody knows is the LOAD. So after
//   power-up the DRIVER is asked once ("check" overlay):
//     "Bucket empty" -> the unit prepares it by itself, incl. the dispenser cycle   (prepare = 0x01)
//     "Bucket full"  -> the unit prepares it by itself, without the dispenser cycle (prepare = 0x02)
//     "Not sure?"    -> nothing moves, the screen tells how to check by hand
//   It is never ready without a preparation (nothing is "armed" until this unit watched one finish).
//   After that it stays "sharp": launch -> REARM_DELAY_MS -> prepare again -> ready ...
//   What the unit believes (`Known`) is only a belief: it is dropped whenever it can no
//   longer be trusted (error, refused/failed prepare, failed launch write, link lost during
//   a preparation, "Check launcher" from the menu) and then the driver is asked again.
//   A launcher error is shown by L2 (fast flash) and the same check; the driver fixes the
//   machine by hand and confirms.
//
// SAFETY RULES:
//  * LAUNCH is a one-shot: sent at most once per button press, never retried, never replayed
//    on reconnect. It needs a connected launcher, a CURRENT state, idle, and the belief
//    "armed" (a preparation this unit watched finish).
//    A rejected launch sends nothing and says why.
//  * The launch write must be reported OK before the next prepare is sent; if it failed, the
//    outcome is unknown and the driver is asked.
//  * PREPARE is sent automatically, at most once per attempt. No reaction within
//    PREPARE_START_TIMEOUT_MS = "refused": the driver is asked.
//  * Relays / fog are idempotent STATE settings. The switch position is the truth: they are
//    compared with what the switchboard reports and re-sent until it matches, which also
//    re-applies them after a reconnect (the switchboard turns everything off when its last
//    client disconnects).
//  * Unknown switch state => fog is stopped and nothing else is changed.
//  * SW5 (automatic mode) is not defined yet and does nothing.
//
// MAINTENANCE (menu "Maintenance", `ControlInputs::maintenance` = the UI is on one of its screens):
//  * All automation pauses: no automatic prepare, SW8 is blocked, the check overlay is held back.
//  * "Release bucket" writes 1 to LAUNCH (a real launch of whatever is in the bucket; the UI asks for
//    confirmation first). One-shot, never retried. Needs a connected launcher that is not preparing.
//  * "Motor control" runs the pull motor by hand: speed +512 (up) / -512 (down) while the knob is held on that row,
//    stop = speed 0. Same safe-stop rules as the dispenser; a new press is refused until the previous stop is confirmed.
//  * "Dispense" runs the dispenser (write 0x02 = until 0x00) while the knob is held on "dispense"; "reverse" (write 0x03,
//    until 0x00) un-jams it. A direction change needs the stop confirmed first, like the motor. The STOP is the
//    safe direction: when the knob is released, the screen is left, maintenance ends or the launcher
//    starts moving, 0x00 is sent at least twice and repeated until the dispenser reports "stopped".
//  * "Dispenser time" (1..60 s, kept by the launcher): the value is read when the screen opens; "save" writes it and READS IT
//    BACK, and only a read-back that equals the written value counts as saved. Refused unless the launcher is connected
//    and not preparing. It changes no physical state (no check question afterwards).
//  * After any release / dispense the physical state is unknown: leaving maintenance asks the driver
//    the normal check question before anything moves.

#include <stdint.h>

#include "ble_devices.h"
#include "control_config.h"
#include "led_pattern.h"

// Switch indices; identical to SwitchId in switches.h (checked in controller.cpp).
constexpr uint8_t CSW_PANIC = 1;
constexpr uint8_t CSW_FOG = 2;
constexpr uint8_t CSW_FLASH = 3;
constexpr uint8_t CSW_AUTO = 4;
constexpr uint8_t CSW_DECOR = 5;
constexpr uint8_t CSW_SIREN = 6;
constexpr uint8_t CSW_LAUNCH = 7;

// What the unit believes about the physical launcher.
enum class Known : uint8_t { Unknown = 0, AtRest = 1, Armed = 2 };
// The driver's answer to the check question.
// The only thing nobody can know after a power-off is what is in the bucket (where the bucket is does not matter: a
// preparation always starts with "going down"). Empty: prepare incl. the dispenser cycle (write 0x01); Full: prepare
// without it (write 0x02); NotSure: nothing moves, the UI explains how to check.
enum class Answer : uint8_t { None, Empty, Full, NotSure };
enum class CheckReason : uint8_t { None, Startup, Error, Requested };

enum class LauncherStatus : uint8_t {
    Offline,     // not connected, or its state is not known yet
    NeedsCheck,  // waiting for the driver's answer
    Error,       // error: waiting for the driver's answer
    Rearming,    // launched, counting down to the next preparation
    Preparing,   // preparation running (or about to start)
    Ready        // armed: the launch button works
};

// Short messages (texts: ui_strings_en.h, <= 20 characters).
enum class Msg : uint8_t {
    None,
    LaunchSent,
    LaunchOffline,
    LaunchStateUnknown,
    LaunchNotReady,
    LaunchBusy,
    LaunchError,
    LaunchCheckFirst,
    LaunchFailed,
    PrepareRefused,
    PrepareTimeout,
    PrepareFailed,
    // maintenance
    MaintBlocksLaunch,
    MaintBusy,
    ReleaseOk,
    ReleaseFailed,
    DispenseFailed,
    DispenseStuck,
    MotorFailed,
    MotorStuck,
    StillStopping,  // a hold-to-run was pressed again (other direction) before the previous stop was confirmed
    DurationSaved,
    DurationFailed
};
const char* msg_text(Msg m);
bool msg_is_problem(Msg m);

enum class CmdTag : uint8_t { None, Launch, Prepare, Release, DispenseStart, MotorStart, DurationWrite, DurationRead, DurationVerify };  // commands whose result is reported back
enum class Result : uint8_t { None, Ok, Failed };

struct Command {
    uint8_t device;     // 0 = launcher, 1 = switchboard
    uint8_t charIndex;  // LauncherChar / SwitchboardChar
    uint8_t len;
    uint8_t data[4];
    CmdTag tag;
    bool read = false;  // true: read the characteristic instead of writing it (len = 0)
};

struct ControlInputs {
    uint32_t now = 0;
    uint32_t random = 0;  // fresh random bits every step (panic mode)

    bool switchesValid = false;
    bool sw[8] = {false, false, false, false, false, false, false, false};  // true = switch active
    bool launchPressed = false;   // SW8 went active since the previous step (edge)
    bool checkRequested = false;  // "Check launcher" chosen in the menu
    Answer answer = Answer::None; // the driver's answer to the check question

    bool maintenance = false;       // the UI is on a maintenance screen (level)
    bool releaseRequested = false;  // "Release bucket" confirmed (edge)
    int8_t dispenseHold = 0;        // the knob is held on "dispense" (+1) or "reverse" (-1) of the Dispense screen, else 0 (level)
    bool durationReadRequest = false;  // the dispenser time screen opened: read the current value (edge)
    bool durationSave = false;         // "save" pressed (edge) with durationValue seconds
    uint8_t durationValue = 0;
    int8_t motorHold = 0;           // the knob is held on "up" (+1) or "down" (-1) of Motor control, else 0 (level)

    bool launcherConnected = false;  // link fully up in this connection
    bool prepValid = false;          // preparation state read in this connection
    uint8_t prep = 0;                // 0 idle, 1 moving down, 2 up slowly, 3 up quickly, 4 error

    bool switchboardConnected = false;
    bool relaysValid = false;
    uint8_t relays[8] = {0, 0, 0, 0, 0, 0, 0, 0};  // 0 off, 1 on, 2 blink
    bool fogStateValid = false;
    uint8_t fogState = 0;  // 0 machine not connected, 1 heating, 2 ready, 3 fogging
    bool fogControlValid = false;
    uint8_t fogControl = 0;  // 1 = fogging

    bool dispenserValid = false;  // dispenser control read in this connection
    uint8_t dispenser = 0;        // 0 = stopped, anything else = dispensing
    bool durationValid = false;   // dispenser time read in this connection
    uint8_t duration = 0;
    bool motorValid = false;      // pull motor speed read in this connection
    int32_t motor = 0;            // commanded speed, -512..512, 0 = stopped

    Result launchResult = Result::None;   // outcome of the launch command sent earlier (reported once)
    Result prepareResult = Result::None;  // same for prepare
    Result releaseResult = Result::None;  // same for "Release bucket"
    Result dispenseResult = Result::None; // same for the dispense START write
    Result motorResult = Result::None;    // same for the motor START write
    Result durationWriteResult = Result::None;  // same for the dispenser time write
    Result durationReadResult = Result::None;   // same for the read-back that verifies it
};

// What a hold-to-run screen (Dispense, Motor control) shows.
enum class RunState : uint8_t { Idle, Starting, Running, Stopping };

struct ControlOutputs {
    Command cmds[8];
    uint8_t cmdCount = 0;
    led_state_t led[12];  // L1..L12; the logic drives L2, L4, L6, L7, L8 and L9 (the rest stays off)
    LauncherStatus launcher = LauncherStatus::Offline;
    Msg message = Msg::None;  // set for exactly one step when something worth telling happened

    bool needsConfirm = false;  // the check overlay must be shown now
    CheckReason reason = CheckReason::None;
    uint32_t rearmInMs = 0;     // time until the next automatic preparation (0 = none pending)
    Known known = Known::Unknown;

    // For the display (no LED belongs to the siren).
    bool launchPending = false;      // launch sent, its outcome not reported yet
    uint8_t launchOkCount = 0;       // grows by one whenever a launch write was reported OK (wraps)
    led_state_t sirenState = LED_OFF;  // same meaning as an LED: on = confirmed, flash = unconfirmed, fast = failing
    RunState dispense = RunState::Idle;
    RunState motor = RunState::Idle;
    bool durationBusy = false;  // saving the dispenser time (write + read-back) is in progress
};

class ControlLogic {
   public:
    ControlLogic();
    void step(const ControlInputs& in, ControlOutputs& out);

   private:
    void results(const ControlInputs& in, ControlOutputs& out);
    void trackLauncher(const ControlInputs& in, ControlOutputs& out);
    void handleAnswer(const ControlInputs& in);
    void handleLaunch(const ControlInputs& in, ControlOutputs& out);
    void autoPrepare(const ControlInputs& in, ControlOutputs& out);
    void handleMaintenance(const ControlInputs& in, ControlOutputs& out);
    void handleDuration(const ControlInputs& in, ControlOutputs& out);
    void addRead(ControlOutputs& out, uint8_t device, uint8_t ci, CmdTag tag);
    void addCommandBytes(ControlOutputs& out, uint8_t device, uint8_t ci, const uint8_t* data, uint8_t len, CmdTag tag);
    led_state_t reconcileRelay(const ControlInputs& in, ControlOutputs& out, uint8_t relay, bool want_on);
    led_state_t handleFog(const ControlInputs& in, ControlOutputs& out);
    led_state_t handlePanic(const ControlInputs& in, ControlOutputs& out);
    void addCommand(ControlOutputs& out, uint8_t device, uint8_t ci, uint8_t b0, uint8_t b1, uint8_t len, CmdTag tag);
    void askDriver(CheckReason why);

    // launcher
    Known known_ = Known::Unknown;
    bool needConfirm_ = false;
    CheckReason reason_ = CheckReason::None;
    bool errorAcked_ = false;  // the driver answered while the launcher still reports its error state
    bool prepRunning_ = false;
    uint32_t prepRunningSince_ = 0;
    bool prepTimedOut_ = false;
    bool prepareRequested_ = false;
    uint32_t prepareRequestedAt_ = 0;
    uint8_t prepFails_ = 0;
    bool launchPending_ = false;
    bool prepareWithDispenser_ = true;  // what the next automatic prepare asks for: 0x01 (incl. dispenser) or 0x02 (excl.)
    uint8_t launchOkCount_ = 0;
    bool rearmValid_ = false;
    uint32_t rearmAt_ = 0;
    bool rejectFlash_ = false;
    uint32_t rejectFlashAt_ = 0;

    // maintenance
    bool maintPrev_ = false;
    bool maintDirty_ = false;  // something was released / dispensed: the physical state is unknown
    // hold-to-run state of the dispenser and of the pull motor (same safe-stop rules, see runHold)
    struct HoldState {
        bool prev = false;       // held in the previous step
        bool owned = false;      // we started it and have not seen it stopped yet
        bool started = false;    // it reported "running"
        bool aborted = false;    // an interruption stopped it: only a new press runs it again
        bool startWarned = false;
        bool stopWarned = false;
        int8_t dir = 0;
        uint8_t stopSends = 0;
        uint32_t startAt = 0;
        uint32_t lastStopSend = 0;
        uint32_t stopSince = 0;
    };
    struct HoldSpec {
        uint8_t chr;
        CmdTag tag;
        Msg failed;
        Msg stuck;
        bool restartWhileStopping;  // false: a new press is refused until the stop is confirmed (a motor must not reverse on the fly)
    };
    RunState runHold(HoldState& h, const HoldSpec& sp, int8_t dir, bool valid, bool running, bool otherOwned,
                     const ControlInputs& in, ControlOutputs& out);
    // saving the dispenser time: write, then read back and compare
    enum class DurPhase : uint8_t { Idle, Writing, Verifying };
    DurPhase durPhase_ = DurPhase::Idle;
    uint8_t durWant_ = 0;
    uint32_t durAt_ = 0;
    HoldState dispenseHold_;
    HoldState motorHold_;

    // relays (index = relay - 1)
    bool relaySent_[8];
    uint32_t lastRelaySend_[8];
    bool pending_[8];
    uint32_t pendingSince_[8];

    // fog
    bool fogStartSent_ = false;
    uint32_t lastFogStart_ = 0;
    bool fogStopSent_ = false;
    uint32_t lastFogStop_ = 0;

    // panic
    bool panicActive_ = false;
    uint8_t panicCurrent_ = 0;  // relay that is on right now (0 = none)
    uint8_t panicLast_ = 0;
    uint32_t panicNextAt_ = 0;
    bool panicCleanup_ = false;
    uint32_t panicCleanupUntil_ = 0;
};
