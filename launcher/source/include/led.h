#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LED_1 = 0,
    LED_2,
    LED_3,
    LED_COUNT
} led_id_t;

typedef enum {
    LED_OFF = 0,
    LED_ON,
    LED_FLASH,
    LED_PULSE
} led_state_t;

void led_init(void);
void led_set_state(led_id_t led, led_state_t state);

#ifdef __cplusplus
}
#endif
