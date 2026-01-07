#include "../include/ble_manager.h"
#include "../include/config.h"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <Arduino.h>

static BLEServer* pServer = nullptr;
static BLEService* pService = nullptr;
static BLECharacteristic* pMotorCharacteristic = nullptr;
static BLECharacteristic* pHornCharacteristic = nullptr;
static BLECharacteristic* pButtonCharacteristic = nullptr;

static motor_write_cb_t motor_cb = nullptr;
static horn_write_cb_t horn_cb = nullptr;
static bool deviceConnected = false;

class ServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* srv) override {
        deviceConnected = true;
        Serial.println("BLE connected");
    }
    void onDisconnect(BLEServer* srv) override {
        deviceConnected = false;
        Serial.println("BLE disconnected");
        srv->startAdvertising();
    }
};

class MotorCharCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* c) override {
        std::string v = c->getValue();
        if (v.length() < 4) return;
        uint32_t speedValue = (static_cast<uint8_t>(v[0]) << 24) |
                              (static_cast<uint8_t>(v[1]) << 16) |
                              (static_cast<uint8_t>(v[2]) << 8) |
                              (static_cast<uint8_t>(v[3]));
        if (motor_cb) motor_cb(speedValue);
    }
};

class HornCharCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* c) override {
        std::string v = c->getValue();
        if (v.length() == 0) return;
        bool on = (v[0] != 0);
        if (horn_cb) horn_cb(on);
    }
};

void ble_init(motor_write_cb_t motorCb, horn_write_cb_t hornCb) {
    motor_cb = motorCb;
    horn_cb = hornCb;

    BLEDevice::init("TOETER BLE");
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());
    pService = pServer->createService(SERVICE_UUID);

    pMotorCharacteristic = pService->createCharacteristic(MOTOR_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_WRITE);
    pMotorCharacteristic->setCallbacks(new MotorCharCallbacks());

    pHornCharacteristic = pService->createCharacteristic(HORN_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_WRITE);
    pHornCharacteristic->setCallbacks(new HornCharCallbacks());

    pButtonCharacteristic = pService->createCharacteristic(BUTTON_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_READ);
    pButtonCharacteristic->addDescriptor(new BLE2902());

    pService->start();

    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
}

bool ble_is_connected() {
    return deviceConnected;
}

void ble_notify_button(uint8_t payload) {
    if (pButtonCharacteristic == nullptr) return;
    std::string val(1, static_cast<char>(payload));
    pButtonCharacteristic->setValue(val);
    pButtonCharacteristic->notify();
}
