#pragma once
#include <stddef.h>
#include <stdint.h>

// Called with ANALOG_INPUT_COUNT values in millivolts when a channel changed noticeably.
typedef void (*analog_inputs_changed_cb_t)(const uint16_t* millivolts, size_t count);

void analog_inputs_init(analog_inputs_changed_cb_t cb);

// Copies ANALOG_INPUT_COUNT values (mV) into `out`.
void analog_inputs_get(uint16_t* out);

// Shared helper: average of `samples` ADC readings in millivolts.
uint16_t adc_read_mv_averaged(uint8_t pin, uint8_t samples);
