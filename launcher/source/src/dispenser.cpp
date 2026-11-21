
#include "../include/dispenser.h"
#include "../include/config.h"
#include "../include/led.h"
#include "../include/stepper_engine.h"
#include "../include/storage.h"

#include <Arduino.h>
#include <FastAccelStepper.h>
#include <atomic>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

// -----------------------------------------------------------------------------
// Configuration
// -----------------------------------------------------------------------------

#define STATE_LED LED_2

static constexpr uint32_t DISPENSER_POLL_MS = 20;
static constexpr uint32_t DISPENSER_START_GRACE_MS = 100;
static constexpr uint32_t DISPENSER_TIMEOUT_MARGIN_MS = 2000;
static constexpr uint32_t DISPENSER_ABORT_RETRY_MS = 500;

static constexpr int32_t DISPENSER_REVERSE_TOLERANCE_STEPS = 50;

static constexpr uint32_t DISPENSER_TASK_STACK_SIZE = 4096;
static constexpr UBaseType_t DISPENSER_TASK_PRIORITY = 2;
static constexpr UBaseType_t DISPENSER_QUEUE_LENGTH = 4;

// -----------------------------------------------------------------------------
// Public stepper pointer
// -----------------------------------------------------------------------------

FastAccelStepper* stepper = nullptr;

// -----------------------------------------------------------------------------
// Internal state
// -----------------------------------------------------------------------------

enum class DispenserState : uint8_t { IDLE, DISPENSING, STOPPING, ABORTING };

static std::atomic<uint8_t> dispenseDuration(DISPENSER_DEFAULT_DURATION);

static QueueHandle_t dispenserQueue = nullptr;
static TaskHandle_t dispenserTaskHandle = nullptr;

static dispenser_notify_cb_t notify_cb = nullptr;

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

static uint8_t clamp_duration(uint8_t seconds) {
    if (seconds < 1) {
        seconds = 1;
    }

    if (seconds > DISPENSER_MAX_DURATION) {
        seconds = DISPENSER_MAX_DURATION;
    }

    return seconds;
}

// -----------------------------------------------------------------------------
// Finish a dispense cycle
//
// Only called from the dispenser task.
// -----------------------------------------------------------------------------

static void dispenser_finished(DispenserState& state, bool& continuous, const char* reason) {
    if (state == DispenserState::IDLE) {
        return;
    }

    state = DispenserState::IDLE;
    continuous = false;

    Serial.println(reason);

    led_set_state(STATE_LED, LED_RAMP_DOWN);

    if (notify_cb) {
        notify_cb(0);
    }
}

// -----------------------------------------------------------------------------
// Start a dispense cycle
//
// Commands:
//   1 = Timed forward dispense
//   2 = Continuous forward dispense
//   3 = Continuous reverse dispense
//
// Only called from the dispenser task.
// -----------------------------------------------------------------------------

static void dispenser_start(uint8_t command, DispenserState& state, bool& continuous, bool& reverse, uint32_t& startMs,
                            uint32_t& limitMs) {
    if (state != DispenserState::IDLE) {
        Serial.println("Dispenser already active, ignoring start");
        return;
    }

    if (stepper == nullptr) {
        Serial.println("Dispenser stepper not initialized");
        return;
    }

    const bool newContinuous = (command == 2 || command == 3);
    const bool newReverse = (command == 3);

    const uint32_t totalMs = newContinuous ? 0 : static_cast<uint32_t>(dispenseDuration.load()) * 1000UL;

    const uint32_t topSpeed = DISPENSER_TOP_SPEED_HZ;

    if (topSpeed == 0) {
        Serial.println("Invalid dispenser top speed");
        return;
    }

    // -------------------------------------------------------------------------
    // Calculate trapezoidal movement profile
    //
    // Ramp up:   rampMs
    // Cruise:    totalMs - 2 * rampMs
    // Ramp down: rampMs
    //
    // Total distance = topSpeed * (totalMs - rampMs)
    //
    // For short movements, limit ramp time to 40% of total duration.
    // -------------------------------------------------------------------------

    uint32_t rampMs = DISPENSER_RAMP_TIME_MS;

    if (!newContinuous) {
        const uint32_t maxRampMs = totalMs * 2UL / 5UL;

        if (rampMs > maxRampMs) {
            rampMs = maxRampMs;
        }
    }

    if (rampMs < 1) {
        rampMs = 1;
    }

    const uint32_t acceleration = static_cast<uint32_t>(static_cast<uint64_t>(topSpeed) * 1000ULL / rampMs);

    const uint64_t steps64 = newContinuous ? 0 : static_cast<uint64_t>(topSpeed) * (totalMs - rampMs) / 1000ULL;

    if (!newContinuous && (steps64 == 0 || steps64 > INT32_MAX)) {
        Serial.println("Invalid dispenser movement distance");
        return;
    }

    const int32_t totalSteps = static_cast<int32_t>(steps64);

    // -------------------------------------------------------------------------
    // Configure stepper
    // -------------------------------------------------------------------------

    stepper->setSpeedInHz(topSpeed);
    stepper->setAcceleration(acceleration);

    // IDLE guarantees that the previous movement has stopped.
    // Reset the software position reference for this new cycle.
    stepper->setCurrentPosition(0);

    Serial.print("Position before move: ");
    Serial.println(stepper->getCurrentPosition());

    Serial.print("Starting dispenser: ");

    if (newContinuous) {
        Serial.print("continuous");
    } else {
        Serial.print(totalMs / 1000);
        Serial.print("s");
    }

    Serial.print(", ");
    Serial.print(topSpeed);
    Serial.print(" Hz, accel ");
    Serial.print(acceleration);
    Serial.print(" steps/s^2, ");
    Serial.print(totalSteps);
    Serial.println(" steps");

    // -------------------------------------------------------------------------
    // Start movement
    // -------------------------------------------------------------------------

    int8_t result;

    if (newContinuous) {
        result = static_cast<int8_t>(newReverse ? stepper->runBackward() : stepper->runForward());
    } else {
        result = static_cast<int8_t>(stepper->move(totalSteps));
    }

    if (result < 0) {
        Serial.print("Stepper move failed with code ");
        Serial.println(result);
        return;
    }

    // Only publish the new state after the movement has been accepted.
    continuous = newContinuous;
    reverse = newReverse;

    startMs = millis();

    limitMs = newContinuous ? 0 : totalMs + DISPENSER_TIMEOUT_MARGIN_MS;

    state = DispenserState::DISPENSING;

    led_set_state(STATE_LED, LED_RAMP_UP);

    if (notify_cb) {
        notify_cb(1);
    }
}

// -----------------------------------------------------------------------------
// Request an abort
//
// The state remains ABORTING until the stepper reports that it has stopped.
// This prevents a new movement from starting prematurely.
// -----------------------------------------------------------------------------

static void dispenser_abort(DispenserState& state, const char*& abortReason, uint32_t& abortRetryMs,
                            const char* reason) {
    if (state == DispenserState::ABORTING || state == DispenserState::IDLE) {
        return;
    }

    abortReason = reason;
    state = DispenserState::ABORTING;

    abortRetryMs = millis();

    Serial.println(reason);

    stepper->forceStop();
}

// -----------------------------------------------------------------------------
// Handle stop command
//
// A user stop uses controlled deceleration.
// -----------------------------------------------------------------------------

static void dispenser_stop(DispenserState& state) {
    if (state != DispenserState::DISPENSING) {
        Serial.println("Dispenser is not dispensing, ignoring stop");
        return;
    }

    Serial.println("Stopping dispenser");

    state = DispenserState::STOPPING;

    stepper->stopMove();
}

// -----------------------------------------------------------------------------
// Update dispenser state
//
// Called periodically from the dedicated dispenser task.
// All dispenser state and stepper operations are serialized here.
// -----------------------------------------------------------------------------

static void dispenser_update(DispenserState& state, bool& continuous, bool& reverse, uint32_t startMs, uint32_t limitMs,
                             const char*& abortReason, uint32_t& abortRetryMs) {
    if (state == DispenserState::IDLE) {
        return;
    }

    if (stepper == nullptr) {
        dispenser_finished(state, continuous, "Dispenser error: stepper unavailable");
        return;
    }

    const uint32_t now = millis();

    // -------------------------------------------------------------------------
    // ABORTING
    //
    // Wait until the stepper has actually stopped before releasing the state.
    // Retry forceStop() if necessary.
    // -------------------------------------------------------------------------

    if (state == DispenserState::ABORTING) {
        if (!stepper->isRunning()) {
            dispenser_finished(state, continuous, abortReason ? abortReason : "Dispenser aborted");
            abortReason = nullptr;
            return;
        }

        if (now - abortRetryMs >= DISPENSER_ABORT_RETRY_MS) {
            abortRetryMs = now;
            stepper->forceStop();
        }

        return;
    }

    // -------------------------------------------------------------------------
    // STOPPING
    //
    // Wait for the controlled deceleration to finish.
    // -------------------------------------------------------------------------

    if (state == DispenserState::STOPPING) {
        if (!stepper->isRunning()) {
            dispenser_finished(state, continuous, "Dispenser stopped");
        }

        return;
    }

    // -------------------------------------------------------------------------
    // DISPENSING
    // -------------------------------------------------------------------------

    const uint32_t elapsed = now - startMs;

    // Avoid false stopped detections immediately after starting.
    if (elapsed < DISPENSER_START_GRACE_MS) {
        return;
    }

    const int32_t position = stepper->getCurrentPosition();

    // -------------------------------------------------------------------------
    // Direction monitoring
    // -------------------------------------------------------------------------

    const bool wrongDirection = (!reverse && position < -DISPENSER_REVERSE_TOLERANCE_STEPS) ||
                                (reverse && position > DISPENSER_REVERSE_TOLERANCE_STEPS);

    if (wrongDirection) {
        Serial.print("Wrong direction detected, position ");
        Serial.println(position);

        dispenser_abort(state, abortReason, abortRetryMs, "Dispenser aborted: wrong direction");

        return;
    }

    // -------------------------------------------------------------------------
    // Normal completion
    // -------------------------------------------------------------------------

    if (!stepper->isRunning()) {
        Serial.print("Final position: ");
        Serial.println(position);

        dispenser_finished(state, continuous, "Dispenser finished");

        return;
    }

    // -------------------------------------------------------------------------
    // Timeout monitoring
    //
    // Continuous movements have no automatic timeout.
    // User-requested stops are handled separately.
    // -------------------------------------------------------------------------

    if (!continuous && elapsed > limitMs) {
        dispenser_abort(state, abortReason, abortRetryMs, "Dispenser timeout, forced stop");
    }
}

// -----------------------------------------------------------------------------
// Dedicated dispenser task
//
// This task is the sole owner of dispenser state and stepper commands.
// BLE callbacks only enqueue commands.
// -----------------------------------------------------------------------------

static void dispenser_task(void* parameter) {
    DispenserState state = DispenserState::IDLE;

    bool continuous = false;
    bool reverse = false;

    uint32_t startMs = 0;
    uint32_t limitMs = 0;

    uint32_t abortRetryMs = 0;
    const char* abortReason = nullptr;

    uint8_t command;

    Serial.println("Dispenser task started");

    for (;;) {
        // Always update the motor state before processing another command.
        dispenser_update(state, continuous, reverse, startMs, limitMs, abortReason, abortRetryMs);

        // Wait for a command, but continue polling periodically.
        if (xQueueReceive(dispenserQueue, &command, pdMS_TO_TICKS(DISPENSER_POLL_MS)) == pdPASS) {
            switch (command) {
                case 0:
                    dispenser_stop(state);
                    break;

                case 1:
                case 2:
                case 3:
                    dispenser_start(command, state, continuous, reverse, startMs, limitMs);
                    break;

                default:
                    Serial.println("Unknown dispenser command");
                    break;
            }
        }
    }
}

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

void dispenser_init(dispenser_notify_cb_t notifyCb) {
    notify_cb = notifyCb;

    // -------------------------------------------------------------------------
    // Load saved duration
    // -------------------------------------------------------------------------

    uint8_t savedDuration;

    if (storage_get(STORAGE_KEY_DISPENSER_DURATION, &savedDuration, sizeof(savedDuration), NULL)) {
        savedDuration = clamp_duration(savedDuration);

        Serial.print("Read dispenser duration from storage: ");
        Serial.print(savedDuration);
        Serial.println(" seconds");

        dispenseDuration.store(savedDuration);
    }

    // -------------------------------------------------------------------------
    // Initialize stepper
    // -------------------------------------------------------------------------

    stepper = stepper_engine().stepperConnectToPin(DISPENSER_STEP_PIN);

    if (stepper == nullptr) {
        Serial.println("Failed to initialize dispenser stepper");
        led_set_state(STATE_LED, LED_OFF);
        return;
    }

    stepper->setDirectionPin(DISPENSER_DIR_PIN);
    stepper->setEnablePin(DISPENSER_EN_PIN);
    stepper->setAutoEnable(true);
    stepper->setSpeedInHz(DISPENSER_TOP_SPEED_HZ);

    stepper->setAcceleration(DISPENSER_TOP_SPEED_HZ * 1000UL / DISPENSER_RAMP_TIME_MS);

    led_set_state(STATE_LED, LED_OFF);

    // -------------------------------------------------------------------------
    // Create command queue
    // -------------------------------------------------------------------------

    dispenserQueue = xQueueCreate(DISPENSER_QUEUE_LENGTH, sizeof(uint8_t));

    if (dispenserQueue == nullptr) {
        Serial.println("Failed to create dispenser command queue");
        stepper = nullptr;
        return;
    }

    // -------------------------------------------------------------------------
    // Create dedicated dispenser task
    // -------------------------------------------------------------------------

    const BaseType_t result = xTaskCreate(dispenser_task, "Dispenser", DISPENSER_TASK_STACK_SIZE, nullptr,
                                          DISPENSER_TASK_PRIORITY, &dispenserTaskHandle);

    if (result != pdPASS) {
        Serial.println("Failed to create dispenser task");

        vQueueDelete(dispenserQueue);
        dispenserQueue = nullptr;

        dispenserTaskHandle = nullptr;
        stepper = nullptr;

        return;
    }

    Serial.println("Dispenser initialized successfully");
}

// -----------------------------------------------------------------------------
// Set dispense duration
// -----------------------------------------------------------------------------

void dispenser_set_duration(uint8_t seconds) {
    seconds = clamp_duration(seconds);

    dispenseDuration.store(seconds);

    if (!storage_set(STORAGE_KEY_DISPENSER_DURATION, &seconds, sizeof(seconds))) {
        Serial.println("Failed to queue dispenser duration for saving");
    }

    Serial.print("Dispenser duration set to ");
    Serial.print(seconds);
    Serial.println(" seconds");
}

// -----------------------------------------------------------------------------
// Get dispense duration
// -----------------------------------------------------------------------------

uint8_t dispenser_get_duration(void) { return dispenseDuration.load(); }

// -----------------------------------------------------------------------------
// Send a dispenser command
//
// Safe to call from NimBLE-Arduino callbacks.
//
// This function never accesses the stepper directly and never waits for the
// dispenser task to process the command.
// -----------------------------------------------------------------------------

void dispenser_control(uint8_t command) {
    if (command > 3) {
        Serial.println("Invalid dispenser command");
        return;
    }

    if (dispenserQueue == nullptr || dispenserTaskHandle == nullptr) {
        Serial.println("Dispenser not initialized");
        return;
    }

    if (xQueueSend(dispenserQueue, &command, 0) != pdPASS) {
        Serial.println("Dispenser command queue full");
    }
}
