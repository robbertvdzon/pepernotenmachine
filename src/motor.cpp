#include "../include/config.h"
#include "../include/motor.h"
#include <Arduino.h>

static bool enabled = true;

void motor_init() {
    pinMode(PULL_MOTOR_PUL_PIN, OUTPUT);
    pinMode(PULL_MOTOR_DIR_PIN, OUTPUT);
    pinMode(PULL_MOTOR_ENA_PIN, OUTPUT);

    // Start disabled to avoid immediate hum and prevent unrelated PWM interactions
    digitalWrite(PULL_MOTOR_ENA_PIN, HIGH);
    digitalWrite(PULL_MOTOR_DIR_PIN, HIGH);
    digitalWrite(PULL_MOTOR_PUL_PIN, LOW);

    // Initialize PWM with default frequency on dedicated channel (channel 7)
    ledcSetup(PWM_CHANNEL, MOTOR_DEFAULT_FREQ, PWM_RESOLUTION);
    ledcAttachPin(PULL_MOTOR_PUL_PIN, PWM_CHANNEL);
    ledcWrite(PWM_CHANNEL, 0);  // start stopped
    enabled = false;
}

void motor_set_speed(int32_t speed) {
    // Stop
    if (speed == 0) {
        Serial.println("Pull motor stop");
        ledcWrite(PWM_CHANNEL, 0);
        motor_enable(false);
        return;
    }

    // Ensure motor is enabled before moving
    motor_enable(true);

    // Direction
    if (speed < 0) {
        motor_set_direction(false);
        speed = -speed;
    } else {
        motor_set_direction(true);
    }

    // Clamp magnitude
    if (speed > MOTOR_SPEED_MAX) speed = MOTOR_SPEED_MAX;

    // speed = 1   -> frequency ≈ 31 Hz (slowest)
    // speed = 512 -> frequency = 16000 Hz (fastest)
    uint32_t newFreq = (speed * 16000UL) / MOTOR_SPEED_MAX;

    ledcWriteTone(PWM_CHANNEL, newFreq);
}

void motor_set_direction(bool forward) { digitalWrite(PULL_MOTOR_DIR_PIN, forward ? LOW : HIGH); }

void motor_enable(bool on) {
    enabled = on;
    digitalWrite(PULL_MOTOR_ENA_PIN, on ? LOW : HIGH);
}
