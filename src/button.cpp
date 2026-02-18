#include "../include/button.h"
#include "../include/config.h"
#include <Arduino.h>

static button_notify_cb_t notify_cb = nullptr;
static volatile int currentState = HIGH;
static volatile uint32_t lastInterruptTime = 0;
static const uint32_t debounceDelay = 20;  // ms

// ISR: fires on pin change, handles debouncing
static void IRAM_ATTR button_isr() {
    uint32_t now = millis();
    // Debounce: ignore interrupts within 20ms of last one
    if (now - lastInterruptTime < debounceDelay) {
        return;
    }
    lastInterruptTime = now;
    
    int newState = digitalRead(BUTTON_PIN);
    if (newState != currentState) {
        currentState = newState;
        uint8_t payload = (currentState == HIGH) ? 0 : 1;
        Serial.print("Button ");
        Serial.println(payload ? "pressed" : "released");
        if (notify_cb) notify_cb(payload);
    }
}

void button_init(button_notify_cb_t cb) {
    notify_cb = cb;
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    currentState = digitalRead(BUTTON_PIN);
    lastInterruptTime = millis();
    
    // Attach interrupt on CHANGE (rising or falling edge)
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), button_isr, CHANGE);
}

uint8_t button_get_state() {
    return (currentState == HIGH) ? 1 : 0;
}
