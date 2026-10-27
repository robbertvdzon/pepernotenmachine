#pragma once

// Minimal MCP23017 driver (IOCON.BANK = 0 register map) over hardware I2C.
//
// Control-unit usage:  port A = LED outputs, port B = switch inputs (pull-ups).
//
// Thread safety: the LED task and the main loop both use this driver, so every
// public bus operation takes an internal mutex (Wire itself is not thread safe).
//
// Error handling: every I2C transaction is checked. A failure marks the
// driver "not ready", is logged (rate limited), and service() keeps trying to
// re-initialise the chip. The driver never pretends the expander is working.

#include <Arduino.h>
#include <Wire.h>

class Mcp23017 {
   public:
    Mcp23017(TwoWire& wire, uint8_t address) : wire_(wire), addr_(address) {}

    // Starts the I2C bus and configures the chip. Returns true if the chip
    // answered and the configuration was verified by read-back.
    bool begin();

    // Call regularly: retries initialisation while the chip is not ready.
    void service(uint32_t now);

    bool isReady() const { return ready_; }

    // Port B: returns raw pin levels (bit n = GPBn, 1 = HIGH).
    bool readPortB(uint8_t& value);

    // Port A: writes the output latch (bit n = GPAn, 1 = HIGH).
    bool writePortA(uint8_t value);

    // Incremented every time the chip is (re)configured. Users that cache
    // output state (LEDs) re-send it when this changes.
    uint32_t configGeneration() const { return generation_; }
    uint32_t errorCount() const { return errors_; }
    int lastError() const { return lastError_; }
    uint8_t address() const { return addr_; }

   private:
    bool configure();
    bool writeReg(uint8_t reg, uint8_t value);
    bool readReg(uint8_t reg, uint8_t& value);
    void noteFailure(const char* op, uint8_t reg, int code);

    TwoWire& wire_;
    uint8_t addr_;
    SemaphoreHandle_t lock_ = nullptr;
    bool busStarted_ = false;
    volatile bool ready_ = false;
    bool hadFailure_ = false;
    bool logged_ = false;
    uint32_t lastAttempt_ = 0;
    uint32_t lastLog_ = 0;
    uint32_t suppressed_ = 0;
    uint32_t errors_ = 0;
    uint32_t generation_ = 0;
    int lastError_ = 0;
};

extern Mcp23017 mcp;
