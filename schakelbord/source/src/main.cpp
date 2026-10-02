#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "../include/config.h"
#include "../include/mcp23017.h"
#include "../include/relays.h"
#include "../include/mosfets.h"
#include "../include/digital_outputs.h"
#include "../include/digital_inputs.h"
#include "../include/analog_inputs.h"
#include "../include/fog_machine.h"
#include "../include/ble_manager.h"

// ---- BLE write callbacks: forward to the hardware modules (all non-blocking) ----

static void relay_write_cb(uint8_t number, uint8_t state, uint16_t intervalMs) {
    relays_set(number, state, intervalMs);
}

static void mosfet_write_cb(uint8_t number, uint8_t percent) {
    mosfets_set(number, percent);
}

static void output_write_cb(uint8_t number, uint8_t state) {
    outputs_set(number, state != 0);
}

static void fog_control_write_cb(uint8_t command) {
    if (command == FOG_CMD_START) fog_start();
    else fog_stop();
}

static void fog_duration_write_cb(uint8_t seconds) {
    fog_set_duration(seconds);
}

// Failsafe: the last client disconnected, so stop everything.
static void all_clients_disconnected_cb() {
    LOGF("All clients disconnected: switching everything off");
    fog_stop();
    relays_all_off();
    mosfets_all_off();
    outputs_all_off();
}

// Push the current values of every module into the BLE characteristics.
static void push_initial_values() {
    uint8_t buf[8];
    relays_get_states(buf);
    ble_update_relays(buf, RELAY_COUNT);
    mosfets_get(buf);
    ble_update_mosfets(buf, MOSFET_COUNT);
    ble_update_digital_outputs(outputs_get_mask());
    ble_update_fog_state(fog_get_state());
    ble_update_fog_control(fog_get_session());
    ble_update_fog_duration(fog_get_duration());
    ble_update_digital_inputs(digital_inputs_get_mask());
}

void setup() {
    Serial.begin(SERIAL_BAUD);
    // Wait for a USB serial monitor, but never forever (the board must also run without a computer)
    uint32_t start = millis();
    while (!Serial && (millis() - start) < SERIAL_WAIT_TIMEOUT_MS) {
        delay(10);
    }

    LOGF("Switchboard starting up...");
    LOGF("Chip: %s", ESP.getChipModel());
    LOGF("Revision: %d", ESP.getChipRevision());
    LOGF("Cores: %d", ESP.getChipCores());
    LOGF("Free heap: %u", ESP.getFreeHeap());
    LOGF("PSRAM: %s", psramFound() ? "YES" : "NO");

    // Outputs first, so everything is in a known safe (off) state before BLE starts
    mcp_init();
    relays_init(ble_update_relays);
    mosfets_init(ble_update_mosfets);
    outputs_init(ble_update_digital_outputs);
    fog_init(ble_update_fog_state, ble_update_fog_control, ble_update_fog_duration);

    ble_callbacks_t callbacks = {};
    callbacks.relay_write = relay_write_cb;
    callbacks.mosfet_write = mosfet_write_cb;
    callbacks.output_write = output_write_cb;
    callbacks.fog_control_write = fog_control_write_cb;
    callbacks.fog_duration_write = fog_duration_write_cb;
    callbacks.all_clients_disconnected = all_clients_disconnected_cb;
    ble_init(callbacks);

    // Inputs start after BLE so their first notification is not lost
    digital_inputs_init(ble_update_digital_inputs);
    analog_inputs_init(ble_update_analog_inputs);

    push_initial_values();
}

void loop() {
    // Everything runs in interrupts, timers and FreeRTOS tasks; just yield.
    vTaskDelay(pdMS_TO_TICKS(1000));
}
