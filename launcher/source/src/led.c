#include "../include/led.h"
#include "../include/config.h"

#include <Arduino.h>
#include <math.h>
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>

#define LED_TIMER_PERIOD_MS 10
#define LED_FLASH_PERIOD_MS 500
#define LED_PULSE_PERIOD_MS 2000

static const int led_pins[LED_COUNT] = {
    LED_1_PIN,
    LED_2_PIN,
    LED_3_PIN
};

static volatile led_state_t led_states[LED_COUNT] = {
    LED_OFF,
    LED_OFF,
    LED_OFF
};

static TimerHandle_t led_timer;
static uint32_t animation_time_ms;

static uint8_t led_duty_for_state(led_state_t state) {
    uint32_t phase_ms;
    uint8_t brightness;

    switch (state) {
        case LED_ON:
            brightness = 255;
            break;
        case LED_FLASH:
            phase_ms = animation_time_ms % LED_FLASH_PERIOD_MS;
            brightness = phase_ms < LED_FLASH_PERIOD_MS / 2 ? 255 : 0;
            break;
        case LED_PULSE:
            phase_ms = animation_time_ms % LED_PULSE_PERIOD_MS;
            brightness = (uint8_t)(127.5f + 127.5f * sinf(
                2.0f * PI * (float)phase_ms / LED_PULSE_PERIOD_MS));
            break;
        case LED_OFF:
        default:
            brightness = 0;
            break;
    }

    // The LEDs sink current, so a low output turns an LED on.
    return 255 - brightness;
}

static void led_timer_callback(TimerHandle_t timer) {
    uint8_t led_index;
    (void)timer;

    animation_time_ms += LED_TIMER_PERIOD_MS;
    for (led_index = 0; led_index < LED_COUNT; ++led_index) {
        analogWrite(led_pins[led_index], led_duty_for_state(led_states[led_index]));
    }
}

void led_init(void) {
    uint8_t led_index;

    animation_time_ms = 0;
    for (led_index = 0; led_index < LED_COUNT; ++led_index) {
        pinMode(led_pins[led_index], OUTPUT);
        analogWrite(led_pins[led_index], 255);
    }

    led_timer = xTimerCreate(
        "LedTimer",
        pdMS_TO_TICKS(LED_TIMER_PERIOD_MS),
        pdTRUE,
        NULL,
        led_timer_callback);
    if (led_timer != NULL) {
        xTimerStart(led_timer, 0);
    }
}

void led_set_state(led_id_t led, led_state_t state) {
    if (led >= LED_COUNT) {
        return;
    }

    led_states[led] = state;
}
