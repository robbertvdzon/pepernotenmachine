#pragma once
#include <stdint.h>

// Simple C-style API for motor control
void motor_init();
void motor_set_delay_us(uint32_t halfPeriodUs);
void motor_set_direction(bool forward);
void motor_enable(bool on);
