#include "../include/config.h"
#include "../include/motor.h"
#include "../include/button.h"
#include "../include/horn.h"
#include "../include/ble_manager.h"
#include "../include/led.h"
#include "../include/servo.h"
#include "../include/routine.h"
#include <Arduino.h>

// callbacks forwarded from BLE manager
static void motor_write_cb(int32_t speed) {
    motor_set_speed(speed);
}

static void horn_write_cb(bool on) {
    horn_set(on);
}

static void servo_write_cb(uint8_t angle) {
    Serial.print("Setting servo angle to ");
    Serial.println(angle);
    servo_set_angle(angle);
}

static void button_notify_cb(uint8_t payload) {
    // payload: 1 = pressed, 0 = released
    ble_notify_button(payload);
}

static void routine_write_cb() {
    routine_start();
}

void setup() {
    Serial.begin(115200);
    while (!Serial) {
        ; // wait for serial port to connect. Needed for native USB
    }
    Serial.println("Launcher starting up...");

    motor_init();
    horn_init();
    led_init();
    servo_init();
    routine_init();
    button_init(button_notify_cb);

    // initialize BLE with callbacks (motor, horn, servo)
    ble_init(motor_write_cb, horn_write_cb, servo_write_cb, routine_write_cb);
}

void loop() {
    // short yield to allow other tasks to execute
    vTaskDelay(10 / portTICK_PERIOD_MS);
}