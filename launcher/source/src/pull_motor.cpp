#include "../include/config.h"
#include "../include/pull_motor.h"
#include "../include/stepper_engine.h"
#include <Arduino.h>

static FastAccelStepper* motor = NULL;

// speed 1 -> slowest, speed 512 -> PULL_MOTOR_MAX_FREQ_HZ (sign ignored)
static uint32_t speed_to_hz(int32_t speed) {
    if (speed < 0) speed = -speed;
    if (speed > PULL_MOTOR_SPEED_MAX) speed = PULL_MOTOR_SPEED_MAX;
    uint32_t hz = (uint32_t)speed * PULL_MOTOR_MAX_FREQ_HZ / PULL_MOTOR_SPEED_MAX;
    return hz > 0 ? hz : 1;
}

void pull_motor_init() {
    // Keep the driver disabled (ENA is active low) until the stepper is configured
    pinMode(PULL_MOTOR_ENA_PIN, OUTPUT);
    digitalWrite(PULL_MOTOR_ENA_PIN, HIGH);

    motor = stepper_engine().stepperConnectToPin(PULL_MOTOR_PUL_PIN);
    if (!motor) {
        Serial.println("Failed to initialize pull motor stepper");
        return;
    }

    // DIR LOW = counting up ("up"), DIR HIGH = counting down ("down"), same as the old pull_motor_set_direction()
    motor->setDirectionPin(PULL_MOTOR_DIR_PIN, false);

    // ENA is active low. Auto enable: driver is enabled while moving and disabled a while after the last step.
    motor->setEnablePin(PULL_MOTOR_ENA_PIN);
    motor->setAutoEnable(true);
    motor->setDelayToDisable(PULL_MOTOR_DISABLE_DELAY_MS);

    motor->setAcceleration(PULL_MOTOR_ACCELERATION);
    motor->setSpeedInHz(speed_to_hz(PULL_MOTOR_SPEED_MAX));
}

void pull_motor_set_speed(int32_t speed) {
    if (!motor) return;

    if (speed == 0) {
        Serial.println("Pull motor stop");
        motor->stopMove();
        return;
    }

    motor->setSpeedInHz(speed_to_hz(speed));
    if (speed > 0) {
        motor->runForward();
    } else {
        motor->runBackward();
    }
}

bool pull_motor_move_to(int32_t position, int32_t speed) {
    if (!motor) return false;

    motor->setSpeedInHz(speed_to_hz(speed));
    auto result = motor->moveTo(position);
    if (result != MOVE_OK) {
        Serial.printf("Pull motor move rejected, code %d\n", (int)result);
        return false;
    }
    return true;
}

void pull_motor_change_speed(int32_t speed) {
    if (!motor) return;

    motor->setSpeedInHz(speed_to_hz(speed));
    motor->applySpeedAcceleration();
}

void pull_motor_stop() { pull_motor_set_speed(0); }

void pull_motor_hard_stop(int32_t new_position) {
    if (!motor) return;
    motor->forceStopAndNewPosition(new_position);
}

int32_t pull_motor_position() { return motor ? motor->getCurrentPosition() : 0; }

bool pull_motor_is_running() { return motor ? motor->isRunning() : false; }
