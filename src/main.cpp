#include "../include/config.h"
#include "../include/servo.h"
#include <Arduino.h>

#include "../include/motor.h"
#include "../include/button.h"
#include "../include/horn.h"
#include "../include/ble_manager.h"
#include "../include/led.h"
#include "../include/routine.h"
#include "../include/dispenser.h"

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
    ble_notify_button(payload);
}

static void routine_write_cb() {
    routine_start();
}

static void dispenser_duration_write_cb(uint8_t seconds) {
    dispenser_set_duration(seconds);
}

static void dispenser_start_write_cb(uint8_t command) {
    dispenser_control(command);
}

static void dispenser_notify_cb(uint8_t state) {
    ble_notify_dispenser_start(state);
}

void setup() {
    Serial.begin(115200);
    while (!Serial) {
        ;  // wait for serial port to connect. Needed for native USB
    }
    Serial.println("Launcher starting up...");

    motor_init();
    horn_init();
    led_init();
    servo_init();
    routine_init();
    button_init(button_notify_cb);
    dispenser_init(dispenser_notify_cb);

    // initialize BLE with callbacks (motor, horn, servo)
    ble_init(motor_write_cb, horn_write_cb, servo_write_cb, routine_write_cb, dispenser_duration_write_cb, dispenser_start_write_cb);
}

void loop() {
    // short yield to allow other tasks to execute
    vTaskDelay(10 / portTICK_PERIOD_MS);
}
