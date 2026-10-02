#pragma once
#include <stdint.h>

// Called when the fog machine state changed (FOG_STATE_*).
typedef void (*fog_state_cb_t)(uint8_t state);
// Called when the fogging session starts/stops: 1 = session active, 0 = stopped.
// Also called with the unchanged value when a start request was rejected.
typedef void (*fog_session_cb_t)(uint8_t active);
// Called with the (clamped) burst duration in seconds after it changed.
typedef void (*fog_duration_cb_t)(uint8_t seconds);

void fog_init(fog_state_cb_t stateCb, fog_session_cb_t sessionCb, fog_duration_cb_t durationCb);

// Request the start of fogging. Only accepted when the state is READY. Non-blocking (queued).
void fog_start();
// Stop fogging. Non-blocking (queued).
void fog_stop();

// Set the burst duration in seconds (clamped to FOG_BURST_MIN/MAX_SECONDS).
void fog_set_duration(uint8_t seconds);

uint8_t fog_get_state();
uint8_t fog_get_session();
uint8_t fog_get_duration();
