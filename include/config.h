// Common configuration: pins, UUIDs, PWM settings
#pragma once

// Pin definitions
static const int MOTOR_PUL_PIN = 14;
static const int MOTOR_DIR_PIN = 20;
static const int MOTOR_ENA_PIN = 22;
static const int HORN_PIN = 32;
static const int BUTTON_PIN = 38;
static const int LED_PIN = 13;

// BLE UUIDs
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define MOTOR_CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define HORN_CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a9"
#define BUTTON_CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26aa"
#define LED_CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26ab"

// PWM (LEDC) defaults
static const int PWM_CHANNEL = 0;
static const int PWM_RESOLUTION = 8; // bits
static const int PWM_DEFAULT_DUTY = 128; // 50%
