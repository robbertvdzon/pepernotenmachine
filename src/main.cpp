#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

int PUL = 14;
int DIR = 20; 
int ENA = 22;
int HORN = 32;

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define MOTOR_CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define HORN_CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a9"

BLEServer* pServer;
BLEService* pService;
BLECharacteristic* pMotorCharacteristic;
BLECharacteristic* pHornCharacteristic;
TaskHandle_t bleTask;

uint32_t motorPulseDelay = 50;
bool deviceConnected = false;
uint32_t lastHornTurnedOnTime = 0;

// PWM (LEDC) configuration for generating step pulses on ESP32
const int pwmChannel = 0;
const int pwmResolution = 8; // 8-bit resolution
const int pwmDuty = 128;     // 50% duty by default (adjust for pulse width)

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
        Serial.println("Connected");
    };

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
        Serial.println("Disconnected");
        pServer->startAdvertising();
    }
};

class MotorCharacteristicCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic) {
        std::string value = pCharacteristic->getValue();

        // Guard against short/invalid writes from a client
        if (value.length() < 2) {
            Serial.println("MotorCharacteristic: write too short, ignoring");
            return;
        }

        uint32_t speedValue = (static_cast<uint8_t>(value[0]) << 8) | static_cast<uint8_t>(value[1]);

        if (speedValue != 0) {
            motorPulseDelay = speedValue;
            Serial.print("Speed set to: ");
            Serial.println(motorPulseDelay);

            uint32_t newFreq = 500000UL / motorPulseDelay;
            if (newFreq < 1) newFreq = 1;
            if (newFreq > 40000) newFreq = 40000;
            // Reconfigure LEDC channel with the new frequency. Duty stays the same.
            ledcSetup(pwmChannel, newFreq, pwmResolution);
            ledcWrite(pwmChannel, pwmDuty);
        }
    }
};

class HornCharacteristicCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic) {
        std::string value = pCharacteristic->getValue();
        if (value[0] != 0) {
            digitalWrite(HORN, HIGH);
            lastHornTurnedOnTime = millis();
            Serial.println("Horn ON");
        } else {
            digitalWrite(HORN, LOW);
            Serial.println("Horn OFF");
        }
    }
};

static void serverTask(void* parameter) {
    Serial.println("Starting BLE work!");

    BLEDevice::init("TOETER BLE");
    pServer = BLEDevice::createServer();
    pService = pServer->createService(SERVICE_UUID);
    pServer->setCallbacks(new MyServerCallbacks());

    // Create characteristics and set callbacks before starting the service.
    pMotorCharacteristic = pService->createCharacteristic(MOTOR_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_WRITE);
    pMotorCharacteristic->setCallbacks(new MotorCharacteristicCallbacks());
    pHornCharacteristic = pService->createCharacteristic(HORN_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_WRITE);
    pHornCharacteristic->setCallbacks(new HornCharacteristicCallbacks());

    pService->start();

    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();

    for (;;) {
        vTaskDelay(100 / portTICK_PERIOD_MS);
        // Auto-turn-off horn after 2 seconds (failsafe)
        if (digitalRead(HORN) == HIGH) {
            if (millis() - lastHornTurnedOnTime >= 2000) {
                digitalWrite(HORN, LOW);
                Serial.println("Horn AUTO OFF");        
            }    
        }               
    }
}

void setup() {
    Serial.begin(115200);

    pinMode(PUL, OUTPUT);
    pinMode(DIR, OUTPUT);
    pinMode(ENA, OUTPUT);

    digitalWrite(ENA, LOW);
    digitalWrite(DIR, HIGH);
    digitalWrite(PUL, LOW);

    pinMode(HORN, OUTPUT);
    digitalWrite(HORN, LOW);

    // Initialize PWM on the PUL pin. We'll set an initial frequency based
    // on the current `motorPulseDelay` value. The `motorPulseDelay` variable is interpreted as
    // the half-period in microseconds (same convention as before), so the
    // step pulse frequency (Hz) = 1 / (2 * motorPulseDelay * 1e-6) = 500000 / motorPulseDelay.
    uint32_t initFreq = 1;
    if (motorPulseDelay != 0) {
        initFreq = 500000UL / motorPulseDelay; // derived from previous timing
    }
    if (initFreq < 1) initFreq = 1;
    // Clamp a reasonable upper limit for PWM frequency (driver dependent)
    if (initFreq > 40000) initFreq = 40000;

    ledcSetup(pwmChannel, initFreq, pwmResolution);
    ledcAttachPin(PUL, pwmChannel);
    ledcWrite(pwmChannel, pwmDuty);

    xTaskCreatePinnedToCore(serverTask, "bleTask", 10000, NULL, 1, &bleTask, 0);
}

void loop() {
    // Nothing else to do in the loop; PWM generates pulses in hardware.
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}