#include "../include/horn.h"
#include "../include/config.h"
#include <Arduino.h>

static uint32_t lastOnTime = 0;
static bool hornState = false;
static const uint32_t AUTO_OFF_MS = 2000; // auto-off after 2 seconds

void horn_init() {
    pinMode(HORN, OUTPUT);
    digitalWrite(HORN, LOW);
    hornState = false;
    lastOnTime = 0;
}

void horn_set(bool on) {
    hornState = on;
    digitalWrite(HORN, on ? HIGH : LOW);
    if (on) {
        lastOnTime = millis();
    }
}

void horn_update() {
    if (hornState) {
        if (millis() - lastOnTime >= AUTO_OFF_MS) {
            hornState = false;
            digitalWrite(HORN, LOW);
            Serial.println("Horn AUTO OFF");
        }
    }
}

bool horn_is_on() {
    return hornState;
}
