#pragma once
#include <stdint.h>

// C-style callback for button notifications: payload 0=pressed,1=released
typedef void (*button_notify_cb_t)(uint8_t payload);

void button_init(button_notify_cb_t cb);
uint8_t button_get_state();
