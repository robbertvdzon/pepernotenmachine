#include "../include/dispenser.h"
#include "../include/config.h"
#include "../include/led.h"
#include "../include/stepper_engine.h"
#include "../include/storage.h"
#include <Arduino.h>
#include <FastAccelStepper.h>
#include <atomic>

#define STATE_LED LED_2

FastAccelStepper* stepper = NULL;

static std::atomic<uint8_t> dispenseDuration(DISPENSER_DEFAULT_DURATION);
static bool isDispensing = false;
static TimerHandle_t dispenseTimer;
static dispenser_notify_cb_t notify_cb = nullptr;

static uint8_t clamp_duration(uint8_t seconds) {
    if (seconds < 1) seconds = 1;
    if (seconds > DISPENSER_MAX_DURATION) seconds = DISPENSER_MAX_DURATION;
    return seconds;
}

static void vDispenseTimerCallback(TimerHandle_t xTimer) {
    Serial.println("Dispense timer expired, stopping dispenser");
    // stepper->stopMove();
    isDispensing = false;
    led_set_state(STATE_LED, LED_RAMP_DOWN);
    if (notify_cb) notify_cb(0);  // notify stopped
}

void dispenser_init(dispenser_notify_cb_t notifyCb) {
    notify_cb = notifyCb;

    uint8_t savedDuration;
    if (storage_get(STORAGE_KEY_DISPENSER_DURATION, &savedDuration, sizeof(savedDuration), NULL)) {
        dispenseDuration.store(clamp_duration(savedDuration));
    }

    stepper = stepper_engine().stepperConnectToPin(DISPENSER_STEP_PIN);

    if (stepper) {
        stepper->setDirectionPin(DISPENSER_DIR_PIN);
        stepper->setEnablePin(DISPENSER_EN_PIN);
        stepper->setAutoEnable(true);
        stepper->setAcceleration(DISPENSER_ACCEL_INITIAL);
        stepper->setSpeedInHz(DISPENSER_TOP_SPEED_HZ);
    } else {
        Serial.println("Failed to initialize dispenser stepper");
    }

    led_set_state(STATE_LED, LED_OFF);

    // Create one-shot timer for duration
    dispenseTimer = xTimerCreate("DispenseTimer", pdMS_TO_TICKS(1000), pdFALSE, (void*)0, vDispenseTimerCallback);
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

uint8_t dispenser_get_duration(void) {
    return dispenseDuration.load();
}

void dispenser_control(uint8_t command) {
    if (command == 1) {
        const uint8_t duration = dispenseDuration.load();
        // Start dispensing
        if (isDispensing) {
            Serial.println("Dispenser already dispensing, ignoring start");
            return;
        }
        Serial.println("Starting dispenser");
        isDispensing = true;
        led_set_state(STATE_LED, LED_RAMP_UP);

        // Calculate acceleration proportional to duration
        // Accelerate in first DISPENSER_ACCEL_PHASE_PERCENT% of duration, decelerate in last DISPENSER_ACCEL_PHASE_PERCENT%
        // accel = topSpeed / accelTime = DISPENSER_TOP_SPEED_HZ / (DISPENSER_ACCEL_PHASE_PERCENT% * dispenseDuration)
        uint32_t accelTime_ms = duration * DISPENSER_ACCEL_PHASE_PERCENT * 10;  // phase% of duration in milliseconds
        float accel = (accelTime_ms > 0) ? ((float)DISPENSER_TOP_SPEED_HZ / (accelTime_ms / 1000.0f)) : DISPENSER_ACCEL_INITIAL;
        
        Serial.print("Duration: ");
        Serial.print(duration);
        Serial.print("s, Acceleration: ");
        Serial.print((int)accel);
        Serial.println(" steps/s²");
        
        stepper->setAcceleration((uint32_t)accel);
        
        // Reset position so moveTo() calculates from 0 on each cycle
        stepper->setCurrentPosition(0);
        
        // Calculate total steps for the full duration
        // At average speed with accel/decel, this ensures the motion profile fits in dispenseDuration
        uint32_t totalSteps = (uint32_t)DISPENSER_TOP_SPEED_HZ * duration;
        
        // Move for the calculated duration with built-in accel/decel
        stepper->moveTo(totalSteps);
        
        if (dispenseTimer != NULL) {
            xTimerChangePeriod(dispenseTimer, pdMS_TO_TICKS(duration * 1000), 0);
            xTimerStart(dispenseTimer, 0);
        }
        if (notify_cb) notify_cb(1);  // notify started
    } else if (command == 0) {
        // Stop dispensing
        if (!isDispensing) {
            Serial.println("Dispenser not dispensing, ignoring stop");
            return;
        }
        Serial.println("Stopping dispenser");
        stepper->stopMove();
        if (dispenseTimer != NULL) {
            xTimerStop(dispenseTimer, 0);
        }
        isDispensing = false;
        led_set_state(STATE_LED, LED_RAMP_DOWN);
        if (notify_cb) notify_cb(0);
    }
}