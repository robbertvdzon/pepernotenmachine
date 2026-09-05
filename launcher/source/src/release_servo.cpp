#include "../include/release_servo.h"
#include "../include/config.h"
#include <Arduino.h>
#include <ESP32Servo.h>

static Servo servo;
static uint8_t currentAngle = 0;

void release_servo_init() {
    servo.attach(RELEASE_SERVO_PIN);
    servo.write(currentAngle);
}

void release_servo_set_angle(uint8_t angle) {
    if (angle > SERVO_MAX_ANGLE) {
        angle = SERVO_MAX_ANGLE;
    }
    currentAngle = angle;
    servo.write(angle);
    Serial.print("Servo angle set to ");
    Serial.println(angle);
}

uint8_t release_servo_get_angle() { return currentAngle; }