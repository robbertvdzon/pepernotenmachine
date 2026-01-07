#include "../include/button.h"
#include "../include/config.h"
#include <Arduino.h>

static button_notify_cb_t notify_cb = nullptr;
static int lastReading = HIGH;
static int stableState = HIGH;
static uint32_t lastDebounceTime = 0;
static const uint32_t debounceDelay = 20;

void button_init(button_notify_cb_t cb) {
    notify_cb = cb;
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    lastReading = digitalRead(BUTTON_PIN);
    stableState = lastReading;
    lastDebounceTime = millis();
}

void button_update() {
    int reading = digitalRead(BUTTON_PIN);
    if (reading != lastReading) {
        lastDebounceTime = millis();
        lastReading = reading;
    }
    if ((millis() - lastDebounceTime) > debounceDelay) {
        if (stableState != reading) {
            stableState = reading;
            uint8_t payload = (stableState == HIGH) ? 0 : 1;
            Serial.print("Button state changed, payload: ");
            Serial.println(payload);
            if (notify_cb) notify_cb(payload);
        }
    }
}

uint8_t button_get_state() {
    return (stableState == HIGH) ? 1 : 0;
}
