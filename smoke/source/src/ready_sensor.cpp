#include "ready_sensor.h"

#include <Arduino.h>
#include <driver/adc.h>
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>

namespace {
constexpr adc1_channel_t READY_SENSOR_CHANNEL = ADC1_CHANNEL_7; // GPIO35 on Adafruit Feather ESP32 V2
constexpr int ADC_MAX_VALUE = 4095;
constexpr int ADC_HALF_VALUE = ADC_MAX_VALUE / 2;

ReadySensorCallback g_ready_sensor_callback = nullptr;
TimerHandle_t g_ready_sensor_timer = nullptr;

void ready_sensor_timer_callback(TimerHandle_t timer) {
    (void)timer;

    if (g_ready_sensor_callback == nullptr) {
        return;
    }

    int raw_value = adc1_get_raw(READY_SENSOR_CHANNEL);
    bool is_ready = raw_value > ADC_HALF_VALUE;
    g_ready_sensor_callback(is_ready);
}
} // namespace

void ready_sensor_setup(ReadySensorCallback callback) {
    g_ready_sensor_callback = callback;

    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(READY_SENSOR_CHANNEL, ADC_ATTEN_DB_12);

    g_ready_sensor_timer = xTimerCreate(
        "readySensorTimer",
        pdMS_TO_TICKS(1000),
        pdTRUE,
        nullptr,
        ready_sensor_timer_callback);

    if (g_ready_sensor_timer != nullptr) {
        xTimerStart(g_ready_sensor_timer, 0);
    }
}
