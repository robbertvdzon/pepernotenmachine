# Launcher

Firmware for the Pepernotenmachine launcher, built for a generic ESP32 Dev Module with PlatformIO and the Arduino framework. The firmware controls the pull motor, release servo, dispenser stepper, three indicator LEDs, a physical button, and a BLE interface.

## Features

- BLE control of the pull motor, release servo, launch sequence, and dispenser
- Pull motor speed and direction control with a signed value from `-512` to `512`
- Release servo positioning from `0` to `180` degrees
- Timed dispenser motion with configurable duration from 1 to 10 seconds
- Debounced button notifications
- Three sinking-current indicator LEDs with off, on, flash, and pulse states
- Support for up to three simultaneous BLE connections

## Hardware and pinout

The pin assignments are defined in [`include/config.h`](include/config.h):

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
| Pull motor end switch | 32 | Active-low input with internal pull-up |
| Release servo | 33 | Servo PWM signal |

The indicator LEDs sink current: a low output turns an LED on. Use the launcher schematic and the pin definitions in `include/config.h` when wiring the board. Connect all motor-driver and servo grounds to the ESP32 ground, and provide suitable external power for the motors and servo.

## BLE service

The device advertises as `TOETER BLE`.

Service UUID:

`4fafc201-1fb5-459e-8fcc-c5c9c331914b`

All multi-byte values use big-endian byte order.

| Characteristic | UUID suffix | Properties | Payload |
| --- | --- | --- | --- |
| Pull motor | `...26a8` | Read, write | Signed 32-bit speed, clamped to `-512..512` |
| Button | `...26aa` | Read, notify | `0x01` pressed, `0x00` released |
| Release servo | `...26ad` | Read, write | One byte, angle `0..180` degrees |
| Launch sequence | `...26ae` | Write | Any non-empty value starts the sequence |
| Dispenser duration | `...26af` | Read, write | One byte, duration in seconds, clamped to `1..10` |
| Dispenser control | `...26b0` | Read, write, notify | `0x01` start, `0x00` stop; notifications report state |

The complete UUIDs are defined in `include/config.h`.

### Examples

Using a BLE client that supports raw hexadecimal writes:

- Set pull motor speed to `512`: `00 00 02 00`
- Set pull motor speed to `-512`: `FF FF FE 00`
- Stop the pull motor: `00 00 00 00`
- Move the release servo to 90 degrees: `5A`
- Start the launch sequence: `01`
- Set dispenser duration to 4 seconds: `04`
- Start the dispenser: `01`
- Stop the dispenser: `00`

The pull motor maps positive and negative values to opposite directions and maps the magnitude to approximately 31 Hz through 16 kHz. A motor value of zero disables the motor.

## Launch sequence

The launch sequence starts only when the pull motor end-switch input is inactive. It then:

1. Moves the pull motor down at speed `-512` and pulses LED 1.
2. On the end-switch transition, moves up slowly at speed `128` and flashes LED 1 for 2 seconds.
3. Moves up quickly at speed `512` for the configured 12-second return interval.

LED 1 is off while idle and on when the sequence completes. LED 2 and LED 3 are available for other indicator states but are not currently assigned by the sequence code.

### Current implementation notes

The sequence code currently has three details to keep in mind when testing:

- `sequence_start()` rejects the `HIGH` end-switch state, despite its comment saying the switch should not be activated.
- The interrupt is registered with `RELEASE_SERVO_PIN` rather than `PULL_MOTOR_END_SWITCH_PIN`.
- The sequence timer callback checks the slow timer twice, so the fast return timer currently reaches its error branch instead of stopping the motor.

## Software structure

- `src/main.cpp` initializes each subsystem and runs the BLE callback wiring.
- `src/ble_manager.cpp` creates the BLE service, characteristics, advertising, and connection handling.
- `src/pull_motor.cpp` controls the pull motor using ESP32 LEDC PWM.
- `src/release_servo.cpp` controls the release servo through `ESP32Servo`.
- `src/dispenser.cpp` controls the dispenser through `FastAccelStepper`.
- `src/sequence.cpp` coordinates the pull motor, end switch, timers, and LED 1.
- `src/button.cpp` debounces the button in an interrupt and FreeRTOS task.
- `src/led.c` drives the three indicator LEDs with a periodic FreeRTOS timer.
- `include/config.h` contains pin assignments, UUIDs, and motion limits.

## Build, upload, and monitor

Install PlatformIO, then run these commands from the project directory:

```bash
pio run
pio run -t upload
pio device monitor
```

The serial monitor runs at 115200 baud. The project dependencies are declared in `platformio.ini`:

- `h2zero/NimBLE-Arduino`
- `madhephaestus/ESP32Servo`
- `gin66/FastAccelStepper`

## Troubleshooting

- If the device does not advertise, reset the ESP32 and check the serial output at 115200 baud.
- If button notifications are inverted, check that the button connects the input to ground when pressed.
- If a motor does not move, check driver enable polarity, shared ground, motor-driver power, and the step/direction wiring.
- If the launch sequence refuses to start, check the end-switch state and wiring.
- The dispenser duration is limited to 1 through 10 seconds by the firmware.

The PlatformIO test directory currently contains the default test documentation; no automated firmware tests are defined.
