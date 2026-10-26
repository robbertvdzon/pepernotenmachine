#pragma once
#include <stdint.h>

// Pull motor driven by FastAccelStepper: every speed change goes through an acceleration ramp.
//
// "speed" values are the same signed units as before: -512..512, where 512 = PULL_MOTOR_MAX_FREQ_HZ.
// Positive = up (counting up), negative = down. Positions are in steps.
typedef void (*pull_motor_speed_changed_cb_t)(int32_t speed);

void pull_motor_init(pull_motor_speed_changed_cb_t speedChangedCb);

// Run continuously at the given speed (ramped). 0 = ramped stop.
void pull_motor_set_speed(int32_t speed);

// Move to an absolute step position, cruising at |speed|, ramping up and ramping down to stop exactly on the target.
// Can't reverse a move that is already running: stop first. Returns false if the move was rejected.
bool pull_motor_move_to(int32_t position, int32_t speed);

// Change the cruise speed of a move/run that is in progress, same direction (ramped).
void pull_motor_change_speed(int32_t speed);

// Ramped stop.
void pull_motor_stop();

// Immediate stop without ramp. The current position is redefined as `new_position`.
void pull_motor_hard_stop(int32_t new_position);

int32_t pull_motor_position();
bool pull_motor_is_running();
