#include "controller.h"

#include <string.h>

#include "ble_link.h"
#include "config.h"
#include "leds.h"
#include "log.h"

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
bool prepare_requested = false;

// one-shot commands in flight (sequence numbers of ble_link_write) and results to report
uint32_t launch_seq = 0;
uint32_t prepare_seq = 0;
Result launch_result = Result::None;
Result prepare_result = Result::None;

// latest message and launcher status for the UI
Msg last_msg = Msg::None;
uint32_t last_msg_at = 0;
uint8_t last_prep = 0;
LauncherStatus last_status = LauncherStatus::Offline;

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
    const uint32_t seq = ble_link_write(kind, c.charIndex, c.data, c.len);

    uint8_t count = 0;
    const CharSpec* specs = ble_char_specs(kind, count);
    LOGI(TAG, "-> %s %s [%u%s%u]%s", device_kind_name(kind), c.charIndex < count ? specs[c.charIndex].name : "?",
         (unsigned)c.data[0], c.len > 1 ? " " : "", c.len > 1 ? (unsigned)c.data[1] : 0u,
         seq == 0 ? "  NOT SENT (link down)" : "");

    // One-shot commands are never retried: a command that could not even be queued has failed.
    if (c.tag == CmdTag::Launch) {
        if (seq == 0) launch_result = Result::Failed; else launch_seq = seq;
    } else if (c.tag == CmdTag::Prepare) {
        if (seq == 0) prepare_result = Result::Failed; else prepare_seq = seq;
    }
}

}  // namespace

void controller_init() {}

void controller_on_switch(const SwitchEvent& e) {
    if (e.id == SW8_LAUNCH && e.active) launch_pressed = true;
    // Not defined yet (see README TODO): visible in the log so a flipped switch is never "silently" ignored.
    if ((e.id == SW2_PANIC || e.id == SW5_AUTO) && e.active) {
        LOGW(TAG, "%s: behavior not defined yet, ignored", switches_name(e.id));
    }
}

void controller_request_prepare() { prepare_requested = true; }

void controller_update(uint32_t now) {
    ControlInputs in;
    in.now = now;
    in.switchesValid = switches_valid();
    for (uint8_t i = 0; i < 8; i++) in.sw[i] = switches_is_active(static_cast<SwitchId>(i));
    in.launchPressed = launch_pressed;
    in.prepareRequested = prepare_requested;
    launch_pressed = false;
    prepare_requested = false;

    uint8_t v[LINK_VALUE_MAX];
    uint8_t len = 0;

    in.launcherConnected = ble_link_state(DeviceKind::Launcher) == LinkState::Connected;
    in.prepValid = read_value(DeviceKind::Launcher, LCH_PREPARATION_STATE, v, sizeof(v), &len);
    if (in.prepValid) in.prep = v[0];

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

    logic.step(in, outs);

    for (uint8_t i = 0; i < outs.cmdCount; i++) execute(outs.cmds[i]);

    leds_set_state(LED_L2_READY_TO_LAUNCH, outs.led[1]);
    leds_set_state(LED_L4_DECORATIVE_LIGHTING, outs.led[3]);
    leds_set_state(LED_L7_FLASHING_LIGHT, outs.led[6]);
    leds_set_state(LED_L8_FOG_STATUS, outs.led[7]);

    last_prep = in.prep;
    last_status = outs.launcher;
    if (outs.message != Msg::None) {
        last_msg = outs.message;
        last_msg_at = now;
        if (msg_is_problem(outs.message)) LOGW(TAG, "%s", msg_text(outs.message));
        else LOGI(TAG, "%s", msg_text(outs.message));
    }
}

LauncherStatus controller_launcher_status() { return last_status; }

void controller_launcher_text(char* out, size_t cap) {
    if (!out || cap == 0) return;
    const char* t = "";
    switch (last_status) {
        case LauncherStatus::Offline: t = "launcher offline"; break;
        case LauncherStatus::Idle: t = "idle, not prepared"; break;
        case LauncherStatus::Ready: t = "READY"; break;
        case LauncherStatus::Error: t = "launcher error"; break;
        case LauncherStatus::Launched: t = "launched"; break;
        case LauncherStatus::Preparing:
            t = last_prep == 1 ? "preparing: down" : (last_prep == 2 ? "preparing: up slow" : "preparing: up fast");
            break;
    }
    strncpy(out, t, cap - 1);
    out[cap - 1] = '\0';
}

bool controller_message(char* out, size_t cap, bool* problem) {
    if (last_msg == Msg::None || static_cast<uint32_t>(millis() - last_msg_at) >= MESSAGE_SHOW_MS) return false;
    if (out && cap) {
        strncpy(out, msg_text(last_msg), cap - 1);
        out[cap - 1] = '\0';
    }
    if (problem) *problem = msg_is_problem(last_msg);
    return true;
}
