#pragma once
#include <stdint.h>

typedef void (*pull_motor_write_cb_t)(int32_t value);
typedef void (*release_servo_write_cb_t)(uint8_t angle);
typedef void (*sequence_write_cb_t)();
typedef void (*dispenser_duration_write_cb_t)(uint8_t seconds);
typedef void (*dispenser_start_write_cb_t)(uint8_t command);

void ble_init(pull_motor_write_cb_t motorCb, release_servo_write_cb_t servoCb, sequence_write_cb_t sequenceCb, dispenser_duration_write_cb_t dispenserDurationCb, dispenser_start_write_cb_t dispenserStartCb);
bool ble_is_connected();
void ble_notify_button(uint8_t payload);
void ble_notify_dispenser_start(uint8_t state);
