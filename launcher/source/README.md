# Launcher

Firmware for the Pepernotenmachine launcher, built for a generic ESP32 Dev Module with PlatformIO and the Arduino framework. The code in this repository controls the pull motor, release servo, dispenser stepper mechanism, three indicator LEDs, a physical button, and a BLE interface.

## Features

- BLE control for motor speed, release-servo angle, launcher preparation and launch, dispenser timing, and preparation state updates
- Pull motor speed control with a signed value from `-512` to `512`
- Release servo angle control from `0` to `180` degrees
- Dispenser timing stored in persistent flash memory with a valid range of `1..60` seconds
- Debounced physical-button handling that prepares the launcher on release and launches on press
- Three sinking-current indicator LEDs with `OFF`, `ON`, `FLASH`, `FLASH_FAST`, `PULSE`, `RAMP_UP`, and `RAMP_DOWN` states
- Up to three simultaneous BLE connections supported by the server

## Hardware and pinout

The pin assignments are defined in `include/config.h`.

| Function | ESP32 pin | Description |
| --- | ---: | --- |
| LED 1 | 19 | Generic indicator LED |
| LED 2 | 18 | Generic indicator LED |
| LED 3 | 4 | Generic indicator LED |
| Button / extra IO 1 | 21 | Active-low button input with internal pull-up |
| Extra IO 2 | 22 | Reserved |
| Dispenser enable | 23 | DRV8825 `EN` |
| Dispenser step | 25 | DRV8825 `STEP` |
| Dispenser direction | 26 | DRV8825 `DIR` |
| Pull motor pulse | 27 | Microstep driver `PUL` |
| Pull motor direction | 16 | Microstep driver `DIR` |
| Pull motor enable | 17 | Microstep driver `ENA` |
| Pull motor end switch | 32 | Active-low end switch input with internal pull-up |
| Release servo | 33 | Servo PWM signal |

The indicator LEDs sink current: a low output turns an LED on. Use the launcher schematic and the values in `include/config.h` when wiring the board. Connect all motor-driver and servo grounds to the ESP32 ground, and provide suitable external power for the motors and servo.

### LED behavior

The LED driver in `src/led.c` keeps per-LED state and supports the following modes:

- `LED_OFF`: off
- `LED_ON`: full brightness
- `LED_FLASH`: 500 ms half-cycle blink
- `LED_FLASH_FAST`: 100 ms blink period
- `LED_PULSE`: sinusoidal pulse with a 2 s period
- `LED_RAMP_UP`: ramps to full brightness over ~500 ms
- `LED_RAMP_DOWN`: ramps to zero over ~500 ms

## BLE service

The device advertises as `Peppernut launcher`.

Service UUID:

`4fafc201-1fb5-459e-8fcc-c5c9c331914b`

All multi-byte values are encoded in big-endian order.

| Characteristic | UUID suffix | Properties | Payload |
| --- | --- | --- | --- |
| Pull motor | `...26a8` | Read, write, notify | Signed 32-bit commanded speed, big-endian and clamped to `-512..512`; notifications follow BLE writes and launcher motor commands |
| Button | `...26aa` | Read, notify | `0x01` pressed, `0x00` released |
| Release servo | `...26ad` | Read, write, notify | One byte, angle `0..180` degrees; notifications report every commanded angle |
| Prepare launcher | `...26ae` | Write | Any non-empty value starts launcher preparation |
| Launch launcher | `...26b2` | Write | Any write releases the latch; it re-locks after 4 seconds |
| Dispenser duration | `...26af` | Read, write | One byte, duration in seconds, clamped to `1..60`; saved across restarts |
| Dispenser control | `...26b0` | Read, write, notify | `0x01` start, `0x00` stop; notifications report state |
| Preparation state | `...26b1` | Read, notify | One byte: `0` idle, `1` moving down, `2` moving up slow, `3` moving up fast, `4` error |


A client that connects during preparation can read the latest published state immediately, because the state characteristic keeps the last value. If preparation is refused because the end switch is already active or preparation is already running, the previous state stays in place and no new status notification is published. Preparation moves the launcher into its ready-to-fire position; launching releases the latch and automatically re-locks it after 4 seconds. The launcher can be prepared again after launch.

The complete UUIDs are defined in `include/config.h`.

The pull-motor characteristic starts at `0` and can be read after connecting. When a client enables notifications, the current value is sent once; later notifications follow each motor speed command.

### Example writes

Using a BLE client that supports raw hexadecimal writes:

- Set pull motor speed to `512`: `00 00 02 00`
- Set pull motor speed to `-512`: `FF FF FE 00`
- Stop the pull motor: `00 00 00 00`
- Move the release servo to 90 degrees: `5A`
- Prepare the launcher: write any non-empty value to `...26ae` (example: `01`)
- Launch the load: write any value to `...26b2` (example: `01`)
- Set dispenser duration to 4 seconds: `04`
- Start the dispenser: `01`
- Stop the dispenser: `00`

The pull motor uses the magnitude of the signed speed value to set the step rate. The mapping is approximately `14 Hz` at `1` to `7200 Hz` at `512`, with sign controlling direction. A value of zero stops the motor.

The button callback is wired into the launcher controls: a button release (`0x00`) prepares the launcher, while a pressed payload (`0x01` and similar) launches the load by opening the release servo to `SERVO_OPEN_ANGLE = 40`. The latch returns to `SERVO_CLOSE_ANGLE = 66` after 4 seconds.

## Launcher operation

Preparation starts only when the pull motor end-switch input is inactive. It:

1. Moves the pull motor down at speed `-512` while LED 1 pulses.
2. Stops at the end switch, then moves up slowly at speed `128` while LED 1 flashes.
3. Moves up quickly at speed `512` for the remainder of the travel while the dispenser is started.
4. Marks preparation idle and leaves LED 1 on; the launcher is ready to fire.

The load launches when the latch is released. After the 4-second auto-lock, the launcher can be prepared again.

Preparation state values are published through the BLE characteristic as `PREPARATION_IDLE`, `PREPARATION_MOVE_DOWN`, `PREPARATION_MOVE_UP_SLOW`, `PREPARATION_MOVE_UP_FAST`, and `PREPARATION_ERROR`. Their wire values remain `0` through `4` in that order.

## Software structure

- `src/main.cpp` initializes each subsystem and wires the BLE callbacks.
- `src/ble_manager.cpp` creates the BLE service, characteristics, advertising, and connection handling.
- `src/pull_motor.cpp` controls the pull motor using FastAccelStepper.
- `src/release_servo.cpp` controls the release servo through `ESP32Servo`.
- `src/dispenser.cpp` controls the dispenser and stores its duration in persistent storage.
- `src/launcher.cpp` coordinates launcher preparation and launch using the pull motor, release servo, end switch, timers, and LED 1.
- `src/button.cpp` debounces the button in an interrupt and FreeRTOS task.
- `src/led.c` drives the three indicator LEDs with a periodic FreeRTOS timer and stops the timer when all LEDs are static to save power.
- `include/config.h` contains pin assignments, UUIDs, and motion limits.

## Build, upload, and monitor

Install PlatformIO, then run the following commands from the project directory:

```bash
pio run
pio run -t upload
pio device monitor
```

The serial monitor runs at 115200 baud. The project dependencies are declared in `platformio.ini`:

- `h2zero/NimBLE-Arduino@^1.4.0`
- `madhephaestus/ESP32Servo@^3.2.0`
- `gin66/FastAccelStepper@^1.2.5`

## Troubleshooting

- If the device does not advertise, reset the ESP32 and check the serial output at 115200 baud.
- If button notifications are inverted, check that the button connects the input to ground when pressed.
- If a motor does not move, check driver enable polarity, shared ground, motor-driver power, and the step/direction wiring.
- If launcher preparation refuses to start, check the end-switch state and wiring.
- The dispenser duration is clamped to 1 through 60 seconds by the firmware.

The PlatformIO test directory currently contains the default test documentation; no automated firmware tests are defined.
