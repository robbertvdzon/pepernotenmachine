// Main orchestrator: wires modules together and runs lightweight loop.
#include "../include/config.h"
#include "../include/motor.h"
#include "../include/button.h"
#include "../include/ble_manager.h"
#include <Arduino.h>

static uint32_t lastHornTurnedOnTime = 0;

// callbacks forwarded from BLE manager
static void motor_write_cb(uint32_t halfPeriodUs) {
    motor_set_delay_us(halfPeriodUs);
}

static void horn_write_cb(bool on) {
    digitalWrite(HORN, on ? HIGH : LOW);
    if (on) lastHornTurnedOnTime = millis();
}

// button notification callback
static void button_notify_cb(uint8_t payload) {
    // payload: 1 = pressed, 0 = released
    ble_notify_button(payload);
}

void setup() {
    Serial.begin(115200);

    motor_init();

    pinMode(HORN, OUTPUT);
    digitalWrite(HORN, LOW);

    // initialize BLE with callbacks
    ble_init(motor_write_cb, horn_write_cb);

    // initialize button handling and set notification callback
    button_init(button_notify_cb);

}

void loop() {
    // Keep button handling responsive and non-blocking
    button_update();

    // Auto-turn-off horn after 2 seconds (failsafe)
    if (digitalRead(HORN) == HIGH) {
        if (millis() - lastHornTurnedOnTime >= 2000) {
            digitalWrite(HORN, LOW);
            Serial.println("Horn AUTO OFF");
        }
    }

    // short yield
    vTaskDelay(1 / portTICK_PERIOD_MS);
}