#include "../include/config.h"
#include "../include/motor.h"
#include <Arduino.h>

#define MOTOR_SPEED_MAX 512
#define MOTOR_HALF_FAST_US 40
#define MOTOR_HALF_SLOW_US 400

static uint32_t currentDelayUs = 0; // half period in microseconds
static bool enabled = true;

void motor_init() {
    pinMode(MOTOR_PUL_PIN, OUTPUT);
    pinMode(MOTOR_DIR_PIN, OUTPUT);
    pinMode(MOTOR_ENA_PIN, OUTPUT);

    digitalWrite(MOTOR_ENA_PIN, LOW);
    digitalWrite(MOTOR_DIR_PIN, HIGH);
    digitalWrite(MOTOR_PUL_PIN, LOW);

    // Configure initial PWM frequency based on currentDelayUs
    uint32_t initFreq = 1;
    if (currentDelayUs != 0) {
        initFreq = 500000UL / currentDelayUs;
    }
    if (initFreq < 1) initFreq = 1;
    if (initFreq > 40000) initFreq = 40000;

    ledcAttachPin(MOTOR_PUL_PIN, PWM_CHANNEL);
}

void motor_set_speed(int32_t speed) {
    Serial.print("Motor speed (int): ");
    Serial.println(speed);

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

    /*
     * Map:
     * speed = 512 -> halfPeriodUs = 40
     * speed =   1 -> halfPeriodUs ≈ 399
     */
    uint32_t range = MOTOR_HALF_SLOW_US - MOTOR_HALF_FAST_US;

    uint32_t halfPeriodUs = MOTOR_HALF_SLOW_US - ((uint32_t)speed * range) / MOTOR_SPEED_MAX;

    // Absolute safety clamp
    if (halfPeriodUs < MOTOR_HALF_FAST_US) halfPeriodUs = MOTOR_HALF_FAST_US;

    currentDelayUs = halfPeriodUs;

    uint32_t newFreq = 500000UL / halfPeriodUs;
    if (newFreq < 1) newFreq = 1;
    if (newFreq > 40000) newFreq = 40000;

    Serial.print("Half period (us): ");
    Serial.println(halfPeriodUs);

    ledcSetup(PWM_CHANNEL, newFreq, PWM_RESOLUTION);
    ledcWrite(PWM_CHANNEL, PWM_DEFAULT_DUTY);
}

void motor_set_direction(bool forward) {
    digitalWrite(MOTOR_DIR_PIN, forward ? LOW : HIGH); // maintain original inversion
}

void motor_enable(bool on) {
    enabled = on;
    digitalWrite(MOTOR_ENA_PIN, on ? LOW : HIGH);
}
