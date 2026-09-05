#include "../include/button.h"
#include "../include/config.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

static button_notify_cb_t notify_cb = nullptr;
static volatile int currentState = HIGH;
static volatile TickType_t lastInterruptTick = 0;
static const TickType_t debounceDelayTicks = pdMS_TO_TICKS(20);  // 20 ms

static QueueHandle_t buttonQueue = NULL;

// Button event task: runs in task context and calls notify_cb
static void button_task(void* pvParameters) {
    (void)pvParameters;
    uint8_t payload;
    for (;;) {
        if (xQueueReceive(buttonQueue, &payload, portMAX_DELAY) == pdTRUE) {
            Serial.print("Button ");
            Serial.println(payload ? "pressed" : "released");
            if (notify_cb) notify_cb(payload);
        }
    }
}

// ISR: fires on pin change, does minimal work and posts event to queue
static void IRAM_ATTR button_isr() {
    TickType_t now = xTaskGetTickCountFromISR();
    int newState = digitalRead(EXTRA_IO_1_PIN);

    // only handle a state change if it has been stable longer than debounce delay
    if (newState != currentState && (now - lastInterruptTick) >= debounceDelayTicks) {
        lastInterruptTick = now;
        currentState = newState;

        uint8_t payload = (currentState == LOW) ? 1 : 0; // 1=pressed, 0=released
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        if (buttonQueue != NULL) {
            xQueueSendFromISR(buttonQueue, &payload, &xHigherPriorityTaskWoken);
            if (xHigherPriorityTaskWoken) portYIELD_FROM_ISR();
        }
    }
}    


void button_init(button_notify_cb_t cb) {
    notify_cb = cb;
    pinMode(EXTRA_IO_1_PIN, INPUT_PULLUP);
    currentState = digitalRead(EXTRA_IO_1_PIN);
    lastInterruptTick = xTaskGetTickCount();

    // Create queue and task for handling button events
    if (buttonQueue == NULL) {
        buttonQueue = xQueueCreate(8, sizeof(uint8_t));
    }
    if (buttonQueue != NULL) {
        xTaskCreate(button_task, "button_task", 2048, NULL, 1, NULL);
    }

    // Attach interrupt on CHANGE (rising or falling edge)
    attachInterrupt(digitalPinToInterrupt(EXTRA_IO_1_PIN), button_isr, CHANGE);
}

uint8_t button_get_state() {
    return (currentState == HIGH) ? 1 : 0;
}
