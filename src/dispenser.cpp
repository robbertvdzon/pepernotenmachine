#include "../include/dispenser.h"
#include "../include/config.h"
#include <Arduino.h>
#include <FastAccelStepper.h>

FastAccelStepperEngine engine = FastAccelStepperEngine();
FastAccelStepper* stepper = NULL;

static uint8_t dispenseDuration = DISPENSER_DEFAULT_DURATION;
static bool isDispensing = false;
static TimerHandle_t dispenseTimer;
static dispenser_notify_cb_t notify_cb = nullptr;

static void vDispenseTimerCallback(TimerHandle_t xTimer) {
    Serial.println("Dispense timer expired, stopping dispenser");
    // stepper->stopMove();
    isDispensing = false;
    if (notify_cb) notify_cb(0);  // notify stopped
}

void dispenser_init(dispenser_notify_cb_t notifyCb) {
    notify_cb = notifyCb;
    engine.init();

    stepper = engine.stepperConnectToPin(DISPENSER_STEP_PIN);

    if (stepper) {
        stepper->setAcceleration(DISPENSER_ACCEL_INITIAL);
        stepper->setSpeedInHz(DISPENSER_TOP_SPEED_HZ);
    } else {
        Serial.println("Failed to initialize dispenser stepper");
    }

    // Create one-shot timer for duration
    dispenseTimer = xTimerCreate("DispenseTimer", pdMS_TO_TICKS(1000), pdFALSE, (void*)0, vDispenseTimerCallback);
}

void dispenser_set_duration(uint8_t seconds) {
    if (seconds < 1) seconds = 1;
    if (seconds > DISPENSER_MAX_DURATION) seconds = DISPENSER_MAX_DURATION;
    dispenseDuration = seconds;
    Serial.print("Dispenser duration set to ");
    Serial.print(seconds);
    Serial.println(" seconds");
}

void dispenser_control(uint8_t command) {
    if (command == 1) {
        // Start dispensing
        if (isDispensing) {
            Serial.println("Dispenser already dispensing, ignoring start");
            return;
        }
        Serial.println("Starting dispenser");
        isDispensing = true;
        
        // Calculate acceleration proportional to duration
        // Accelerate in first DISPENSER_ACCEL_PHASE_PERCENT% of duration, decelerate in last DISPENSER_ACCEL_PHASE_PERCENT%
        // accel = topSpeed / accelTime = DISPENSER_TOP_SPEED_HZ / (DISPENSER_ACCEL_PHASE_PERCENT% * dispenseDuration)
        uint32_t accelTime_ms = dispenseDuration * DISPENSER_ACCEL_PHASE_PERCENT * 10;  // phase% of duration in milliseconds
        float accel = (accelTime_ms > 0) ? ((float)DISPENSER_TOP_SPEED_HZ / (accelTime_ms / 1000.0f)) : DISPENSER_ACCEL_INITIAL;
        
        Serial.print("Duration: ");
        Serial.print(dispenseDuration);
        Serial.print("s, Acceleration: ");
        Serial.print((int)accel);
        Serial.println(" steps/s²");
        
        stepper->setAcceleration((uint32_t)accel);
        
        // Reset position so moveTo() calculates from 0 on each cycle
        stepper->setCurrentPosition(0);
        
        // Calculate total steps for the full duration
        // At average speed with accel/decel, this ensures the motion profile fits in dispenseDuration
        uint32_t totalSteps = (uint32_t)DISPENSER_TOP_SPEED_HZ * dispenseDuration;
        
        // Move for the calculated duration with built-in accel/decel
        stepper->moveTo(totalSteps);
        
        if (dispenseTimer != NULL) {
            xTimerChangePeriod(dispenseTimer, pdMS_TO_TICKS(dispenseDuration * 1000), 0);
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
        if (notify_cb) notify_cb(0);
    }
}