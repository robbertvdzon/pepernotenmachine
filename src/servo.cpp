#include "../include/servo.h"
#include "../include/config.h"
#include <Arduino.h>
#include <ESP32Servo.h>

static Servo servo;
static uint8_t currentAngle = 0;

void servo_init() {
    servo.attach(SERVO_PIN);
    servo.write(currentAngle);
}

void servo_set_angle(uint8_t angle) {
    if (angle > SERVO_MAX_ANGLE) {
        angle = SERVO_MAX_ANGLE;
    }
    currentAngle = angle;
    servo.write(angle);
    Serial.print("Servo angle set to ");
    Serial.println(angle);
}

uint8_t servo_get_angle() { return currentAngle; }