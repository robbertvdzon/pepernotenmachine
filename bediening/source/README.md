# Pepernoten control unit

Tractor-cab control unit (Arduino Nano ESP32, ST7735S TFT in landscape 160x128, rotary encoder,
MCP23017 @ 0x20, 12 LEDs, 8 switches). It connects over BLE (NimBLE) to the **Peppernut launcher**
and the **IO Switchboard**, keeps both connected, and turns the switches into commands.

**Status:** hardware layer, LEDs, BLE discovery/selection/saved devices, always-on connections,
setup UI, and the first part of the control logic (lights, siren, fog, launcher prepare/launch).
Not done yet: panic mode, automatic mode, the final main screen. See [TODO](#todo) and
[open questions](#open-questions).

## Build

```bash
pio run                  # build (env: arduino_nano_esp32)
pio run -t upload        # flash
pio device monitor       # 115200 baud
```

Upload does not start? Double-tap reset (DFU bootloader) and retry. "opening dependency file ...
No such file or directory" means something else touched `.pio` during the build: close other
`pio`/VS Code PlatformIO processes, `rm -rf .pio`, rebuild.

Tested toolchain: espressif32 7.0.1, LVGL 9.6.0 (9.1 also supported), LovyanGFX 1.2.35,
NimBLE-Arduino 1.4.3. Log level: `-D APP_LOG_LEVEL=4` (debug) in `build_flags`.

Host tests (pure logic, no hardware):

```bash
g++ -std=c++11 -Wall -Wextra -Iinclude test/host/test_logic.cpp src/ble_devices.cpp -o /tmp/test_logic && /tmp/test_logic
g++ -std=c++11 -Wall -Wextra -Iinclude test/host/test_control.cpp src/control_logic.cpp src/ble_devices.cpp -o /tmp/test_control && /tmp/test_control
```

## Using it

| Control | Behavior |
| --- | --- |
| Boot | Straight into the setup screen if the launcher or the switchboard is not saved yet (Back = home) |
| Encoder, home screen | **Short press** = Actions menu (Prepare launcher), **hold 0.7 s** = Setup |
| Encoder, menus | Turn = move focus, push = select. Flip `ENCODER_DIRECTION_INVERT` in `config.h` if reversed |
| SW1 Back | One screen back. On the scan screen it also stops the scan |
| SW3 Fog | ON = keep fogging whenever the machine is ready; OFF = stop |
| SW4 Flashing light | Relay `RELAY_FLASHING_LIGHT` |
| SW6 Decorative lighting | Relay `RELAY_DECORATIVE_LIGHT` |
| SW7 Siren | Momentary: relay `RELAY_SIREN` follows the button |
| SW8 Launch | Fires immediately **if** the launcher is ready (see interlocks), otherwise says why not |
| SW2 Panic, SW5 Automatic | Not defined yet: logged and ignored |

Setup: Setup -> Launcher or Switchboard -> **Scan & select** (devices of that kind: address, signal,
`UUID` = identified by service UUID, `name` = name only) -> pick one: it is saved and connected at once.
**Forget** erases it (with confirmation). Connections keep running while the setup is open; only during
the 10 s scan no new connection attempt is started.

### LEDs

| LED | Meaning |
| --- | --- |
| L1 | Automatic mode (not defined yet, off) |
| L2 | Launcher: on = ready to launch, pulse = preparing, fast flash = error, also 1.5 s fast flash after a rejected launch/prepare |
| L3 / L5 | Launcher / switchboard connection: off = nothing saved, flash = searching or connecting, on = connected, fast flash = error |
| L4 / L7 | Decorative lighting / flashing light: on = relay confirmed on, flash = wanted but not confirmed, fast flash = not confirmed after 3 s, off = off |
| L6 | Panic mode (not defined yet, off) |
| L8 | Fog: off = machine not connected, flash = heating, on = ready, pulse = fogging, fast flash = fog wanted but impossible |
| L9..L12 | Reserved, off |

Each LED can be OFF, ON, FLASH (1 Hz), FLASH_FAST (5 Hz), PULSE, RAMP_UP, RAMP_DOWN (`leds_set_state`).

## Control logic and safety rules

All rules live in `control_logic.cpp` (pure, unit-tested); `controller.cpp` connects it to the hardware.
Relay numbers and all timings are in `include/control_config.h` (**nothing is wired yet: the relay numbers
are placeholders, change them there**).

- **Launch is a one-shot.** Sent at most once per button press, never retried, never replayed on reconnect.
  It requires: launcher connected, preparation state **current** (read in this connection), state idle,
  a preparation that this unit **watched finish** during the current connection, and no launch in the last
  6 s. Anything else sends nothing and shows why ("not prepared", "launcher offline", "still preparing",
  "launcher error", "just launched", ...). After a launch, a reconnect or a restart the launcher counts as
  *not prepared* until it is prepared again. A write that fails is reported ("LAUNCH FAILED"), not repeated.
- **Prepare** (Actions menu): needs a connected launcher with current state, not already preparing, not within
  6 s of a launch. If the launcher does not start within 3 s -> "prepare refused"; if it runs longer than
  90 s -> "prepare timeout" (error state). It can be retried from the error state.
- **Relays and fog are state settings.** The switch position is the truth: the wanted state is compared with
  what the switchboard reports and re-sent (every 0.5 s) until it matches. This also re-applies them after a
  reconnect (the switchboard turns everything off when its last client disconnects).
- **Unknown switch state** (expander failure): nothing is changed, and fog is stopped.
- **Stale state is never shown as current:** on disconnect all values of that device become invalid.
- Panic and automatic mode do nothing until they are defined.

## Hardware and software layout

| File | Responsibility |
| --- | --- |
| `include/config.h` | ALL pins, active levels, I2C address, timings, UI/link tunables |
| `include/control_config.h` | Relay numbers and the control-logic timings (edit here) |
| `src/main.cpp`, `src/app.cpp` | `setup()`/`loop()`; init order and one non-blocking pass of the periodic work |
| `src/control_logic.cpp` | Pure control logic: switches + device states -> commands, LEDs, messages |
| `src/controller.cpp` | Glue: gathers inputs, runs the logic, executes commands via `ble_link_write`, drives L2/L4/L7/L8 |
| `src/ble_link.cpp` | Connection manager: one BLE task keeps both devices connected; queued writes |
| `src/ble_scanner.cpp` | NimBLE scan; the callback only copies into a locked queue |
| `src/ble_devices.cpp` | Service UUIDs and the characteristics used per device |
| `src/device_store.cpp` | The ONE saved launcher and switchboard in NVS (validated records) |
| `src/ui.cpp`, `include/ui_theme.h`, `fonts.h`, `ui_compat.h` | Screens, palette, fonts, LVGL 9.1..9.6 compatibility |
| `src/display.cpp` | LovyanGFX ST7735S + LVGL (full-frame direct render) |
| `src/fonts/font_pixel_16.c`, `tools/make_fonts.sh` | Generated 8x16 1-bit pixel font (Unscii, public domain) |
| `src/leds.cpp` | 12 LEDs with animation states (own task, software PWM for the expander LEDs) |
| `src/switches.cpp`, `buttons.cpp`, `rotary_encoder.cpp`, `mcp23017.cpp` | Inputs and the thread-safe I2C expander driver |
| `include/led_pattern.h`, `link_state.h`, `ble_identity.h`, `ble_address.h`, `device_record.h`, `debounce.h`, `encoder_decoder.h`, `event_queue.h` | Pure logic, host-tested |

### BLE connections

- Per device: `NotConfigured`, `Searching`, `Connecting`, `Connected`, `Error` (answered but unusable: wrong
  service or a required characteristic missing; retried every 3 s).
- Reconnect never gives up (2 s attempt, 0.5 s pause); a lost link is noticed within ~2 s. After every
  (re)connect services are rediscovered, notifications re-subscribed and values re-read.
- Identification: advertised service UUID = strong, name only = weak, name and UUID disagreeing = conflict (ignored).
  Both devices advertise **public** addresses (observed), so saved addresses are stable.
- Writes go through a per-device queue in the BLE task, executed once, result available; never retried there.

### Display

- LVGL renders into a 40 KB full-frame buffer (direct mode); only changed rows are sent to the panel.
- **LVGL's RGB565 is native-endian = `lgfx::rgb565_t`.** Passing a plain `uint16_t*` to LovyanGFX swaps the
  bytes (wrong colors, green grays, odd anti-aliasing at rounded corners). `flush_cb` casts correctly.
- The panel is wired **BGR**: `DISPLAY_RGB_ORDER` defaults to `true`. `DISPLAY_SPI_HZ`, `DISPLAY_INVERT` and
  `DISPLAY_ROTATION` are in `config.h`.
- `ui.cpp` also ran off-target (scripted encoder session, 24 frames) and rendered pixel-identical on LVGL 9.1
  and 9.6.0; that harness is not part of this repository.

## Decisions and assumptions

- Switches: closed = connected to GND = active; expander pull-ups on. LEDs: active LOW.
- Expander failure: switch state is UNKNOWN (never "unchanged"); the driver retries every 250 ms.
- Flashing light uses relay state ON (the switchboard also offers a blink mode, see questions).
- Fonts: 1-bit pixel fonts only (LVGL's built-in `lv_font_unscii_16` is the 8x8 font doubled, 16 px per
  character, so the project uses its own 8x16 font; `lv_font_unscii_8` for small captions).
- SW9..SW12 (A2, A3, A6, A7) are on the schematic but have no function and are not configured.

## Manual test checklist

- [ ] Boots into Setup with nothing saved; into the home screen with both saved
- [ ] **Colors**: orange "Launcher", teal "Switchboard", amber/green status, indigo focus highlight, neutral gray cards, no green pixels at rounded corners, no blank lines in cards or menu rows
- [ ] Setup flow: scan lists only the chosen kind, first device gets the focus, pick saves + connects, "Scan again" appears only after the scan, Forget confirms
- [ ] Hold encoder on home = Setup (the release does not select anything); short press = Actions
- [ ] Both devices on: log shows `Connected` and the initial values; L3/L5 solid; cards green "connected"
- [ ] Switch a device off: `link LOST` within ~2 s, "searching", reconnects by itself, values re-read
- [ ] SW4 ON: log `-> switchboard RELAYS [1 1]`, then `RELAYS = 01 00 ...`; L7 flashes, then solid; SW4 OFF reverses it
- [ ] SW6 ON/OFF: relay 2 and L4, the same way. SW7 held: relay 3 on, released: off
- [ ] Reconnect with SW4 ON: the relay is switched on again by itself
- [ ] SW3 ON with the fog machine ready: fog starts, L8 pulses; after the burst it starts again; SW3 OFF stops it
- [ ] SW8 before preparing: "not prepared", nothing sent, L2 fast flash
- [ ] Actions -> Prepare launcher: L2 pulses, status "preparing: ...", then L2 solid and "READY"
- [ ] SW8 when READY: launches once, "LAUNCHED"; pressing again: "just launched"; later: "not prepared"
- [ ] Power-cycle / reconnect the launcher while ready: it is NOT ready any more (prepare again)
- [ ] SW2 / SW5 switched: only a warning in the log
- [ ] Expander disconnected: clear error, retries, no freeze; recovery restores LEDs and switches

## TODO

Remaining work from the original design prompt, in the order we agreed (Phase 4 before Phase 3):

### Phase 4 - orchestration (remaining)
- [ ] **Panic mode (SW2)**: define exactly what happens ON and OFF (fog, siren, lights, automatic mode, launcher) and its priority; then implement + tests; light L6.
- [ ] **Automatic mode (SW5)**: define the full sequence, triggers, timings, conditions, abort behavior; implement + tests; light L1.
- [ ] **Siren**: confirm "relay follows the button" or define a pulse/hold duration; confirm the relay.
- [ ] **Flashing light**: decide plain ON or the switchboard's blink mode (relay state 2 + interval).
- [ ] **Back button (SW1)**: decide whether it has an operational role besides navigation (e.g. cancel/stop an operation).
- [ ] **Dispenser**: confirm that the launcher starting the dispenser during preparation is desired; decide on a dispenser start/stop/duration control.
- [ ] **Settings on the TFT**: decide which parameters are adjustable (dispenser duration, fog burst duration, ...) and which are read-only; persist them (NVS).
- [ ] **Freshness policy**: today a value is "current" while the link is up (notifications + read on connect). Decide whether a periodic re-read / staleness timeout is wanted for the launch interlock.
- [ ] **Wiring**: set the real relay numbers in `control_config.h` once the equipment is wired; decide about the MOSFETs, digital outputs and inputs, analog inputs of the switchboard (not used yet).
- [ ] Document the safety assumptions that depend on hardware or operator procedure (final safety section).

### Phase 3 - main operating screen
- [ ] Decide which status items must always be visible (launcher, switchboard, preparation/readiness, fog state, automatic, panic, flashing light, decorative lighting, siren, warnings/errors).
- [ ] Layout for 160x128 with icons (e.g. Material icons converted to a 1-bit/pixel icon font via `lv_font_conv`) and the app palette; critical states unambiguous.
- [ ] A warning/error model (active problems list) instead of the 3 s message line.
- [ ] Decide whether the encoder menu may control functions that also have physical switches.
- [ ] Show the fog state, relay confirmation and launcher preparation progress on the main screen.

### Quality, testing, polish
- [ ] Verify on the hardware: colors after the byte-order fix, no blank lines/green corner pixels; check free heap with the 40 KB frame buffer and NimBLE.
- [ ] More host tests: UI navigation state, BLE payload encode/decode helpers (when more payloads are used), persistence defaults.
- [ ] Move the off-target UI harness into the repository (`test/`), so UI layouts stay checkable.
- [ ] Verify the switchboard characteristic UUIDs against its `include/config.h` (currently taken from its README).
- [ ] Panel gamma/color calibration if the dark grays still look tinted.
- [ ] Decide what to do with SW9..SW12 and L9..L12.
- [ ] Optional: a proper `lv_conf.h` instead of build flags; OTA/diagnostics; backlight control (not on the schematic).
- [ ] Final manual hardware checklist for panic/automatic mode once defined.

## Open questions

Answered:
- Relays for lights/siren: nothing wired yet, any numbers, easy to change (`control_config.h`).
- Fog switch: keep fogging whenever possible. Launch: immediately, no confirmation. Preparation: a menu option for now.
- BLE addresses: public and stable (observed). Build environment: see above.

Assumed, please confirm:
1. **"Ready to launch"** = this unit watched a preparation finish in the current connection and no launch since. After a reconnect/restart an already prepared launcher must be prepared again. Acceptable?
2. **Siren** = relay follows the button (no pulse duration).
3. **Flashing light** = relay plain ON.
4. **Switch is the truth for relays:** if someone else (e.g. the Flutter app) changes a relay that a switch controls, this unit switches it back within 0.5 s. Is this wanted, or should the unit only act when its own switch changes?

Open:
5. Panic mode (SW2): what exactly happens when it is switched ON and OFF?
6. Automatic mode (SW5): the full sequence, triggers, timings, conditions, abort.
7. Dispenser: is starting it during preparation (launcher firmware behavior) the desired final behavior?
8. Back button (SW1): navigation only, or should it also cancel/stop an operation?
9. Which parameters should be adjustable on the TFT (dispenser duration, fog burst duration, ...)?
10. Which equipment will be on which relay / output once wired (flashing light, decorative lighting, siren, other)?
11. What should SW9..SW12 and L9..L12 be used for?
12. Is the launch lockout (6 s after a launch) and the prepare timeout (90 s) right for the real launcher?
