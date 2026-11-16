#pragma once

// =============================================================================
// WHAT every switchboard device does in automatic mode (SW5) and in panic mode (SW2),
// and with which dedicated manual switch it is switched on. THIS is the file to edit when
// timings or the behavior of a device change. Plain constants only (host-testable).
//
// Priority (decided per device, every step):
//   1. its dedicated manual switch is ON            -> steady on, whatever SW2 / SW5 say
//   2. panic mode (SW2) is ON                       -> its "panic" behavior (also when that is Off)
//   3. automatic mode (SW5) is ON                   -> its "auto" behavior
//   4. otherwise                                    -> off
//
// A PATTERN is a cycle of PHASES that repeats forever. A phase has a duration drawn at random between
// minMs and maxMs (equal = fixed) and is either on or off. Examples:
//   "3 s on, 1 s off, each with up to 2 s extra at random":   {sec(3), sec(5), true}, {sec(1), sec(3), false}
//   "exactly 2 s on, then 2 s off":                           {sec(2), sec(2), true}, {sec(2), sec(2), false}
// With periodMs > 0 the cycle lasts EXACTLY that long: the LAST phase gets whatever is left of the period (its own
// minMs / maxMs are ignored; use {0, 0, false} for it), which gives "once every 10 seconds" kinds of patterns.
// To add a device: add an entry to DeviceId and to DEVICES (the relay numbers are in control_config.h).
// =============================================================================

#include <stdint.h>

#include "control_config.h"

constexpr uint32_t sec(uint32_t s) { return s * 1000u; }

struct Phase {
    uint32_t minMs;
    uint32_t maxMs;
    bool on;
};

struct Pattern {
    const Phase* phases;
    uint8_t count;
    uint32_t periodMs;  // 0 = free running; otherwise the last phase fills the period
};

enum class Mode : uint8_t { Off, On, Pattern };

struct Behaviour {
    Mode mode;
    const Pattern* pattern;  // only for Mode::Pattern
};

constexpr Behaviour BEH_OFF = {Mode::Off, nullptr};
constexpr Behaviour BEH_ON = {Mode::On, nullptr};

// ---- the patterns --------------------------------------------------------------------------------------------
// Red flashlight (auto): 3 s on, 1 s off, both with up to 2 s extra.
constexpr Phase PH_RED_AUTO[] = {{sec(3), sec(5), true}, {sec(1), sec(3), false}};
constexpr Pattern PAT_RED_AUTO = {PH_RED_AUTO, 2, 0};

// Orange flashlight outside right front (auto): 1 s off, 3 s on, both with up to 2 s extra.
constexpr Phase PH_ORANGE_RIGHT_AUTO[] = {{sec(1), sec(3), false}, {sec(3), sec(5), true}};
constexpr Pattern PAT_ORANGE_RIGHT_AUTO = {PH_ORANGE_RIGHT_AUTO, 2, 0};

// Orange flashlight outside left front (auto): 2 s on, 2 s off, 4 s on, 2 s off (fixed).
constexpr Phase PH_ORANGE_LEFT_AUTO[] = {{sec(2), sec(2), true}, {sec(2), sec(2), false}, {sec(4), sec(4), true}, {sec(2), sec(2), false}};
constexpr Pattern PAT_ORANGE_LEFT_AUTO = {PH_ORANGE_LEFT_AUTO, 4, 0};

// Illumination (auto): on for 30..90 s, then a short grid-failure disruption (off / on / off for a fraction of a second).
constexpr Phase PH_ILLUMINATION_AUTO[] = {{sec(30), sec(90), true}, {400, 1000, false}, {200, 500, true}, {300, 800, false}};
constexpr Pattern PAT_ILLUMINATION_AUTO = {PH_ILLUMINATION_AUTO, 4, 0};

// Illumination (panic): 1 s on, 1 s off.
constexpr Phase PH_ILLUMINATION_PANIC[] = {{sec(1), sec(1), true}, {sec(1), sec(1), false}};
constexpr Pattern PAT_ILLUMINATION_PANIC = {PH_ILLUMINATION_PANIC, 2, 0};

// Fog machine (auto): on for 1 s once per 2 minutes, at a random moment in those 2 minutes (the cycle is exactly 120 s).
constexpr Phase PH_FOG_AUTO[] = {{0, sec(119), false}, {sec(1), sec(1), true}, {0, 0, false}};
constexpr Pattern PAT_FOG_AUTO = {PH_FOG_AUTO, 3, sec(120)};

// Strobe light inside (panic): 2 s on, 1 s off
constexpr Phase PH_STROBE_PANIC[] = {{sec(2), sec(4), true}, {sec(1), sec(2), false}};
constexpr Pattern PAT_STROBE_PANIC = {PH_STROBE_PANIC, 2, 0};

// ---- the devices ---------------------------------------------------------------------------------------------
enum DeviceId : uint8_t {
    DEV_RED_FLASH,
    DEV_ORANGE_RIGHT,
    DEV_SIREN,
    DEV_ORANGE_LEFT,
    DEV_ILLUMINATION,
    DEV_AIRHORN,
    DEV_ORANGE_INSIDE,
    DEV_STROBE_INSIDE,
    DEV_FOG,
    DEVICE_COUNT
};

struct DeviceCfg {
    uint8_t relay;           // switchboard relay 1..8; 0 = the fog output
    int8_t manualSwitch;     // CSW_* of the dedicated manual switch, CSW_NONE = none
    Behaviour autoMode;      // SW5
    Behaviour panicMode;     // SW2
};

constexpr DeviceCfg DEVICES[DEVICE_COUNT] = {
    // relay                manual       automatic mode (SW5)                    panic mode (SW2)
    {RELAY_RED_FLASH, CSW_FLASH, {Mode::Pattern, &PAT_RED_AUTO}, BEH_ON},
    {RELAY_ORANGE_RIGHT, CSW_FLASH, {Mode::Pattern, &PAT_ORANGE_RIGHT_AUTO}, BEH_ON},
    {RELAY_SIREN, CSW_NONE, BEH_OFF, BEH_ON},
    {RELAY_ORANGE_LEFT, CSW_FLASH, {Mode::Pattern, &PAT_ORANGE_LEFT_AUTO}, BEH_ON},
    {RELAY_ILLUMINATION, CSW_DECOR, {Mode::Pattern, &PAT_ILLUMINATION_AUTO}, {Mode::Pattern, &PAT_ILLUMINATION_PANIC}},
    {RELAY_AIRHORN, CSW_HORN, BEH_OFF, BEH_OFF},
    {RELAY_ORANGE_INSIDE, CSW_NONE, BEH_OFF, BEH_ON},
    {RELAY_STROBE_INSIDE, CSW_NONE, BEH_OFF, {Mode::Pattern, &PAT_STROBE_PANIC}},
    {0, CSW_FOG, {Mode::Pattern, &PAT_FOG_AUTO}, BEH_ON},  // fog machine
};
