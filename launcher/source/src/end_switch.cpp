
#include "../include/end_switch.h"
#include "../include/config.h"

#include <Arduino.h>
#include <atomic>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// -----------------------------------------------------------------------------
// Configuration
// -----------------------------------------------------------------------------

static constexpr uint32_t END_SWITCH_POLL_MS = 5;
static constexpr uint32_t END_SWITCH_DEBOUNCE_MS = 15;

static constexpr uint32_t END_SWITCH_TASK_STACK = 2048;
static constexpr UBaseType_t END_SWITCH_TASK_PRIORITY = 1;

// -----------------------------------------------------------------------------
// Internal state
// -----------------------------------------------------------------------------

static end_switch_notify_cb_t notifyCb = nullptr;

static std::atomic<uint8_t> currentState(0);

static TaskHandle_t endSwitchTaskHandle = nullptr;

// -----------------------------------------------------------------------------
// Hardware access
// -----------------------------------------------------------------------------

bool end_switch_is_activated(void) { return digitalRead(PULL_MOTOR_END_SWITCH_PIN) == LOW; }

// -----------------------------------------------------------------------------
// Publish state change
// -----------------------------------------------------------------------------

static void publish_state(uint8_t payload) {
    currentState.store(payload);

    Serial.print("End switch ");
    Serial.println(payload ? "activated" : "deactivated");

    if (notifyCb != nullptr) {
        notifyCb(payload);
    }
}

// -----------------------------------------------------------------------------
// End-switch monitoring task
//
// Debounces both activation and deactivation.
// Only publishes actual stable state changes.
// -----------------------------------------------------------------------------

static void end_switch_task(void* arg) {
    (void)arg;

    bool candidateState = end_switch_is_activated();
    uint32_t candidateSince = millis();

    const TickType_t pollTicks = (pdMS_TO_TICKS(END_SWITCH_POLL_MS) > 0) ? pdMS_TO_TICKS(END_SWITCH_POLL_MS) : 1;

    for (;;) {
        vTaskDelay(pollTicks);

        const bool rawState = end_switch_is_activated();
        const uint32_t now = millis();

        // Detect a change in the physical input.
        if (rawState != candidateState) {
            candidateState = rawState;
            candidateSince = now;
        }

        // Publish only after the candidate state has remained stable.
        const uint8_t publishedState = currentState.load();

        if (candidateState != (publishedState != 0) && now - candidateSince >= END_SWITCH_DEBOUNCE_MS) {
            publish_state(candidateState ? 1 : 0);
        }
    }
}

// -----------------------------------------------------------------------------
// Initialization
// -----------------------------------------------------------------------------

void end_switch_init(end_switch_notify_cb_t cb) {
    // Prevent accidental duplicate initialization.
    if (endSwitchTaskHandle != nullptr) {
        Serial.println("End switch already initialized");
        return;
    }

    notifyCb = cb;

    pinMode(PULL_MOTOR_END_SWITCH_PIN, INPUT_PULLUP);

    // Read initial hardware state.
    const uint8_t initialState = end_switch_is_activated() ? 1 : 0;

    currentState.store(initialState);

    Serial.print("End switch initial state: ");
    Serial.println(initialState ? "activated" : "deactivated");

    // Create monitoring task.
    const BaseType_t result = xTaskCreate(end_switch_task, "end_switch", END_SWITCH_TASK_STACK, nullptr,
                                          END_SWITCH_TASK_PRIORITY, &endSwitchTaskHandle);

    if (result != pdPASS) {
        endSwitchTaskHandle = nullptr;
        Serial.println("Failed to create end-switch task");
        return;
    }

    Serial.println("End switch initialized successfully");
}

// -----------------------------------------------------------------------------
// Get current stable state
// -----------------------------------------------------------------------------

uint8_t end_switch_get_state(void) { return currentState.load(); }
