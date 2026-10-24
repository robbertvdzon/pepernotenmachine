#pragma once
#include <stdint.h>

enum SequenceState : uint8_t { IDLE, MOVE_DOWN, MOVE_UP_SLOW, MOVE_UP_FAST, ERROR };

typedef void (*sequence_state_cb_t)(uint8_t state);

void sequence_init(sequence_state_cb_t stateCb);
void sequence_start(); //Start the sequence that sets up the laucher to be ready for launching.
void sequence_launch(); //Perform the the actual launch of the pepernoten by triggering the release servo.
void sequence_abort_launch(); //Just stops the release servo from moving automatically back to the closed position after a launch. This is used when the release servo is controlled manually via BLE or button input.