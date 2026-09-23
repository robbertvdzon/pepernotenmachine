#include <Arduino.h>

#include "../include/ready_sensor.h"

namespace {
bool g_last_ready_state = false;

void ready_sensor_callback(bool is_ready) {
    if (is_ready != g_last_ready_state) {
        Serial.print("Ready sensor changed: ");
        Serial.println(is_ready ? "true" : "false");
        g_last_ready_state = is_ready;
    }
}
} // namespace

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("Starting ready sensor");
    ready_sensor_setup(ready_sensor_callback);
}

void loop() {
    delay(10);
}
