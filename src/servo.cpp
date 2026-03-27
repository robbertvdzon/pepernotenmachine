#include "../include/servo.h"
#include "../include/config.h"
#include <Arduino.h>
#include <ESP32Servo.h>

static Servo servo;
static uint8_t currentAngle = 0;

void servo_init() {
    // attach servo to configured pin and move to initial position
    servo.setPeriodHertz(50); // explicitly 50Hz refresh for servo
    servo.attach(SERVO_PIN);
    servo.write(currentAngle);
    Serial.print("Servo initialized on pin ");
    Serial.print(SERVO_PIN);
    Serial.print(" at angle ");
    Serial.println(currentAngle);
}

void servo_set_angle(uint8_t angle) {
    if (angle > 180) {
        angle = 180;
    }
    currentAngle = angle;
    servo.write(angle);
    Serial.print("Servo angle set to ");
    Serial.println(angle);
}

uint8_t servo_get_angle() { return currentAngle; }