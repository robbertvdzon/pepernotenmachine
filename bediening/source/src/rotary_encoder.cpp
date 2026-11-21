#include "rotary_encoder.h"

#include "config.h"
#include "encoder_decoder.h"
#include "log.h"

static const char* TAG = "ENC";

static QuadratureDecoder decoder;
static volatile int32_t pending_detents = 0;
static portMUX_TYPE enc_mux = portMUX_INITIALIZER_UNLOCKED;

static inline uint8_t read_state() {
    return static_cast<uint8_t>((digitalRead(ROTARY_PIN_CLK) << 1) | digitalRead(ROTARY_PIN_CT));
}

// Runs on every edge of CLK or CT. Kept tiny: decode + accumulate.
static void IRAM_ATTR on_encoder_edge() {
    const int8_t d = decoder.step(read_state());
    if (d != 0) {
        portENTER_CRITICAL_ISR(&enc_mux);
        pending_detents += d;
        portEXIT_CRITICAL_ISR(&enc_mux);
    }
}

void rotary_encoder_init() {
    pinMode(ROTARY_PIN_CLK, INPUT_PULLUP);
    pinMode(ROTARY_PIN_CT, INPUT_PULLUP);
    decoder.reset(read_state());
    pending_detents = 0;
    attachInterrupt(ROTARY_PIN_CLK, on_encoder_edge, CHANGE);
    attachInterrupt(ROTARY_PIN_CT, on_encoder_edge, CHANGE);
    LOGI(TAG, "encoder ready (CLK=%d CT=%d, interrupt driven)", (int)ROTARY_PIN_CLK, (int)ROTARY_PIN_CT);
}

int32_t rotary_encoder_take_delta() {
    portENTER_CRITICAL(&enc_mux);
    const int32_t d = pending_detents;
    pending_detents = 0;
    portEXIT_CRITICAL(&enc_mux);
    return d;
}
