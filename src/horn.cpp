#include "../include/horn.h"
#include "../include/config.h"
#include <Arduino.h>
#include <freertos/timers.h>

static bool hornState = false;
static TimerHandle_t autoOffTimer = nullptr;
static const uint32_t AUTO_OFF_MS = 2000;  // auto-off after 2 seconds

// FreeRTOS timer callback: auto-off the horn
static void auto_off_timer_callback(TimerHandle_t xTimer) {
    hornState = false;
    digitalWrite(HORN_PIN, LOW);
    Serial.println("Horn AUTO OFF");
}

void horn_init() {
    pinMode(HORN_PIN, OUTPUT);
    digitalWrite(HORN_PIN, LOW);
    hornState = false;

    // Create FreeRTOS timer (one-shot: auto reload = false)
    autoOffTimer = xTimerCreate(
        "horn_auto_off",           // timer name
        pdMS_TO_TICKS(AUTO_OFF_MS),  // timeout in ticks (2000ms)
        pdFALSE,                      // don't auto-reload (one-shot)
        nullptr,                      // timer id (unused)
        auto_off_timer_callback       // callback function
    );
}

void horn_set(bool on) {
    hornState = on;
    digitalWrite(HORN_PIN, on ? HIGH : LOW);
    Serial.print("Horn");
    if (on) {
        Serial.println(" ON");
        // Start auto-off timer (one-shot)
        xTimerStart(autoOffTimer, 0);
    } else {
        Serial.println(" OFF");
        // Stop the timer if horn turned off manually
        xTimerStop(autoOffTimer, 0);
    }
}

bool horn_is_on() {
    return hornState;
}
