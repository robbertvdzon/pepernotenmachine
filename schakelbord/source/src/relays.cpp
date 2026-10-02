#include "../include/relays.h"
#include "../include/config.h"
#include "../include/mcp23017.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/timers.h>

struct Relay {
    uint8_t state;
    uint16_t intervalMs;
    bool level;  // Logical level: true = relay energized
    TimerHandle_t timer;
};

static Relay relays[RELAY_COUNT];
static SemaphoreHandle_t lock = NULL;
static relays_changed_cb_t changedCb = nullptr;

static void apply_pin(uint8_t index) {
    bool pinLevel = relays[index].level ? RELAY_ACTIVE_HIGH : !RELAY_ACTIVE_HIGH;
    mcp_set_pin(RELAY_MCP_PORT, RELAY_MCP_BITS[index], pinLevel);
}

// Blink timer: toggles the relay each period. Runs in the FreeRTOS timer service task.
static void blink_cb(TimerHandle_t timer) {
    uint8_t index = (uint8_t)(uintptr_t)pvTimerGetTimerID(timer);
    if (index >= RELAY_COUNT) return;
    if (xSemaphoreTake(lock, pdMS_TO_TICKS(RELAY_LOCK_TIMEOUT_MS)) != pdTRUE) return;
    if (relays[index].state == RELAY_STATE_BLINK) {
        relays[index].level = !relays[index].level;
        apply_pin(index);
    }
    xSemaphoreGive(lock);
}

// Caller must hold the lock.
static void set_locked(uint8_t index, uint8_t state, uint16_t intervalMs) {
    Relay& r = relays[index];
    switch (state) {
        case RELAY_STATE_OFF:
            xTimerStop(r.timer, 0);
            r.level = false;
            break;
        case RELAY_STATE_ON:
            xTimerStop(r.timer, 0);
            r.level = true;
            break;
        case RELAY_STATE_BLINK:
            if (intervalMs == 0) intervalMs = RELAY_BLINK_DEFAULT_MS;
            if (intervalMs < RELAY_BLINK_MIN_MS) intervalMs = RELAY_BLINK_MIN_MS;
            if (intervalMs > RELAY_BLINK_MAX_MS) intervalMs = RELAY_BLINK_MAX_MS;
            r.level = true;  // Start with the relay on
            r.intervalMs = intervalMs;
            // Changes the period and (re)starts the timer
            xTimerChangePeriod(r.timer, pdMS_TO_TICKS(intervalMs), 0);
            break;
        default:
            return;
    }
    r.state = state;
    if (state != RELAY_STATE_BLINK) r.intervalMs = 0;
    apply_pin(index);
}

static void snapshot_locked(uint8_t* out) {
    for (uint8_t i = 0; i < RELAY_COUNT; i++) out[i] = relays[i].state;
}

void relays_init(relays_changed_cb_t cb) {
    changedCb = cb;
    lock = xSemaphoreCreateMutex();
    for (uint8_t i = 0; i < RELAY_COUNT; i++) {
        relays[i].state = RELAY_STATE_OFF;
        relays[i].intervalMs = 0;
        relays[i].level = false;
        relays[i].timer = xTimerCreate("relay_blink", pdMS_TO_TICKS(RELAY_BLINK_DEFAULT_MS), pdTRUE,
                                       (void*)(uintptr_t)i, blink_cb);
        apply_pin(i);
    }
}

bool relays_set(uint8_t number, uint8_t state, uint16_t intervalMs) {
    if (number < 1 || number > RELAY_COUNT) return false;
    if (state > RELAY_STATE_BLINK) return false;
    if (lock == NULL) return false;

    uint8_t states[RELAY_COUNT];
    xSemaphoreTake(lock, portMAX_DELAY);  // Held only for a few register updates
    set_locked(number - 1, state, intervalMs);
    snapshot_locked(states);
    xSemaphoreGive(lock);

    LOGF("Relay %u -> state %u (interval %u ms)", number, state, intervalMs);
    if (changedCb) changedCb(states, RELAY_COUNT);
    return true;
}

void relays_all_off() {
    if (lock == NULL) return;
    uint8_t states[RELAY_COUNT];
    xSemaphoreTake(lock, portMAX_DELAY);
    for (uint8_t i = 0; i < RELAY_COUNT; i++) set_locked(i, RELAY_STATE_OFF, 0);
    snapshot_locked(states);
    xSemaphoreGive(lock);
    if (changedCb) changedCb(states, RELAY_COUNT);
}

void relays_get_states(uint8_t* out) {
    if (lock == NULL) {
        for (uint8_t i = 0; i < RELAY_COUNT; i++) out[i] = RELAY_STATE_OFF;
        return;
    }
    xSemaphoreTake(lock, portMAX_DELAY);
    snapshot_locked(out);
    xSemaphoreGive(lock);
}
