#include "../include/digital_inputs.h"
#include "../include/config.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static digital_inputs_changed_cb_t changedCb = nullptr;
static TaskHandle_t inputTask = nullptr;
static volatile uint8_t currentMask = 0;

static uint8_t read_mask() {
    uint8_t mask = 0;
    for (uint8_t i = 0; i < DIGITAL_INPUT_COUNT; i++) {
        if (digitalRead(DIGITAL_INPUT_PINS[i]) == LOW) mask |= (uint8_t)(1u << i);  // Active LOW
    }
    return mask;
}

// ISR: any edge on any input just wakes the task. Debouncing happens in task context.
static void IRAM_ATTR input_isr() {
    BaseType_t woken = pdFALSE;
    if (inputTask != nullptr) vTaskNotifyGiveFromISR(inputTask, &woken);
    if (woken) portYIELD_FROM_ISR();
}

static void input_task(void* pvParameters) {
    (void)pvParameters;
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);                    // Sleep until an edge happens
        vTaskDelay(pdMS_TO_TICKS(DIGITAL_INPUT_DEBOUNCE_MS));       // Let the contact bounce settle
        ulTaskNotifyTake(pdTRUE, 0);                                // Drop edges that occurred while settling

        uint8_t mask = read_mask();
        if (mask != currentMask) {
            currentMask = mask;
            LOGF("Digital inputs: 0x%02X", mask);
            if (changedCb) changedCb(mask);
        }
    }
}

void digital_inputs_init(digital_inputs_changed_cb_t cb) {
    changedCb = cb;
    for (uint8_t i = 0; i < DIGITAL_INPUT_COUNT; i++) {
        pinMode(DIGITAL_INPUT_PINS[i], DIGITAL_INPUT_USE_PULLUP ? INPUT_PULLUP : INPUT);
    }
    currentMask = read_mask();

    xTaskCreate(input_task, "din_task", DIGITAL_INPUT_TASK_STACK, NULL, DIGITAL_INPUT_TASK_PRIORITY, &inputTask);

    for (uint8_t i = 0; i < DIGITAL_INPUT_COUNT; i++) {
        attachInterrupt(digitalPinToInterrupt(DIGITAL_INPUT_PINS[i]), input_isr, CHANGE);
    }
}

uint8_t digital_inputs_get_mask() {
    return currentMask;
}
