// Main orchestrator: wires modules together and runs lightweight loop.
#include "../include/config.h"
#include "../include/motor.h"
#include "../include/button.h"
#include "../include/horn.h"
#include "../include/ble_manager.h"
#include "../include/led.h"
#include <Arduino.h>

// callbacks forwarded from BLE manager
static void motor_write_cb(uint32_t halfPeriodUs) {
    motor_set_delay_us(halfPeriodUs);
}

static void horn_write_cb(bool on) {
    horn_set(on);
}

// button notification callback
static void button_notify_cb(uint8_t payload) {
    // payload: 1 = pressed, 0 = released
    ble_notify_button(payload);
}

void setup() {
    Serial.begin(115200);

    motor_init();
    horn_init();
    led_init();

    // initialize BLE with callbacks
    ble_init(motor_write_cb, horn_write_cb);

    // initialize button handling and set notification callback
    button_init(button_notify_cb);
}

void loop() {
    // Keep button handling responsive and non-blocking
    button_update();
    // Service horn auto-off and other horn tasks
    horn_update();
    // service LED non-blocking effects
    led_update();

    // short yield
    vTaskDelay(1 / portTICK_PERIOD_MS);
}