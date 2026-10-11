#include "../include/fog_machine.h"
#include "../include/analog_inputs.h"
#include "../include/config.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

// Fogging works in bursts: D9 active for the configured time, then D9 low for FOG_MEASURE_DELAY_MS
// so A0 can be measured. If the machine is still hot, the next burst starts; otherwise the session ends.
// The published state stays FOGGING during the pause between bursts.

enum class Phase : uint8_t {
    IDLE,          // Not fogging: state follows the measurements
    BURST,         // D9 active
    MEASURE_WAIT,  // D9 low, waiting before measuring A0
    STOP_SETTLE    // Stopped by client/failsafe: D9 low, waiting before re-evaluating the state
};

static QueueHandle_t commandQueue = NULL;
static fog_state_cb_t stateCb = nullptr;
static fog_session_cb_t sessionCb = nullptr;
static fog_duration_cb_t durationCb = nullptr;

static volatile uint8_t publishedState = FOG_STATE_NOT_CONNECTED;
static volatile uint8_t sessionActive = 0;
static volatile uint8_t burstSeconds = FOG_BURST_DEFAULT_SECONDS;

// ---- Task-private state (only touched by fog_task) ----
static Phase phase = Phase::IDLE;
static uint32_t phaseStartMs = 0;
static uint32_t sessionStartMs = 0;
static bool stateInitialized = false;

static bool machine_connected() {
    return digitalRead(FOG_CONNECTED_PIN) == HIGH;
}

static uint16_t read_sense_mv() {
    return adc_read_mv_averaged(FOG_SENSE_PIN, FOG_ADC_OVERSAMPLE);
}

static void trigger(bool on) {
    digitalWrite(FOG_TRIGGER_PIN, on ? HIGH : LOW);
}

static void publish_state(uint8_t state) {
    if (stateInitialized && state == publishedState) return;
    stateInitialized = true;
    publishedState = state;
    LOGF("Fog state: %u", state);
    if (stateCb) stateCb(state);
}

static void publish_session(uint8_t active, bool force) {
    if (!force && active == sessionActive) return;
    sessionActive = active;
    if (sessionCb) sessionCb(active);
}

// Determine the state from the inputs while not fogging.
static void evaluate_idle() {
    if (!machine_connected()) {
        publish_state(FOG_STATE_NOT_CONNECTED);
        return;
    }
    uint16_t mv = read_sense_mv();
    bool wasReady = (publishedState == FOG_STATE_READY);
    uint16_t limit = wasReady ? (FOG_READY_THRESHOLD_MV - FOG_HYSTERESIS_MV) : (FOG_READY_THRESHOLD_MV + FOG_HYSTERESIS_MV);
    publish_state(mv >= limit ? FOG_STATE_READY : FOG_STATE_HEATING);
}

static void begin_stop_settle() {
    trigger(false);
    phase = Phase::STOP_SETTLE;
    phaseStartMs = millis();
    publish_session(0, false);
}

static void abort_not_connected() {
    trigger(false);
    phase = Phase::IDLE;
    publish_session(0, false);
    publish_state(FOG_STATE_NOT_CONNECTED);
}

static void handle_command(uint8_t cmd) {
    if (cmd == FOG_CMD_START) {
        if (sessionActive) {
            publish_session(1, true);  // Already fogging
        } else if (phase == Phase::IDLE && publishedState == FOG_STATE_READY && machine_connected()) {
            trigger(true);
            phase = Phase::BURST;
            phaseStartMs = sessionStartMs = millis();
            publish_state(FOG_STATE_FOGGING);
            publish_session(1, false);
            LOGF("Fogging started (burst %u s)", burstSeconds);
        } else {
            LOGF("Fog start rejected (state %u)", publishedState);
            publish_session(0, true);  // Tell the client it did not start
        }
    } else {
        if (sessionActive) {
            LOGF("Fogging stopped");
            begin_stop_settle();
        } else {
            publish_session(0, true);
        }
    }
}

static bool session_cap_reached(uint32_t now) {
    return FOG_MAX_SESSION_MS != 0 && (now - sessionStartMs) >= FOG_MAX_SESSION_MS;
}

static void tick() {
    uint32_t now = millis();
    switch (phase) {
        case Phase::IDLE:
            evaluate_idle();
            break;

        case Phase::BURST:
            if (!machine_connected()) {
                LOGF("Fog machine disconnected while fogging");
                abort_not_connected();
            } else if (session_cap_reached(now)) {
                LOGF("Fogging stopped: session limit reached");
                begin_stop_settle();
            } else if ((now - phaseStartMs) >= (uint32_t)burstSeconds * 1000UL) {
                trigger(false);  // Pause to measure; state stays FOGGING
                phase = Phase::MEASURE_WAIT;
                phaseStartMs = now;
            }
            break;

        case Phase::MEASURE_WAIT:
            if (!machine_connected()) {
                LOGF("Fog machine disconnected while fogging");
                abort_not_connected();
            } else if (session_cap_reached(now)) {
                LOGF("Fogging stopped: session limit reached");
                begin_stop_settle();
            } else if ((now - phaseStartMs) >= FOG_MEASURE_DELAY_MS) {
                uint16_t mv = read_sense_mv();
                if (mv >= (FOG_READY_THRESHOLD_MV - FOG_HYSTERESIS_MV)) {
                    trigger(true);  // Still hot enough: next burst
                    phase = Phase::BURST;
                    phaseStartMs = now;
                } else {
                    LOGF("Fogging ended: machine cooled down (%u mV)", mv);
                    phase = Phase::IDLE;
                    publish_session(0, false);
                    evaluate_idle();
                }
            }
            break;

        case Phase::STOP_SETTLE:
            if ((now - phaseStartMs) >= FOG_MEASURE_DELAY_MS) {
                phase = Phase::IDLE;
                evaluate_idle();
            }
            break;
    }
}

static void fog_task(void* pvParameters) {
    (void)pvParameters;
    for (;;) {
        // Sleeps until a command arrives or the poll interval elapses
        uint8_t cmd;
        if (xQueueReceive(commandQueue, &cmd, pdMS_TO_TICKS(FOG_POLL_INTERVAL_MS)) == pdTRUE) {
            handle_command(cmd);
        }
        tick();
    }
}

void fog_init(fog_state_cb_t stateCallback, fog_session_cb_t sessionCallback, fog_duration_cb_t durationCallback) {
    stateCb = stateCallback;
    sessionCb = sessionCallback;
    durationCb = durationCallback;

    pinMode(FOG_TRIGGER_PIN, OUTPUT);
    digitalWrite(FOG_TRIGGER_PIN, LOW);
    pinMode(FOG_CONNECTED_PIN, INPUT);
    pinMode(FOG_SENSE_PIN, INPUT);

    commandQueue = xQueueCreate(FOG_COMMAND_QUEUE_LENGTH, sizeof(uint8_t));
    xTaskCreate(fog_task, "fog_task", FOG_TASK_STACK, NULL, FOG_TASK_PRIORITY, NULL);
}

static void send_command(uint8_t cmd) {
    if (commandQueue != NULL) xQueueSend(commandQueue, &cmd, 0);
}

void fog_start() { send_command(FOG_CMD_START); }
void fog_stop() { send_command(FOG_CMD_STOP); }

void fog_set_duration(uint8_t seconds) {
    if (seconds < FOG_BURST_MIN_SECONDS) seconds = FOG_BURST_MIN_SECONDS;
    if (seconds > FOG_BURST_MAX_SECONDS) seconds = FOG_BURST_MAX_SECONDS;
    burstSeconds = seconds;
    LOGF("Fog burst duration: %u s", seconds);
    if (durationCb) durationCb(seconds);
}

uint8_t fog_get_state() { return publishedState; }
uint8_t fog_get_session() { return sessionActive; }
uint8_t fog_get_duration() { return burstSeconds; }
