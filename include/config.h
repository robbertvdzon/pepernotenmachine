// Common configuration: pins, UUIDs, PWM settings
#pragma once

#include <stdint.h>

#if defined(BOARD_FEATHER)

// Feather ESP32 V2
static const int MOTOR_PUL_PIN = 14;
static const int MOTOR_DIR_PIN = 20;
static const int MOTOR_ENA_PIN = 22;
static const int HORN_PIN = 32;
static const int BUTTON_PIN = 38;
static const int LED_PIN = 0;
static const int LED_POWER_PIN = 2;
static const int SERVO_PIN = 15;
static const int ROUTINE_SWITCH_PIN = 33;
static const int DISPENSER_STEP_PIN = 27;

#elif defined(BOARD_LOLIN32)

// WEMOS LOLIN32
static const int MOTOR_PUL_PIN = 18;
static const int MOTOR_DIR_PIN = 19;
static const int MOTOR_ENA_PIN = 23;
static const int HORN_PIN = 25;
static const int BUTTON_PIN = 32;
static const int LED_PIN = 26;
static const int LED_POWER_PIN = 27;
static const int SERVO_PIN = 13;
static const int ROUTINE_SWITCH_PIN = 33;
static const int DISPENSER_STEP_PIN = 14;

#else

#error "No supported board selected"

#endif

// BLE UUIDs
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define MOTOR_CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define HORN_CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a9"
#define BUTTON_CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26aa"
#define LED_CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26ab"
#define LED_CROSSFADE_STATE_CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26ac"
#define SERVO_CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26ad"
#define ROUTINE_CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26ae"
#define DISPENSER_DURATION_CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26af"
#define DISPENSER_CONTROL_CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26b0"

// PWM (LEDC) defaults
// Motor uses channel 7 (group1 timer3) so ESP32Servo can safely allocate channel 0+.
static const int PWM_CHANNEL = 7;
static const int PWM_RESOLUTION = 8; // bits
static const int PWM_DEFAULT_DUTY = 128; // 50%

// Motor settings
static const int MOTOR_SPEED_MAX = 512;
static const int MOTOR_DEFAULT_FREQ = 1000; // Hz

// Servo settings
static const uint8_t SERVO_MAX_ANGLE = 180;

// Dispenser PWM settings
static const int DISPENSER_PWM_FREQ = 1000; // Hz - back to 1000 Hz for microstepping

// Dispenser default settings
static const uint8_t DISPENSER_DEFAULT_DURATION = 4; // seconds
static const uint8_t DISPENSER_MAX_DURATION = 10; // seconds

// Dispenser motion control settings
static const uint16_t DISPENSER_TOP_SPEED_HZ = 2000; // maximum speed in Hz
static const uint16_t DISPENSER_ACCEL_INITIAL = 200; // initial/fallback acceleration in steps/s²
static const uint8_t DISPENSER_ACCEL_PHASE_PERCENT = 20; // % of duration for acceleration phase
