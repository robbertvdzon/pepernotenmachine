#include "../include/release_servo.h"
#include "../include/config.h"
#include "../include/led.h"
#include <Arduino.h>
#include <ESP32Servo.h>

#define STATE_LED LED_3

static Servo servo;
static uint8_t currentAngle = 0;

void release_servo_init() {
    servo.attach(RELEASE_SERVO_PIN);
    servo.write(currentAngle);
    led_set_state(STATE_LED, LED_OFF);
}

void release_servo_set_angle(uint8_t angle) {
    if (angle > SERVO_MAX_ANGLE) {
        angle = SERVO_MAX_ANGLE;
    }
    currentAngle = angle;
    servo.write(angle);
    led_set_state(STATE_LED, LED_ON);
    led_set_state(STATE_LED, LED_RAMP_DOWN);
    Serial.print("Servo angle set to ");
    Serial.print(angle);
    Serial.println(" degrees");
}

uint8_t release_servo_get_angle() { return currentAngle; }