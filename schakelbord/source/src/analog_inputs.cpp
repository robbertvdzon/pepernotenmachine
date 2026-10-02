#include "../include/analog_inputs.h"
#include "../include/config.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdlib.h>

static analog_inputs_changed_cb_t changedCb = nullptr;
static uint16_t latest[ANALOG_INPUT_COUNT];
static portMUX_TYPE latestMux = portMUX_INITIALIZER_UNLOCKED;

uint16_t adc_read_mv_averaged(uint8_t pin, uint8_t samples) {
    if (samples == 0) samples = 1;
    uint32_t sum = 0;
    for (uint8_t i = 0; i < samples; i++) sum += analogReadMilliVolts(pin);
    return (uint16_t)(sum / samples);
}

static void analog_task(void* pvParameters) {
    (void)pvParameters;
    uint16_t notified[ANALOG_INPUT_COUNT] = {0};
    uint32_t lastNotifyMs = 0;
    bool first = true;
    TickType_t lastWake = xTaskGetTickCount();

    for (;;) {
        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(ANALOG_SAMPLE_INTERVAL_MS));

        uint16_t current[ANALOG_INPUT_COUNT];
        for (uint8_t i = 0; i < ANALOG_INPUT_COUNT; i++) {
            current[i] = adc_read_mv_averaged(ANALOG_INPUT_PINS[i], ANALOG_OVERSAMPLE);
        }

        portENTER_CRITICAL(&latestMux);
        for (uint8_t i = 0; i < ANALOG_INPUT_COUNT; i++) latest[i] = current[i];
        portEXIT_CRITICAL(&latestMux);

        bool changed = false;
        for (uint8_t i = 0; i < ANALOG_INPUT_COUNT; i++) {
            if (abs((int)current[i] - (int)notified[i]) >= ANALOG_NOTIFY_THRESHOLD_MV) changed = true;
        }

        uint32_t now = millis();
        if (first || (changed && (now - lastNotifyMs) >= ANALOG_NOTIFY_MIN_INTERVAL_MS)) {
            first = false;
            lastNotifyMs = now;
            for (uint8_t i = 0; i < ANALOG_INPUT_COUNT; i++) notified[i] = current[i];
            if (changedCb) changedCb(current, ANALOG_INPUT_COUNT);
        }
    }
}

void analog_inputs_init(analog_inputs_changed_cb_t cb) {
    changedCb = cb;
    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);
    for (uint8_t i = 0; i < ANALOG_INPUT_COUNT; i++) {
        pinMode(ANALOG_INPUT_PINS[i], INPUT);
        latest[i] = 0;
    }
    xTaskCreate(analog_task, "ain_task", ANALOG_TASK_STACK, NULL, ANALOG_TASK_PRIORITY, NULL);
}

void analog_inputs_get(uint16_t* out) {
    portENTER_CRITICAL(&latestMux);
    for (uint8_t i = 0; i < ANALOG_INPUT_COUNT; i++) out[i] = latest[i];
    portEXIT_CRITICAL(&latestMux);
}
