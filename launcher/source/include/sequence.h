#pragma once
#include <stdint.h>

enum SequenceState : uint8_t { IDLE, MOVE_DOWN, MOVE_UP_SLOW, MOVE_UP_FAST, ERROR };

typedef void (*sequence_state_cb_t)(uint8_t state);

void sequence_init(sequence_state_cb_t stateCb);
void sequence_start();