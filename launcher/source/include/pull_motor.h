#pragma once
#include <stdint.h>

void pull_motor_init();
void pull_motor_set_speed(int32_t speed);
void pull_motor_set_direction(bool forward);
void pull_motor_enable(bool on);
