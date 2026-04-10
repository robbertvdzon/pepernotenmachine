#include "../include/config.h"
#include "../include/motor.h"
#include <Arduino.h>

#define MOTOR_SPEED_MAX 512

static bool enabled = true;

void motor_init() {
    pinMode(MOTOR_PUL_PIN, OUTPUT);
    pinMode(MOTOR_DIR_PIN, OUTPUT);
    pinMode(MOTOR_ENA_PIN, OUTPUT);

    // Start disabled to avoid immediate hum and prevent unrelated PWM interactions
    digitalWrite(MOTOR_ENA_PIN, HIGH);
    digitalWrite(MOTOR_DIR_PIN, HIGH);
    digitalWrite(MOTOR_PUL_PIN, LOW);

    // Initialize PWM with default frequency on dedicated channel (channel 7)
    ledcSetup(PWM_CHANNEL, 1000, PWM_RESOLUTION);
    ledcAttachPin(MOTOR_PUL_PIN, PWM_CHANNEL);
    ledcWrite(PWM_CHANNEL, 0);  // start stopped
    enabled = false;
}

void motor_set_speed(int32_t speed) {
    // Stop
    if (speed == 0) {
        Serial.println("Motor stop");
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

void motor_set_direction(bool forward) {
    digitalWrite(MOTOR_DIR_PIN, forward ? LOW : HIGH);
}

void motor_enable(bool on) {
    enabled = on;
    digitalWrite(MOTOR_ENA_PIN, on ? LOW : HIGH);
}
