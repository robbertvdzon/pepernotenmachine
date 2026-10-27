#pragma once

// =========================================================================
// Central hardware configuration for the control unit (Arduino Nano ESP32).
// All pin assignments, active levels, and timings live here.
// NOTE: these are the CONTROL UNIT pins. The remote switchboard / launcher
// firmware have their own, unrelated pin maps.
// =========================================================================

#include <Arduino.h>

// ---------------------------------------------------------------- Display
// ST7735S, native panel 128x160, used in LANDSCAPE (160x128).
constexpr int8_t TFT_PIN_SCLK = D13;
constexpr int8_t TFT_PIN_MOSI = D11;
constexpr int8_t TFT_PIN_DC = D9;
constexpr int8_t TFT_PIN_CS = D10;
constexpr int8_t TFT_PIN_RST = D8;

constexpr int16_t DISPLAY_WIDTH = 160;   // after rotation
constexpr int16_t DISPLAY_HEIGHT = 128;  // after rotation
// LovyanGFX rotation: 1 = landscape. Use 3 for the opposite landscape
// orientation if the panel is mounted upside down.
constexpr uint8_t DISPLAY_ROTATION = 1;
// Panel properties. Overridable with -D, but normally just edit them here.
#ifndef DISPLAY_SPI_HZ
#define DISPLAY_SPI_HZ 15000000  // lower (e.g. 8000000) if the image shows noise
#endif
#ifndef DISPLAY_INVERT
#define DISPLAY_INVERT false
#endif
#ifndef DISPLAY_RGB_ORDER
#define DISPLAY_RGB_ORDER true  // this panel is wired BGR; false would swap red and blue
#endif

// ---------------------------------------------------------------- UI input
constexpr uint32_t UI_LONG_PRESS_MS = 700;   // hold the encoder button on the home screen = open setup
constexpr bool ENCODER_DIRECTION_INVERT = false;  // flip if turning clockwise moves the focus up
constexpr uint32_t UI_SCAN_SECONDS = 10;

// -------------------------------------------------------- Rotary encoder
constexpr int8_t ROTARY_PIN_CLK = A0;
constexpr int8_t ROTARY_PIN_CT = A1;
constexpr int8_t ROTARY_PIN_SWITCH = D7;  // push switch, active LOW (pull-up)

// ------------------------------------------------- I2C / MCP23017 expander
constexpr int8_t I2C_PIN_SDA = A4;
constexpr int8_t I2C_PIN_SCL = A5;
constexpr uint32_t I2C_FREQ_HZ = 400000;  // external 4k7 pull-ups on the board
constexpr uint16_t I2C_TIMEOUT_MS = 20;
constexpr uint8_t MCP23017_ADDRESS = 0x20;  // A0..A2 tied to GND
constexpr uint32_t MCP_RETRY_INTERVAL_MS = 250;  // re-init attempts while not ready
// MCP23017 INTA/INTB are not wired -> switches are polled over I2C.
// MCP23017 /RESET is tied high -> the chip keeps its config across ESP resets,
// so the driver always writes the full configuration at init.

// ------------------------------------------------------------------- LEDs
// L1..L8  = MCP23017 GPA0..GPA7
// L9..L12 = Nano D2..D5
// LEDs light when the output is LOW (active LOW).
constexpr uint8_t LED_COUNT = 12;
constexpr uint8_t LED_COUNT_MCP = 8;
constexpr int8_t LED_DIRECT_PINS[4] = {D2, D3, D4, D5};
constexpr bool LED_ACTIVE_LOW = true;

// --------------------------------------------------------------- Switches
// SW1..SW8 = MCP23017 GPB0..GPB7, wired to GND when closed.
// The MCP23017 internal ~100k pull-ups (GPPUB) are enabled.
// Closed (to GND) = pin LOW = "active".
// SW9..SW12 are wired to Nano A2, A3, A6, A7 on the schematic but have no
// defined function yet; they are intentionally NOT configured by the firmware.
constexpr uint8_t SWITCH_COUNT = 8;
constexpr bool SWITCH_ACTIVE_LOW = true;
constexpr uint32_t SWITCH_SCAN_INTERVAL_MS = 5;
constexpr uint32_t SWITCH_DEBOUNCE_MS = 25;  // level must be stable this long

// Encoder push switch debounce (direct GPIO).
constexpr uint32_t BUTTON_DEBOUNCE_MS = 20;

// -------------------------------------------------------------- BLE links
// Each saved device is kept connected by a never-give-up reconnect loop.
// NimBLE 1.4 connects with a BLOCKING call, so all of it runs in its own task.
constexpr uint8_t LINK_CONNECT_TIMEOUT_S = 2;          // one connect attempt (whole seconds)
constexpr uint32_t LINK_RETRY_GAP_MS = 500;            // pause between failed attempts (radio duty)
constexpr uint32_t LINK_RECONNECT_FIRST_DELAY_MS = 200; // after losing an established link
constexpr uint32_t LINK_ERROR_BACKOFF_MS = 3000;       // after a failed setup (Error state)
constexpr uint32_t LINK_TASK_PERIOD_MS = 50;
// Connection parameters (units: interval 1.25 ms, supervision timeout 10 ms).
// 2 s supervision timeout = a device that went out of range is noticed within ~2 s.
constexpr uint16_t LINK_CONN_INTERVAL_MIN = 16;  // 20 ms
constexpr uint16_t LINK_CONN_INTERVAL_MAX = 32;  // 40 ms
constexpr uint16_t LINK_CONN_LATENCY = 0;
constexpr uint16_t LINK_SUPERVISION_TIMEOUT = 200;
