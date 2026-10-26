#pragma once
#include <stdint.h>

enum PreparationState : uint8_t { PREPARATION_IDLE, PREPARATION_MOVE_DOWN, PREPARATION_MOVE_UP_SLOW, PREPARATION_MOVE_UP_FAST, PREPARATION_ERROR };

typedef void (*preparation_state_cb_t)(uint8_t state);

void launcher_prepare_init(preparation_state_cb_t stateCb);
void launcher_prepare();
void launcher_launch();
void launcher_cancel_auto_lock();