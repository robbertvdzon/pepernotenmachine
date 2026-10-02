#include "../include/mcp23017.h"
#include "../include/config.h"
#include <Wire.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// MCP23017 registers (IOCON.BANK = 0)
static const uint8_t REG_IODIRA = 0x00;
static const uint8_t REG_OLATA = 0x14;

static uint8_t shadow[2];  // Desired output latch values: [0] = port A, [1] = port B
static portMUX_TYPE shadowMux = portMUX_INITIALIZER_UNLOCKED;
static TaskHandle_t mcpTask = nullptr;
static volatile bool chipOk = false;

// Write two consecutive registers (port A, port B) in one transaction.
static bool write_pair(uint8_t reg, uint8_t a, uint8_t b) {
    Wire.beginTransmission(MCP23017_I2C_ADDRESS);
    Wire.write(reg);
    Wire.write(a);
    Wire.write(b);
    return Wire.endTransmission() == 0;
}

static void copy_shadow(uint8_t out[2]) {
    portENTER_CRITICAL(&shadowMux);
    out[0] = shadow[0];
    out[1] = shadow[1];
    portEXIT_CRITICAL(&shadowMux);
}

static void mcp_task(void* pvParameters) {
    (void)pvParameters;
    bool configured = false;
    TickType_t wait = 0;  // First pass runs immediately to configure the chip

    for (;;) {
        // Wake up on a pin change, or after the refresh/retry interval
        uint32_t notified = ulTaskNotifyTake(pdTRUE, wait);
        bool periodic = (notified == 0);

        uint8_t v[2];
        copy_shadow(v);

        bool ok;
        if (!configured || periodic) {
            // Latch the desired levels first, then make the pins outputs (no glitch on the loads)
            ok = write_pair(REG_OLATA, v[0], v[1]) && write_pair(REG_IODIRA, 0x00, 0x00);
            if (ok && !configured) LOGF("MCP23017: configured");
            configured = ok;
        } else {
            ok = write_pair(REG_OLATA, v[0], v[1]);
            if (!ok) configured = false;
        }

        if (!ok && chipOk) LOGF("MCP23017: I2C error");
        chipOk = ok;
        wait = pdMS_TO_TICKS(ok ? MCP_REFRESH_INTERVAL_MS : MCP_RETRY_INTERVAL_MS);
    }
}

void mcp_init() {
    // Idle (inactive) levels so nothing switches on at boot
    uint8_t idleA = OUTPUT_ACTIVE_HIGH ? 0x00 : 0xFF;
    uint8_t idleB = RELAY_ACTIVE_HIGH ? 0x00 : 0xFF;
    shadow[MCP_PORT_A] = idleA;
    shadow[MCP_PORT_B] = idleB;

    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, MCP_I2C_FREQ_HZ);
    Wire.setTimeOut(MCP_I2C_TIMEOUT_MS);

    xTaskCreate(mcp_task, "mcp_task", MCP_TASK_STACK, NULL, MCP_TASK_PRIORITY, &mcpTask);
}

void mcp_set_pin(uint8_t port, uint8_t bit, bool level) {
    if (port > 1 || bit > 7) return;
    uint8_t mask = (uint8_t)(1u << bit);

    portENTER_CRITICAL(&shadowMux);
    if (level) shadow[port] |= mask;
    else shadow[port] &= (uint8_t)~mask;
    portEXIT_CRITICAL(&shadowMux);

    if (mcpTask != nullptr) xTaskNotifyGive(mcpTask);
}

bool mcp_is_ok() {
    return chipOk;
}
