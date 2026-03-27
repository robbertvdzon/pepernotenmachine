#include "../include/ble_manager.h"
#include "../include/config.h"
#include "../include/led.h"
#include <NimBLEDevice.h>
#include <Arduino.h>

static NimBLEServer* pServer = nullptr;
static NimBLEService* pService = nullptr;
static NimBLECharacteristic* pMotorCharacteristic = nullptr;
static NimBLECharacteristic* pHornCharacteristic = nullptr;
static NimBLECharacteristic* pButtonCharacteristic = nullptr;
static NimBLECharacteristic* pLedCharacteristic = nullptr;
static NimBLECharacteristic* pLedCrossFadeStateCharacteristic = nullptr;
static NimBLECharacteristic* pServoCharacteristic = nullptr;
static NimBLECharacteristic* pRoutineCharacteristic = nullptr;

static motor_write_cb_t motor_cb = nullptr;
static horn_write_cb_t horn_cb = nullptr;
static servo_write_cb_t servo_cb = nullptr;
static routine_write_cb_t routine_cb = nullptr;
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
        if (speedValue > 512) speedValue = 512;
        if (speedValue < -512) speedValue = -512;

        if (motor_cb) motor_cb(speedValue);

        pMotorCharacteristic->setValue(data, v.size());
    }
};

class LedCharCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* c) override {
        NimBLEAttValue v = c->getValue();
        if (v.size() < 4) return;
        // forward raw bytes to led module
        led_set_from_bytes(reinterpret_cast<const uint8_t*>(v.data()), v.size());
        // update the characteristic value so clients can read the current state
        // store as 4-byte state: mode,r,g,b
        uint8_t state[4];
        led_get_state(state);
        c->setValue(state, 4);
    }
};

class HornCharCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* c) override {
        NimBLEAttValue v = c->getValue();
        if (v.size() == 0) return;
        bool on = (v.data()[0] != 0);
        if (horn_cb) horn_cb(on);
    }
};

class ServoCharCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* c) override {
        Serial.println("Received write to servo characteristic");
        NimBLEAttValue v = c->getValue();
        Serial.print("Value size: "); Serial.println(v.size());
        if (v.size() < 1) return;
        uint8_t angle = v.data()[0];
        if (angle > 180) angle = 180;
        if (servo_cb) servo_cb(angle);
        // update characteristic so a read returns the latest value
        pServoCharacteristic->setValue(&angle, 1);
    }
};

class RoutineCharCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* c) override {
        Serial.println("Received write to routine characteristic");
        NimBLEAttValue v = c->getValue();
        Serial.print("Value size: ");
        Serial.println(v.size());
        if (v.size() == 0) return;
        bool on = (v.data()[0] != 0);
        if (routine_cb) routine_cb();
    }
};

void ble_init(motor_write_cb_t motorCb, horn_write_cb_t hornCb, servo_write_cb_t servoCb, routine_write_cb_t routineCb) {
    motor_cb = motorCb;
    horn_cb = hornCb;
    servo_cb = servoCb;
    routine_cb = routineCb;

    NimBLEDevice::init("TOETER BLE");
    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());
    pService = pServer->createService(SERVICE_UUID);

    // Motor characteristic: read/write
    pMotorCharacteristic =
        pService->createCharacteristic(MOTOR_CHARACTERISTIC_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::READ);
    pMotorCharacteristic->setCallbacks(new MotorCharCallbacks());
    add_ccc_descriptor(pMotorCharacteristic);

    // Horn characteristic: write
    pHornCharacteristic = pService->createCharacteristic(HORN_CHARACTERISTIC_UUID, NIMBLE_PROPERTY::WRITE);
    pHornCharacteristic->setCallbacks(new HornCharCallbacks());

    // Servo characteristic: read/write
    pServoCharacteristic = pService->createCharacteristic(SERVO_CHARACTERISTIC_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::READ);
    pServoCharacteristic->setCallbacks(new ServoCharCallbacks());
    add_ccc_descriptor(pServoCharacteristic);
    uint8_t initAngle = 0;
    pServoCharacteristic->setValue(&initAngle, 1);

    // Button characteristic: read/notify
    pButtonCharacteristic = pService->createCharacteristic(BUTTON_CHARACTERISTIC_UUID, NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ);
    add_ccc_descriptor(pButtonCharacteristic);

    // LED characteristic: read/write/notify
    pLedCharacteristic = pService->createCharacteristic(LED_CHARACTERISTIC_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    add_ccc_descriptor(pLedCharacteristic);
    pLedCharacteristic->setCallbacks(new LedCharCallbacks());

    // LED crossfade state characteristic: read/notify
    pLedCrossFadeStateCharacteristic = pService->createCharacteristic(
        LED_CROSSFADE_STATE_CHARACTERISTIC_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    add_ccc_descriptor(pLedCrossFadeStateCharacteristic);
    uint8_t zero = 0; // initial state = not in crossfade
    pLedCrossFadeStateCharacteristic->setValue(&zero, 1);

    // Routine characteristic: write
    pRoutineCharacteristic = pService->createCharacteristic(ROUTINE_CHARACTERISTIC_UUID, NIMBLE_PROPERTY::WRITE);
    pRoutineCharacteristic->setCallbacks(new RoutineCharCallbacks());

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

void ble_notify_led_crossfade_started() {
    if (pLedCrossFadeStateCharacteristic == nullptr) return;
    Serial.println("Notifying LED crossfade started");
    uint8_t one = 1;
    pLedCrossFadeStateCharacteristic->setValue(&one, 1);
    pLedCrossFadeStateCharacteristic->notify();
}

void ble_notify_led_crossfade_done() {
    if (pLedCrossFadeStateCharacteristic == nullptr) return;
    Serial.println("Notifying LED crossfade done");
    uint8_t zero = 0;
    pLedCrossFadeStateCharacteristic->setValue(&zero, 1);
    pLedCrossFadeStateCharacteristic->notify();
}
