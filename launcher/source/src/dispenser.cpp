#include "../include/dispenser.h"
#include "../include/config.h"
#include "../include/led.h"
#include "../include/stepper_engine.h"
#include "../include/storage.h"
#include <Arduino.h>
#include <FastAccelStepper.h>
#include <atomic>

#define STATE_LED LED_2

// Poll interval for detecting that the move has finished
static const uint32_t DISPENSER_POLL_MS = 20;
// Don't trust isRunning() during the first moments after starting a move
static const uint32_t DISPENSER_START_GRACE_MS = 100;
// Safety net: force a stop when the move takes this much longer than the duration
static const uint32_t DISPENSER_TIMEOUT_MARGIN_MS = 2000;
// The position must never go below this (we only move forward from 0)
static const int32_t DISPENSER_REVERSE_TOLERANCE_STEPS = 50;

FastAccelStepper* stepper = NULL;

static std::atomic<uint8_t> dispenseDuration(DISPENSER_DEFAULT_DURATION);
static std::atomic<bool> isDispensing(false);
static TimerHandle_t pollTimer = nullptr;
static uint32_t dispenseStartMs = 0;
static uint32_t dispenseLimitMs = 0;
static dispenser_notify_cb_t notify_cb = nullptr;

static uint8_t clamp_duration(uint8_t seconds) {
    if (seconds < 1) seconds = 1;
    if (seconds > DISPENSER_MAX_DURATION) seconds = DISPENSER_MAX_DURATION;
    return seconds;
}

// Single place where a dispense cycle ends (finished, stopped or aborted).
// The exchange() makes sure we only notify once per cycle.
static void dispenser_finished(const char* reason) {
    if (pollTimer != nullptr) xTimerStop(pollTimer, 0);
    if (!isDispensing.exchange(false)) return;
    Serial.println(reason);
    led_set_state(STATE_LED, LED_RAMP_DOWN);
    if (notify_cb) notify_cb(0);  // notify stopped
}

// Runs every DISPENSER_POLL_MS while dispensing: detects the end of the move
static void vPollTimerCallback(TimerHandle_t xTimer) {
    if (!isDispensing.load()) {
        xTimerStop(xTimer, 0);
        return;
    }

    const uint32_t elapsed = millis() - dispenseStartMs;
    if (elapsed < DISPENSER_START_GRACE_MS) return;

    const int32_t pos = stepper->getCurrentPosition();

    if (!stepper->isRunning()) {
        Serial.print("Final position: ");
        Serial.println(pos);
        dispenser_finished("Dispenser finished");
    } else if (pos < -DISPENSER_REVERSE_TOLERANCE_STEPS) {
        // The library itself is driving backwards: abort immediately
        stepper->forceStopAndNewPosition(0);
        Serial.print("Wrong direction detected, position ");
        Serial.println(pos);
        dispenser_finished("Dispenser aborted: wrong direction");
    } else if (elapsed > dispenseLimitMs) {
        stepper->forceStopAndNewPosition(0);
        dispenser_finished("Dispenser timeout, forced stop");
    }
}

void dispenser_init(dispenser_notify_cb_t notifyCb) {
    notify_cb = notifyCb;

    uint8_t savedDuration;
    if (storage_get(STORAGE_KEY_DISPENSER_DURATION, &savedDuration, sizeof(savedDuration), NULL)) {
        Serial.print("Read dispenser duration from storage (now set to ");
        Serial.print(savedDuration);
        Serial.println(" seconds)");
        dispenseDuration.store(clamp_duration(savedDuration));
    }

    stepper = stepper_engine().stepperConnectToPin(DISPENSER_STEP_PIN);

    if (stepper) {
        stepper->setDirectionPin(DISPENSER_DIR_PIN);
        stepper->setEnablePin(DISPENSER_EN_PIN);
        stepper->setAutoEnable(true);
        stepper->setSpeedInHz(DISPENSER_TOP_SPEED_HZ);
        stepper->setAcceleration(DISPENSER_TOP_SPEED_HZ * 1000UL / DISPENSER_RAMP_TIME_MS);
    } else {
        Serial.println("Failed to initialize dispenser stepper");
    }

    led_set_state(STATE_LED, LED_OFF);

    // Periodic timer, only running while a dispense cycle is active
    pollTimer = xTimerCreate("DispensePoll", pdMS_TO_TICKS(DISPENSER_POLL_MS), pdTRUE, (void*)0, vPollTimerCallback);
}

void dispenser_set_duration(uint8_t seconds) {
    seconds = clamp_duration(seconds);
    dispenseDuration.store(seconds);
    if (!storage_set(STORAGE_KEY_DISPENSER_DURATION, &seconds, sizeof(seconds))) {
        Serial.println("Failed to queue dispenser duration for saving");
    }
    Serial.print("Dispenser duration set to ");
    Serial.print(seconds);
    Serial.println(" seconds");
}

uint8_t dispenser_get_duration(void) { return dispenseDuration.load(); }

void dispenser_control(uint8_t command) {
    if (command == 1) {
        if (stepper == nullptr) {
            Serial.println("Dispenser stepper not initialised, ignoring start");
            return;
        }

        bool expected = false;
        if (!isDispensing.compare_exchange_strong(expected, true)) {
            Serial.println("Dispenser already dispensing, ignoring start");
            return;
        }

        const uint32_t totalMs = (uint32_t)dispenseDuration.load() * 1000UL;
        const uint32_t topSpeed = DISPENSER_TOP_SPEED_HZ;

        // Trapezoid profile that takes exactly totalMs:
        //   ramp up for rampMs, cruise, ramp down for rampMs.
        //   accel = topSpeed / rampMs
        //   steps = topSpeed * (totalMs - rampMs)   (two half-ramps equal one full ramp of distance)
        // The ramp is a fixed time (not a percentage of the duration) so a short ramp
        // keeps the motor from lingering in the low-speed resonance range.
        // For very short durations the ramp is limited to 40% of the total time.
        uint32_t rampMs = DISPENSER_RAMP_TIME_MS;
        const uint32_t maxRampMs = totalMs * 2 / 5;
        if (rampMs > maxRampMs) rampMs = maxRampMs;
        if (rampMs < 1) rampMs = 1;

        const uint32_t accel = (uint32_t)((uint64_t)topSpeed * 1000ULL / rampMs);
        const uint32_t totalSteps = (uint32_t)((uint64_t)topSpeed * (totalMs - rampMs) / 1000ULL);

        Serial.print("Starting dispenser: ");
        Serial.print(totalMs / 1000);
        Serial.print("s, ");
        Serial.print(topSpeed);
        Serial.print(" Hz, accel ");
        Serial.print(accel);
        Serial.print(" steps/s², ");
        Serial.print(totalSteps);
        Serial.println(" steps");

        stepper->setSpeedInHz(topSpeed);
        stepper->setAcceleration(accel);
        stepper->setCurrentPosition(0);  // motor is stopped here, so this is safe
        Serial.print("Position before move: ");
        Serial.println(stepper->getCurrentPosition());

        // Relative move: the direction depends only on the sign of totalSteps,
        // not on where the library thinks the current position is.
        const int8_t result = (int8_t)stepper->move((int32_t)totalSteps);
        if (result < 0) {
            Serial.print("moveTo failed with code ");
            Serial.println(result);
            isDispensing.store(false);
            return;
        }

        led_set_state(STATE_LED, LED_RAMP_UP);
        dispenseStartMs = millis();
        dispenseLimitMs = totalMs + DISPENSER_TIMEOUT_MARGIN_MS;
        if (pollTimer != nullptr) xTimerStart(pollTimer, 0);
        if (notify_cb) notify_cb(1);  // notify started
    } else if (command == 0) {
        if (!isDispensing.load()) {
            Serial.println("Dispenser not dispensing, ignoring stop");
            return;
        }
        Serial.println("Stopping dispenser");
        // Decelerates with the configured acceleration; the poll timer sees
        // that the motor has stopped and sends the 'stopped' notification.
        stepper->stopMove();
    }
}