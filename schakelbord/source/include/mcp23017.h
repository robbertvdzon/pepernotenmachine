#pragma once
#include <stdint.h>

// MCP23017 port expander driver.
// All I2C traffic is owned by one FreeRTOS task, so callers never block on the bus:
// mcp_set_pin() only updates a shadow register and wakes the task.

void mcp_init();

// Set output `bit` (0..7) of `port` (MCP_PORT_A / MCP_PORT_B) to `level` (true = HIGH). Non-blocking.
void mcp_set_pin(uint8_t port, uint8_t bit, bool level);

// True when the last I2C transaction with the chip succeeded.
bool mcp_is_ok();
