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

static motor_write_cb_t motor_cb = nullptr;
static horn_write_cb_t horn_cb = nullptr;
static bool deviceConnected = false;

class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* srv) override {
        deviceConnected = true;
        Serial.println("BLE connected");
    }
    void onDisconnect(NimBLEServer* srv) override {
        deviceConnected = false;
        Serial.println("BLE disconnected");
        // restart advertising
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

void ble_init(motor_write_cb_t motorCb, horn_write_cb_t hornCb) {
    motor_cb = motorCb;
    horn_cb = hornCb;

    NimBLEDevice::init("TOETER BLE");
    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());
    pService = pServer->createService(SERVICE_UUID);

    pMotorCharacteristic =
        pService->createCharacteristic(MOTOR_CHARACTERISTIC_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::READ);
    pMotorCharacteristic->setCallbacks(new MotorCharCallbacks());
    // Explicitly add CCC (0x2902) descriptor so older/quirky clients can find it
    {
        NimBLEDescriptor* ccc = new NimBLEDescriptor(NimBLEUUID((uint16_t)0x2902), 0, 2, pMotorCharacteristic);
        uint8_t off[2] = {0x00, 0x00};
        ccc->setValue(off, 2);
        pMotorCharacteristic->addDescriptor(ccc);
    }

    pHornCharacteristic = pService->createCharacteristic(HORN_CHARACTERISTIC_UUID, NIMBLE_PROPERTY::WRITE);
    pHornCharacteristic->setCallbacks(new HornCharCallbacks());

    pButtonCharacteristic = pService->createCharacteristic(BUTTON_CHARACTERISTIC_UUID, NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ);
    // Explicitly add CCC (0x2902) descriptor so older/quirky clients can find it
    {
        NimBLEDescriptor* ccc = new NimBLEDescriptor(NimBLEUUID((uint16_t)0x2902), 0, 2, pButtonCharacteristic);
        uint8_t off[2] = {0x00, 0x00};
        ccc->setValue(off, 2);
        pButtonCharacteristic->addDescriptor(ccc);
    }

    // LED characteristic: read/write/notify
    pLedCharacteristic = pService->createCharacteristic(LED_CHARACTERISTIC_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    // Explicitly add CCC (0x2902) descriptor for LED notify
    {
        NimBLEDescriptor* ccc2 = new NimBLEDescriptor(NimBLEUUID((uint16_t)0x2902), 0, 2, pLedCharacteristic);
        uint8_t off2[2] = {0x00, 0x00};
        ccc2->setValue(off2, 2);
        pLedCharacteristic->addDescriptor(ccc2);
    }
    pLedCharacteristic->setCallbacks(new LedCharCallbacks());

    // LED crossfade state characteristic: read/notify
    pLedCrossFadeStateCharacteristic = pService->createCharacteristic(
        LED_CROSSFADE_STATE_CHARACTERISTIC_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    // Explicitly add CCC (0x2902) descriptor for crossfade state notify
    {
        NimBLEDescriptor* ccc2 =
            new NimBLEDescriptor(NimBLEUUID((uint16_t)0x2902), 0, 2, pLedCrossFadeStateCharacteristic);
        uint8_t off2[2] = {0x00, 0x00};
        ccc2->setValue(off2, 2);
        pLedCrossFadeStateCharacteristic->addDescriptor(ccc2);
    }
    uint8_t zero = 0; // initial state = not in crossfade
    pLedCrossFadeStateCharacteristic->setValue(&zero, 1);

    pService->start();

    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    // start advertising
    pAdvertising->start();
}

bool ble_is_connected() {
    return deviceConnected;
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
