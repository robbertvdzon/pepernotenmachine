#include "controller.h"

#include <stdio.h>
#include <string.h>

#include "ble_link.h"
#include "config.h"
#include "device_store.h"
#include "leds.h"
#include "log.h"
#include "ui_strings.h"

static const char* TAG = "CTRL";

// The logic indexes switches by number: it must match SwitchId.
static_assert(SW2_PANIC == CSW_PANIC && SW3_FOG == CSW_FOG && SW4_FLASH == CSW_FLASH && SW5_AUTO == CSW_AUTO &&
                  SW6_DECOR == CSW_DECOR && SW7_SIREN == CSW_SIREN && SW8_LAUNCH == CSW_LAUNCH,
              "control_logic.h switch indices differ from SwitchId");

namespace {

ControlLogic logic;
ControlOutputs outs;

// requests collected between two updates
bool launch_pressed = false;
bool check_requested = false;
Answer pending_answer = Answer::None;
bool maintenance = false;
bool release_requested = false;
int8_t dispense_hold = 0;
int8_t motor_hold = 0;

// one-shot commands in flight (sequence numbers of ble_link_write) and results to report
uint32_t launch_seq = 0;
uint32_t prepare_seq = 0;
uint32_t release_seq = 0;
uint32_t dispense_seq = 0;
uint32_t motor_seq = 0;
uint32_t duration_write_seq = 0;
uint32_t duration_verify_seq = 0;
Result launch_result = Result::None;
Result prepare_result = Result::None;
Result release_result = Result::None;
Result dispense_result = Result::None;
Result motor_result = Result::None;
Result duration_write_result = Result::None;
Result duration_verify_result = Result::None;
bool duration_read_request = false;
bool duration_save_request = false;
uint8_t duration_save_value = 0;
bool last_duration_valid = false;
uint8_t last_duration = 0;
bool last_duration_busy = false;
RunState last_dispense = RunState::Idle;
RunState last_motor = RunState::Idle;

// latest message and launcher status for the UI
Msg last_msg = Msg::None;
uint32_t last_msg_at = 0;
uint8_t last_prep = 0;
bool last_prep_valid = false;
bool last_sb_connected = false;
bool last_dispenser_valid = false;
bool last_dispenser_on = false;
bool last_switch_valid = false;
bool last_switch_pressed = false;
LauncherStatus last_status = LauncherStatus::Offline;
bool last_needs_confirm = false;
CheckReason last_reason = CheckReason::None;
uint32_t last_rearm_ms = 0;

Known stored_known = Known::Unknown;  // loaded at start-up: a hint only
Known saved_known = Known::Unknown;   // what is in NVS right now

bool read_value(DeviceKind kind, uint8_t ci, uint8_t* out, uint8_t cap, uint8_t* len) {
    return ble_link_get_value(kind, ci, out, cap, len, nullptr) && *len >= 1;
}

// Result of a one-shot write: reported exactly once. A lost result counts as failed.
Result poll_result(uint32_t& seq) {
    if (seq == 0) return Result::None;
    switch (ble_link_write_state(seq)) {
        case WriteState::Pending: return Result::None;
        case WriteState::Ok: seq = 0; return Result::Ok;
        default: seq = 0; return Result::Failed;
    }
}

void execute(const Command& c) {
    const DeviceKind kind = c.device == 0 ? DeviceKind::Launcher : DeviceKind::Switchboard;
    const uint32_t seq = c.read ? ble_link_read(kind, c.charIndex) : ble_link_write(kind, c.charIndex, c.data, c.len);

    uint8_t count = 0;
    const CharSpec* specs = ble_char_specs(kind, count);
    if (c.read) {
        LOGI(TAG, "-> %s read %s%s", device_kind_name(kind), c.charIndex < count ? specs[c.charIndex].name : "?",
             seq == 0 ? "  NOT SENT (link down)" : "");
    } else {
        LOGI(TAG, "-> %s %s [%u%s%u]%s", device_kind_name(kind), c.charIndex < count ? specs[c.charIndex].name : "?",
             (unsigned)c.data[0], c.len > 1 ? " " : "", c.len > 1 ? (unsigned)c.data[1] : 0u,
             seq == 0 ? "  NOT SENT (link down)" : "");
    }

    // One-shot commands are never retried: a command that could not even be queued has failed.
    if (c.tag == CmdTag::Launch) {
        if (seq == 0) launch_result = Result::Failed; else launch_seq = seq;
    } else if (c.tag == CmdTag::Prepare) {
        if (seq == 0) prepare_result = Result::Failed; else prepare_seq = seq;
    } else if (c.tag == CmdTag::Release) {
        if (seq == 0) release_result = Result::Failed; else release_seq = seq;
    } else if (c.tag == CmdTag::MotorStart) {
        if (seq == 0) motor_result = Result::Failed; else motor_seq = seq;
    } else if (c.tag == CmdTag::DispenseStart) {
        if (seq == 0) dispense_result = Result::Failed; else dispense_seq = seq;  // (the stop is not one-shot: it is repeated)
    } else if (c.tag == CmdTag::DurationWrite) {
        if (seq == 0) duration_write_result = Result::Failed; else duration_write_seq = seq;
    } else if (c.tag == CmdTag::DurationVerify) {
        if (seq == 0) duration_verify_result = Result::Failed; else duration_verify_seq = seq;
    }  // (DurationRead, a plain refresh, has no result to follow)
}

}  // namespace

void controller_init() {
    stored_known = static_cast<Known>(launcher_memory_load());
    saved_known = stored_known;
    LOGI(TAG, "last known launcher state: %s (hint only, the driver is always asked)",
         stored_known == Known::Armed ? "armed" : (stored_known == Known::AtRest ? "at rest" : "unknown"));
}

void controller_on_switch(const SwitchEvent& e) {
    if (e.id == SW8_LAUNCH && e.active) launch_pressed = true;
    // Not defined yet (see README TODO): visible in the log so a flipped switch is never "silently" ignored.
    if (e.id == SW5_AUTO && e.active) LOGW(TAG, "%s: behavior not defined yet, ignored", switches_name(e.id));
}

void controller_request_check() { check_requested = true; }

void controller_set_maintenance(bool on) { maintenance = on; }
void controller_request_release() { release_requested = true; }
void controller_set_dispense_hold(int8_t dir) { dispense_hold = dir > 0 ? 1 : (dir < 0 ? -1 : 0); }
RunState controller_dispense_state() { return last_dispense; }
void controller_set_motor_hold(int8_t dir) { motor_hold = dir > 0 ? 1 : (dir < 0 ? -1 : 0); }
RunState controller_motor_state() { return last_motor; }
void controller_request_duration_read() { duration_read_request = true; }
void controller_request_duration_save(uint8_t seconds) {
    duration_save_request = true;
    duration_save_value = seconds;
}
bool controller_duration(uint8_t* seconds) {
    if (seconds) *seconds = last_duration;
    return last_duration_valid;
}
bool controller_duration_busy() { return last_duration_busy; }
bool controller_bucket_switch(bool* pressed) {
    if (pressed) *pressed = last_switch_pressed;
    return last_switch_valid;
}
Msg controller_last_message(uint32_t* at) {
    if (at) *at = last_msg_at;
    return last_msg;
}

void controller_answer(Answer a) {
    if (a != Answer::None) pending_answer = a;
}

void controller_update(uint32_t now) {
    ControlInputs in;
    in.now = now;
    in.random = static_cast<uint32_t>(random(0x7FFFFFFF)) ^ (static_cast<uint32_t>(random(0x7FFFFFFF)) << 1);
    in.switchesValid = switches_valid();
    for (uint8_t i = 0; i < 8; i++) in.sw[i] = switches_is_active(static_cast<SwitchId>(i));
    in.launchPressed = launch_pressed;
    in.checkRequested = check_requested;
    in.answer = pending_answer;
    in.maintenance = maintenance;
    in.releaseRequested = release_requested;
    in.dispenseHold = dispense_hold;
    in.motorHold = motor_hold;
    in.durationReadRequest = duration_read_request;
    in.durationSave = duration_save_request;
    in.durationValue = duration_save_value;
    duration_read_request = false;
    duration_save_request = false;
    launch_pressed = false;
    check_requested = false;
    release_requested = false;
    pending_answer = Answer::None;

    uint8_t v[LINK_VALUE_MAX];
    uint8_t len = 0;

    in.launcherConnected = ble_link_state(DeviceKind::Launcher) == LinkState::Connected;
    in.prepValid = read_value(DeviceKind::Launcher, LCH_PREPARATION_STATE, v, sizeof(v), &len);
    if (in.prepValid) in.prep = v[0];

    // dispenser (optional characteristic): 0x00 = stopped, anything else = dispensing.
    in.dispenserValid = read_value(DeviceKind::Launcher, LCH_DISPENSER_CONTROL, v, sizeof(v), &len);
    if (in.dispenserValid) in.dispenser = v[0];
    last_dispenser_valid = in.dispenserValid;
    last_dispenser_on = in.dispenserValid && v[0] != 0;

    // switch at the bottom of the launcher (optional, display only): 0x01 = pressed
    last_switch_valid = ble_link_get_value(DeviceKind::Launcher, LCH_BUCKET_SWITCH, v, sizeof(v), &len, nullptr) && len >= 1;
    last_switch_pressed = last_switch_valid && v[0] != 0;

    // dispenser time (optional, one byte)
    in.durationValid = ble_link_get_value(DeviceKind::Launcher, LCH_DISPENSER_DURATION, v, sizeof(v), &len, nullptr) && len >= 1;
    if (in.durationValid) in.duration = v[0];
    last_duration_valid = in.durationValid;
    last_duration = in.duration;

    // pull motor (optional): signed 32-bit big-endian commanded speed, 0 = stopped
    in.motorValid = ble_link_get_value(DeviceKind::Launcher, LCH_PULL_MOTOR, v, sizeof(v), &len, nullptr) && len >= 4;
    if (in.motorValid) {
        in.motor = static_cast<int32_t>((static_cast<uint32_t>(v[0]) << 24) | (static_cast<uint32_t>(v[1]) << 16) |
                                        (static_cast<uint32_t>(v[2]) << 8) | v[3]);
    }

    in.switchboardConnected = ble_link_state(DeviceKind::Switchboard) == LinkState::Connected;
    in.relaysValid = ble_link_get_value(DeviceKind::Switchboard, SCH_RELAYS, v, sizeof(v), &len, nullptr) && len >= 8;
    if (in.relaysValid) memcpy(in.relays, v, 8);
    in.fogStateValid = read_value(DeviceKind::Switchboard, SCH_FOG_STATE, v, sizeof(v), &len);
    if (in.fogStateValid) in.fogState = v[0];
    in.fogControlValid = read_value(DeviceKind::Switchboard, SCH_FOG_CONTROL, v, sizeof(v), &len);
    if (in.fogControlValid) in.fogControl = v[0];

    // outcome of one-shot commands sent earlier (each reported once)
    const Result lr = poll_result(launch_seq);
    const Result pr = poll_result(prepare_seq);
    in.launchResult = lr != Result::None ? lr : launch_result;
    in.prepareResult = pr != Result::None ? pr : prepare_result;
    launch_result = Result::None;
    prepare_result = Result::None;
    const Result rr = poll_result(release_seq);
    const Result dr = poll_result(dispense_seq);
    const Result mr = poll_result(motor_seq);
    const Result dwr = poll_result(duration_write_seq);
    const Result dvr = poll_result(duration_verify_seq);
    in.releaseResult = rr != Result::None ? rr : release_result;
    in.dispenseResult = dr != Result::None ? dr : dispense_result;
    in.motorResult = mr != Result::None ? mr : motor_result;
    release_result = Result::None;
    dispense_result = Result::None;
    motor_result = Result::None;
    in.durationWriteResult = dwr != Result::None ? dwr : duration_write_result;
    in.durationReadResult = dvr != Result::None ? dvr : duration_verify_result;
    duration_write_result = Result::None;
    duration_verify_result = Result::None;

    logic.step(in, outs);
    last_dispense = outs.dispense;
    last_motor = outs.motor;
    last_duration_busy = outs.durationBusy;

    for (uint8_t i = 0; i < outs.cmdCount; i++) execute(outs.cmds[i]);

    leds_set_state(LED_L2_READY_TO_LAUNCH, outs.led[1]);
    leds_set_state(LED_L4_DECORATIVE_LIGHTING, outs.led[3]);
    leds_set_state(LED_L6_PANIC_MODE, outs.led[5]);
    leds_set_state(LED_L7_FLASHING_LIGHT, outs.led[6]);
    leds_set_state(LED_L8_FOG_STATUS, outs.led[7]);
    leds_set_state(LED_L9_PREPARING, outs.led[8]);

    // Remember what the unit believes (only the two real states), as a hint for the next power-up.
    if (outs.known != Known::Unknown && outs.known != saved_known) {
        saved_known = outs.known;
        launcher_memory_save(static_cast<uint8_t>(saved_known));
    }

    last_prep = in.prep;
    last_prep_valid = in.prepValid;
    last_sb_connected = in.switchboardConnected;
    last_status = outs.launcher;
    last_needs_confirm = outs.needsConfirm;
    last_reason = outs.reason;
    last_rearm_ms = outs.rearmInMs;
    if (outs.message != Msg::None) {
        last_msg = outs.message;
        last_msg_at = now;
        if (msg_is_problem(outs.message)) LOGW(TAG, "%s", msg_text(outs.message));
        else LOGI(TAG, "%s", msg_text(outs.message));
    }
}

bool controller_needs_confirm(CheckReason* reason) {
    // After an answer the overlay must not come back before the logic has processed it.
    const bool needed = last_needs_confirm && pending_answer == Answer::None;
    if (reason) *reason = needed ? last_reason : CheckReason::None;
    return needed;
}

Known controller_stored_state() { return stored_known; }

void controller_home_inputs(HomeInputs* h) {
    if (!h) return;
    h->launcher = last_status;
    h->prepValid = last_prep_valid;
    h->prep = last_prep;
    h->rearmInMs = last_rearm_ms;
    h->launchPending = outs.launchPending;
    h->launchOkCount = outs.launchOkCount;
    h->switchboardConnected = last_sb_connected;
    h->fogLed = outs.led[7];    // L8
    h->panicLed = outs.led[5];  // L6
    h->flashLed = outs.led[6];  // L7
    h->lampLed = outs.led[3];   // L4
    h->sirenLed = outs.sirenState;
    h->dispenserValid = last_dispenser_valid;
    h->dispenserOn = last_dispenser_on;
    h->message = last_msg;
    h->messageAt = last_msg_at;
}
