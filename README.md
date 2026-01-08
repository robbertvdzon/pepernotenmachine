# Launcher (TOETER) — ESP32 Feather project

This project runs on an Adafruit Feather ESP32 and provides motor, horn, button and NeoPixel LED control over BLE. It is built with PlatformIO (Arduino framework) and uses the NimBLE-Arduino (lightweight BLE) and Adafruit NeoPixel libraries.

## Features

- Stepper motor control (step pulses generated using ESP32 LEDC PWM)
- Horn toggle control
- Physical push-button with notify on press/release
- Single NeoPixel LED (onboard/external) with multiple non-blocking effects and crossfade support
- BLE service exposing characteristics to control motor, horn and LED and to receive button notifications
- Modular structure: `motor`, `button`, `horn`, `led`, `ble_manager` modules

## Hardware / Wiring

Pins are defined centrally in `include/config.h`. Defaults in this repo:

- MOTOR_PUL_PIN: 14
- MOTOR_DIR_PIN: 20
- MOTOR_ENA_PIN: 22
- HORN_PIN: 32
- BUTTON_PIN: 38
- LED_PIN: 13 (NeoPixel data)

Adjust pins in `include/config.h` if your wiring differs.

Important: ensure the NeoPixel power and ground share the ESP32 ground, and use a suitable power source for the LED(s) if you increase brightness or add more pixels.

## BLE Service & Characteristics

Service UUID:
- 4fafc201-1fb5-459e-8fcc-c5c9c331914b

Characteristics (UUIDs and payloads):

1) Motor characteristic (write only)
- UUID: beb5483e-36e1-4688-b7f5-ea07361b26a8
- Payload: 4 bytes (big-endian unsigned 32-bit)
- Semantics: half-period in microseconds (uint32_t). The motor module's function `motor_set_delay_us(uint32_t halfPeriodUs)` receives this value — e.g., smaller half-period = faster stepping.
- Example: half-period = 1000 µs -> bytes `00 00 03 E8` (hex)

2) Horn characteristic (write only)
- UUID: beb5483e-36e1-4688-b7f5-ea07361b26a9
- Payload: 1 byte: `0x00` = off, any non-zero = on

3) Button characteristic (read + notify)
- UUID: beb5483e-36e1-4688-b7f5-ea07361b26aa
- Notifications: 1 byte payload
  - `0x00` = pressed
  - `0x01` = released
- Recommended: subscribe to notifications on connect to observe presses/releases

4) LED characteristic (read + write + notify)
- UUID: beb5483e-36e1-4688-b7f5-ea07361b26ab
- Read value: 4 bytes [mode, R, G, B]
- Write payloads:
  - Basic (4 bytes): [mode, R, G, B]
    - mode values:
      - 0 = OFF
      - 1 = ON (static color R,G,B)
      - 2 = SINE (sine pulsing around R,G,B)
      - 3 = FLASH (150 ms on/off of R,G,B)
      - 4 = RAINBOW (soft 5s color cycle, ignores RGB)
      - 5 = CROSSFADE (requires 6 bytes)
  - Crossfade (6 bytes): [5, R, G, B, duration_hi, duration_lo]
    - duration is 16-bit big-endian milliseconds for the crossfade duration (e.g., `00 64` = 100 ms)
    - On completion, the device sends a 1-byte notification (value `0x01`) on the LED characteristic to indicate completion.
- Notification payload for crossfade completion: single byte `0x01`.

Examples (using a BLE client that lets you write raw hex bytes):
- Set LED static to purple (mode=1, R=128, G=0, B=128): `01 80 00 80` (hex)
- Start crossfade to white over 2 seconds: `05 FF FF FF 07 D0` (hex). `07 D0` = 2000 ms in big-endian.

## Software structure

Key files and modules:

- `src/main.cpp` — orchestrates init and main loop.
- `include/config.h` — central pin and UUID definitions.
- `include/motor.h`, `src/motor.cpp` — motor control (LEDC pwm).
- `include/button.h`, `src/button.cpp` — non-blocking debounce and notify.
- `include/horn.h`, `src/horn.cpp` — horn control with auto-off.
- `include/led.h`, `src/led.cpp` — NeoPixel driver and non-blocking effects.
- `include/ble_manager.h`, `src/ble_manager.cpp` — BLE service and characteristics wiring.

All modules use small C-style callbacks to forward BLE writes to the appropriate subsystem.

### Notes about BLE implementation

- The project was migrated from the Bluedroid-based ESP32 BLE Arduino stack to NimBLE (NimBLE-Arduino) to reduce flash usage. NimBLE is functionally equivalent for GATT server use (read/write/notify) and is significantly smaller.
- Size impact seen in this workspace (approximate, measured after migration):
  - Before (Bluedroid build): ~1,168 KB total firmware
  - After (NimBLE build): ~632 KB total firmware
- `platformio.ini` now includes `h2zero/NimBLE-Arduino` as a dependency.
- The code explicitly adds the Client Characteristic Configuration (CCC) descriptor (UUID 0x2902) for the Button and LED characteristics so clients that look for that descriptor object will find it. NimBLE normally handles notify/indicate behavior, but the explicit descriptor improves compatibility with some clients.

## Build & Upload (PlatformIO)

From the project root, build:

```bash
pio run
```

To upload to the connected board (PlatformIO will attempt to auto-detect the serial port):

```bash
pio run -t upload
```

Open the serial monitor (default 115200) to see runtime logs:

```bash
pio device monitor
```

If you prefer the VS Code PlatformIO UI, use the Build/Upload/Monitor icons in the bottom bar or in the PlatformIO toolbar.

## Runtime notes & testing steps

1. Power the board and open the serial monitor. The program prints BLE connect/disconnect events and button debug messages.
2. Using a BLE client app (nRF Connect is recommended), scan and connect to the device named `TOETER BLE`.
3. Subscribe to the Button and LED characteristics to receive notifications.
4. Use the Motor characteristic to set stepping speed (half-period µs). Start with a conservative half-period (e.g., 2000 µs) and test.
5. Use the Horn characteristic to sound the horn (write `01`), and `00` to silence.
6. Use the LED characteristic to set colors and crossfades as described in the examples above.

## Troubleshooting

- No BLE advertising: make sure the ESP32 has power and is not in a previous crash state; reset the board.
- Button notifications not received: check wiring of `BUTTON_PIN` and ensure pull-up/down as required by your hardware; the repo expects an active-low or -high depending on wiring — check the button code for details.
- NeoPixel doesn't light: verify wiring (data to `LED_PIN`, GND common) and that the pixel is powered. Some pixels require 5V — if so, use a level shifter or a 5V supply with proper ground.
- Motor not moving: check enable pin wiring (`MOTOR_ENA_PIN`), ensure motor driver logic levels match the driver (step/dir), and use safe half-period values to avoid missed steps.

## Modifying configuration

All pin assignments and BLE UUIDs are in `include/config.h`. Edit there and re-build if you need to change pins or UUIDs.

## Contributing / Extending

- Add more LED effects in `src/led.cpp` and expose new modes.
- Add readbacks or richer notification payloads for the LED characteristic if your client needs them.
- Add unit tests or a simulator harness for the LED timing logic (non-blocking).

## License & Contact

This repository contains code and is maintained by the project owner. Modify and reuse as you like; add a LICENSE file to set explicit terms.

If you'd like, I can also:
- Add example client code (Python or mobile) that performs the BLE writes and subscribes to notifications.
- Add a small `docs/` folder with wiring diagrams.

---

README generated using the repository state on 2026-01-07. Adjust any examples if you change endianness or payload formats in code.
