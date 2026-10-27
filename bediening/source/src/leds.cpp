#include "leds.h"

#include "config.h"
#include "log.h"
#include "mcp23017.h"

static const char* TAG = "LED";

// Desired state per LED. Single-byte writes are atomic, so the main loop can
// set them while the LED task reads them without a lock.
static volatile uint8_t states[LED_COUNT];
static bool task_started = false;

// ---- output stage (only ever called from the LED task) ----------------------
static uint32_t seen_generation = 0;
static bool mcp_written = false;       // last_latch is valid
static uint8_t last_latch = 0;
static uint8_t last_direct = 0xFF;     // 0xFF = unknown, forces a write

// `on_mask` bit n = LED n+1 lit.
static void write_outputs(uint16_t on_mask) {
    // Expander (L1..L8). Rewrite after it was re-initialised (it comes back all-off).
    if (mcp.configGeneration() != seen_generation) {
        seen_generation = mcp.configGeneration();
        mcp_written = false;
    }
    const uint8_t low = static_cast<uint8_t>(on_mask & 0xFF);
    const uint8_t latch = LED_ACTIVE_LOW ? static_cast<uint8_t>(~low) : low;
    if (mcp.isReady()) {
        if (!mcp_written || latch != last_latch) {
            if (mcp.writePortA(latch)) {
                last_latch = latch;
                mcp_written = true;
            } else {
                mcp_written = false;  // driver logged the error; main loop re-inits the chip
            }
        }
    } else {
        mcp_written = false;
    }

    // Direct pins (L9..L12).
    const uint8_t direct = static_cast<uint8_t>((on_mask >> LED_COUNT_MCP) & 0x0F);
    if (direct != last_direct) {
        for (uint8_t k = 0; k < 4; k++) {
            const bool on = (direct >> k) & 1;
            digitalWrite(LED_DIRECT_PINS[k], (on == LED_ACTIVE_LOW) ? LOW : HIGH);
        }
        last_direct = direct;
    }
}

static void led_task(void*) {
    const TickType_t tick_1ms = pdMS_TO_TICKS(1) > 0 ? pdMS_TO_TICKS(1) : 1;
    const TickType_t tick_idle = pdMS_TO_TICKS(10) > 0 ? pdMS_TO_TICKS(10) : 1;
    TickType_t last_wake = xTaskGetTickCount();

    for (;;) {
        const uint32_t now = millis();
        uint8_t level[LED_COUNT];
        bool fading = false;
        for (uint8_t i = 0; i < LED_COUNT; i++) {
            const led_state_t st = static_cast<led_state_t>(states[i]);
            level[i] = led_brightness_to_level(led_pattern_brightness(st, now));
            if (led_state_is_fading(st)) fading = true;
        }

        if (!fading) {
            // Only OFF / ON / FLASH states: every LED is fully on or off. No PWM
            // needed, so no I2C traffic unless something changes.
            uint16_t mask = 0;
            for (uint8_t i = 0; i < LED_COUNT; i++)
                if (level[i] == LED_PWM_LEVELS) mask |= static_cast<uint16_t>(1u << i);
            write_outputs(mask);
            vTaskDelayUntil(&last_wake, tick_idle);
            continue;
        }

        // One PWM cycle = LED_PWM_LEVELS slots of 1 ms. Writes only happen when
        // the pattern changes between slots, so the bus stays mostly idle.
        for (uint8_t slot = 0; slot < LED_PWM_LEVELS; slot++) {
            uint16_t mask = 0;
            for (uint8_t i = 0; i < LED_COUNT; i++)
                if (slot < level[i]) mask |= static_cast<uint16_t>(1u << i);
            write_outputs(mask);
            vTaskDelayUntil(&last_wake, tick_1ms);
        }
    }
}

void leds_init() {
    for (uint8_t i = 0; i < LED_COUNT; i++) states[i] = LED_OFF;
    for (uint8_t k = 0; k < 4; k++) {
        // Set the OFF level before enabling the output to avoid a flash.
        digitalWrite(LED_DIRECT_PINS[k], LED_ACTIVE_LOW ? HIGH : LOW);
        pinMode(LED_DIRECT_PINS[k], OUTPUT);
    }
    seen_generation = mcp.configGeneration();
    if (!task_started) {
        // Priority above the Arduino loop task so animation timing holds; the
        // task does only tiny amounts of work per wake-up.
        const BaseType_t ok = xTaskCreatePinnedToCore(led_task, "leds", 4096, nullptr, 2, nullptr, 1);
        task_started = (ok == pdPASS);
        if (!task_started) LOGE(TAG, "failed to start LED task");
    }
}

void leds_set_state(uint8_t index, led_state_t state) {
    if (index >= LED_COUNT || !led_state_is_valid(state)) return;
    states[index] = static_cast<uint8_t>(state);
}

led_state_t leds_get_state(uint8_t index) {
    if (index >= LED_COUNT) return LED_OFF;
    return static_cast<led_state_t>(states[index]);
}

void leds_set(uint8_t index, bool on) { leds_set_state(index, on ? LED_ON : LED_OFF); }

void leds_all_off() {
    for (uint8_t i = 0; i < LED_COUNT; i++) states[i] = LED_OFF;
}
