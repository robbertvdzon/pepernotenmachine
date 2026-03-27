#include "../include/motor.h"
#include <Arduino.h>

#define SWITCH_PIN 33

enum RoutineState {
    IDLE,
    MOVE_DOWN,
    MOVE_UP_SLOW,
    MOVE_UP_FAST,
};

static RoutineState routineState = IDLE;
static volatile int currentState = HIGH;
static volatile TickType_t lastInterruptTick = 0;
static const TickType_t debounceDelayTicks = pdMS_TO_TICKS(20);  // 20 ms
static TimerHandle_t moveUpTimer;

static void vTimerCallback(TimerHandle_t xTimer) {
    Serial.println("Timer expired, moving up fast");
    motor_set_speed(512);
    routineState = MOVE_UP_FAST;
}

static void IRAM_ATTR switch_isr() {
    TickType_t now = xTaskGetTickCountFromISR();
    int newState = digitalRead(SWITCH_PIN);

    // only handle a state change if it has been stable longer than debounce delay
    if (newState != currentState && (now - lastInterruptTick) >= debounceDelayTicks) {
        lastInterruptTick = now;
        currentState = newState;
        Serial.println("Received switch interrupt, state: " + String(currentState == LOW ? "LOW" : "HIGH"));
        if (routineState == MOVE_DOWN && currentState == HIGH) {
            Serial.println("Reached bottom, moving up");
            motor_set_speed(128);
            routineState = MOVE_UP_SLOW;
            if (moveUpTimer != NULL) {
                xTimerReset(moveUpTimer, 0);
            }
        }
    }
}

void routine_init() {
    pinMode(SWITCH_PIN, INPUT_PULLUP);
    currentState = digitalRead(SWITCH_PIN);
    lastInterruptTick = xTaskGetTickCount();
    attachInterrupt(digitalPinToInterrupt(SWITCH_PIN), switch_isr, CHANGE);

    moveUpTimer = xTimerCreate("MoveUpTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0, vTimerCallback);
}

void routine_start() { 
    //Make sure the switch is not currently activated!!
    if (currentState == HIGH) {
        Serial.println("Cannot start routine: switch is currently activated");
        return;
    }

    routineState = MOVE_DOWN;
    Serial.println("Routine started: moving down");
    motor_set_speed(-512);
}
