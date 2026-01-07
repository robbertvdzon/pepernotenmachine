#include "../include/config.h"
#include "../include/motor.h"
#include <Arduino.h>

static uint32_t currentDelayUs = 50; // half period in microseconds
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

    ledcSetup(PWM_CHANNEL, initFreq, PWM_RESOLUTION);
    ledcAttachPin(MOTOR_PUL_PIN, PWM_CHANNEL);
    ledcWrite(PWM_CHANNEL, PWM_DEFAULT_DUTY);
}

void motor_set_delay_us(uint32_t halfPeriodUs) {
    if (halfPeriodUs == 0) return;
    currentDelayUs = halfPeriodUs;
    uint32_t newFreq = 500000UL / currentDelayUs;
    if (newFreq < 1) newFreq = 1;
    if (newFreq > 40000) newFreq = 40000;
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
