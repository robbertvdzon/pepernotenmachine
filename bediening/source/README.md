# Pepernoten control unit

Tractor-cab control unit (Arduino Nano ESP32, ST7735S TFT in landscape 160x128, rotary encoder,
MCP23017 @ 0x20, 12 LEDs, 8 switches). It connects over BLE (NimBLE) to the **Peppernut launcher**
and the **IO Switchboard**, keeps both connected, and turns the switches into commands.

**Audience and design rule:** the tractor driver cannot look at the display or turn the knob during the
ride, and is a different person each year (the system is used once a year). So the LEDs and switches
do the work; the display is for set-up and for confirming things at the start. Keep everything as simple
as possible.

## Status (v0.20)

### Verified on the real hardware (confirmed by the owner)
Display, encoder, switches, LEDs, colors, BLE scan / select / save / reconnect, setup flow, the old relays / lights, launcher check overlay (old three-answer
version), automatic preparation, launch + automatic re-arm, the animated home screen, the scrolling menus, the whole **Maintenance menu (release bucket,
dispense, motor control)**, the display glitch fix (top line clean) and **LED L9** (pulses while preparing, off when ready, fast flash on error).

### Written, unit-tested and checked off-target, but NOT yet run on the hardware
- **Start-up splash screen with the owner's artwork (v0.19, see [Splash screen](#splash-screen))**: the picture (160x69, Sinterklaas + cannon + flame + flying nuts) at the top, below it on the picture's own background color the title "nut-o-matic"
  (red letters, dark outline, overlapping the bottom of the picture a little), the subtitle, the link row "1 [BT] ))) [BT] 2" (badges pulse while waiting, turn green when connected, arcs travel, solid green when both are connected) and the version. No "Connecting..." text: the animation says it. "push = continue" (after 20 s) appears at the very top over the picture.
- **Automatic mode (SW5) and panic mode (SW2)** with the owner's device table, priorities and patterns (v0.17, see [Devices, automatic mode and panic mode](#devices-automatic-mode-and-panic-mode)).
  Every wired relay is now driven by the unit; the relay numbers are the owner's latest wiring; SW7 is the airhorn. The old random bells/horns panic is gone.
- **"Anything in bucket?" check** (Bucket empty = prepare `0x01` with the dispenser, Bucket full = prepare `0x02` without it, Not sure? = help) and the 4 s re-arm delay (v0.13).
- **Hold to reverse** (`0x03`) on the Dispense screen, the **Bottom switch** screen (`...26aa`) (v0.14).
- **Dispenser time** screen (read / write `...26af` with read-back) and the run time on the Dispense screen (v0.15); this also added `ble_link_read` (see below).
- The toast text centering (v0.12.2).
- "Ask the driver again" paths (error, refused prepare, failed write) and fog control (no fog machine available yet).

### Dependencies on the launcher firmware (owner's side)
Prepare accepts `0x01` (with dispenser) and `0x02` (without); dispenser control accepts `0x03` (reverse until `0x00`) besides `0x01` / `0x02` / `0x00`; the dispenser time `...26af`
is read/write (1..60 s, kept by the launcher); the bottom switch `...26aa` and the pull motor `...26a8` notify. **Still to be done there: stop the dispenser and the pull motor by itself
when the BLE link drops** (the unit also stops them, but cannot when it is the one that died).

### Loose ends and caveats (read this first when taking over)
- **Not compile-checked here: `src/ble_link.cpp`** (needs NimBLE, which the sandbox does not have). v0.15 added `ble_link_read` and a read path in `drain_commands`; it was re-read
  carefully but never compiled. Everything else that touches LVGL was syntax-checked against LVGL 9.1 and 9.6 with stubs (`test/offtarget/stubs/Arduino.h`); `display.cpp` needs LovyanGFX and is not.
- **The illumination "grid failure" disruption in automatic mode is my guess** (off 0.4..1.0 s, on 0.2..0.5 s, off 0.3..0.8 s, then on for 30..90 s): tune it in `control_patterns.h`.
- **Splash screen: artwork is in (v0.19).** The owner's LVGL 8 converter output (`lv_img_dsc_t`, indexed) does not work with LVGL 9, so the bitmap is generated from the original image by `tools/make_splash_art.py` (source image kept in `tools/preview/splash_art_source.jpeg`).
- **Toast (minor, owner does not mind):** while it slides in (0.2 s) its top edge can be drawn wrong, and because the toast area is never re-sent in the steady state the damage stays until the toast changes.
  Options: no slide, or one forced clean redraw after the slide, or a fade (TODO).
- **Display efficiency (not changed):** the launcher band is flushed ~28x a second instead of the intended 12 (the progress value changes every ~25 ms); quantizing it to the bar's 78 pixels would remove ~60 % of the traffic (TODO).
- A pending question list is under [Open questions](#open-questions), everything else that is open is in the [TODO](#todo) (Settings menu, Dutch translation, SW9..12 / L10..12, menu structure review, hardware checklist, optional ideas).

### Display notes (what was learned, so it is not repeated)
- LVGL with this project's defaults does **not** pad canvas rows (`LV_DRAW_BUF_STRIDE_ALIGN` = 1); the home canvases are 160 px wide anyway and a compile-time check guards the stride.
- The top-line glitch was caused by the home screen being **scrollable**: the toast sliding in from below the screen edge made LVGL draw a scrollbar. Screens must never scroll, and nothing may hang partly off-screen
  (the toast now slides inside a fixed box). Partial flushes (the default) are clean; **sending the full frame on every flush is worse** (the glitch returns and flickers); a lower SPI speed changes nothing.
- `test/offtarget/panel_check.cpp` applies every LVGL flush to an emulated panel (like `display.cpp`) and checks areas, panel == frame buffer, incremental frame == full redraw, and that the screen never scrolls.

### Change history (short)
- **v0.20** splash: "Connecting..." line removed (title / subtitle / link row moved down), "push = continue" at the top over the picture on a semi-transparent patch, arcs centered between the badges (host test measures the air on both sides), **Dutch build**: `include/ui_strings_nl.h` + PlatformIO environment `arduino_nano_esp32_nl` (`-D UI_LANG_NL`).
- **v0.19** splash with the artwork at the top (`splash_art.*` generated by `tools/make_splash_art.py`, RGB565 in flash, 22 KB), splash background = the picture's bottom-row color, title overlapping the picture, tighter link row (20 px), "Connecting..." left aligned, version hidden while "push = continue" shows.
- **v0.18** start-up splash screen (text-only): `splash_model.*`, `splash_sprites.*`, `ui_splash.cpp`, `splash_config.h`, `version.h`, two splash fonts; `ui.cpp` starts on it; the knob push that ends the splash no longer also opens the menu.
- **v0.17** automatic and panic mode, device table and pattern engine (`control_patterns.h`, `pattern_runner.*`), SW7 = airhorn, new relay numbers, L1 = auto, L6 = panic, auto tile.
- **v0.16** L9 = launcher preparing. **v0.15** dispenser time, run time on the Dispense screen, `ble_link_read`. **v0.14** hold to reverse, bottom switch screen.
- **v0.13** "Anything in bucket?" check and the new prepare values. **v0.12** motor control, stray-click guard, toast fixes. **v0.11** Maintenance menu, scrolling menus.
- **v0.10** animated home screen. Earlier: setup flow, BLE link, check overlay, launcher logic.

## Build

```bash
pio run                  # build (env: arduino_nano_esp32)
pio run -t upload        # flash
pio device monitor       # 115200 baud
```

Upload does not start? Double-tap reset (DFU bootloader) and retry. "opening dependency file ...
No such file or directory" means something else touched `.pio` during the build: close other
`pio`/VS Code PlatformIO processes, `rm -rf .pio`, rebuild.

Toolchain in use: espressif32 7.0.1, LVGL 9.6.0 (9.1 also supported), LovyanGFX 1.2.35,
NimBLE-Arduino 1.4.3. Log level: `-D APP_LOG_LEVEL=4` (debug) in `build_flags`. Keep your own flags in
`platformio.ini` when you copy files over (e.g. `DISPLAY_RGB_ORDER`).

Host tests (pure logic, no hardware needed):

```bash
g++ -std=c++11 -Wall -Wextra -Iinclude test/host/test_logic.cpp src/ble_devices.cpp -o /tmp/test_logic && /tmp/test_logic
g++ -std=c++11 -Wall -Wextra -Iinclude test/host/test_control.cpp src/control_logic.cpp src/pattern_runner.cpp src/ble_devices.cpp -o /tmp/test_control && /tmp/test_control
g++ -std=c++11 -Wall -Wextra -Iinclude test/host/test_patterns.cpp src/pattern_runner.cpp -o /tmp/test_patterns && /tmp/test_patterns
g++ -std=c++11 -Wall -Wextra -Iinclude test/host/test_splash.cpp src/splash_model.cpp src/splash_sprites.cpp src/gfx.cpp -o /tmp/test_splash && /tmp/test_splash
g++ -std=c++11 -Wall -Wextra -Iinclude test/host/test_home.cpp src/home_model.cpp src/home_sprites.cpp src/gfx.cpp src/control_logic.cpp src/pattern_runner.cpp src/ble_devices.cpp -o /tmp/test_home && /tmp/test_home
```

Off-target checks on a host build of LVGL 9.1 and 9.6, built with the same 64-byte canvas stride as the unit
(needs git, gcc, python3; the first run compiles LVGL and takes several minutes): `test/offtarget/run.sh /tmp/out`.
`test/offtarget/main.cpp` plays a scripted launcher session on the home screen; `test/offtarget/panel_check.cpp` is the flush consistency check described under Status. `test/offtarget/ui_main.cpp` runs the REAL
`ui.cpp` (menus, maintenance screens, scrolling, hold-to-dispense) against a stubbed controller and asserts what the UI asks
of it. Both write PPM frames for a visual check; `run.sh` also plays the whole session once more with `-DUI_LANG_NL`. `ui_main` also plays the splash twice (both devices connect; one never connects and the driver pushes to continue) and checks that the splash never
touches the screen edges, never flushes the full frame and stays far below the home band's traffic; `run.sh` compiles LVGL in parallel.
`test/host/render_home.cpp` renders all sprites without LVGL. `test/offtarget/stubs/Arduino.h` is a tiny Arduino stand-in
for `g++ -fsyntax-only` checks of `ui.cpp` / `controller.cpp`.

Code must stay C++11-compatible (older Arduino cores) and work with LVGL 9.1 .. 9.6.

## Using it

| Control | Behavior |
| --- | --- |
| Boot | Straight into the setup screen if the launcher or the switchboard is not saved yet (Back = home); otherwise the start-up splash until both devices are connected |
| Launcher check | Appears by itself once the launcher is connected and its state must be confirmed (see below) |
| Encoder on the home screen | Turning does nothing. **Push** = menu (Check launcher, Maintenance, Setup). There is no hint on the screen on purpose |
| Encoder in menus | Turn = move focus (long lists scroll), push = select. Flip `ENCODER_DIRECTION_INVERT` in `config.h` if reversed |
| SW1 Back | One screen back. On the scan screen it also stops the scan. Does nothing on the home screen and on the check overlay (it must be answered) |
| SW2 Panic | Panic mode: flashlights, inside lights and fog on, illumination blinks, siren pulses, see [Devices](#devices-automatic-mode-and-panic-mode). Unrelated to the launcher |
| SW3 Fog | ON = keep fogging whenever the machine is ready (manual, beats automatic and panic mode); OFF = follow the modes |
| SW4 Flashlights | The three outside flashlights (red, orange right front, orange left front) steady on (beats automatic and panic mode) |
| SW5 Automatic mode | Automatic mode: flashlights, illumination and fog run their patterns, see [Devices](#devices-automatic-mode-and-panic-mode). NOT the launcher |
| SW6 Illumination | The exterior illumination steady on (beats automatic and panic mode) |
| SW7 Airhorn | Momentary: the airhorn relay follows the button (never part of automatic or panic mode) |
| SW8 Launch | Fires immediately **if** the launcher is ready, otherwise shows why not (toast on the home screen, L2 flashes fast). Blocked while a maintenance screen is open |

Setup: Menu -> Setup -> Launcher or Switchboard -> **Scan & select** (devices of that kind: address, signal,
`UUID` = recognised by service UUID, `name` = by name only) -> pick one: it is saved and connected at once.
**Forget** erases it (with confirmation). Connections keep running while the setup is open; only during the
10 s scan no new connection attempt is started.

### Maintenance

Menu -> **Maintenance** (title and rows in amber/red: this is not for the ride). While any of its screens is open
(`controller_set_maintenance`):
- **All automation pauses**: no automatic prepare / re-arm, SW8 is blocked (message "in maintenance", L2 flashes), the
  launcher check overlay is held back. The info line under the title shows the latest result or problem.
- **Release bucket** -> confirmation (**Cancel is first**) -> writes `1` to the launch characteristic (`...26b2`) once,
  never retried. It is a real launch of whatever is in the bucket. Refused if the launcher is offline, its state is unknown or it is
  preparing ("launcher busy"). Result: "bucket released" / "release failed".
- **Dispense** screen: rows "Hold to dispense" (write `0x02` = run until `0x00`), "Hold to reverse" (write `0x03` = run backwards
  until `0x00`, to un-jam the dispenser) and Back (pushing it never dispenses). The row is fixed when the knob goes down (turning
  while holding changes nothing). Status (starting / DISPENSING / REVERSING / stopping) replaces the hint line. Safety: the STOP is
  sent at once when the knob is released, the screen is left (Back button, menu), maintenance ends, the launcher starts moving or
  the link was interrupted; it is always sent at least twice and repeated every 0.5 s until the dispenser reports `0x00`;
  "dispenser stuck?" after 3 s without confirmation. After an interruption only a **new press** runs it again. Not accepted within
  2 s -> "dispense failed". Refused while the launcher is preparing or the dispenser is already running. **A change of direction
  needs the stop confirmed first** ("still stopping"), pressing the same direction again while it stops just runs again.
- **Motor control** screen: rows "Hold to go up", "Hold to go down", Back (the speeds +512 / -512 are only on the wire). Holding the knob on a row writes the commanded
  speed to the pull motor characteristic (`...26a8`, signed 32-bit big-endian: `+512` = `00 00 02 00`, `-512` = `FF FF FE 00`);
  stop = speed `0` (`00 00 00 00`). The row is fixed when the knob goes down (turning while holding changes nothing). The stop rules
  are the dispenser's (immediately on release / Back / end of maintenance / launcher moving / link interruption; at least twice,
  repeated until the motor reports speed 0; "motor stuck?" after 3 s; after an interruption only a new press runs it again).
  Extra for the motor: a new press is refused ("motor stopping") until the previous stop is confirmed, so it can never be
  reversed on the fly; and the motor and the dispenser are never run at the same time. Refused while the launcher is preparing
  or the motor already reports a non-zero speed. The status ("MOTOR RUNNING" / starting / stopping) replaces the hint line.
  There are no end stops in this unit: the operator watches the machine (an elastic band that is over-tensioned makes the motor slip).
- **Dispenser time** screen: the seconds of the dispense cycle that the launcher uses in a preparation (`...26af`, one byte, 1..60, kept by
  the launcher). Opening the screen refreshes the reading. Turning the knob changes the value (one click = one second, nothing is sent;
  the number turns amber with "(unsaved)"), pushing saves: the byte is written, then **read back**, and only a read-back equal to the written
  value shows "time saved" (else "save failed"; also after 3 s without a read-back). Refused ("launcher busy") while the launcher is preparing
  and when it is offline; no reading = "--" and nothing can be saved. The Back button leaves without saving. It changes nothing physical, so no
  check question follows. The Dispense screen shows the forward run time, from the first "running" report to the stop.
- **Bottom switch** screen (read only, nothing is sent): shows the live state of the switch the launcher uses to see that the
  bucket reached the bottom (`...26aa`, read + notify: `0x01` PRESSED = "bucket is down", `0x00` RELEASED = "bucket not down";
  "NO READING" when the link is down or the characteristic is missing). Handy for checking the switch by hand.
- **After a release, a dispense or a motor run** the machine is in a state the unit cannot know: when maintenance is left the normal
  launcher check question is asked before anything moves (not when you only looked at the menu).

### The launcher (fully automatic, no switch involved)

How the machine works: a bucket hangs on two tension springs under a frame several meters high. A rope runs
from the bucket to a coil driven by a stepper (pull motor). *Prepare* = pull the bucket down until a pin hits
a switch, close the lock (release servo), reverse and unwind the rope (the switch loses contact again), while
the dispenser stepper fills the bucket (unless the driver said the bucket is already full); the end of the unwinding is time based. *Launch* opens the lock.
The launcher cannot know at power-up whether it is filled / locked / unwound, and neither can this unit. **It does
not matter where the bucket is**: a preparation always starts with "going down", so with the bucket already in the lock
the pin hits the switch almost at once and the preparation continues with the unwinding. The only thing nobody can
know is **what is in the bucket**. So the **driver is asked once after power-up** (check overlay, title
`! CHECK LAUNCHER !`, question "Anything in bucket?"):

| Answer | Meaning | What the unit does |
| --- | --- | --- |
| Bucket empty | nothing in it | prepares it by itself, **including** the dispenser cycle (writes `0x01` to prepare) |
| Bucket full | already loaded | prepares it by itself, **without** the dispenser cycle (writes `0x02` to prepare) |
| Not sure? | | nothing moves; a help page ("check by hand": look inside, anything in it = full) explains, then asks again |

Either way the launcher is **never ready without a preparation** (about 30 s after the answer: a short "down" phase,
then the ~25 s unwinding); there is no "armed, nothing moves" shortcut any more. A wrong answer costs: "empty" on a full
bucket overfills it, "full" on an empty bucket gives one empty shot (the automatic preparation after a launch always
fills, `0x01`). The prepare characteristic (`...26ae`) therefore means: `0x01` start + dispenser cycle, `0x02` start
without the dispenser cycle (the launcher has to accept `0x02`; no backwards compatibility with older launcher firmware).

After that it stays "sharp": **launch -> 4 s -> prepare again -> ready**. The overlay cannot be dismissed without
an answer; the other switches (fog, lights, siren) keep working while it is open. The answer matching the
last stored state is pre-selected (a hint only, the driver is always asked).

The unit only *believes* things about the machine. The belief is dropped, and the driver is asked again
(L2 flashing, same overlay, title `! LAUNCHER ERROR !` after an error), when it can no longer be trusted:
the launcher reports an error, a preparation runs too long (90 s) or is refused (no reaction within 3 s,
a real refusal), a prepare write failed 3 times, the launch write failed (outcome unknown), the link
dropped *during* a preparation, or the driver chose Menu -> Check launcher. A short link dropout while armed
or at rest keeps the belief (a bump on the road must not bother the driver). A preparation that this unit
watched finish is trusted without asking. Launcher errors are shown only by L2 and the status text; the driver
fixes things by hand and confirms.

Safety rules (all in `control_logic.cpp`, pure and unit-tested; mutation-tested):
- **Launch is a one-shot**: sent at most once per press, never retried, never replayed on reconnect. Needs a
  connected launcher, a *current* state, idle, no question open, and the belief "armed". A rejected launch sends
  nothing and says why ("not ready yet", "still preparing", "check launcher", "launcher offline", ...).
- The next prepare is only sent after the launch write was reported OK.
- Relays and fog are **state settings**: what the switches / modes want (see Devices) is compared with what
  the switchboard reports and re-sent (every 0.5 s, at once when the wish changes) until it matches. This also re-applies them after a reconnect
  (the switchboard turns everything off when its last client disconnects).
- Unknown switch state (expander failure): no relay is changed, fog is stopped, SW2 / SW5 count as off.
- Stale state is never shown as current: on disconnect all values of that device become invalid.

### Devices, automatic mode and panic mode

Everything the switchboard drives is in one table (`include/control_patterns.h`, relay numbers in `control_config.h`). Per device: the
dedicated manual switch, what it does in **automatic mode (SW5)** and in **panic mode (SW2)**.

| Device | Physical placement | Voltage | Connection | Activation | Behaviour in auto mode (SW5) | Behaviour in panic mode (SW2) |
| --- | --- | --- | --- | --- | --- | --- |
| Red flashlight | Outside right back | 220 Vac | Relay 1 | SW4 / Auto / Panic | Repeating 3 s on, 1 s off, both with a random variation of up to 2 s extra | On |
| Orange flashlight | Inside | 220 Vac | Relay 2 | Panic mode | Off | On |
| Strobe light | Inside | 220 Vac | Relay 3 | Panic mode | Off | On |
| Illumination | Exterior | 220 Vac | Relay 4 | SW6 / Auto / Panic | Continuously on, with a short disruption every now and then (simulates a grid power failure) | Blink 1 s on, 1 s off |
| Orange flashlight | Outside right front | 24 Vdc | Relay 5 | SW4 / Auto / Panic | Repeating 1 s off, 3 s on, both with a random variation of up to 2 s extra | On |
| Airhorn | Outside right front | 24 Vdc | Relay 6 | SW7 | Off | Off |
| Siren | Outside left back | 12 Vdc | Relay 7 | Panic mode | Off | Briefly on for 400..600 ms (random), once every 10 s (a very loud siren) |
| Orange flashlight | Outside left front | 12 Vdc | Relay 8 | SW4 / Auto / Panic | Repeating 2 s on, 2 s off, 4 s on, 2 s off | On |
| Fog machine | Inside | 220 Vac | Switchboard fog output (D9) | SW3 / Auto / Panic | Randomly on for 2 s, once per minute | On |

(The relay numbers are the owner's current wiring; the physical placement column is for reference only, the software does not use it.)

**Priority**, decided per device, every control step (`ControlLogic::handleDevices`):
1. its dedicated manual switch is ON -> steady on, whatever SW2 and SW5 say (e.g. SW4 on = the three flashlights are always on; SW6 on = the illumination
   is steady even in panic mode; SW3 on = no 2 s fog bursts);
2. panic mode (SW2) is ON -> the device's panic behaviour (also when that is "off"): **panic beats automatic mode**;
3. automatic mode (SW5) is ON -> its auto behaviour;
4. otherwise off. Every wired relay is driven all the time, so leaving a mode switches its devices off again by itself. When another source takes over, a
   pattern starts again at its first phase.

**Patterns** (`control_patterns.h`, "how to edit" is at the top of that file): a pattern is a cycle of phases; a phase is "on" or "off" for a duration drawn at
random between `minMs` and `maxMs` (equal = fixed). A pattern with `periodMs` > 0 lasts exactly that long: its last phase gets whatever is left of the period
(this gives "once every 10 seconds" for the siren and "once per minute at a random moment" for the fog machine, without drift). The patterns in use:
red flashlight `{3..5 s on, 1..3 s off}`; orange right front `{1..3 s off, 3..5 s on}`; orange left front `{2 s on, 2 s off, 4 s on, 2 s off}`; illumination (auto)
`{30..90 s on, 0.4..1.0 s off, 0.2..0.5 s on, 0.3..0.8 s off}` (**my guess of a "short disruption": tune it**); illumination (panic) `{1 s on, 1 s off}`;
siren (panic) `{0.4..0.6 s on, rest of 10 s off}`; fog (auto) `{0..58 s off, 2 s on, rest of 60 s off}`.
New patterns or devices: add a `Phase` array + `Pattern`, a `DeviceId` and a row in `DEVICES`; the tests (`test_patterns.cpp`, the spec test in `test_control.cpp`) check the table.
Siren / flashlights use the plain relay on/off (the switchboard's own blink mode is not used any more, so the random variation is possible).

LEDs and the home screen: L1 = SW5, L6 = SW2 (fast flash = switchboard unreachable), L7 / L4 = the flashlight / illumination group is active (any source) and in
sync; the "airhorn" tile follows SW7, the "auto" tile follows SW5, the panic tile SW2.

### LEDs

| LED | Meaning |
| --- | --- |
| L1 | Automatic mode (SW5): on = automatic mode on, fast flash = on but the switchboard cannot be reached |
| L2 | Launcher: on = ready, pulse = preparing, slow flash = waiting for the driver's answer or re-arming, fast flash = error (also 1.5 s after a rejected launch) |
| L3 / L5 | Launcher / switchboard connection: off = nothing saved, flash = searching or connecting, on = connected, fast flash = error |
| L4 / L7 | Illumination / flashlights: on = active (their switch, panic or automatic mode) and every relay follows, flash = a relay does not follow for > 0.7 s, fast flash = for > 3 s, off = not active |
| L6 | Panic (SW2): on = panic mode on, fast flash = on but the switchboard cannot be reached |
| L8 | Fog: off = machine not connected, flash = heating, on = ready, pulse = fogging, fast flash = fog wanted but impossible |
| L9 | Launcher busy: **pulse = preparing** (from the start of a preparation until it is done), off = ready to launch (and in every other state), fast flash = launcher error (like L2). L2 turns on when L9 turns off. |
| L10..L12 | Reserved, off |

Each LED can be OFF, ON, FLASH (1 Hz), FLASH_FAST (5 Hz), PULSE, RAMP_UP, RAMP_DOWN (`leds_set_state`).

## Project layout

| File | Responsibility |
| --- | --- |
| `include/config.h` | ALL pins, active levels, I2C address, timings, display/UI/link tunables |
| `include/control_config.h` | Relay numbers (the owner's wiring), switch indices and all control timings: **edit here** |
| `include/control_patterns.h`, `pattern_runner.h`, `src/pattern_runner.cpp` | The device table (manual switch / automatic / panic behaviour per device) and the patterns: **edit here for timings**; the pattern engine is pure and host-tested |
| `include/ui_strings_en.h`, `ui_strings.h` | ALL display texts in one file, with compile-time length checks; `ui_strings_nl.h` is the Dutch version: build it with `pio run -e arduino_nano_esp32_nl` (= `-D UI_LANG_NL`; the default environment builds English). Add another language: copy, translate, add a branch in `ui_strings.h`. Logging stays English |
| `src/main.cpp`, `src/app.cpp` | `setup()`/`loop()`; init order and one non-blocking pass of the periodic work |
| `src/control_logic.cpp` | Pure control logic: switches + device states + driver answers -> commands, LEDs, messages |
| `src/controller.cpp` | Glue: gathers inputs, runs the logic, executes commands via `ble_link_write`, drives L2/L4/L6/L7/L8/L9, stores the launcher belief hint |
| `src/ble_link.cpp` | Connection manager: one BLE task keeps both devices connected; queued one-shot writes with results |
| `src/ble_scanner.cpp` | NimBLE scan; the callback only copies into a locked queue |
| `src/ble_devices.cpp` | Service UUIDs and the characteristics used per device (the one table to extend) |
| `src/device_store.cpp` | The ONE saved launcher and switchboard in NVS (validated records) + the launcher state hint |
| `src/ui.cpp`, `include/ui_theme.h`, `fonts.h`, `ui_compat.h` | Menus, setup, check overlay and navigation; palette, fonts, LVGL 9.1..9.6 compatibility wrappers |
| `src/control_logic.cpp` (maintenance part) | `handleMaintenance`: release, dispense start/stop, pause of the automation; host-tested |
| `src/ui_home.cpp` | The animated home screen on LVGL: 3 canvases (band, tiles, toast) + labels; redraws only what changed |
| `src/home_model.cpp`, `include/home_model.h`, `home_config.h` | PURE model of the home screen (band state, progress, tiles, toast) and its timings/layout. Host-tested |
| `src/home_sprites.cpp`, `src/gfx.cpp` | PURE pixel art (RGB565 surfaces, clipped primitives): band animations, tile icons, toast. Host-tested |
| `include/splash_config.h`, `splash_model.h`, `src/splash_model.cpp` | Start-up splash: timings, layout, colors; PURE model (when it is over, badge states, dots, "push = continue"). Host-tested |
| `include/splash_sprites.h`, `src/splash_sprites.cpp`, `src/ui_splash.cpp` | The link row pixel art (pure, host-tested) and the LVGL splash screen (title with outline, subtitle, canvas, status line, version) |
| `include/splash_art.h`, `src/splash_art.cpp`, `tools/make_splash_art.py` | GENERATED: the splash picture as a const RGB565 array (160x69) and the background color for the rest of the splash. Regenerate: `python3 tools/make_splash_art.py <image 160 px wide>` (needs Pillow) |
| `src/fonts/font_splash_title.c`, `font_splash_sub.c`, `tools/make_splash_fonts.sh`, `include/version.h` | Splash fonts: Passion One Bold 26 px (title) and Barlow Condensed Bold 17 px (subtitle), both SIL OFL, 1-bit; `APP_VERSION` shown on the splash |
| `src/display.cpp` | LovyanGFX ST7735S + LVGL (full-frame direct render) |
| `src/fonts/font_pixel_16.c`, `tools/make_fonts.sh` | Generated 8x16 1-bit pixel font (Unscii, public domain, all Latin-1 glyphs) |
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
- Writes go through a per-device queue in the BLE task and are executed once; the result can be followed
  (`ble_link_write_state`); never retried there.
- Launcher characteristics used: preparation state (`...26b1`), prepare (`...26ae`), launch (`...26b2`, also used by *Release bucket*), and the
  dispenser control (`...26b0`, read/write/notify: `0x01` start, `0x00` stop, `0x02` run until `0x00`, `0x03` run backwards until `0x00`;
  any non-zero reading = running), bottom switch (`...26aa`, read/notify: `0x01` pressed, `0x00` released; display only), dispenser time (`...26af`, read/write, one byte of seconds,
  read at connect and on demand via `ble_link_read`, which travels the same queue as the writes).
  The dispenser duration (`...26af`) is not used by this unit yet.
- Pull motor (`...26a8`, optional, read/write/notify): signed 32-bit big-endian commanded speed, clamped to `-512..512`; notifications follow
  BLE writes and launcher motor commands. Used only by *Motor control* (any non-zero speed = running).

### Display

- LVGL renders into a 40 KB full-frame buffer (direct mode); only changed rows are sent to the panel.
- **LVGL's RGB565 is native-endian = `lgfx::rgb565_t`.** Passing a plain `uint16_t*` to LovyanGFX swaps the bytes
  (green grays, wrong accents, odd anti-aliasing at rounded corners). `flush_cb` casts correctly. The earlier
  "blank lines in the cards" artefact disappeared with the full-frame render.
- The panel is wired **BGR**: `DISPLAY_RGB_ORDER` defaults to `true`. `DISPLAY_SPI_HZ`, `DISPLAY_INVERT`,
  `DISPLAY_ROTATION` are in `config.h`.
- Fonts: 1-bit pixel fonts only. LVGL's built-in `lv_font_unscii_16` is the 8x8 font doubled (16 px per
  character, only 10 per row), so the project uses its own 8x16 `font_pixel_16` (20 characters per row) and
  `lv_font_unscii_8` for small captions. All layout numbers derive from these metrics and are `static_assert`ed.
- The home screen, the menus and the maintenance screens were checked off-target (`test/offtarget/`): scripted sessions on
  host builds of LVGL 9.1 and 9.6, frames compared pixel for pixel (identical). The setup/scan screens were checked earlier
  the same way (that older harness is not in the repository).

### Home screen

160x128, no connection row (L3/L5 show that), no hint. From top to bottom (`home_config.h`):
- **Launcher band** (160x78 canvas, 12 fps while the home screen is visible): left stripe in the state color, animation
  left, a big word and a small word right (LVGL labels), thin progress bar while preparing.
  Offline (antenna ripples) / check ("?") / error (blinking triangle) / starting (spinner) / **down** (arrows down) /
  **lock** (padlock closes + click, plays once when the launcher is SEEN to switch from down to up; not when it is first
  seen already going up) / **up** (arrows up; the launcher's "slow" and "fast" are one phase) / **ready** (rocket on the
  pad with smoke) / **launched** (rocket flies out; only after the launch write was reported OK, until then the band keeps
  showing READY) / **re-arm** (big 4-3-2-1 countdown, stripe alternates amber/red = stand clear).
- **Progress bars are time based** (`PREP_DOWN_MS` = `PREP_UP_MS` = 25 s, nominal) and only decorate: the real launcher
  state decides when a phase ends. The up bar starts after the lock animation and finishes `PREP_UP_MS` after the switch
  from down to up (the lock time is subtracted); all bars are clamped to 100 %.
- **7 tiles**: fog, panic, flashing light, decorative lamp, airhorn, auto (SW5), dispenser.
  Tile state comes from the same states as the LEDs: off (gray), on (steady), active (animated icon), pending (amber blink =
  wanted, not confirmed), problem (red blink). Colors: teal = switchboard, orange = launcher. Fog: ready = steady,
  fogging = drifting dashes. The dispenser tile is active while the launcher's dispenser characteristic reads `0x01`
  (`0x00` = not dispensing; a missing characteristic = off).
- **Toast** (slides in at the bottom): events (a fresh message, 3 s) win over conditions (shown while true, the most
  important plus "+N": switchboard offline for > 3 s, fog problem, relay problem). Launched messages are not toasts (the rocket says it).

## Decisions and assumptions

- Switches: closed = connected to GND = active; expander pull-ups on. LEDs: active LOW.
- Expander failure: switch state is UNKNOWN (never "unchanged"); the driver retries every 250 ms.
- Flashlights, siren etc. use plain relay on/off (patterns are timed by this unit, the switchboard's blink mode is not used).
- SW9..SW12 (A2, A3, A6, A7) are on the schematic but have no function and are not configured.
- A prepare write that failed is retried with the SAME choice (with / without the dispenser). With a launcher error, "Bucket full" prepares too (without the dispenser).
- After a launch the next prepare starts 4 s later (`REARM_DELAY_MS`, compile-time checked to be >= the latch re-lock time `LATCH_RELOCK_MS`); the launch write must be confirmed first.
- The UI uses `RELEASED` (not `CLICKED`) events so a slow button press still counts.

## Manual test checklist

- [ ] Boots into Setup with nothing saved; into the home screen with both saved
- [x] Colors: orange "Launcher", teal "Switchboard", amber/green status, indigo focus highlight, neutral gray cards, no green pixels at rounded corners, no blank lines
- [ ] Setup flow: scan lists only the chosen kind, first device gets the focus, pick saves + connects, "Scan again" only after the scan, Forget confirms
- [ ] Both devices on: the check overlay appears; "Not sure" shows the help page and asks again; Back does nothing on the overlay
- [ ] "Bucket empty" (launcher `0x01`): preparation starts by itself, the dispenser runs at the down -> up switch, L2 pulses and **L9 pulses** (L9 off when ready), then L2 solid; the band shows spinner, arrows down + bar, padlock, arrows up + bar, rocket
- [ ] "Bucket full" (launcher `0x02`): preparation starts by itself, the dispenser does NOT run, the bucket keeps its load and the launch works afterwards; also with the bucket already locked at the bottom (short down phase) and with it hanging at the top
- [ ] After a launch the automatic preparation fills the bucket again (`0x01`)
- [x] (2 s version; re-test with 4 s) SW8 when ready: launches once, "LAUNCHED", countdown "re-arming in ..", prepares again after ~4 s, ready again
- [ ] SW8 when not ready: nothing sent, toast ("not ready yet" ...) + L2 fast flash. Never fires twice for one press
- [x] **Top of the screen is clean** with both boards offline at power-up (verified on the panel with v0.12.1). Still to see: the toast text is vertically centered in its bar (v0.12.2), also with the fog switch on without a machine.
- [x] (verified on the panel by the owner) **Motor control** (launcher connected, band slack): holding "up" / "down" moves the motor in the expected direction at the expected speed and stops at once on release and on Back; turning the knob while holding changes nothing; a quick release+press is refused for a moment; unplugging the launcher while it runs stops it (launcher side).
- [ ] **Dispenser time**: the screen opens with the launcher's current value; turning changes it 1 s per click within 1..60 and sends nothing; pushing saves and shows "time saved" only after the read-back matched; the value is still there after a power cycle of the launcher; a save during a preparation is refused; "Hold to dispense" shows the running time and "last: x.x s".
- [ ] **Hold to reverse** un-jams / runs the dispenser backwards while held and stops at once on release; pressing "dispense" right after "reverse" is refused for a moment. **Bottom switch** screen follows the switch live when you press it by hand, and shows NO READING with the launcher off.
- [x] (verified on the panel by the owner) **Maintenance** (launcher connected): the menu scrolls; automation pauses (no prepare starts, SW8 blocked); **Release bucket** asks first, Cancel does nothing, Yes releases the lock; afterwards leaving maintenance asks the check question. **Dispense** runs only while the knob is held and stops at once on release / Back; with the launcher unplugged mid-run it stops on its own (launcher side).
- [ ] **Home screen on the panel**: colors/animations look right, no tearing or flicker, switches/LEDs/BLE still react immediately while the band animates (watch for stalls), SW8 rocket + 4-3-2-1 countdown, dispenser tile lights while the dispenser runs, fog tile states (needs the machine), toast for a missing switchboard (after 3 s)
- [ ] Switch the launcher off/on while ready: no question (kept); switch it off during a preparation: asked again
- [ ] Launcher error (or refused/failed prepare): L2 and L9 fast flash + overlay with the error title; fix by hand, answer, preparation retried
- [ ] Menu -> Check launcher: overlay opens, automation stops until answered
- [ ] SW4 / SW6 / SW7 (manual): log `-> switchboard RELAYS [n 1]`; SW4 = the three outside flashlights (relays 1, 5, 8), SW6 = illumination (relay 4), SW7 = airhorn (relay 6) only while pressed; L4 / L7 on, L7 flashes when a relay does not follow
- [ ] Reconnect the switchboard with SW4 ON: the relays are switched on again by itself
- [ ] **Panic (SW2)**: red, both orange outside, orange inside, strobe on; illumination blinks 1 s / 1 s; the siren sounds briefly (400..600 ms) once every ~10 s (TEST WITH EAR PROTECTION); fog on; the airhorn stays off; SW2 off = everything off. L6 on
- [ ] **Automatic mode (SW5)**: the red flashlight cycles about 3..5 s on / 1..3 s off, the orange right front 1..3 s off / 3..5 s on, the orange left front 2 on / 2 off / 4 on / 2 off; the illumination is on with an occasional short disruption; the fog gives one 2 s burst per minute; siren / airhorn / inside lights stay off. L1 on
- [ ] Priorities: SW4 on keeps the flashlights steady in automatic and panic mode; SW6 on keeps the illumination steady in panic mode; panic beats automatic mode when both are on; SW3 on = continuous fog
- [ ] Disrupted switchboard link in automatic / panic mode: L1 / L6 fast flash, everything is set again after the reconnection
- [ ] Expander disconnected: clear error, retries, no freeze; recovery restores LEDs and switches

## TODO

Everything that is still open, grouped. Items marked **(you)** need a decision or hardware work from the owner first.

### Next up
- [x] **Start-up splash, text-only version** (v0.18, not yet run on the hardware).
- [x] **Splash artwork** (v0.19, not yet run on the hardware; the notes below are the original plan, kept for the record): : the owner supplies the image of Sinterklaas and the cannon WITHOUT any text (the example picture has a pixel grid over it and baked-in lettering, so it is not usable as a source). Then: `tools/` script PNG -> static RGB565 `.h` (160 px wide,
  about 160x70 = 22 KB), place it at the top, re-tune the fonts / positions (title and subtitle may have to get smaller, the whole text block is about 94 px high now), optional animation of the smoke puff / a flying nut. Show renders / a GIF of the real 160x128 result BEFORE changing firmware.
- [ ] Splash polish (optional): check on the real panel how the title overlaps the picture (Santa's feet / staff), the flame colors and the background seam (RGB565 rounding), animate the smoke / a flying nut; confirm the badge numbering (1 = launcher, 2 = switchboard, as assumed), the subtitle in capitals, the version string (see Open questions), tune the pulse / arc speeds on the real panel.
- [x] (done, kept for the record) **Start-up splash screen** specification: full-screen logo ("nut-o-matic" / "peppernut maker"; Dutch "noot-o-maat" / "pepernoten maker"),
  no tiles and no toast, animated "connecting", shown from power-up until BOTH devices are connected and at least 2 s have passed; the OFFLINE band stays for a device that
  drops out later. The owner has an example image (Sinterklaas with the nut cannon, big red "nut-o-matic" lettering, "PEPERNOTEN LAUNCHER", "1 (BT) ))) (BT) 2", "Connecting...", "v1.0").
  My first five concepts (`tools/preview/make_splash_preview.py`: Salvo, The automat, Rooftops, Arcade boot, Minimal orbit) were **rejected: the pixel art is too simplistic
  and nothing like the example**; next time start from the owner's artwork (static bitmap) and only animate the connection part on top. Behavior still to agree on: skip it when
  setup is incomplete, the check question comes after it, after 20 s offer "push = continue". Technical: a static RGB565 bitmap in flash (a 160x70 area is ~22 KB).
- [x] Maintenance menu: Release bucket, Dispense, scrolling menus, toast fix (v0.11, verified on the hardware).
- [ ] **Review the whole menu structure** now that Maintenance exists: does everything still sit in the right menu (Check launcher, Setup, Maintenance), or must something move?
- [x] Maintenance: **Motor control** (v0.12, verified on the hardware).
- [x] Maintenance: **Bottom switch** screen and **Hold to reverse** (v0.14, built; not yet run on the hardware).
- [ ] Toast (low priority, not changed yet): a photo of the unit shows the toast geometry and text as designed, but the top rows of the first letters can look
  distorted, and a glitch seen during the 0.2 s slide stays visible afterwards. **Measured off-target: in the steady state the toast area is never re-sent
  to the panel** (0 flushes/s), so damage done while it slides in is never repaired. LVGL's data is clean (`panel_check`), so the cause is between `flush_cb`
  and the glass (probably tearing of the many small partial writes while it moves). Options: no slide (one single flush), or keep the slide and force one
  clean redraw of the toast area when it has arrived, or a fade. 
- [ ] Display efficiency (found while measuring, not changed yet): the launcher band is flushed ~28 times a second instead of the intended 12, because the progress bar value
  (1/1000 steps) triggers a redraw every ~25 ms; quantizing it to the bar's pixels (78 steps) would remove ~60 % of the band traffic without any visible change.
- [ ] Optional: understand why a full-frame flush brings the top-line glitch back (tearing? how LovyanGFX handles big `setAddrWindow` + `writePixels` transfers); not needed while partial flushes work.
- [ ] **(you)** Launcher firmware: stop the dispenser **and the pull motor** by itself when the BLE link drops (the unit also stops them, but cannot when it is dead).
- [ ] Motor control: decide whether the unit should refuse "down" at some point (no end stop / limit is known to it), or whether the launcher enforces limits.
- [ ] Dispense: the unit treats any non-zero dispenser value as "running". Confirm what the launcher reports while `0x02` / `0x03` is active.
- [x] Dispenser time from the Maintenance menu + run time on the Dispense screen (v0.15, not yet run on the hardware).

### Define / decide (needs the owner)
- [ ] **Back button (SW1)**: decide whether it has an operational role besides navigation.
- [ ] **Dispenser**: confirm that the launcher starting the dispenser during preparation is the desired final behavior.
- [ ] **Settings on the TFT**: decide which parameters are adjustable (dispenser duration `...26af`, fog burst duration, ...) and persist them (NVS). A "Settings" entry fits in the menu. Note: the dispenser duration is also used by the automatic cycle.
- [x] Automatic mode (SW5) and panic mode (SW2) with the owner's device table (v0.17, not yet run on the hardware); relay numbers are the owner's current wiring.
- [ ] Decide about the MOSFETs, digital outputs/inputs and analog inputs (unused).
- [ ] Decide what to do with SW9..SW12 and L10..L12 (L9 is now the "launcher preparing" LED).
- [ ] **Freshness policy**: a value is "current" while the link is up (notifications + read on connect). Decide whether a periodic re-read / staleness timeout is wanted for the launch interlock.
- [ ] Is the prepare timeout of 90 s and the 3 s "refused" window right for the real launcher? 

### Hardware verification
- [ ] Panel: "ask the driver again" paths (error, refused prepare, failed write), fog (needs the machine), launch + re-arm with the 4 s delay.
- [ ] Home screen: loop time / responsiveness while the band animates, look and speeds of the animations, dispenser tile.
- [ ] Free heap with the 40 KB frame buffer, the 38 KB canvases and NimBLE.
- [ ] Verify the switchboard characteristic UUIDs against its `include/config.h` (currently from its README).
- [ ] Panel gamma/color calibration if dark grays still look tinted.
- [ ] Final manual hardware checklist (above).

### Polish / later
- [x] Dutch translation (v0.20): `ui_strings_nl.h`, built with `pio run -e arduino_nano_esp32_nl`; all length checks pass and the off-target session renders every screen in Dutch. NOT yet seen on the hardware; **the owner should review the wording** (technical words like launcher / check / setup / prepare are deliberately kept English; "bucket" = "emmer", "switchboard" = "schakelbord", "dispenser time" = "doseertijd", "bottom switch" = "eindschakelaar").
- [ ] Tune the automatic-mode patterns on the real equipment (esp. the illumination disruption, which is my guess).
- [ ] Optional: hidden "Demo" menu entry that steps through all band/tile states with fake data and sends nothing.
- [ ] Optional: use the launcher's `release servo` characteristic as a sanity check at power-up (does the lock position match the answer?). Not reliable after a year in a barn, so never rely on it.
- [ ] Optional: a latch / "bucket at the bottom" sensor on the machine would give a real armed signal.
- [ ] Optional (launcher firmware): a persistent launcher state (rest / preparing / armed / error) exposed over BLE would survive control-unit restarts and dropouts and be shared with the Flutter app.
- [ ] Optional: a proper `lv_conf.h` instead of build flags; OTA/diagnostics.
- [ ] More host tests: UI navigation of the setup/scan screens, persistence defaults; bring an off-target harness for them into `test/offtarget/`.
- [ ] Document the safety assumptions that depend on hardware or operator procedure (final safety section).

## Open questions

Answered: relays are placeholders and must be easy to change; fog = keep fogging whenever possible; launch immediately,
no confirmation; the launcher is fully automatic (power-up check, auto prepare, auto re-prepare after ~4 s);
errors are only shown via L2/display and the driver just confirms; SW5 = automatic mode (not the launcher) and SW2 = panic mode as in the device table above; BLE addresses are public and stable; build environment as above.

Assumed, please confirm:
1. After a **power-up** the driver is always asked, even if the launcher was armed when it was switched off (the stored state is only a pre-selection).
2. ~~Siren / flashing light~~ Decided by the owner's device table: SW7 = airhorn (momentary), flashlights = SW4 + patterns.
3. **This unit is the truth for ALL eight relays:** it drives every wired relay all the time (switches / modes / patterns), so if someone else (e.g. the Flutter app) changes a relay, it is switched back within 0.5 s. Wanted, or should the unit only act when its own switch changes?
4. ~~Re-arm delay~~ Decided: **4 s** (>= latch re-lock time, compile-time checked). The 4-3-2-1 countdown with the alternating amber/red stripe is the "stand clear" warning; an extra LED/buzzer is not planned.

Splash (assumed in v0.18, please confirm):
S1. Badge 1 = launcher (orange ring), badge 2 = switchboard (teal ring). S2. The splash shows both badges "connected" for 0.5 s after the last device connected (so it is at least 2 s, and at least 0.5 s after both are up). S3. After 20 s without both devices it offers "push = continue"; a push before that does nothing. S4. The version text is `APP_VERSION` in `include/version.h` ("0.18" now; the example picture shows "v1.0"): which number do you want, and should it come from the build (`-D APP_VERSION=...`)? S5. Subtitle text in capitals ("PEPPERNUT MAKER" / "PEPERNOTEN MAKER"), as in the example. S6. The splash background is the picture's bottom-row color (blue-gray), not black like the rest of the UI. S7. The picture's average background is lighter at the top than at the bottom (a gradient); I matched the bottom edge so the picture melts into the lower part.

Open (see also the TODO list):
5. Is my guess for the illumination's grid-failure disruption OK (off 0.4..1.0 s, on 0.2..0.5 s, off 0.3..0.8 s, then on for 30..90 s)?
6. Dispenser: is the launcher starting it during preparation the desired final behavior?
7. Back button (SW1): navigation only, or should it also cancel/stop something?
8. Which parameters should be adjustable on the TFT (dispenser duration, fog burst duration, ...)?
9. ~~Relay numbers~~ Done: see the device table.
10. What should SW9..SW12 and L10..L12 be used for?
11. Is the prepare timeout of 90 s and the 3 s "refused" window right for the real launcher?
12. ~~Pull-motor characteristic~~ Used by Motor control (read / write / notify); a "stuck motor" detection is not planned.

## Splash screen
Layout (160x128): picture rows 0..68, title label y=61 (its ink starts ~3 px lower, so only the tall letters and the outline reach into the picture), subtitle y=80, link row canvas y=99 (96x20), version bottom right; "push = continue" at y=2 over the picture. All in `splash_config.h`. The background under the picture is the median color of the picture's bottom row, converted exactly like the picture's pixels (`SPLASH_ART_BG_*`), so there is no seam.
`ui.cpp` starts on `Screen::Splash` when both devices are saved (otherwise on Setup, no splash). Each loop pass `splash_step` feeds `SplashModel` with `ble_link_state()` of the two devices; when the model says "over" the UI enters the home screen, and only there the
"Anything in bucket?" check can open (it is blocked on the splash). Back and the encoder do nothing on the splash; a push only counts after the "push = continue" offer. The link row is a 96x22 canvas redrawn at 12 fps (about 25 k px per second to the panel; the home band is 150 k);
the rest is static labels. Texts: `STR_SPLASH_*` in `ui_strings_en.h` / `ui_strings_nl.h` ("noot-o-maat" / "PEPERNOTEN MAKER" / "druk = doorgaan"; title max 11 letters, subtitle 16; the Dutch splash is rendered by the off-target run). Fonts are regenerated with `tools/make_splash_fonts.sh` (OFL fonts, download instructions inside).

## Design study tools
`tools/preview/make_splash_preview.py` draws my first (rejected) start-up splash concepts (GIFs + a contact sheet, English or Dutch: `python3 make_splash_preview.py out nl`); only useful as a starting point for the lettering and the badge animation.
`tools/preview/make_preview.py` (Pillow + Unscii fonts) is the original Python design study of the home screen. It is superseded by
`src/home_sprites.cpp` (the firmware drawing code, same look) and is no longer kept in sync (e.g. its fog icon is the old one).
