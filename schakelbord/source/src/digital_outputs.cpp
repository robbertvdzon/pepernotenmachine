#include "../include/digital_outputs.h"
#include "../include/config.h"
#include "../include/mcp23017.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

static uint8_t outputMask = 0;
static SemaphoreHandle_t lock = NULL;
static outputs_changed_cb_t changedCb = nullptr;

static void apply_pin(uint8_t index, bool on) {
    mcp_set_pin(OUTPUT_MCP_PORT, OUTPUT_MCP_BITS[index], on ? OUTPUT_ACTIVE_HIGH : !OUTPUT_ACTIVE_HIGH);
}

void outputs_init(outputs_changed_cb_t cb) {
    changedCb = cb;
    lock = xSemaphoreCreateMutex();
    outputMask = 0;
    for (uint8_t i = 0; i < OUTPUT_COUNT; i++) apply_pin(i, false);
}

bool outputs_set(uint8_t number, bool on) {
    if (number < 1 || number > OUTPUT_COUNT || lock == NULL) return false;

    xSemaphoreTake(lock, portMAX_DELAY);
    if (on) outputMask |= (uint8_t)(1u << (number - 1));
    else outputMask &= (uint8_t)~(1u << (number - 1));
    apply_pin(number - 1, on);
    uint8_t mask = outputMask;
    xSemaphoreGive(lock);

    LOGF("Output %u -> %s", number, on ? "on" : "off");
    if (changedCb) changedCb(mask);
    return true;
}

void outputs_all_off() {
    if (lock == NULL) return;
    xSemaphoreTake(lock, portMAX_DELAY);
    outputMask = 0;
    for (uint8_t i = 0; i < OUTPUT_COUNT; i++) apply_pin(i, false);
    xSemaphoreGive(lock);
    if (changedCb) changedCb(0);
}

uint8_t outputs_get_mask() {
    return outputMask;
}
