#pragma once

// What the control unit needs from each remote device: service UUID and the
// characteristics it uses. This is the ONE place to extend when the UI or the
// control logic needs more of a device's state.
//
// UUID sources: launcher = launcher include/config.h; switchboard = switchboard
// README characteristic table. `required` characteristics must exist, otherwise
// the link is reported as Error (wrong device or incompatible firmware).
//
// Payload formats (big-endian, see the device READMEs):
//   launcher PREPARATION_STATE: 0 idle, 1 moving down, 2 up slowly, 3 up quickly, 4 error
//   switchboard FOG_STATE: 0 not connected, 1 heating, 2 ready, 3 fogging

#include <stddef.h>
#include <stdint.h>

#include "ble_identity.h"

struct CharSpec {
    const char* name;
    const char* uuid;
    bool required;    // must exist
    bool read;        // read the current value right after connecting
    bool subscribe;   // subscribe to notifications
    bool write;       // must be writable (commands are sent in a later stage)
};

constexpr uint8_t LINK_MAX_CHARS = 8;
constexpr uint8_t LINK_VALUE_MAX = 16;  // bytes kept per characteristic

enum LauncherChar : uint8_t {
    LCH_PREPARATION_STATE = 0,
    LCH_DISPENSER_CONTROL,
    LCH_PREPARE,
    LCH_LAUNCH,
    LCH_COUNT
};

enum SwitchboardChar : uint8_t {
    SCH_RELAYS = 0,
    SCH_MOSFETS,
    SCH_FOG_STATE,
    SCH_FOG_CONTROL,
    SCH_FOG_DURATION,
    SCH_DIGITAL_OUTPUTS,
    SCH_COUNT
};

extern const CharSpec LAUNCHER_CHARS[LCH_COUNT];
extern const CharSpec SWITCHBOARD_CHARS[SCH_COUNT];

const CharSpec* ble_char_specs(DeviceKind kind, uint8_t& count);
const char* ble_service_uuid(DeviceKind kind);
