#include "../include/end_switch.h"
#include "../include/config.h"
#include <Arduino.h>
#include <atomic>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static end_switch_notify_cb_t notifyCb = nullptr;
static std::atomic<uint8_t> currentState(0);
static const TickType_t pollTicks = (pdMS_TO_TICKS(2) > 0) ? pdMS_TO_TICKS(2) : 1;
static const uint32_t debounceMs = 10;
static TaskHandle_t endSwitchTaskHandle = NULL;

bool end_switch_is_activated() { return digitalRead(PULL_MOTOR_END_SWITCH_PIN) == LOW; }

static uint8_t read_state() { return end_switch_is_activated() ? 1 : 0; }

static void publish_state(uint8_t payload) {
    currentState.store(payload);
    Serial.print("End switch ");
    Serial.println(payload ? "activated" : "deactivated");
    if (notifyCb) notifyCb(payload);
}

static void end_switch_task(void* arg) {
    (void)arg;
    bool candidateActive = end_switch_is_activated();
    uint32_t candidateSince = millis();

    for (;;) {
        vTaskDelay(pollTicks);
        const bool activated = end_switch_is_activated();

        if (!activated) {
            candidateActive = false;
            candidateSince = millis();
            if (currentState.load() != 0) publish_state(0);
            continue;
        }

        if (!candidateActive) {
            candidateActive = true;
            candidateSince = millis();
        }

        if (currentState.load() == 0 && millis() - candidateSince >= debounceMs) {
            publish_state(1);
        }
    }
}

void end_switch_init(end_switch_notify_cb_t cb) {
    notifyCb = cb;
    pinMode(PULL_MOTOR_END_SWITCH_PIN, INPUT_PULLUP);
    currentState.store(read_state());

    if (endSwitchTaskHandle == NULL &&
        xTaskCreate(end_switch_task, "end_switch", 2048, NULL, 1, &endSwitchTaskHandle) != pdPASS) {
        endSwitchTaskHandle = NULL;
        Serial.println("Failed to create end-switch task");
    }
}

uint8_t end_switch_get_state() { return currentState.load(); }