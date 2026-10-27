#pragma once

// Pure device identification from advertisement data (host-testable).
//
// Strength of evidence:
//   strong = the advertised service UUID matched. Preferred.
//   weak   = only the advertised name matched (names can be spoofed or reused).
// If name and service UUID disagree, or both service UUIDs are present, the
// device is NOT classified (conflict) rather than guessed.

#include <stdint.h>
#include <string.h>

#include "ble_config.h"

enum class DeviceKind : uint8_t { Unknown = 0, Launcher = 1, Switchboard = 2 };

struct Identity {
    DeviceKind kind;
    bool strong;
    bool conflict;
};

inline const char* device_kind_name(DeviceKind k) {
    switch (k) {
        case DeviceKind::Launcher: return "launcher";
        case DeviceKind::Switchboard: return "switchboard";
        default: return "unknown";
    }
}

inline DeviceKind device_kind_from_name(const char* name) {
    if (!name) return DeviceKind::Unknown;
    if (strcmp(name, LAUNCHER_ADV_NAME) == 0) return DeviceKind::Launcher;
    if (strcmp(name, SWITCHBOARD_ADV_NAME) == 0) return DeviceKind::Switchboard;
    return DeviceKind::Unknown;
}

inline Identity classify_advertisement(const char* name, bool has_launcher_uuid, bool has_switchboard_uuid) {
    if (has_launcher_uuid && has_switchboard_uuid) return {DeviceKind::Unknown, false, true};

    const DeviceKind name_kind = device_kind_from_name(name);
    DeviceKind uuid_kind = DeviceKind::Unknown;
    if (has_launcher_uuid) uuid_kind = DeviceKind::Launcher;
    else if (has_switchboard_uuid) uuid_kind = DeviceKind::Switchboard;

    if (uuid_kind != DeviceKind::Unknown) {
        if (name_kind != DeviceKind::Unknown && name_kind != uuid_kind) return {DeviceKind::Unknown, false, true};
        return {uuid_kind, true, false};
    }
    if (name_kind != DeviceKind::Unknown) return {name_kind, false, false};
    return {DeviceKind::Unknown, false, false};
}
