#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>

// #define ENDURANCE_TEST  // TEMPORARY TEST CODE: run the sequence repeatedly without BLE or button input

#include "../include/config.h"
#include "../include/release_servo.h"
#include "../include/pull_motor.h"
#include "../include/button.h"
#include "../include/ble_manager.h"
#include "../include/sequence.h"
#include "../include/dispenser.h"
#include "../include/led.h"
#include "../include/storage.h"

static void pull_motor_write_cb(int32_t speed) { pull_motor_set_speed(speed); }

static void release_servo_write_cb(uint8_t angle) {
    sequence_abort_launch();  // if the sequence is running, stop it and return to IDL
    release_servo_set_angle(angle);
}

static void button_notify_cb(uint8_t payload) {
    ble_notify_button(payload);

    if (payload == 0) {
        release_servo_set_angle(SERVO_CLOSE_ANGLE);
        sequence_start();
    } else {
        sequence_launch();
    }
}

static void sequence_write_cb() { sequence_start(); }

#ifdef ENDURANCE_TEST
// ===== TEMPORARY TEST CODE: replaces the normal loop() =====
static const int TEST_RUNS = 20;
static const uint32_t TEST_PAUSE_MS = 3000;          // pause between runs
static const uint32_t TEST_RUN_TIMEOUT_MS = 120000;  // give up if one run takes longer than this
// Same numbers as the BLE sequence state values
static const uint8_t TEST_STATE_IDLE = 0;
static const uint8_t TEST_STATE_MOVE_DOWN = 1;
static const uint8_t TEST_STATE_ERROR = 4;
static volatile uint8_t testState = TEST_STATE_IDLE;
static const uint32_t TEST_SERVO_OPEN_MS = 3000;
// ===== END TEMPORARY TEST CODE =====
#endif

static void sequence_state_notify_cb(uint8_t state) {
#ifdef ENDURANCE_TEST
    testState = state;  // TEMP test
#endif
    ble_notify_sequence_state(state);
}

static void dispenser_duration_write_cb(uint8_t seconds) { dispenser_set_duration(seconds); }

static void dispenser_start_write_cb(uint8_t command) { dispenser_control(command); }

static void dispenser_notify_cb(uint8_t state) { ble_notify_dispenser_start(state); }

void setup() {
    Serial.begin(115200);
    do {
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

     if (!storage_init()) {
         Serial.println("Failed to initialize persistent storage");
     }

    led_init();
    pull_motor_init();
    release_servo_init();
    dispenser_init(dispenser_notify_cb);
    sequence_init(sequence_state_notify_cb);
    button_init(button_notify_cb);

    // initialize BLE with callbacks (motor, horn, servo)
    ble_init(pull_motor_write_cb, release_servo_write_cb, sequence_write_cb, dispenser_duration_write_cb,
             dispenser_start_write_cb);
}

// ===== TEMPORARY TEST CODE: replaces the normal loop() =====
void loop() {
#ifndef ENDURANCE_TEST
    // short yield to allow other tasks to execute
    vTaskDelay(10 / portTICK_PERIOD_MS);
#else

    static int run = 0;
    static bool finished = false;

    // if (finished) {
    if (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        return;
    }

    run++;
    Serial.printf("TEST: run %d/%d starting\n", run, TEST_RUNS);
    const uint32_t t0 = millis();
    sequence_start();

    // The MOVE_DOWN state is published inside sequence_start(), so if it isn't set now the start was refused
    if (testState != TEST_STATE_MOVE_DOWN) {
        Serial.println("TEST STOPPED: sequence did not start (end switch pressed, or already running)");
        finished = true;
        return;
    }

    // Wait for the sequence to end. Polled every 10 ms so the servo opens right after MOVE_UP_FAST is done.
    while (testState != TEST_STATE_IDLE && testState != TEST_STATE_ERROR) {
        if (millis() - t0 > TEST_RUN_TIMEOUT_MS) {
            Serial.printf("TEST STOPPED: run %d did not finish within %lu s\n", run,
                          (unsigned long)(TEST_RUN_TIMEOUT_MS / 1000));
            finished = true;
            return;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    if (testState == TEST_STATE_ERROR) {
        Serial.printf("TEST FAILED: ERROR state in run %d/%d (reason is in the log above)\n", run, TEST_RUNS);
        finished = true;
        return;
    }

    Serial.printf("TEST: run %d/%d sequence OK (%lu s), releasing\n", run, TEST_RUNS,
                  (unsigned long)((millis() - t0) / 1000));

    // Unlock the launch pod, then lock it again after a short time
    release_servo_set_angle(SERVO_OPEN_ANGLE);
    vTaskDelay(pdMS_TO_TICKS(TEST_SERVO_OPEN_MS));
    release_servo_set_angle(SERVO_CLOSE_ANGLE);

    if (run >= TEST_RUNS) {
        Serial.printf("TEST PASSED: %d runs without errors\n", TEST_RUNS);
        finished = true;
        return;
    }

    vTaskDelay(pdMS_TO_TICKS(TEST_PAUSE_MS));  // let the servo settle before the next run
    // ===== END TEMPORARY TEST CODE =====
#endif
}
