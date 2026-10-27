#pragma once

// Pure control logic (no hardware, no BLE, no Arduino): turns the switch states, the
// device states and the operator's requests into device commands, LED states and
// messages. Called once per main-loop pass; all timing comes from `ControlInputs::now`.
//
// SAFETY RULES implemented here (documented in README.md):
//  * LAUNCH is a one-shot. It is sent at most once per button press, never retried and
//    never replayed on reconnect. It needs a connected launcher, a CURRENT preparation
//    state (valid in this connection), state idle, and a completed preparation that this
//    unit itself watched finish during the current connection and no launch since.
//    After a reconnect or restart the launcher is therefore "not prepared" until it is
//    prepared again. A rejected launch sends nothing and says why.
//  * Relays / fog are idempotent STATE settings. The switch position is the truth: they
//    are compared with the state the switchboard reports and re-sent until it matches,
//    which also re-applies them after a reconnect (the switchboard turns everything off
//    when its last client disconnects).
//  * Unknown switch state => fog is stopped and nothing else is changed.
//  * Panic (SW2) and automatic mode (SW5) are not defined yet and do nothing.

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

enum class LauncherStatus : uint8_t { Offline, Idle, Preparing, Ready, Error, Launched };

// Short operator messages (texts <= 20 characters, see msg_text()).
enum class Msg : uint8_t {
    None,
    LaunchSent,
    LaunchOffline,
    LaunchStateUnknown,
    LaunchNotPrepared,
    LaunchBusy,
    LaunchError,
    LaunchLockout,
    LaunchFailed,
    PrepareSent,
    PrepareOffline,
    PrepareStateUnknown,
    PrepareBusy,
    PrepareLockout,
    PrepareRefused,
    PrepareTimeout,
    PrepareFailed
};
const char* msg_text(Msg m);
bool msg_is_problem(Msg m);

enum class CmdTag : uint8_t { None, Launch, Prepare };  // one-shot commands whose result is reported back
enum class Result : uint8_t { None, Ok, Failed };

struct Command {
    uint8_t device;     // 0 = launcher, 1 = switchboard
    uint8_t charIndex;  // LauncherChar / SwitchboardChar
    uint8_t len;
    uint8_t data[4];
    CmdTag tag;
};

struct ControlInputs {
    uint32_t now = 0;

    bool switchesValid = false;
    bool sw[8] = {false, false, false, false, false, false, false, false};  // true = switch active
    bool launchPressed = false;    // SW8 went active since the previous step (edge)
    bool prepareRequested = false; // operator picked "Prepare launcher" since the previous step

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

    Result launchResult = Result::None;   // outcome of the launch command sent earlier (reported once)
    Result prepareResult = Result::None;  // same for prepare
};

struct ControlOutputs {
    Command cmds[6];
    uint8_t cmdCount = 0;
    led_state_t led[8];  // L1..L8; the logic drives L2, L4, L7, L8 (the rest is not its business)
    LauncherStatus launcher = LauncherStatus::Offline;
    Msg message = Msg::None;  // set for exactly one step when something worth telling happened
};

class ControlLogic {
   public:
    ControlLogic();
    void step(const ControlInputs& in, ControlOutputs& out);

   private:
    void trackLauncher(const ControlInputs& in, ControlOutputs& out);
    void handleLaunch(const ControlInputs& in, ControlOutputs& out);
    void handlePrepare(const ControlInputs& in, ControlOutputs& out);
    led_state_t reconcileRelay(const ControlInputs& in, ControlOutputs& out, uint8_t relay, bool want_on);
    led_state_t handleFog(const ControlInputs& in, ControlOutputs& out);
    void addCommand(ControlOutputs& out, uint8_t device, uint8_t ci, uint8_t b0, uint8_t b1, uint8_t len, CmdTag tag);
    bool lockedOut(uint32_t now) const;

    // launcher
    bool prepared_ = false;
    bool prevKnown_ = false;
    bool prepRunning_ = false;
    uint32_t prepRunningSince_ = 0;
    bool prepTimedOut_ = false;
    bool prepareRequested_ = false;
    uint32_t prepareRequestedAt_ = 0;
    bool launched_ = false;
    uint32_t launchedAt_ = 0;
    bool rejectFlash_ = false;
    uint32_t rejectFlashAt_ = 0;

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
};
