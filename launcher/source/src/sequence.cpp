#include <Arduino.h>
#include "../include/pull_motor.h"
#include "../include/config.h"
#include "../include/led.h"
#include "../include/release_servo.h"
#include "../include/dispenser.h"
#include "../include/sequence.h"
#include <atomic>

#define STATE_LED LED_1

// The whole sequence runs in its own task, polling the end switch instead of using an ISR. That keeps every
// call into the stepper / LED / dispenser code in normal task context.
static std::atomic<SequenceState> sequenceState(IDLE);
static TaskHandle_t sequenceTask = NULL;

static sequence_state_cb_t stateCb = NULL;

static void publish_state(SequenceState s) {
    if (stateCb) stateCb((uint8_t)s);
}

static void set_state(SequenceState s) {
    sequenceState = s;
    publish_state(s);
}

static const TickType_t pollTicks = (pdMS_TO_TICKS(2) > 0) ? pdMS_TO_TICKS(2) : 1;
static const uint32_t switchDebounceMs = 10;  // switch must stay pressed this long to count
static const uint32_t moveUpSlowSteps = SEQUENCE_MOVE_UP_SLOW_STEPS;
static const uint32_t moveUpTotalSteps = SEQUENCE_MOVE_UP_SLOW_STEPS + SEQUENCE_MOVE_UP_FAST_STEPS;

static bool switch_pressed() { return digitalRead(PULL_MOTOR_END_SWITCH_PIN) == LOW; }

static void abort_sequence(const char* reason) {
    Serial.printf("Sequence aborted: %s\n", reason);
    pull_motor_stop();
    led_set_state(STATE_LED, LED_FLASH_FAST);
    set_state(ERROR);
}

static void run_sequence() {
    // ---- MOVE_DOWN: full speed (after the ramp-up) until the end switch is pressed -------------------------
    led_set_state(STATE_LED, LED_PULSE);
    Serial.println("Sequence started: moving down");
    pull_motor_set_speed(-SEQUENCE_MOVE_DOWN_SPEED);

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

        if (millis() - downStart > SEQUENCE_MOVE_DOWN_TIMEOUT_MS) {
            abort_sequence("end switch not reached in time");
            return;
        }
    }

    // ---- Bottom reached: stop at once; this spot is now step 0 -----------------------------------------------
    Serial.println("Reached bottom, moving up");
    pull_motor_hard_stop(0);
    const uint32_t stopStart = millis();
    while (pull_motor_is_running()) {
        if (millis() - stopStart > 200) {
            abort_sequence("motor did not stop at the bottom");
            return;
        }
        vTaskDelay(pollTicks);
    }

    // ---- MOVE_UP_SLOW: one continuous move to the top; the first part is done at slow speed -------------------
    set_state(MOVE_UP_SLOW);
    led_set_state(STATE_LED, LED_FLASH);
    if (!pull_motor_move_to(moveUpTotalSteps, SEQUENCE_MOVE_UP_SLOW_SPEED)) {
        abort_sequence("could not start move up");
        return;
    }

    const uint32_t upStart = millis();
    while (pull_motor_position() < (int32_t)moveUpSlowSteps) {
        if (millis() - upStart > 50 && !pull_motor_is_running()) {
            abort_sequence("motor stopped during slow move up");
            return;
        }
        vTaskDelay(pollTicks);
    }

    // ---- MOVE_UP_FAST: speed up for the rest of the distance, start the dispenser ----------------------------
    Serial.println("Slow distance done, moving up fast");
    set_state(MOVE_UP_FAST);
    pull_motor_change_speed(SEQUENCE_MOVE_UP_FAST_SPEED);
    dispenser_set_duration(SEQUENCE_DISPENSE_DURATION_SECONDS);
    dispenser_control(1);

    while (pull_motor_is_running()) {
        vTaskDelay(pollTicks);
    }

    // The move ends with a ramp-down exactly on the target; the driver is disabled shortly after the last step
    Serial.println("Reached top, sequence done");
    led_set_state(STATE_LED, LED_ON);
    set_state(IDLE);
}

static void sequence_task(void* arg) {
    (void)arg;
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);  // sleep until sequence_start()
        run_sequence();
        ulTaskNotifyTake(pdTRUE, 0);              // drop start requests that arrived while running
    }
}

void sequence_init(sequence_state_cb_t cb) {
    stateCb = cb;
    led_set_state(STATE_LED, LED_OFF);
    pinMode(PULL_MOTOR_END_SWITCH_PIN, INPUT_PULLUP);
    xTaskCreate(sequence_task, "sequence", 4096, NULL, 2, &sequenceTask);
}

void sequence_start() {
    SequenceState previous = IDLE;
    if (!sequenceState.compare_exchange_strong(previous, MOVE_DOWN)) {
        previous = ERROR;
        if (!sequenceState.compare_exchange_strong(previous, MOVE_DOWN)) {
            Serial.println("Cannot start sequence: already running");
            return;
        }
    }

    release_servo_set_angle(SERVO_CLOSE_ANGLE);

    if (switch_pressed()) {
        Serial.println("Cannot start sequence: switch is currently activated");
        sequenceState = previous;  // not published: clients still have the old state
        return;
    }

    publish_state(MOVE_DOWN);
    xTaskNotifyGive(sequenceTask);
}