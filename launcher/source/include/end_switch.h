#pragma once
#include <stdint.h>

typedef void (*end_switch_notify_cb_t)(uint8_t payload);

void end_switch_init(end_switch_notify_cb_t cb);
uint8_t end_switch_get_state();
bool end_switch_is_activated();