#include "mcp23017.h"

#include "config.h"
#include "log.h"

static const char* TAG = "MCP";

// Register addresses (IOCON.BANK = 0).
namespace reg {
constexpr uint8_t IODIRA = 0x00;
constexpr uint8_t IODIRB = 0x01;
constexpr uint8_t IPOLA = 0x02;
constexpr uint8_t IPOLB = 0x03;
constexpr uint8_t GPINTENA = 0x04;
constexpr uint8_t GPINTENB = 0x05;
constexpr uint8_t IOCON = 0x0A;
constexpr uint8_t GPPUA = 0x0C;
constexpr uint8_t GPPUB = 0x0D;
constexpr uint8_t GPIOB = 0x13;
constexpr uint8_t OLATA = 0x14;
}  // namespace reg

Mcp23017 mcp(Wire, MCP23017_ADDRESS);

// RAII lock; a no-op before begin() has created the mutex.
namespace {
struct BusLock {
    SemaphoreHandle_t h;
    explicit BusLock(SemaphoreHandle_t s) : h(s) {
        if (h) xSemaphoreTake(h, portMAX_DELAY);
    }
    ~BusLock() {
        if (h) xSemaphoreGive(h);
    }
};
}  // namespace

static const char* wire_error_name(int code) {
    switch (code) {
        case 1: return "data too long";
        case 2: return "NACK on address (chip not responding)";
        case 3: return "NACK on data";
        case 4: return "bus error";
        case 5: return "timeout";
        case -1: return "no data returned";
        default: return "unknown";
    }
}

bool Mcp23017::begin() {
    if (!lock_) lock_ = xSemaphoreCreateMutex();
    wire_.begin(I2C_PIN_SDA, I2C_PIN_SCL, I2C_FREQ_HZ);
    wire_.setTimeOut(I2C_TIMEOUT_MS);
    busStarted_ = true;
    LOGI(TAG, "I2C started (SDA=%d SCL=%d %lu Hz), probing 0x%02X", (int)I2C_PIN_SDA, (int)I2C_PIN_SCL,
         (unsigned long)I2C_FREQ_HZ, addr_);
    return configure();
}

void Mcp23017::service(uint32_t now) {
    if (ready_ || !busStarted_) return;
    if (static_cast<uint32_t>(now - lastAttempt_) < MCP_RETRY_INTERVAL_MS) return;
    configure();
}

bool Mcp23017::configure() {
    BusLock guard(lock_);
    lastAttempt_ = millis();
    ready_ = false;

    // Probe: is anything at this address?
    wire_.beginTransmission(addr_);
    int err = wire_.endTransmission();
    if (err != 0) {
        noteFailure("probe", 0xFF, err);
        return false;
    }

    // Order matters: preload the LED latch with "all off" BEFORE port A becomes
    // an output, so the LEDs never flash on at start-up (LEDs are active LOW).
    const uint8_t led_off = LED_ACTIVE_LOW ? 0xFF : 0x00;
    bool ok = writeReg(reg::IOCON, 0x00) &&           // BANK=0, sequential mode
              writeReg(reg::GPINTENA, 0x00) &&        // no interrupts (INTA/INTB unwired)
              writeReg(reg::GPINTENB, 0x00) &&
              writeReg(reg::OLATA, led_off) &&
              writeReg(reg::IPOLA, 0x00) &&
              writeReg(reg::GPPUA, 0x00) &&
              writeReg(reg::IODIRA, 0x00) &&          // GPA0..7 = outputs (LEDs)
              writeReg(reg::IPOLB, 0x00) &&           // raw levels; polarity handled in switches.cpp
              writeReg(reg::GPPUB, 0xFF) &&           // internal pull-ups on switches
              writeReg(reg::IODIRB, 0xFF);            // GPB0..7 = inputs (switches)
    if (!ok) return false;

    // Verify by read-back; do not claim success on a chip that ignores writes.
    uint8_t iodira = 0xAA, iodirb = 0xAA, gppub = 0xAA, olata = 0xAA;
    ok = readReg(reg::IODIRA, iodira) && readReg(reg::IODIRB, iodirb) && readReg(reg::GPPUB, gppub) &&
         readReg(reg::OLATA, olata);
    if (!ok) return false;
    if (iodira != 0x00 || iodirb != 0xFF || gppub != 0xFF || olata != led_off) {
        noteFailure("verify", 0xFF, 4);
        LOGE(TAG, "read-back mismatch: IODIRA=%02X IODIRB=%02X GPPUB=%02X OLATA=%02X", iodira, iodirb, gppub, olata);
        return false;
    }

    ready_ = true;
    generation_++;
    if (hadFailure_) {
        LOGI(TAG, "MCP23017 @0x%02X recovered and re-configured (%lu errors so far)", addr_, (unsigned long)errors_);
    } else {
        LOGI(TAG, "MCP23017 @0x%02X initialised: GPA=LED outputs (active LOW, all off), GPB=switch inputs (pull-ups)",
             addr_);
    }
    hadFailure_ = false;
    return true;
}

bool Mcp23017::readPortB(uint8_t& value) {
    BusLock guard(lock_);
    if (!ready_) return false;
    if (!readReg(reg::GPIOB, value)) {
        ready_ = false;  // service() will re-initialise
        return false;
    }
    return true;
}

bool Mcp23017::writePortA(uint8_t value) {
    BusLock guard(lock_);
    if (!ready_) return false;
    if (!writeReg(reg::OLATA, value)) {
        ready_ = false;
        return false;
    }
    return true;
}

bool Mcp23017::writeReg(uint8_t r, uint8_t value) {
    wire_.beginTransmission(addr_);
    wire_.write(r);
    wire_.write(value);
    int err = wire_.endTransmission();
    if (err != 0) {
        noteFailure("write", r, err);
        return false;
    }
    return true;
}

bool Mcp23017::readReg(uint8_t r, uint8_t& value) {
    wire_.beginTransmission(addr_);
    wire_.write(r);
    int err = wire_.endTransmission(false);  // repeated start
    if (err != 0) {
        noteFailure("read(addr)", r, err);
        return false;
    }
    if (wire_.requestFrom(static_cast<int>(addr_), 1) != 1 || wire_.available() < 1) {
        noteFailure("read", r, -1);
        return false;
    }
    value = static_cast<uint8_t>(wire_.read());
    return true;
}

void Mcp23017::noteFailure(const char* op, uint8_t r, int code) {
    errors_++;
    lastError_ = code;
    hadFailure_ = true;
    uint32_t now = millis();
    // Rate limit: the first error is always logged, then at most one per 2 s.
    if (!logged_ || static_cast<uint32_t>(now - lastLog_) >= 2000) {
        LOGE(TAG, "I2C %s reg 0x%02X failed: %s (code %d)%s", op, r, wire_error_name(code), code,
             suppressed_ ? " [repeats suppressed]" : "");
        logged_ = true;
        lastLog_ = now;
        suppressed_ = 0;
    } else {
        suppressed_++;
    }
}
