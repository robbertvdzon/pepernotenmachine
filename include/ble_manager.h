#pragma once
#include <stdint.h>

typedef void (*motor_write_cb_t)(uint32_t halfPeriodUs);
typedef void (*horn_write_cb_t)(bool on);

void ble_init(motor_write_cb_t motorCb, horn_write_cb_t hornCb);
bool ble_is_connected();
void ble_notify_button(uint8_t payload);
// Notify that LED crossfade completed
void ble_notify_led_done();
