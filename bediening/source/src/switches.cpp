#include "switches.h"

#include "config.h"
#include "debounce.h"
#include "event_queue.h"
#include "log.h"
#include "mcp23017.h"

static const char* TAG = "SW";

static const char* const kNames[SWITCH_COUNT] = {
    "SW1 Back", "SW2 Panic", "SW3 Fog", "SW4 Flash", "SW5 Auto", "SW6 Decor", "SW7 Siren", "SW8 Launch",
};
// Momentary: SW1, SW7, SW8. Maintained: SW2..SW6.
static const bool kMomentary[SWITCH_COUNT] = {true, false, false, false, false, false, true, true};

static Debouncer debouncers[SWITCH_COUNT];
static EventQueue<SwitchEvent, 16> events;
static bool have_first_read = false;  // debouncers seeded from real pin levels
static bool valid = false;            // last scan succeeded
static uint32_t last_scan_ms = 0;
static uint32_t reported_drops = 0;

static inline bool pin_active(uint8_t raw, uint8_t bit) {
    const bool high = (raw >> bit) & 1;
    return SWITCH_ACTIVE_LOW ? !high : high;
}

void switches_init() {
    have_first_read = false;
    valid = false;
    last_scan_ms = 0;
    switches_update(millis());  // may fail if the expander is absent; handled in update
}

void switches_update(uint32_t now) {
    if (have_first_read && static_cast<uint32_t>(now - last_scan_ms) < SWITCH_SCAN_INTERVAL_MS) return;
    last_scan_ms = now;

    uint8_t raw = 0;
    if (!mcp.isReady() || !mcp.readPortB(raw)) {
        if (valid) {
            valid = false;
            LOGW(TAG, "switch state UNKNOWN (expander not responding)");
        }
        return;  // never feed garbage into the debouncers
    }

    if (!have_first_read) {
        // Seed from the real levels so a switch that is already ON at boot
        // does not produce a spurious event.
        for (uint8_t i = 0; i < SWITCH_COUNT; i++) debouncers[i].reset(pin_active(raw, i), now);
        have_first_read = true;
        valid = true;
        LOGI(TAG, "initial switch state (raw GPB=0x%02X):", raw);
        for (uint8_t i = 0; i < SWITCH_COUNT; i++) {
            LOGI(TAG, "  %-10s %s", kNames[i], debouncers[i].state() ? "ACTIVE (closed)" : "inactive (open)");
        }
        return;
    }

    if (!valid) {
        valid = true;
        LOGI(TAG, "switch state valid again");
        // No reset: any change that happened during the outage is reported
        // as a normal debounced event below.
    }

    for (uint8_t i = 0; i < SWITCH_COUNT; i++) {
        if (debouncers[i].update(pin_active(raw, i), now, SWITCH_DEBOUNCE_MS)) {
            SwitchEvent e{static_cast<SwitchId>(i), debouncers[i].state(), now};
            events.push(e);
        }
    }

    if (events.dropped() != reported_drops) {
        reported_drops = events.dropped();
        LOGW(TAG, "event queue overflow, %lu events dropped in total", (unsigned long)reported_drops);
    }
}

bool switches_is_active(SwitchId id) { return id < SWITCH_COUNT && debouncers[id].state(); }
bool switches_valid() { return valid; }
bool switches_poll_event(SwitchEvent& out) { return events.pop(out); }
const char* switches_name(SwitchId id) { return id < SWITCH_COUNT ? kNames[id] : "?"; }
bool switches_is_momentary(SwitchId id) { return id < SWITCH_COUNT && kMomentary[id]; }
