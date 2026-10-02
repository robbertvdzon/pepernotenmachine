#include "../include/mosfets.h"
#include "../include/config.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

static uint8_t percentValue[MOSFET_COUNT];
static SemaphoreHandle_t lock = NULL;
static mosfets_changed_cb_t changedCb = nullptr;

// LEDC API differs between Arduino core 2.x (channel based) and 3.x (pin based)
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
static void pwm_attach(uint8_t index) { ledcAttach(MOSFET_PINS[index], MOSFET_PWM_FREQ_HZ, MOSFET_PWM_RESOLUTION_BITS); }
static void pwm_write(uint8_t index, uint32_t duty) { ledcWrite(MOSFET_PINS[index], duty); }
#else
static void pwm_attach(uint8_t index) {
    ledcSetup(MOSFET_PWM_CHANNEL_BASE + index, MOSFET_PWM_FREQ_HZ, MOSFET_PWM_RESOLUTION_BITS);
    ledcAttachPin(MOSFET_PINS[index], MOSFET_PWM_CHANNEL_BASE + index);
}
static void pwm_write(uint8_t index, uint32_t duty) { ledcWrite(MOSFET_PWM_CHANNEL_BASE + index, duty); }
#endif

static uint32_t percent_to_duty(uint8_t percent) {
    const uint32_t maxDuty = (1u << MOSFET_PWM_RESOLUTION_BITS) - 1;
    return (percent * maxDuty + MOSFET_MAX_PERCENT / 2) / MOSFET_MAX_PERCENT;
}

void mosfets_init(mosfets_changed_cb_t cb) {
    changedCb = cb;
    lock = xSemaphoreCreateMutex();
    for (uint8_t i = 0; i < MOSFET_COUNT; i++) {
        percentValue[i] = 0;
        pwm_attach(i);
        pwm_write(i, 0);
    }
}

bool mosfets_set(uint8_t number, uint8_t percent) {
    if (number < 1 || number > MOSFET_COUNT || lock == NULL) return false;
    if (percent > MOSFET_MAX_PERCENT) percent = MOSFET_MAX_PERCENT;

    uint8_t snapshot[MOSFET_COUNT];
    xSemaphoreTake(lock, portMAX_DELAY);
    percentValue[number - 1] = percent;
    pwm_write(number - 1, percent_to_duty(percent));
    for (uint8_t i = 0; i < MOSFET_COUNT; i++) snapshot[i] = percentValue[i];
    xSemaphoreGive(lock);

    LOGF("MOSFET %u -> %u%%", number, percent);
    if (changedCb) changedCb(snapshot, MOSFET_COUNT);
    return true;
}

void mosfets_all_off() {
    if (lock == NULL) return;
    uint8_t snapshot[MOSFET_COUNT];
    xSemaphoreTake(lock, portMAX_DELAY);
    for (uint8_t i = 0; i < MOSFET_COUNT; i++) {
        percentValue[i] = 0;
        pwm_write(i, 0);
        snapshot[i] = 0;
    }
    xSemaphoreGive(lock);
    if (changedCb) changedCb(snapshot, MOSFET_COUNT);
}

void mosfets_get(uint8_t* out) {
    if (lock == NULL) {
        for (uint8_t i = 0; i < MOSFET_COUNT; i++) out[i] = 0;
        return;
    }
    xSemaphoreTake(lock, portMAX_DELAY);
    for (uint8_t i = 0; i < MOSFET_COUNT; i++) out[i] = percentValue[i];
    xSemaphoreGive(lock);
}
