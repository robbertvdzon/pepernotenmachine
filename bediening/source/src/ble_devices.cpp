#include "ble_devices.h"

#include "ble_config.h"

//                                      name                uuid                                    req    read   sub    write
const CharSpec LAUNCHER_CHARS[LCH_COUNT] = {
    {"PREPARATION_STATE", "beb5483e-36e1-4688-b7f5-ea07361b26b1", true,  true,  true,  false},
    {"DISPENSER_CONTROL", "beb5483e-36e1-4688-b7f5-ea07361b26b0", false, true,  true,  false},
    {"PREPARE",           "beb5483e-36e1-4688-b7f5-ea07361b26ae", true,  false, false, true},
    {"LAUNCH",            "beb5483e-36e1-4688-b7f5-ea07361b26b2", true,  false, false, true},
};

const CharSpec SWITCHBOARD_CHARS[SCH_COUNT] = {
    {"RELAYS",          "3dc93701-33d3-4883-ae7b-877c14fd3eca", true,  true,  true,  true},
    {"MOSFETS",         "3dc93702-33d3-4883-ae7b-877c14fd3eca", false, true,  true,  false},
    {"FOG_STATE",       "3dc93703-33d3-4883-ae7b-877c14fd3eca", true,  true,  true,  false},
    {"FOG_CONTROL",     "3dc93704-33d3-4883-ae7b-877c14fd3eca", true,  true,  true,  true},
    {"FOG_DURATION",    "3dc93705-33d3-4883-ae7b-877c14fd3eca", false, true,  false, false},
    {"DIGITAL_OUTPUTS", "3dc93708-33d3-4883-ae7b-877c14fd3eca", false, true,  true,  false},
};

const CharSpec* ble_char_specs(DeviceKind kind, uint8_t& count) {
    switch (kind) {
        case DeviceKind::Launcher:
            count = LCH_COUNT;
            return LAUNCHER_CHARS;
        case DeviceKind::Switchboard:
            count = SCH_COUNT;
            return SWITCHBOARD_CHARS;
        default:
            count = 0;
            return nullptr;
    }
}

const char* ble_service_uuid(DeviceKind kind) {
    switch (kind) {
        case DeviceKind::Launcher: return LAUNCHER_SERVICE_UUID;
        case DeviceKind::Switchboard: return SWITCHBOARD_SERVICE_UUID;
        default: return "";
    }
}
