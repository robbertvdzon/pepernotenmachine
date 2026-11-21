# Switchboard firmware

This project implements the BLE-controlled IO board for the KiCad `schakelbord` design. The firmware runs on an Arduino Nano ESP32 and exposes relay, MOSFET, digital output, digital input, analog input, and fog-machine control over BLE.

The current implementation is organized around a small FreeRTOS task per subsystem, with non-blocking BLE callbacks and interrupt-driven input handling. The authoritative configuration lives in [`include/config.h`](include/config.h), and the runtime behavior is implemented in the firmware sources under [`src/`](src/).

## What the board exposes

- 8 relay channels on the MCP23017 port expander
- 3 PWM MOSFET channels
- 8 digital outputs on MCP23017 port A
- 4 digital inputs with interrupt-driven debouncing
- 4 analog inputs sampled and reported in millivolts
- 1 fog machine control interface with a ready/heating/fogging state machine

## Hardware mapping

All pin assignments, thresholds, timings, UUIDs, and protocol values are in [`include/config.h`](include/config.h).

| Function | Connector | Pins | Notes |
| --- | --- | --- | --- |
| Relays 1-2 | J21 | MCP23017 GPB0-GPB1 | Active HIGH, driven through BC337 |
| Relays 3-4 | J20 | MCP23017 GPB2-GPB3 | Active HIGH |
| Relays 5-6 | J19 | MCP23017 GPB4-GPB5 | Active HIGH |
| Relays 7-8 | J15 | MCP23017 GPB6-GPB7 | Active HIGH |
| Digital outputs 1-4 | J17 | MCP23017 GPA0-GPA3 | Active LOW (sinking) |
| Digital outputs 5-8 | J18 | MCP23017 GPA4-GPA7 | Active LOW (sinking) |
| MOSFETs 1-3 | J4 | D2, D3, D4 | PWM at 1 kHz |
| Digital inputs 1-4 | J12 | D5, D6, D7, D8 | Active LOW, internal pull-up enabled |
| Analog inputs 1-4 | J16 | A1, A2, A3, A6 | Reported in millivolts |
| Fog trigger | J1 | D9 | HIGH turns on the fog machine trigger |
| Fog heat sensor | J1 | A0 | Analog reading used for readiness detection |
| Fog connected sense | J1 | A7 | HIGH when machine is connected |
| MCP23017 bus | - | SDA = A4, SCL = A5 | I2C address 0x20 |

## BLE service

The firmware creates a single BLE service with device name `IO Switchboard` and supports up to 3 simultaneous connections.

Service UUID:

- `3dc93769-33d3-4883-ae7b-877c14fd3eca`

### Characteristics

| Characteristic | UUID | Properties | Payload |
| --- | --- | --- | --- |
| Relays | `3dc93701-33d3-4883-ae7b-877c14fd3eca` | Read, write, notify | Write: `[relay, state]` or `[relay, state, intervalHi, intervalLo]`. State values: `0` off, `1` on, `2` blink. Interval is in ms and is clamped to the configured minimum/maximum values. Notify/read returns 8 bytes with one relay state per relay. |
| MOSFETs | `3dc93702-33d3-4883-ae7b-877c14fd3eca` | Read, write, notify | Write: `[mosfet, percent]` where percent is `0..100`. Notify/read returns 3 bytes, one byte per MOSFET percentage. |
| Fog state | `3dc93703-33d3-4883-ae7b-877c14fd3eca` | Read, notify | `0` not connected, `1` heating up, `2` ready, `3` fogging. |
| Fog control | `3dc93704-33d3-4883-ae7b-877c14fd3eca` | Read, write, notify | Write `1` to start, `0` to stop. Start is accepted only when the machine is ready. Read/notify reports `1` if a fogging session is currently active, otherwise `0`. |
| Fog duration | `3dc93705-33d3-4883-ae7b-877c14fd3eca` | Read, write | Single-byte burst length in seconds. Default value is 5 seconds. |
| Digital inputs | `3dc93706-33d3-4883-ae7b-877c14fd3eca` | Read, notify | 1-byte bitmask; bit 0 = input 1 and `1` means the input is active (pin is LOW). |
| Analog inputs | `3dc93707-33d3-4883-ae7b-877c14fd3eca` | Read, notify | 4 x `uint16` millivolts, encoded big-endian, 8 bytes total. Notifications are sent when any channel moves by at least 50 mV and no more often than every 200 ms. |
| Digital outputs | `3dc93708-33d3-4883-ae7b-877c14fd3eca` | Read, write, notify | Write: `[output, 0/1]` where output is `1..8`. Read/notify returns a bitmask byte; bit 0 = output 1. |

The long-form UUIDs are defined in [`include/config.h`](include/config.h). The characteristic UUIDs differ only in the first 4 bytes, matching the service UUID block shown above.

### Raw write examples

- Relay 3 ON: `03 01`
- Relay 3 OFF: `03 00`
- Relay 3 blink at the default 500 ms interval: `03 02`
- Relay 3 blink every 250 ms: `03 02 00 FA`
- MOSFET 2 at 75%: `02 4B`
- Output 5 ON: `05 01`
- Set fog burst duration to 8 seconds: `08` on the fog-duration characteristic
- Start fogging: `01` on the fog-control characteristic
- Stop fogging: `00` on the fog-control characteristic

## Fog machine behavior

The fog machine logic is implemented in [`src/fog_machine.cpp`](src/fog_machine.cpp) and uses the current threshold values from [`include/config.h`](include/config.h):

- `FOG_READY_THRESHOLD_MV = 1000`
- `FOG_HYSTERESIS_MV = 100`
- `FOG_BURST_DEFAULT_SECONDS = 5`
- `FOG_MAX_SESSION_MS = 120000` (0 disables the safety cap)

The effective logic is:

1. The machine is considered connected only when `A7` reads HIGH.
2. The machine is considered ready when the heat sensor at `A0` reads at or above 1000 mV, with 100 mV hysteresis. In practice the code keeps the state READY until the reading drops below 900 mV, and it becomes READY again only once it rises back to 1100 mV.
3. A `start` command is accepted only while the machine is in the READY state and is connected.
4. When a burst starts, `D9` goes HIGH and the firmware publishes `FOG_STATE_FOGGING`.
5. After the configured burst duration elapses, the trigger is turned off and a 200 ms settle window is enforced before measuring A0 again.
6. If the sensor is still hot enough, the next burst starts automatically. Otherwise the fogging session ends and the state is re-evaluated as HEATING or READY.
7. A stop command turns the trigger off immediately and then re-evaluates the state after the 200 ms settle delay.
8. If the machine becomes disconnected while fogging, the trigger is disabled and the state transitions to NOT_CONNECTED.
9. Every state transition is notified over BLE. A rejected start request is signaled by a notification on the fog-control characteristic with value `0`.

## Safety and fail-safe behavior

The firmware includes a connection-level fail-safe:

- When the last BLE client disconnects, all relays, MOSFETs and digital outputs are switched off.
- Fogging is stopped immediately.
- The board returns to a safe idle state with no retained client control.

## Software structure

The project layout matches the current implementation:

- [`src/main.cpp`](src/main.cpp): startup, module initialization, BLE callback wiring, and the global failsafe.
- [`src/ble_manager.cpp`](src/ble_manager.cpp): BLE server, advertising, characteristics, and connection handling.
- [`src/mcp23017.cpp`](src/mcp23017.cpp): MCP23017 I2C task and shared port-expander access.
- [`src/relays.cpp`](src/relays.cpp): relay state machine and blink-timer logic.
- [`src/mosfets.cpp`](src/mosfets.cpp): LEDC PWM generation for the 3 MOSFETs.
- [`src/digital_outputs.cpp`](src/digital_outputs.cpp): generic digital output behavior.
- [`src/digital_inputs.cpp`](src/digital_inputs.cpp): interrupt-driven input edge detection and debouncing.
- [`src/analog_inputs.cpp`](src/analog_inputs.cpp): periodic ADC sampling and change-based BLE notifications.
- [`src/fog_machine.cpp`](src/fog_machine.cpp): fogging state machine and burst timing.
- [`include/config.h`](include/config.h): all pin assignments, UUIDs, protocol values, timings, and thresholds.

## Building, uploading, and monitoring

Use PlatformIO from the project root:

```bash
pio run
pio run -t upload
pio device monitor
```

The Nano ESP32 uses a DFU bootloader. If upload does not start, double-tap the reset button and retry the upload.

## Notes

The firmware is designed to avoid blocking behavior in normal operation:

- input edges are handled by interrupts
- relay blink timing is handled by FreeRTOS timers
- fogging, analog sampling, debouncing, and the MCP23017 I2C task each run in their own task or timer context
