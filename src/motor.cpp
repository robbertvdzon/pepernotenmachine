#include "../include/config.h"
#include "../include/motor.h"
#include <Arduino.h>

#define MOTOR_SPEED_MAX 512
#define MOTOR_HALF_FAST_US 40
#define MOTOR_HALF_SLOW_US 400

static bool enabled = true;

void motor_init() {
    pinMode(MOTOR_PUL_PIN, OUTPUT);
    pinMode(MOTOR_DIR_PIN, OUTPUT);
    pinMode(MOTOR_ENA_PIN, OUTPUT);

    digitalWrite(MOTOR_ENA_PIN, LOW);
    digitalWrite(MOTOR_DIR_PIN, HIGH);
    digitalWrite(MOTOR_PUL_PIN, LOW);

    // Initialize PWM with default frequency
    ledcSetup(PWM_CHANNEL, 1000, PWM_RESOLUTION);
    ledcAttachPin(MOTOR_PUL_PIN, PWM_CHANNEL);
    ledcWrite(PWM_CHANNEL, 0);  // Start with motor stopped
}

void motor_set_speed(int32_t speed) {
    // Stop
    if (speed == 0) {
        Serial.println("Motor stop");
        ledcWrite(PWM_CHANNEL, 0);
        return;
    }

    // Direction
    if (speed < 0) {
        motor_set_direction(false);
        speed = -speed;
    } else {
        motor_set_direction(true);
    }

    // Clamp magnitude
    if (speed > MOTOR_SPEED_MAX) speed = MOTOR_SPEED_MAX;

    // speed = 1   -> frequency ≈ 24 Hz (slowest)
    // speed = 512 -> frequency = 12500 Hz (fastest)
    uint32_t newFreq = (speed * 12500UL) / MOTOR_SPEED_MAX;

    ledcSetup(PWM_CHANNEL, newFreq, PWM_RESOLUTION);
    ledcWrite(PWM_CHANNEL, PWM_DEFAULT_DUTY);
}

void motor_set_direction(bool forward) {
    digitalWrite(MOTOR_DIR_PIN, forward ? LOW : HIGH);
}

void motor_enable(bool on) {
    enabled = on;
    digitalWrite(MOTOR_ENA_PIN, on ? LOW : HIGH);
}
