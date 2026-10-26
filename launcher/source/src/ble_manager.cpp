#include <Arduino.h>
#include <NimBLEDevice.h>
#include "../include/ble_manager.h"
#include "../include/config.h"
#include "../include/dispenser.h"
#include "../include/release_servo.h"

static NimBLEServer* pServer = nullptr;
static NimBLEService* pService = nullptr;
static NimBLECharacteristic* pPullMotorCharacteristic = nullptr;
static NimBLECharacteristic* pButtonCharacteristic = nullptr;
static NimBLECharacteristic* pReleaseServoCharacteristic = nullptr;
static NimBLECharacteristic* pPrepareCharacteristic = nullptr;
static NimBLECharacteristic* pLaunchCharacteristic = nullptr;
static NimBLECharacteristic* pDispenserDurationCharacteristic = nullptr;
static NimBLECharacteristic* pDispenserControlCharacteristic = nullptr;
static NimBLECharacteristic* pPreparationStateCharacteristic = nullptr;

static pull_motor_write_cb_t pull_motor_cb = nullptr;
static release_servo_write_cb_t release_servo_cb = nullptr;
static prepare_write_cb_t prepare_cb = nullptr;
static launch_write_cb_t launch_cb = nullptr;
static dispenser_duration_write_cb_t dispenser_duration_cb = nullptr;
static dispenser_start_write_cb_t dispenser_start_cb = nullptr;
static int connectedCount = 0;
static const int MAX_CONNECTIONS = 3;

// Helper: add Client Characteristic Configuration descriptor (0x2902)
static void add_ccc_descriptor(NimBLECharacteristic* c) {
    if (c == nullptr) return;
    NimBLEDescriptor* d = new NimBLEDescriptor(NimBLEUUID((uint16_t)0x2902), 0, 2, c);
    uint8_t off[2] = {0x00, 0x00};
    d->setValue(off, 2);
    c->addDescriptor(d);
}

class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* srv) override {
        connectedCount++;
        Serial.print("BLE connected (");
        Serial.print(connectedCount);
        Serial.print("/");
        Serial.print(MAX_CONNECTIONS);
        Serial.println(")");
        // continue advertising if we haven't reached the max connections
        if (connectedCount < MAX_CONNECTIONS) {
            NimBLEDevice::startAdvertising();
            Serial.println("Continuing advertising for additional connections");
        } else {
            Serial.println("Max connections reached, stopping advertising for now");
        }
    }
    void onDisconnect(NimBLEServer* srv) override {
        if (connectedCount > 0) connectedCount--;
        Serial.print("BLE disconnected (");
        Serial.print(connectedCount);
        Serial.print("/");
        Serial.print(MAX_CONNECTIONS);
        Serial.println(")");
        // restart advertising to accept new connections
        NimBLEDevice::startAdvertising();
    }
};

class MotorCharCallbacks : public NimBLECharacteristicCallbacks {
    void onSubscribe(NimBLECharacteristic* c, ble_gap_conn_desc* desc, uint16_t subValue) override {
        if (subValue & 0x0001) c->notify();
    }

    void onWrite(NimBLECharacteristic* c) override {
        NimBLEAttValue v = c->getValue();
        if (v.size() < 4) return;

        const uint8_t* data = v.data();

        int32_t speedValue = (static_cast<int32_t>(data[0]) << 24) | (static_cast<int32_t>(data[1]) << 16) |
                             (static_cast<int32_t>(data[2]) << 8) | (static_cast<int32_t>(data[3]));

        // Clamp to allowed range
        if (speedValue > PULL_MOTOR_SPEED_MAX) speedValue = PULL_MOTOR_SPEED_MAX;
        if (speedValue < -PULL_MOTOR_SPEED_MAX) speedValue = -PULL_MOTOR_SPEED_MAX;

        if (pull_motor_cb) pull_motor_cb(speedValue);

    }
};

class ReleaseServoCharCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* c) override {
        Serial.println("Received write to servo characteristic");
        NimBLEAttValue v = c->getValue();
        Serial.print("Value size: "); Serial.println(v.size());
        if (v.size() < 1) return;
        uint8_t angle = v.data()[0];
        if (angle > SERVO_MAX_ANGLE) angle = SERVO_MAX_ANGLE;
        if (release_servo_cb) release_servo_cb(angle);
    }
};

class PrepareLaunchCharCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* c) override {
        Serial.println("Received write to prepare characteristic");
        NimBLEAttValue v = c->getValue();
        Serial.print("Value size: ");
        Serial.println(v.size());
        if (v.size() == 0) return;
        if (prepare_cb) prepare_cb();
    }
};

class LaunchCharCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* c) override {
        if (launch_cb) launch_cb();
    }
};

class DispenserDurationCharCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* c) override {
        NimBLEAttValue v = c->getValue();
        if (v.size() < 1) return;
        uint8_t duration = v.data()[0];
        if (dispenser_duration_cb) {
            dispenser_duration_cb(duration);
            duration = dispenser_get_duration();
        }
        pDispenserDurationCharacteristic->setValue(&duration, 1);
    }
};

class DispenserControlCharCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* c) override {
        NimBLEAttValue v = c->getValue();
        if (v.size() < 1) return;
        uint8_t command = v.data()[0];
        if (dispenser_start_cb) dispenser_start_cb(command);
        pDispenserControlCharacteristic->setValue(&command, 1);
        pDispenserControlCharacteristic->notify();
    }
};

void ble_init(pull_motor_write_cb_t motorCb, release_servo_write_cb_t releaseServoCb, prepare_write_cb_t prepareCb, launch_write_cb_t launchCb, dispenser_duration_write_cb_t dispenserDurationCb, dispenser_start_write_cb_t dispenserStartCb) {
    pull_motor_cb = motorCb;
    release_servo_cb = releaseServoCb;
    prepare_cb = prepareCb;
    launch_cb = launchCb;
    dispenser_duration_cb = dispenserDurationCb;
    dispenser_start_cb = dispenserStartCb;

    NimBLEDevice::init("Peppernut launcher");
    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());
    pService = pServer->createService(SERVICE_UUID);

    // Motor characteristic: read/write
    pPullMotorCharacteristic =
        pService->createCharacteristic(PULL_MOTOR_CHARACTERISTIC_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    pPullMotorCharacteristic->setCallbacks(new MotorCharCallbacks());
    add_ccc_descriptor(pPullMotorCharacteristic);
    uint8_t initSpeed[4] = {0, 0, 0, 0};
    pPullMotorCharacteristic->setValue(initSpeed, sizeof(initSpeed));

    // Servo characteristic: read/write
    pReleaseServoCharacteristic = pService->createCharacteristic(RELEASE_SERVO_CHARACTERISTIC_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    pReleaseServoCharacteristic->setCallbacks(new ReleaseServoCharCallbacks());
    add_ccc_descriptor(pReleaseServoCharacteristic);
    uint8_t initAngle = release_servo_get_angle();
    pReleaseServoCharacteristic->setValue(&initAngle, 1);

    // Button characteristic: read/notify
    pButtonCharacteristic = pService->createCharacteristic(BUTTON_CHARACTERISTIC_UUID, NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ);
    add_ccc_descriptor(pButtonCharacteristic);

    // Prepare launch characteristic: write
    pPrepareCharacteristic = pService->createCharacteristic(PREPARE_LAUNCH_CHARACTERISTIC_UUID, NIMBLE_PROPERTY::WRITE);
    pPrepareCharacteristic->setCallbacks(new PrepareLaunchCharCallbacks());

    // Launch characteristic: write
    pLaunchCharacteristic = pService->createCharacteristic(LAUNCH_CHARACTERISTIC_UUID, NIMBLE_PROPERTY::WRITE);
    pLaunchCharacteristic->setCallbacks(new LaunchCharCallbacks());

    // Preparation state characteristic: read/notify
    pPreparationStateCharacteristic = pService->createCharacteristic(PREPARATION_STATE_CHARACTERISTIC_UUID, NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ);
    add_ccc_descriptor(pPreparationStateCharacteristic);
    uint8_t initState = 0;  // IDLE
    pPreparationStateCharacteristic->setValue(&initState, 1);

    // Dispenser duration characteristic: read/write
    pDispenserDurationCharacteristic = pService->createCharacteristic(DISPENSER_DURATION_CHARACTERISTIC_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::READ);
    pDispenserDurationCharacteristic->setCallbacks(new DispenserDurationCharCallbacks());
    add_ccc_descriptor(pDispenserDurationCharacteristic);
    uint8_t initDuration = dispenser_get_duration();
    pDispenserDurationCharacteristic->setValue(&initDuration, 1);

    // Dispenser start characteristic: read/write/notify
    pDispenserControlCharacteristic = pService->createCharacteristic(DISPENSER_CONTROL_CHARACTERISTIC_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    pDispenserControlCharacteristic->setCallbacks(new DispenserControlCharCallbacks());
    add_ccc_descriptor(pDispenserControlCharacteristic);
    uint8_t initStart = 0; // initial state = stopped
    pDispenserControlCharacteristic->setValue(&initStart, 1);

    pService->start();

    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->start();
}

bool ble_is_connected() {
    return connectedCount > 0;
}

void ble_notify_button(uint8_t payload) {
    if (pButtonCharacteristic == nullptr) return;
    uint8_t b = payload;
    pButtonCharacteristic->setValue(&b, 1);
    pButtonCharacteristic->notify();
}

void ble_notify_pull_motor_speed(int32_t speed) {
    if (pPullMotorCharacteristic == nullptr) return;
    if (speed > PULL_MOTOR_SPEED_MAX) speed = PULL_MOTOR_SPEED_MAX;
    if (speed < -PULL_MOTOR_SPEED_MAX) speed = -PULL_MOTOR_SPEED_MAX;
    uint32_t value = static_cast<uint32_t>(speed);
    uint8_t payload[4] = {
        static_cast<uint8_t>(value >> 24),
        static_cast<uint8_t>(value >> 16),
        static_cast<uint8_t>(value >> 8),
        static_cast<uint8_t>(value),
    };
    pPullMotorCharacteristic->setValue(payload, sizeof(payload));
    pPullMotorCharacteristic->notify();
}

void ble_notify_servo_angle(uint8_t angle) {
    if (pReleaseServoCharacteristic == nullptr) return;
    pReleaseServoCharacteristic->setValue(&angle, 1);
    pReleaseServoCharacteristic->notify();
}

void ble_notify_dispenser_start(uint8_t state) {
    if (pDispenserControlCharacteristic == nullptr) return;
    Serial.print("Notifying dispenser start state: ");
    Serial.println(state);
    pDispenserControlCharacteristic->setValue(&state, 1);
    pDispenserControlCharacteristic->notify();
}

void ble_notify_preparation_state(uint8_t state) {
    if (pPreparationStateCharacteristic == nullptr) return;
    pPreparationStateCharacteristic->setValue(&state, 1);
    pPreparationStateCharacteristic->notify();
}
