#include "../include/ble_manager.h"
#include "../include/config.h"
#include <Arduino.h>
#include <NimBLEDevice.h>

static NimBLEServer* pServer = nullptr;
static NimBLEService* pService = nullptr;
static NimBLECharacteristic* pRelaysCharacteristic = nullptr;
static NimBLECharacteristic* pMosfetsCharacteristic = nullptr;
static NimBLECharacteristic* pFogStateCharacteristic = nullptr;
static NimBLECharacteristic* pFogControlCharacteristic = nullptr;
static NimBLECharacteristic* pFogDurationCharacteristic = nullptr;
static NimBLECharacteristic* pDigitalInputsCharacteristic = nullptr;
static NimBLECharacteristic* pAnalogInputsCharacteristic = nullptr;
static NimBLECharacteristic* pDigitalOutputsCharacteristic = nullptr;

static ble_callbacks_t cbs = {};
static int connectedCount = 0;

// Set the characteristic value and optionally notify subscribers.
static void update_value(NimBLECharacteristic* c, const uint8_t* data, size_t len, bool notify) {
    if (c == nullptr) return;
    c->setValue(data, len);
    if (notify) c->notify();
}

class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* srv) override {
        connectedCount++;
        LOGF("BLE connected (%d/%d)", connectedCount, BLE_MAX_CONNECTIONS);
        // continue advertising if we haven't reached the max connections
        if (connectedCount < BLE_MAX_CONNECTIONS) {
            NimBLEDevice::startAdvertising();
        } else {
            LOGF("Max connections reached, stopping advertising for now");
        }
    }
    void onDisconnect(NimBLEServer* srv) override {
        if (connectedCount > 0) connectedCount--;
        LOGF("BLE disconnected (%d/%d)", connectedCount, BLE_MAX_CONNECTIONS);
        // Failsafe: nobody is in control anymore, so switch everything off
        if (connectedCount == 0 && cbs.all_clients_disconnected) cbs.all_clients_disconnected();
        // restart advertising to accept new connections
        NimBLEDevice::startAdvertising();
    }
};

// Relays: write [relay, state] or [relay, state, intervalHi, intervalLo]
class RelaysCharCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* c) override {
        NimBLEAttValue v = c->getValue();
        const uint8_t* d = v.data();
        if (v.size() == 2) {
            if (cbs.relay_write) cbs.relay_write(d[0], d[1], 0);
        } else if (v.size() == 4) {
            uint16_t interval = (uint16_t)((d[2] << 8) | d[3]);
            if (cbs.relay_write) cbs.relay_write(d[0], d[1], interval);
        } else {
            LOGF("Relay write ignored: expected 2 or 4 bytes, got %u", (unsigned)v.size());
        }
    }
};

// MOSFETs: write [mosfet, percent]
class MosfetsCharCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* c) override {
        NimBLEAttValue v = c->getValue();
        if (v.size() != 2) {
            LOGF("MOSFET write ignored: expected 2 bytes, got %u", (unsigned)v.size());
            return;
        }
        if (cbs.mosfet_write) cbs.mosfet_write(v.data()[0], v.data()[1]);
    }
};

// Generic outputs: write [output, 0/1]
class OutputsCharCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* c) override {
        NimBLEAttValue v = c->getValue();
        if (v.size() != 2 || v.data()[1] > 1) {
            LOGF("Output write ignored: expected [number, 0/1]");
            return;
        }
        if (cbs.output_write) cbs.output_write(v.data()[0], v.data()[1]);
    }
};

// Fog control: write 0 = stop, 1 = start
class FogControlCharCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* c) override {
        NimBLEAttValue v = c->getValue();
        if (v.size() < 1 || v.data()[0] > 1) return;
        if (cbs.fog_control_write) cbs.fog_control_write(v.data()[0]);
    }
};

// Fog burst duration: write seconds
class FogDurationCharCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* c) override {
        NimBLEAttValue v = c->getValue();
        if (v.size() < 1) return;
        if (cbs.fog_duration_write) cbs.fog_duration_write(v.data()[0]);
    }
};

void ble_init(const ble_callbacks_t& callbacks) {
    cbs = callbacks;

    NimBLEDevice::init(BLE_DEVICE_NAME);
    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());
    pService = pServer->createService(SERVICE_UUID);

    const uint32_t RW = NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE;
    const uint32_t RN = NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY;
    const uint32_t RWN = RW | NIMBLE_PROPERTY::NOTIFY;

    // Relays: read/write/notify. Read/notify payload: one state byte per relay (0 off, 1 on, 2 blinking).
    pRelaysCharacteristic = pService->createCharacteristic(RELAYS_CHARACTERISTIC_UUID, RWN);
    pRelaysCharacteristic->setCallbacks(new RelaysCharCallbacks());

    // MOSFETs: read/write/notify. Read/notify payload: one percentage byte per MOSFET.
    pMosfetsCharacteristic = pService->createCharacteristic(MOSFETS_CHARACTERISTIC_UUID, RWN);
    pMosfetsCharacteristic->setCallbacks(new MosfetsCharCallbacks());

    // Fog state: read/notify, one byte.
    pFogStateCharacteristic = pService->createCharacteristic(FOG_STATE_CHARACTERISTIC_UUID, RN);

    // Fog control: read/write/notify, one byte (0 = not fogging, 1 = fogging session active).
    pFogControlCharacteristic = pService->createCharacteristic(FOG_CONTROL_CHARACTERISTIC_UUID, RWN);
    pFogControlCharacteristic->setCallbacks(new FogControlCharCallbacks());

    // Fog burst duration: read/write, one byte in seconds.
    pFogDurationCharacteristic = pService->createCharacteristic(FOG_DURATION_CHARACTERISTIC_UUID, RW);
    pFogDurationCharacteristic->setCallbacks(new FogDurationCharCallbacks());

    // Digital inputs: read/notify, one bitmask byte.
    pDigitalInputsCharacteristic = pService->createCharacteristic(DIGITAL_INPUTS_CHARACTERISTIC_UUID, RN);

    // Analog inputs: read/notify, big-endian uint16 millivolts per channel.
    pAnalogInputsCharacteristic = pService->createCharacteristic(ANALOG_INPUTS_CHARACTERISTIC_UUID, RN);

    // Digital outputs: read/write/notify. Read/notify payload: one bitmask byte.
    pDigitalOutputsCharacteristic = pService->createCharacteristic(DIGITAL_OUTPUTS_CHARACTERISTIC_UUID, RWN);
    pDigitalOutputsCharacteristic->setCallbacks(new OutputsCharCallbacks());

    // Sensible initial values so a read before the first update returns something valid
    uint8_t zeros[8] = {0};
    pRelaysCharacteristic->setValue(zeros, RELAY_COUNT);
    pMosfetsCharacteristic->setValue(zeros, MOSFET_COUNT);
    pFogStateCharacteristic->setValue(zeros, 1);
    pFogControlCharacteristic->setValue(zeros, 1);
    uint8_t duration = FOG_BURST_DEFAULT_SECONDS;
    pFogDurationCharacteristic->setValue(&duration, 1);
    pDigitalInputsCharacteristic->setValue(zeros, 1);
    pAnalogInputsCharacteristic->setValue(zeros, ANALOG_INPUT_COUNT * 2);
    pDigitalOutputsCharacteristic->setValue(zeros, 1);

    pService->start();

    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->start();
    LOGF("BLE advertising as \"%s\"", BLE_DEVICE_NAME);
}

bool ble_is_connected() {
    return connectedCount > 0;
}

void ble_update_relays(const uint8_t* states, size_t count) {
    update_value(pRelaysCharacteristic, states, count, true);
}

void ble_update_mosfets(const uint8_t* percent, size_t count) {
    update_value(pMosfetsCharacteristic, percent, count, true);
}

void ble_update_digital_outputs(uint8_t mask) {
    update_value(pDigitalOutputsCharacteristic, &mask, 1, true);
}

void ble_update_fog_state(uint8_t state) {
    update_value(pFogStateCharacteristic, &state, 1, true);
}

void ble_update_fog_control(uint8_t active) {
    update_value(pFogControlCharacteristic, &active, 1, true);
}

void ble_update_fog_duration(uint8_t seconds) {
    update_value(pFogDurationCharacteristic, &seconds, 1, false);
}

void ble_update_digital_inputs(uint8_t mask) {
    update_value(pDigitalInputsCharacteristic, &mask, 1, true);
}

void ble_update_analog_inputs(const uint16_t* millivolts, size_t count) {
    uint8_t buf[2 * 8];
    if (count > 8) count = 8;
    for (size_t i = 0; i < count; i++) {
        buf[2 * i] = (uint8_t)(millivolts[i] >> 8);
        buf[2 * i + 1] = (uint8_t)(millivolts[i] & 0xFF);
    }
    update_value(pAnalogInputsCharacteristic, buf, count * 2, true);
}
