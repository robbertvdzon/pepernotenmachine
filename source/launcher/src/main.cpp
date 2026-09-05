
#include <Arduino.h>

#include "../include/config.h"
#include "../include/release_servo.h"
#include "../include/pull_motor.h"
#include "../include/button.h"
#include "../include/ble_manager.h"
#include "../include/sequence.h"
#include "../include/dispenser.h"
#include "../include/led.h"

static void pull_motor_write_cb(int32_t speed) {
    pull_motor_set_speed(speed);
}

static void release_servo_write_cb(uint8_t angle) {
    Serial.print("Setting release servo angle to ");
    Serial.println(angle);
    release_servo_set_angle(angle);
}

static void button_notify_cb(uint8_t payload) {
    ble_notify_button(payload);
}

static void sequence_write_cb() {
    sequence_start();
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
     do  {
        ;  // wait for serial port to connect. Needed for native USB
        delay(100);
     } while (!Serial);

     Serial.println("Launcher starting up...");
     Serial.printf("Chip: %s\n", ESP.getChipModel());
     Serial.printf("Revision: %d\n", ESP.getChipRevision());
     Serial.printf("Cores: %d\n", ESP.getChipCores());
     Serial.printf("Free heap: %u\n", ESP.getFreeHeap());
     Serial.printf("PSRAM: %s\n", psramFound() ? "YES" : "NO");
     Serial.println("");

     led_init();
     pull_motor_init();
     release_servo_init();
     sequence_init();
     button_init(button_notify_cb);
     dispenser_init(dispenser_notify_cb);

     // initialize BLE with callbacks (motor, horn, servo)
     ble_init(pull_motor_write_cb, release_servo_write_cb, sequence_write_cb, dispenser_duration_write_cb,
              dispenser_start_write_cb);
}

void loop() {
    // short yield to allow other tasks to execute
    vTaskDelay(10 / portTICK_PERIOD_MS);
}
