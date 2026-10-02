// Common configuration: pins, UUIDs, protocol values, timings and thresholds.
// Every constant and configurable option of the firmware lives in this file.
#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <stdint.h>

// ---------------------------------------------------------------------------
// General
// ---------------------------------------------------------------------------
#define LOG_ENABLED 1  // 1 = print debug output on Serial, 0 = silent
#if LOG_ENABLED
#define LOGF(fmt, ...) Serial.printf(fmt "\n", ##__VA_ARGS__)
#else
#define LOGF(fmt, ...) ((void)0)
#endif

static const uint32_t SERIAL_BAUD = 115200;
static const uint32_t SERIAL_WAIT_TIMEOUT_MS = 2000;  // Max wait for a USB serial monitor at boot (never blocks forever)

// ---------------------------------------------------------------------------
// Pins (Arduino Nano ESP32)
// ---------------------------------------------------------------------------
// J2: generic digital inputs, active LOW. Input number n (1..4) is entry n-1 of this array.
// NOTE: input 1 = D2 ... input 4 = D5 (J2 pin 4 = D2 ... J2 pin 1 = D5). Re-order here if you prefer J2 pin order.
static const uint8_t DIGITAL_INPUT_COUNT = 4;
static const uint8_t DIGITAL_INPUT_PINS[DIGITAL_INPUT_COUNT] = {D2, D3, D4, D5};
static const bool DIGITAL_INPUT_USE_PULLUP = true;  // Internal pull-up (inputs are active LOW)

// J3: MOSFET gate drivers (IRLZ44N, active HIGH, gate pull-down on the board)
static const uint8_t MOSFET_COUNT = 3;
static const uint8_t MOSFET_PINS[MOSFET_COUNT] = {D6, D7, D8};

// J4: analog inputs
static const uint8_t ANALOG_INPUT_COUNT = 4;
static const uint8_t ANALOG_INPUT_PINS[ANALOG_INPUT_COUNT] = {A0, A1, A2, A3};

// J5: fog machine
static const uint8_t FOG_TRIGGER_PIN = D9;    // HIGH = transistor on = shorts the two trigger pins of the fog machine
static const uint8_t FOG_SENSE_PIN = A6;      // Analog: ~0 V while heating up, ~2.5 V when ready
static const uint8_t FOG_CONNECTED_PIN = A7;  // Digital: HIGH when the fog machine is connected / switched on

// I2C bus for the MCP23017 port expander (default Nano ESP32 pins: SDA = A4, SCL = A5)
static const uint8_t I2C_SDA_PIN = SDA;
static const uint8_t I2C_SCL_PIN = SCL;

// ---------------------------------------------------------------------------
// MCP23017 port expander
// ---------------------------------------------------------------------------
static const uint8_t MCP_PORT_A = 0;
static const uint8_t MCP_PORT_B = 1;
static const uint8_t MCP23017_I2C_ADDRESS = 0x20;         // A0..A2 tied to GND
static const uint32_t MCP_I2C_FREQ_HZ = 400000;
static const uint16_t MCP_I2C_TIMEOUT_MS = 20;            // Wire timeout per transaction
static const uint32_t MCP_REFRESH_INTERVAL_MS = 1000;     // Periodically re-write config + outputs (recovers from glitches)
static const uint32_t MCP_RETRY_INTERVAL_MS = 250;        // Retry delay after a failed I2C transaction
static const uint32_t MCP_TASK_STACK = 3072;
static const UBaseType_t MCP_TASK_PRIORITY = 2;

// ---------------------------------------------------------------------------
// Relays (K1..K8 on J8..J11, driven through BC337 transistors from MCP23017 port B)
// ---------------------------------------------------------------------------
static const uint8_t RELAY_COUNT = 8;
static const uint8_t RELAY_MCP_PORT = MCP_PORT_B;
static const uint8_t RELAY_MCP_BITS[RELAY_COUNT] = {0, 1, 2, 3, 4, 5, 6, 7};  // Relay n -> GPB(bit)
static const bool RELAY_ACTIVE_HIGH = true;               // Pin HIGH = relay on
static const uint16_t RELAY_BLINK_DEFAULT_MS = 500;       // Used when no interval is given (or interval = 0)
static const uint16_t RELAY_BLINK_MIN_MS = 50;            // Lower limit for custom blink intervals
static const uint16_t RELAY_BLINK_MAX_MS = 65535;         // Upper limit for custom blink intervals
static const uint32_t RELAY_LOCK_TIMEOUT_MS = 50;

// Relay states (BLE protocol)
static const uint8_t RELAY_STATE_OFF = 0;
static const uint8_t RELAY_STATE_ON = 1;
static const uint8_t RELAY_STATE_BLINK = 2;

// ---------------------------------------------------------------------------
// Generic digital outputs (J6 / J7, MCP23017 port A, sinking: LOW = on)
// ---------------------------------------------------------------------------
static const uint8_t OUTPUT_COUNT = 8;
static const uint8_t OUTPUT_MCP_PORT = MCP_PORT_A;
static const uint8_t OUTPUT_MCP_BITS[OUTPUT_COUNT] = {0, 1, 2, 3, 4, 5, 6, 7};  // Output n -> GPA(bit)
static const bool OUTPUT_ACTIVE_HIGH = false;             // Pin LOW = output on

// ---------------------------------------------------------------------------
// MOSFET PWM (LEDC)
// ---------------------------------------------------------------------------
static const uint32_t MOSFET_PWM_FREQ_HZ = 1000;          // Shared by all channels
static const uint8_t MOSFET_PWM_RESOLUTION_BITS = 10;
static const uint8_t MOSFET_PWM_CHANNEL_BASE = 0;         // Channels BASE..BASE+COUNT-1 (Arduino core 2.x only)
static const uint8_t MOSFET_MAX_PERCENT = 100;

// ---------------------------------------------------------------------------
// Digital inputs
// ---------------------------------------------------------------------------
static const uint32_t DIGITAL_INPUT_DEBOUNCE_MS = 20;
static const uint32_t DIGITAL_INPUT_TASK_STACK = 2560;
static const UBaseType_t DIGITAL_INPUT_TASK_PRIORITY = 2;

// ---------------------------------------------------------------------------
// Analog inputs
// ---------------------------------------------------------------------------
static const uint32_t ANALOG_SAMPLE_INTERVAL_MS = 50;
static const uint8_t ANALOG_OVERSAMPLE = 8;               // ADC readings averaged per sample
static const uint16_t ANALOG_NOTIFY_THRESHOLD_MV = 50;    // Notify when a channel changed at least this much since the last notification
static const uint32_t ANALOG_NOTIFY_MIN_INTERVAL_MS = 200;  // Never notify more often than this
static const uint32_t ANALOG_TASK_STACK = 3072;
static const UBaseType_t ANALOG_TASK_PRIORITY = 1;

// ---------------------------------------------------------------------------
// Fog machine
// ---------------------------------------------------------------------------
// States (BLE protocol)
static const uint8_t FOG_STATE_NOT_CONNECTED = 0;
static const uint8_t FOG_STATE_HEATING = 1;
static const uint8_t FOG_STATE_READY = 2;
static const uint8_t FOG_STATE_FOGGING = 3;

// Control commands (BLE protocol)
static const uint8_t FOG_CMD_STOP = 0;
static const uint8_t FOG_CMD_START = 1;

static const uint16_t FOG_READY_THRESHOLD_MV = 1000;      // A6 at/above this = heated up
static const uint16_t FOG_HYSTERESIS_MV = 100;            // Ready at >= threshold + hysteresis, back to heating at < threshold - hysteresis
static const uint8_t FOG_BURST_DEFAULT_SECONDS = 5;       // Time D9 stays active per burst
static const uint8_t FOG_BURST_MIN_SECONDS = 1;
static const uint8_t FOG_BURST_MAX_SECONDS = 60;
static const uint32_t FOG_MEASURE_DELAY_MS = 200;         // D9 low -> wait this long -> measure A6
static const uint32_t FOG_MAX_SESSION_MS = 120000;        // Safety cap on one continuous fogging session (0 = disabled)
static const uint32_t FOG_POLL_INTERVAL_MS = 50;          // State evaluation interval
static const uint8_t FOG_ADC_OVERSAMPLE = 8;
static const uint32_t FOG_TASK_STACK = 4096;
static const UBaseType_t FOG_TASK_PRIORITY = 2;
static const uint8_t FOG_COMMAND_QUEUE_LENGTH = 4;

// ---------------------------------------------------------------------------
// BLE
// ---------------------------------------------------------------------------
#define BLE_DEVICE_NAME "IO Switchboard"
static const int BLE_MAX_CONNECTIONS = 3;

// Service + characteristic UUIDs
#define SERVICE_UUID "3dc93769-33d3-4883-ae7b-877c14fd3eca"
#define RELAYS_CHARACTERISTIC_UUID "3dc93701-33d3-4883-ae7b-877c14fd3eca"          // R/W/N
#define MOSFETS_CHARACTERISTIC_UUID "3dc93702-33d3-4883-ae7b-877c14fd3eca"         // R/W/N
#define FOG_STATE_CHARACTERISTIC_UUID "3dc93703-33d3-4883-ae7b-877c14fd3eca"       // R/N
#define FOG_CONTROL_CHARACTERISTIC_UUID "3dc93704-33d3-4883-ae7b-877c14fd3eca"     // R/W/N
#define FOG_DURATION_CHARACTERISTIC_UUID "3dc93705-33d3-4883-ae7b-877c14fd3eca"    // R/W
#define DIGITAL_INPUTS_CHARACTERISTIC_UUID "3dc93706-33d3-4883-ae7b-877c14fd3eca"  // R/N
#define ANALOG_INPUTS_CHARACTERISTIC_UUID "3dc93707-33d3-4883-ae7b-877c14fd3eca"   // R/N
#define DIGITAL_OUTPUTS_CHARACTERISTIC_UUID "3dc93708-33d3-4883-ae7b-877c14fd3eca"  // R/W/N
