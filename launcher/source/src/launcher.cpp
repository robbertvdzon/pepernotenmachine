#include <Arduino.h>
#include "../include/pull_motor.h"
#include "../include/config.h"
#include "../include/led.h"
#include "../include/release_servo.h"
#include "../include/dispenser.h"
#include "../include/launcher.h"
#include <atomic>

#define STATE_LED LED_1

// Preparation runs in its own task, polling the end switch instead of using an ISR. That keeps every
// call into the stepper / LED / dispenser code in normal task context.
static std::atomic<PreparationState> preparationState(PREPARATION_IDLE);
static TaskHandle_t preparationTask = NULL;

static preparation_state_cb_t stateCb = NULL;

static TimerHandle_t autoLockTimer = NULL;

static void auto_lock_timer_cb(TimerHandle_t xTimer) { release_servo_set_angle(SERVO_CLOSE_ANGLE); }

static void publish_state(PreparationState s) {
    if (stateCb) stateCb((uint8_t)s);
}

static void set_state(PreparationState s) {
    preparationState = s;
    publish_state(s);
}

static const TickType_t pollTicks = (pdMS_TO_TICKS(2) > 0) ? pdMS_TO_TICKS(2) : 1;
static const uint32_t switchDebounceMs = 10;  // switch must stay pressed this long to count
static const uint32_t moveUpSlowSteps = PREPARATION_MOVE_UP_SLOW_STEPS;
static const uint32_t moveUpTotalSteps = PREPARATION_MOVE_UP_SLOW_STEPS + PREPARATION_MOVE_UP_FAST_STEPS;

static bool switch_pressed() { return digitalRead(PULL_MOTOR_END_SWITCH_PIN) == LOW; }

static void abort_preparation(const char* reason) {
    Serial.printf("Preparation aborted: %s\n", reason);
    pull_motor_stop();
    led_set_state(STATE_LED, LED_FLASH_FAST);
    set_state(PREPARATION_ERROR);
}

static void run_preparation() {
    // ---- MOVE_DOWN: full speed (after the ramp-up) until the end switch is pressed -------------------------
    led_set_state(STATE_LED, LED_PULSE);
    Serial.println("Preparation started: moving down");
    pull_motor_set_speed(-PREPARATION_MOVE_DOWN_SPEED);

    const uint32_t downStart = millis();
    bool pressed = false;
    uint32_t pressedSince = 0;
    for (;;) {
        vTaskDelay(pollTicks);

        if (switch_pressed()) {
            if (!pressed) {
                pressed = true;
                pressedSince = millis();
            }
            if (millis() - pressedSince >= switchDebounceMs) break;
        } else {
            pressed = false;
        }

        if (millis() - downStart > PREPARATION_MOVE_DOWN_TIMEOUT_MS) {
            abort_preparation("end switch not reached in time");
            return;
        }
    }

    // ---- Bottom reached: stop at once; this spot is now step 0 -----------------------------------------------
    Serial.println("Reached bottom, moving up");
    pull_motor_hard_stop(0);
    const uint32_t stopStart = millis();
    while (pull_motor_is_running()) {
        if (millis() - stopStart > 200) {
            abort_preparation("motor did not stop at the bottom");
            return;
        }
        vTaskDelay(pollTicks);
    }

    // ---- MOVE_UP_SLOW: one continuous move to the top; the first part is done at slow speed -------------------
    set_state(PREPARATION_MOVE_UP_SLOW);
    led_set_state(STATE_LED, LED_FLASH);
    if (!pull_motor_move_to(moveUpTotalSteps, PREPARATION_MOVE_UP_SLOW_SPEED)) {
        abort_preparation("could not start move up");
        return;
    }

    const uint32_t upStart = millis();
    while (pull_motor_position() < (int32_t)moveUpSlowSteps) {
        if (millis() - upStart > 50 && !pull_motor_is_running()) {
            abort_preparation("motor stopped during slow move up");
            return;
        }
        vTaskDelay(pollTicks);
    }

    // ---- MOVE_UP_FAST: speed up for the rest of the distance, start the dispenser ----------------------------
    Serial.println("Slow distance done, moving up fast");
    set_state(PREPARATION_MOVE_UP_FAST);
    pull_motor_change_speed(PREPARATION_MOVE_UP_FAST_SPEED);
    dispenser_control(1);

    while (pull_motor_is_running()) {
        vTaskDelay(pollTicks);
    }

    // The move ends with a ramp-down exactly on the target; the driver is disabled shortly after the last step
    Serial.println("Launcher preparation complete");
    led_set_state(STATE_LED, LED_ON);
    set_state(PREPARATION_IDLE);
}

static void preparation_task(void* arg) {
    (void)arg;
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);  // sleep until launcher_prepare()
        run_preparation();
        ulTaskNotifyTake(pdTRUE, 0);              // drop start requests that arrived while running
    }
}

void launcher_prepare_init(preparation_state_cb_t cb) {
    stateCb = cb;
    led_set_state(STATE_LED, LED_OFF);
    pinMode(PULL_MOTOR_END_SWITCH_PIN, INPUT_PULLUP);
    xTaskCreate(preparation_task, "prepare", 4096, NULL, 2, &preparationTask);
}

void launcher_prepare() {
    PreparationState previous = PREPARATION_IDLE;
    if (!preparationState.compare_exchange_strong(previous, PREPARATION_MOVE_DOWN)) {
        previous = PREPARATION_ERROR;
        if (!preparationState.compare_exchange_strong(previous, PREPARATION_MOVE_DOWN)) {
            Serial.println("Cannot prepare launcher: preparation already running");
            return;
        }
    }

    release_servo_set_angle(SERVO_CLOSE_ANGLE);

    if (switch_pressed()) {
        Serial.println("Cannot prepare launcher: switch is currently activated");
        preparationState = previous;  // not published: clients still have the old state
        return;
    }

    publish_state(PREPARATION_MOVE_DOWN);
    xTaskNotifyGive(preparationTask);
}

void launcher_launch() {
    release_servo_set_angle(SERVO_OPEN_ANGLE);

    if (autoLockTimer == NULL) {
        autoLockTimer = xTimerCreate("autoLock", pdMS_TO_TICKS(4000), pdFALSE, NULL, auto_lock_timer_cb);
    }

    if (autoLockTimer != NULL) {
        xTimerStop(autoLockTimer, 0);
        xTimerStart(autoLockTimer, 0);
    }
}

void launcher_cancel_auto_lock() {
    if (autoLockTimer != NULL) {
        xTimerStop(autoLockTimer, 0);
    }
}