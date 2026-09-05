#include "../include/pull_motor.h"
#include "../include/config.h"
#include "../include/led.h"
#include <Arduino.h>

#define STATE_LED LED_1
#define MOVE_UP_SLOW_DURATION_MS 2000
#define MOVE_UP_FAST_DURATION_MS 12000

enum SequenceState {
    IDLE,
    MOVE_DOWN,
    MOVE_UP_SLOW,
    MOVE_UP_FAST,
};

static SequenceState sequenceState = IDLE;
static volatile int currentState = HIGH;
static volatile TickType_t lastInterruptTick = 0;
static const TickType_t debounceDelayTicks = pdMS_TO_TICKS(20);
static TimerHandle_t moveUpSlowTimer;
static TimerHandle_t moveUpFastTimer;

static void vTimerCallback(TimerHandle_t xTimer) {
    if (xTimer == moveUpSlowTimer) {
        Serial.println("Timer expired, moving up fast");
        pull_motor_set_speed(512);
        sequenceState = MOVE_UP_FAST;
    } else if (xTimer == moveUpSlowTimer) {
        Serial.println("Timer expired, disabling motor");
        pull_motor_set_speed(0);
        pull_motor_enable(false);
        led_set_state(STATE_LED, LED_ON);
        sequenceState = IDLE;
    } else {
        Serial.println("Error: Unknown timer handle in vTimerCallback");
    }
}

static void IRAM_ATTR switch_isr() {
    TickType_t now = xTaskGetTickCountFromISR();
    int newState = digitalRead(PULL_MOTOR_END_SWITCH_PIN);

    // only handle a state change if it has been stable longer than debounce delay
    if (newState != currentState && (now - lastInterruptTick) >= debounceDelayTicks) {
        lastInterruptTick = now;
        currentState = newState;
        Serial.println("Received switch interrupt, state: " + String(currentState == LOW ? "LOW" : "HIGH"));
        if (sequenceState == MOVE_DOWN && currentState == HIGH) {
            Serial.println("Reached bottom, moving up");
            pull_motor_set_speed(128);
            sequenceState = MOVE_UP_SLOW;
            led_set_state(STATE_LED, LED_FLASH);
            if (moveUpSlowTimer != NULL) {
                xTimerReset(moveUpSlowTimer, 0);
            }
        }
    }
}

void sequence_init() {
    pull_motor_enable(false);
    led_set_state(STATE_LED, LED_OFF);

    pinMode(PULL_MOTOR_END_SWITCH_PIN, INPUT_PULLUP);
    currentState = digitalRead(PULL_MOTOR_END_SWITCH_PIN);
    lastInterruptTick = xTaskGetTickCount();
    attachInterrupt(digitalPinToInterrupt(RELEASE_SERVO_PIN), switch_isr, CHANGE);

    moveUpSlowTimer =
        xTimerCreate("MoveUpSlowTimer", pdMS_TO_TICKS(MOVE_UP_SLOW_DURATION_MS), pdFALSE, (void*)0, vTimerCallback);
    moveUpFastTimer =
        xTimerCreate("MoveUpFastTimer", pdMS_TO_TICKS(MOVE_UP_FAST_DURATION_MS), pdFALSE, (void*)0, vTimerCallback);
}

void sequence_start() { 
    //Make sure the switch is not currently activated!!
    if (currentState == HIGH) {
        Serial.println("Cannot start sequence: switch is currently activated");
        return;
    }

    sequenceState = MOVE_DOWN;
    led_set_state(STATE_LED, LED_PULSE);
    Serial.println("Sequence started: moving down");
    pull_motor_enable(true);
    pull_motor_set_speed(-512);
}
