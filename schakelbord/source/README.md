# Switchboard

Firmware for the BLE general purpose IO board (KiCad project `schakelbord`), built for an **Arduino Nano ESP32** with PlatformIO and the Arduino framework. It switches 8 AC relays, 3 PWM MOSFET channels, 8 generic digital outputs and a fog machine, and reports 4 digital inputs and 4 analog inputs, all over BLE.

Nothing in the firmware blocks: input edges use interrupts, relay blinking uses FreeRTOS timers, and the fog machine, analog sampling, debouncing and the I2C bus each run in their own small FreeRTOS task.

## Hardware mapping

All pins and options are in [`include/config.h`](include/config.h).

| Function | Connector | Pins | Notes |
| --- | --- | --- | --- |
| Relays 1-8 | J8-J11 | MCP23017 GPB0-GPB7 | Active HIGH (via BC337) |
| Generic outputs 1-8 | J6, J7 | MCP23017 GPA0-GPA7 | Active LOW (sinking) |
| MOSFETs 1-3 | J3 | D6, D7, D8 | PWM, 1 kHz |
| Generic digital inputs 1-4 | J2 | D2-D5 | Active LOW, internal pull-up |
| Analog inputs 1-4 | J4 | A0-A3 | Reported in millivolts |
| Fog machine trigger | J5 | D9 | HIGH = fogging |
| Fog machine heat sensor | J5 | A6 | ~0 V heating, ~2.5 V ready |
| Fog machine connected | J5 | A7 | HIGH = connected |
| MCP23017 I2C | | SDA = A4, SCL = A5 | Address 0x20 |

## BLE service

Device name: `IO Switchboard` (see `BLE_DEVICE_NAME`). Up to 3 simultaneous connections. All multi-byte values are big-endian.

Service UUID: `3dc93769-33d3-4883-ae7b-877c14fd3eca`

| Characteristic | UUID | Properties | Payload |
| --- | --- | --- | --- |
| Relays | `3dc93701-...` | Read, write, notify | Write: `[relay 1-8, state]` or `[relay, state, intervalHi, intervalLo]`. State: `0` off, `1` on, `2` blink. Interval in ms (default 500, limits in config). Read/notify: 8 bytes, one state per relay |
| MOSFETs | `3dc93702-...` | Read, write, notify | Write: `[mosfet 1-3, percent 0-100]`. Read/notify: 3 bytes, percent per MOSFET |
| Fog state | `3dc93703-...` | Read, notify | `0` not connected, `1` heating up, `2` ready, `3` fogging |
| Fog control | `3dc93704-...` | Read, write, notify | Write `1` start (only accepted when state is ready), `0` stop. Read/notify: `1` fogging session active, `0` not |
| Fog burst duration | `3dc93705-...` | Read, write | Seconds, default 5 (limits in config) |
| Digital inputs | `3dc93706-...` | Read, notify | 1 byte bitmask, bit 0 = input 1, `1` = active (pin LOW) |
| Analog inputs | `3dc93707-...` | Read, notify | 4 x uint16 millivolts (8 bytes), notified on change of at least 50 mV |
| Digital outputs | `3dc93708-...` | Read, write, notify | Write: `[output 1-8, 0/1]`. Read/notify: 1 byte bitmask, bit 0 = output 1 |

The full UUIDs are in `include/config.h` (only the first byte group differs).

### Examples (raw hex writes)

- Relay 3 on: `03 01`
- Relay 3 off: `03 00`
- Relay 3 blink, default 500 ms: `03 02`
- Relay 3 blink every 250 ms: `03 02 00 FA`
- MOSFET 2 at 75%: `02 4B`
- Output 5 on: `05 01`
- Set fog burst to 8 seconds: `08` (duration characteristic)
- Start fogging: `01`, stop fogging: `00` (control characteristic)

## Fog machine behavior

State is derived from A7 (connected) and A6 (heat sensor, with 100 mV hysteresis around 1 V: ready at 1.1 V or more, back to heating below 0.9 V).

1. Start is only accepted in state *ready*. D9 goes HIGH and the state becomes *fogging*.
2. After the burst duration, D9 goes LOW (the state stays *fogging*). After 200 ms A6 is measured.
3. If the machine is still hot enough, the next burst starts. Otherwise the session ends and the state is re-evaluated (normally *heating up*).
4. A stop command sets D9 LOW immediately and, after 200 ms, the state is re-evaluated.
5. If A7 goes LOW while fogging, fogging stops and the state becomes *not connected*.
6. A session is stopped after `FOG_MAX_SESSION_MS` (default 120 s, `0` disables this safety cap).

Every state change is notified. A rejected start request is answered with a notification on the control characteristic (value `0`).

## Failsafe

When the last connected client disconnects, all relays, MOSFETs and outputs are switched off and fogging is stopped.

## Software structure

- `src/main.cpp` initializes all modules and wires the BLE callbacks to them.
- `src/ble_manager.cpp` creates the service, characteristics, advertising and connection handling.
- `src/mcp23017.cpp` owns the I2C bus in one task; other modules only set shadow bits.
- `src/relays.cpp` relay states and blink timers.
- `src/mosfets.cpp` LEDC PWM per MOSFET.
- `src/digital_outputs.cpp` generic outputs on J6/J7.
- `src/digital_inputs.cpp` interrupt driven, debounced inputs.
- `src/analog_inputs.cpp` periodic sampling with change notifications.
- `src/fog_machine.cpp` fog machine state machine.
- `include/config.h` all pins, UUIDs, protocol values, timings and thresholds.

## Build, upload and monitor

```bash
pio run
pio run -t upload
pio device monitor
```

The Nano ESP32 uses a DFU bootloader: if the upload does not start, double-tap the reset button and upload again.
