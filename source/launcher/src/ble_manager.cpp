#include "../include/ble_manager.h"
#include "../include/config.h"
#include <NimBLEDevice.h>
#include <Arduino.h>

static NimBLEServer* pServer = nullptr;
static NimBLEService* pService = nullptr;
static NimBLECharacteristic* pPullMotorCharacteristic = nullptr;
static NimBLECharacteristic* pButtonCharacteristic = nullptr;
static NimBLECharacteristic* pReleaseServoCharacteristic = nullptr;
static NimBLECharacteristic* pSequenceCharacteristic = nullptr;
static NimBLECharacteristic* pDispenserDurationCharacteristic = nullptr;
static NimBLECharacteristic* pDispenserControlCharacteristic = nullptr;

static pull_motor_write_cb_t pull_motor_cb = nullptr;
static release_servo_write_cb_t release_servo_cb = nullptr;
static sequence_write_cb_t sequence_cb = nullptr;
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
    void onWrite(NimBLECharacteristic* c) override {
        NimBLEAttValue v = c->getValue();
        if (v.size() < 4) return;

        const uint8_t* data = v.data();

        int32_t speedValue = (static_cast<int32_t>(data[0]) << 24) | (static_cast<int32_t>(data[1]) << 16) |
                             (static_cast<int32_t>(data[2]) << 8) | (static_cast<int32_t>(data[3]));

        // Clamp to allowed range
        if (speedValue > MOTOR_SPEED_MAX) speedValue = MOTOR_SPEED_MAX;
        if (speedValue < -MOTOR_SPEED_MAX) speedValue = -MOTOR_SPEED_MAX;

        if (pull_motor_cb) pull_motor_cb(speedValue);

        pPullMotorCharacteristic->setValue(data, v.size());
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
        // update characteristic so a read returns the latest value
        pReleaseServoCharacteristic->setValue(&angle, 1);
    }
};

class SequenceCharCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* c) override {
        Serial.println("Received write to sequence characteristic");
        NimBLEAttValue v = c->getValue();
        Serial.print("Value size: ");
        Serial.println(v.size());
        if (v.size() == 0) return;
        bool on = (v.data()[0] != 0);
        if (sequence_cb) sequence_cb();
    }
};

class DispenserDurationCharCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* c) override {
        NimBLEAttValue v = c->getValue();
        if (v.size() < 1) return;
        uint8_t duration = v.data()[0];
        if (dispenser_duration_cb) dispenser_duration_cb(duration);
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

void ble_init(pull_motor_write_cb_t motorCb, release_servo_write_cb_t releaseServoCb, sequence_write_cb_t sequenceCb, dispenser_duration_write_cb_t dispenserDurationCb, dispenser_start_write_cb_t dispenserStartCb) {
    pull_motor_cb = motorCb;
    release_servo_cb = releaseServoCb;
    sequence_cb = sequenceCb;
    dispenser_duration_cb = dispenserDurationCb;
    dispenser_start_cb = dispenserStartCb;

    NimBLEDevice::init("TOETER BLE");
    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());
    pService = pServer->createService(SERVICE_UUID);

    // Motor characteristic: read/write
    pPullMotorCharacteristic =
        pService->createCharacteristic(PULL_MOTOR_CHARACTERISTIC_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::READ);
    pPullMotorCharacteristic->setCallbacks(new MotorCharCallbacks());
    add_ccc_descriptor(pPullMotorCharacteristic);

    // Servo characteristic: read/write
    pReleaseServoCharacteristic = pService->createCharacteristic(RELEASE_SERVO_CHARACTERISTIC_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::READ);
    pReleaseServoCharacteristic->setCallbacks(new ReleaseServoCharCallbacks());
    add_ccc_descriptor(pReleaseServoCharacteristic);
    uint8_t initAngle = 0;
    pReleaseServoCharacteristic->setValue(&initAngle, 1);

    // Button characteristic: read/notify
    pButtonCharacteristic = pService->createCharacteristic(BUTTON_CHARACTERISTIC_UUID, NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ);
    add_ccc_descriptor(pButtonCharacteristic);

    // Sequence characteristic: write
    pSequenceCharacteristic = pService->createCharacteristic(SEQUENCE_CHARACTERISTIC_UUID, NIMBLE_PROPERTY::WRITE);
    pSequenceCharacteristic->setCallbacks(new SequenceCharCallbacks());

    // Dispenser duration characteristic: read/write
    pDispenserDurationCharacteristic = pService->createCharacteristic(DISPENSER_DURATION_CHARACTERISTIC_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::READ);
    pDispenserDurationCharacteristic->setCallbacks(new DispenserDurationCharCallbacks());
    add_ccc_descriptor(pDispenserDurationCharacteristic);
    uint8_t initDuration = DISPENSER_DEFAULT_DURATION; // default duration
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

void ble_notify_dispenser_start(uint8_t state) {
    if (pDispenserControlCharacteristic == nullptr) return;
    Serial.print("Notifying dispenser start state: ");
    Serial.println(state);
    pDispenserControlCharacteristic->setValue(&state, 1);
    pDispenserControlCharacteristic->notify();
}
