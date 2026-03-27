#pragma once
#include <stdint.h>

typedef void (*motor_write_cb_t)(int32_t value);
typedef void (*horn_write_cb_t)(bool on);
typedef void (*servo_write_cb_t)(uint8_t angle);
typedef void (*routine_write_cb_t)();

void ble_init(motor_write_cb_t motorCb, horn_write_cb_t hornCb, servo_write_cb_t servoCb, routine_write_cb_t routineCb);
bool ble_is_connected();
void ble_notify_button(uint8_t payload);
void ble_notify_led_crossfade_started();
void ble_notify_led_crossfade_done();
