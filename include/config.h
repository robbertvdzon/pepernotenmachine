// Common configuration: pins, UUIDs, PWM settings
#pragma once

#include <stdint.h>

static const int LED_1_PIN = 19;           // Generic indicator LED (sinking current)
static const int LED_2_PIN = 18;           // Generic indicator LED (sinking current)
static const int LED_3_PIN = 4;            // Generic indicator LED (sinking current)
static const int EXTRA_IO_1_PIN = 21;      // Not yet assigned
static const int EXTRA_IO_2_PIN = 22;      // Not yet assigned
static const int DISPENSER_EN_PIN = 23;    // Connected to EN(9) pin of DRV8825 stepper driver
static const int DISPENSER_STEP_PIN = 25;  // Connected to STEP(15) pin of DRV8825 stepper driver
static const int DISPENSER_DIR_PIN = 26;   // Connected to DIR(16) pin of DRV8825 stepper driver
static const int PULL_MOTOR_PUL_PIN = 27;  // Connected to PUL pin of MicroStep driver for pull motor
static const int PULL_MOTOR_DIR_PIN = 16;  // Connected to DIR pin of MicroStep driver for pull motor
static const int PULL_MOTOR_ENA_PIN = 17;  // Connected to ENA pin of MicroStep driver for pull motor
static const int PULL_MOTOR_END_SWITCH_PIN = 32; // Connected to end switch for pull motor (connected to GND when activated)
static const int RELEASE_SERVO_PIN = 33;  // Conected to servo for release mechanism

// BLE UUIDs
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define MOTOR_CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define BUTTON_CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26aa"
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
