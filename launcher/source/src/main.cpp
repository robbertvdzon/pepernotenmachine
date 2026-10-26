#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>

// #define ENDURANCE_TEST  // TEMPORARY TEST CODE: prepare and launch repeatedly without BLE or button input

#include "../include/config.h"
#include "../include/release_servo.h"
#include "../include/pull_motor.h"
#include "../include/button.h"
#include "../include/ble_manager.h"
#include "../include/launcher.h"
#include "../include/dispenser.h"
#include "../include/led.h"
#include "../include/storage.h"

static void pull_motor_write_cb(int32_t speed) { pull_motor_set_speed(speed); }

static void release_servo_write_cb(uint8_t angle) {
    launcher_cancel_auto_lock();
    release_servo_set_angle(angle);
}

static void button_notify_cb(uint8_t payload) {
    ble_notify_button(payload);

    if (payload == 0) {
        release_servo_set_angle(SERVO_CLOSE_ANGLE);
        launcher_prepare();
    } else {
        launcher_launch();
    }
}

static void prepare_ble_cb() { launcher_prepare(); }
static void launch_ble_cb() { launcher_launch(); }

#ifdef ENDURANCE_TEST
// ===== TEMPORARY TEST CODE: replaces the normal loop() =====
static const int TEST_RUNS = 20;
static const uint32_t TEST_PAUSE_MS = 3000;          // pause between runs
static const uint32_t TEST_RUN_TIMEOUT_MS = 120000;  // give up if one run takes longer than this
// Same numbers as the BLE preparation state values
static const uint8_t TEST_STATE_IDLE = PREPARATION_IDLE;
static const uint8_t TEST_STATE_MOVE_DOWN = PREPARATION_MOVE_DOWN;
static const uint8_t TEST_STATE_ERROR = PREPARATION_ERROR;
static volatile uint8_t testState = TEST_STATE_IDLE;
static const uint32_t TEST_SERVO_OPEN_MS = 3000;
// ===== END TEMPORARY TEST CODE =====
#endif

static void preparation_state_notify_cb(uint8_t state) {
#ifdef ENDURANCE_TEST
    testState = state;  // TEMP test
#endif
    ble_notify_preparation_state(state);
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
    pull_motor_init(ble_notify_pull_motor_speed);
    release_servo_init(ble_notify_servo_angle);
    dispenser_init(dispenser_notify_cb);
    launcher_prepare_init(preparation_state_notify_cb);
    button_init(button_notify_cb);

    // initialize BLE with callbacks (motor, horn, servo)
    ble_init(pull_motor_write_cb, release_servo_write_cb, prepare_ble_cb, launch_ble_cb, dispenser_duration_write_cb,
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
    launcher_prepare();

    // The MOVE_DOWN state is published inside launcher_prepare(), so if it isn't set now preparation was refused
    if (testState != TEST_STATE_MOVE_DOWN) {
        Serial.println("TEST STOPPED: preparation did not start (end switch pressed, or already running)");
        finished = true;
        return;
    }

    // Wait for preparation to finish. Polled every 10 ms.
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

    Serial.printf("TEST: run %d/%d preparation OK (%lu s), ready to launch\n", run, TEST_RUNS,
                  (unsigned long)((millis() - t0) / 1000));

    launcher_launch();
    vTaskDelay(pdMS_TO_TICKS(TEST_SERVO_OPEN_MS));

    if (run >= TEST_RUNS) {
        Serial.printf("TEST PASSED: %d runs without errors\n", TEST_RUNS);
        finished = true;
        return;
    }

    vTaskDelay(pdMS_TO_TICKS(TEST_PAUSE_MS));  // let the servo settle before the next run
    // ===== END TEMPORARY TEST CODE =====
#endif
}
