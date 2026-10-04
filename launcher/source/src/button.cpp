#include "../include/button.h"
#include "../include/config.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define BUTTON_PIN EXTRA_IO_1_PIN

static button_notify_cb_t notify_cb = nullptr;
static volatile int currentState = HIGH;
static const TickType_t debounceDelayTicks = (pdMS_TO_TICKS(20) > 0) ? pdMS_TO_TICKS(20) : 1;

static TaskHandle_t buttonTaskHandle = NULL;

// Wait for a quiet interval after the last edge, then report the stable state.
static void button_task(void* pvParameters) {
    (void)pvParameters;

    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        while (ulTaskNotifyTake(pdTRUE, debounceDelayTicks) > 0) {
        }

        const int stableState = digitalRead(BUTTON_PIN);
        if (stableState != currentState) {
            currentState = stableState;
            const uint8_t payload = (stableState == LOW) ? 1 : 0;  // 1=pressed, 0=released
            Serial.print("Button ");
            Serial.println(payload ? "pressed" : "released");
            if (notify_cb) notify_cb(payload);
        }
    }
}

// ISR: wake the task so it can debounce the input outside interrupt context.
static void IRAM_ATTR button_isr() {
    BaseType_t higherPriorityTaskWoken = pdFALSE;
    if (buttonTaskHandle != NULL) {
        vTaskNotifyGiveFromISR(buttonTaskHandle, &higherPriorityTaskWoken);
        if (higherPriorityTaskWoken) portYIELD_FROM_ISR();
    }
}


void button_init(button_notify_cb_t cb) {
    notify_cb = cb;
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    currentState = digitalRead(BUTTON_PIN);

    if (buttonTaskHandle == NULL &&
        xTaskCreate(button_task, "button_task", 2048, NULL, 1, &buttonTaskHandle) != pdPASS) {
        buttonTaskHandle = NULL;
        Serial.println("Failed to create button task");
        return;
    }

    // Attach interrupt on CHANGE (rising or falling edge)
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), button_isr, CHANGE);
}

uint8_t button_get_state() {
    return (currentState == HIGH) ? 1 : 0;
}
