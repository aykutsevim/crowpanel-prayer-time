# Hardware and architecture

This file fixes what is known about the hardware, recommends a software stack, and sets the code structure that the slices build on. Recommendations are a starting point for `/speckit.plan`; facts marked ⚠ must be confirmed there. Library and SDK versions move quickly: look up current documentation during planning and pin exact versions in the plan, rather than relying on memory.

## 1. The board

### 1.1 Default target: CrowPanel 7.0" basic, model DIS08070H

Source: Elecrow wiki page “CrowPanel ESP32 HMI 7.0-inch Display”, read 17 November 2026.
Vendor repository: `https://github.com/Elecrow-RD/CrowPanel-7.0-HMI-ESP32-Display-800x480` (factory firmware, examples, schematic).

| Item | Value |
|---|---|
| Module | ESP32-S3-WROOM-1-N4R8: 4 MB flash, 8 MB PSRAM, 2.4 GHz Wi-Fi, Bluetooth 5 LE |
| Panel | 7.0" TN TFT, 800×480, 16-bit parallel RGB, driver EK9716BD3 + EK73002ACGB |
| Touch | GT911 capacitive, I²C: SDA GPIO19, SCL GPIO20 |
| Backlight | GPIO2 |
| Audio | on-board amplifier and `SPK` connector, I²S: LRCLK GPIO18, BCLK GPIO42, SDIN GPIO17, plus an amplifier control line named `I2S-CTRL` (⚠ find its GPIO or expander pin in the schematic) |
| SD card (SPI) | MOSI GPIO11, MISO GPIO13, CLK GPIO12, CS GPIO10 |
| UART0 | RX GPIO44, TX GPIO43 (also on USB-C through a USB-UART bridge) |
| Spare | `GPIO_D` connector: GPIO38; I²C connector shares GPIO19/20 with touch |
| Power | 5 V 2 A, USB-C or battery connector |

RGB wiring and timing from the vendor’s panel configuration:

| Signal | GPIO | Signal | GPIO |
|---|---|---|---|
| B0…B4 | 15, 7, 6, 5, 4 | DE | 41 |
| G0…G5 | 9, 46, 3, 8, 16, 1 | VSYNC | 40 |
| R0…R4 | 14, 21, 47, 48, 45 | HSYNC | 39 |
| | | PCLK | 0 |

Pixel clock 15 MHz, active on the falling edge. Horizontal: front porch 40, pulse 48, back porch 40. Vertical: front porch 1, pulse 31, back porch 13. Sync polarities 0.

Hardware revisions matter. The wiki states that **revision 3.0 added an I²C I/O expander (PCA9557) that must be sequenced at start-up or touch will not work**; the vendor’s sequence is: reset the expander, set outputs, drive IO0 and IO1 low, wait 20 ms, drive IO0 high, wait 100 ms, set IO1 to input. ⚠ Read the revision printed on the PCB, the schematic, and the vendor example for that revision before writing board code. Do not guess pins.

PCLK is on GPIO0, a boot-strapping pin. Keep that in mind when diagnosing boot or flashing problems.

### 1.2 Second target: CrowPanel Advance 7.0", model DIS02170A

Support it as a separate build profile only if the owner has this model. Everything above the board layer is identical.

What is known (vendor product page and independent reviews, read 20 November 2026; pins were **not** collected and must come from the vendor wiki and schematic for the exact revision):

| Item | Value |
|---|---|
| Module | ESP32-S3-WROOM-1-N16R8: 16 MB flash, 8 MB PSRAM |
| Panel | 7.0" IPS, 800×480, 16-bit colour, driver SC7277, 178° viewing angle |
| Touch | GT911 capacitive |
| Sound-capable parts | audio amplifier with speaker port and volume control, built-in microphone, **and a buzzer** |
| Clock | **on-board real-time clock with coin-cell backup** |
| Other | microSD, battery charging, a socket for add-on wireless modules, DIP switches (`S0`, `S1`) that route shared GPIOs between the wireless module, microphone, speaker and SD card |
| Revisions | V1.0 to V1.3 at least; reviewers describe an auxiliary microcontroller on later revisions that acts as I/O expander and peripheral manager ⚠ |

Wiki: “CrowPanel Advance 7.0-HMI ESP32 AI Display”. Vendor repository: `https://github.com/Elecrow-RD/CrowPanel-Advance-7-HMI-ESP32-S3-AI-Powered-IPS-Touch-Screen-800x480` (examples are organised per hardware revision).

Consequences for this project if the Advance is the target:

- **Silence needs more care.** Three things can make noise: the amplifier, the buzzer, and anything the auxiliary controller drives. All must be identified in the schematic and held inactive from the first instruction after reset (SIL-02, SIL-03). The DIP switches should be set so that the speaker path is not selected, and that position recorded in the owner guide.
- **TIME-08 comes for free.** The on-board clock removes the “no time after a power cut without Wi-Fi” gap without an external module. Slice 10b then targets the on-board chip.
- **Flash is not a constraint.** 16 MB allows a second application slot, so over-the-air update becomes possible after version 1. The 2.9 MB image budget stays as a discipline, not a necessity.
- **Better panel.** IPS removes the viewing-angle concern noted for the basic model’s TN panel.

### 1.3 How to tell which board it is

The model number is printed on the back label and silk-screen (`DIS08070H` or `DIS02170A`), and the hardware revision on the PCB (`V2.x`, `V3.0`, …). The owner confirms this in Slice 1 before any flashing.

## 2. Hardware risks to settle in Slice 1

| Risk | Why it matters | How Slice 1 settles it |
|---|---|---|
| Picture instability on RGB panels while Wi-Fi and flash are active | The ESP32-S3 drives the panel from a frame buffer in PSRAM; cache and flash activity can disturb the pixel stream and show as horizontal shift or flicker. This class of problem is widely reported for ESP32-S3 RGB panels. | A stress screen with fine vertical lines while the firmware scans Wi-Fi, makes HTTPS requests and writes flash in a loop. The owner watches for 10 minutes. If unstable: lower the pixel clock, enable bounce buffers or the equivalent in the chosen driver, or change driver stack (§3). |
| Touch on revision 3.0 | needs the expander sequence | touch test screen; owner taps four corners |
| Backlight dimming | PWM may be audible or unsupported by the backlight driver | sweep duty at ≥ 20 kHz; owner listens in a quiet room and reports. If any noise or no dimming: backlight fixed on, brightness settings hidden (SET-04) |
| Amplifier and, on the Advance model, buzzer | an enabled amplifier can hiss or pop; a floating buzzer pin can chirp at reset | drive every sound-capable control line to its inactive level first thing at boot; owner listens at the speaker connector or built-in speaker during boot and Wi-Fi activity |
| 4 MB flash | firmware with TLS, LVGL and fonts must fit without OTA | size report in every build; budget in §5 |

## 3. Recommended stack

| Layer | Recommendation | Notes |
|---|---|---|
| Build | PlatformIO, driven from the command line | lets Claude Code build, flash, monitor and run host tests with one tool |
| Framework | Arduino core for ESP32 | **Corrected in Slice 0 (20 November 2026):** PlatformIO’s official `espressif32` platform ships the 3.x core from release 8.0.0 (Arduino 3.3.12); the 7.x line ships 2.0.17. The community `pioarduino` platform also ships 3.x. The vendor’s current PlatformIO example for the basic board (`example/V3.0/PlatformIO70`) uses pioarduino with Arduino core **3.3.8**, not 2.x. The project pins `espressif32@8.0.0`; see `docs/decisions/0001-platform.md`. Always pin an exact release, never a moving `stable` link. |
| Panel driver | start from the vendor’s verified LovyanGFX `Bus_RGB` / `Panel_RGB` configuration above | known-good pins and timing for this exact board |
| Graphics | LVGL, latest stable major at planning time, 16-bit colour | UI code must depend on LVGL only, never on the panel driver |
| Touch | GT911 driver compatible with the chosen stack | plus the PCA9557 sequence on revision 3.0 |
| Wi-Fi provisioning | own access point + DNS responder + HTTP server | a small custom portal is preferred over a general-purpose Wi-Fi manager library because the page also selects the location and must match `02-ui-spec.md` §10; evaluate both in research |
| HTTP / TLS | the framework’s HTTPS client with the bundled root-certificate store | never `setInsecure()` or equivalent |
| JSON | a streaming parser with field filtering | responses repeat large objects per record |
| Storage | NVS for settings; LittleFS for the day store | |
| Prayer calculation | a vendored, pinned port of *adhan* | used only for fallbacks and plausibility checks |
| QR codes | a small QR generator library | setup and status screens |

Alternative to evaluate if the stress test fails or integration is rough: Espressif’s own display stack (`esp_lcd` RGB panel with bounce buffer, `esp_lvgl_port`, or the `ESP32_Display_Panel` Arduino library, which ⚠ may ship a preset for this board), or building on ESP-IDF directly. The module boundaries in §4 make this swap local to the platform layer.

Decision rule: prefer the option closest to a vendor-verified example for the first pixels on screen; change stack only on evidence from the Slice 1 stress test. Record the decision and the evidence in Slice 1’s `research.md`.

## 4. Code structure

```
lib/core/        Pure C++ (no Arduino, ESP-IDF or LVGL headers).
                 Day records, validation, the prayer-time engine, text formatting
                 (durations, dates, period names), settings model, source ranks and
                 merge rules, calibration maths.
lib/providers/   Pure C++. One parser per provider: bytes in, records out. The refresh
                 planner as a state machine that asks an abstract HTTP interface for
                 data, so it runs on a development machine with canned responses.
lib/ui/          LVGL only. Screens and widgets from 02-ui-spec.md. Takes a plain view
                 model struct; knows nothing about Wi-Fi, flash or time sources.
src/platform/    Board bring-up (audio off, panel, touch, backlight), storage, Wi-Fi,
                 access point + DNS + HTTP server, HTTPS client, time, tasks, watchdog.
src/app/         Mode state machine (BOOT / SETUP / CONNECTING / RUN), wiring of
                 platform ↔ core ↔ ui, development serial commands.
data/            Bundled place list, portal and settings pages (minified, gzipped).
assets/fonts/    Converted fonts and the script that regenerates them.
tools/           Place-list generator, silence check, snapshot tooling, size report.
test/            Host unit tests and fixtures (recorded provider responses).
sim/             Host UI harness (§7).
```

Rules:

- `lib/core` and `lib/providers` compile and pass their tests on the development machine with no hardware.
- `lib/ui` compiles for the host harness and for the device from the same sources.
- Only `src/platform` may include board, driver or SDK headers.
- No dynamic allocation in steady state inside the UI update path.

## 5. Flash and memory

Basic board (4 MB), no over-the-air update:

| Partition | Size (target) |
|---|---|
| NVS | 24 kB |
| PHY init | 4 kB |
| Application | 3.0 MB, firmware must stay ≤ 2.9 MB |
| LittleFS (day store, nothing else) | remainder, ≥ 512 kB |

Fonts are the main controllable cost. The number fonts contain only digits, colon and hyphen; text fonts contain the Turkish set in `02-ui-spec.md` §2.2; 4 bits per pixel. Budget for all fonts: 300 kB.

Frame buffer and LVGL draw buffers live in PSRAM. Keep internal RAM for Wi-Fi and TLS; a TLS handshake needs a few tens of kB of contiguous heap. Fetches run one at a time (§10 of `04-data-sources.md`).

Over-the-air update is out of scope on 4 MB. On the Advance board (16 MB) it can be considered after version 1.

## 6. Concurrency

- **UI task**: LVGL timer handler and screen updates. Reads an immutable snapshot of application state once per second (and on change notifications). Never blocks on I/O (UI-04).
- **App task**: mode state machine, Wi-Fi events, time sync, refresh planner, storage. Publishes snapshots.
- **HTTP server**: runs in its own context provided by the framework; handlers only read snapshots or enqueue commands for the app task.
- One mutex guards LVGL. One queue carries commands to the app task. No other shared mutable state.
- Watchdogs per SYS-01.

## 7. Seeing the UI without the hardware

Claude Code cannot look at the panel. Two mechanisms close that gap; build both in Slice 3 and use them in every later slice that touches the UI.

1. **Host UI harness.** Compile `lib/ui`, `lib/core` and LVGL for the development machine with a headless display driver that renders into an 800×480 buffer and writes PNG files. A command-line entry point renders a named fixture, for example time 14:32:07 with system state `ok`, to a file. A script renders all fixtures listed in `02-ui-spec.md` §11 into `out/snapshots/`. The agent opens each snapshot next to its reference render in `handover/design/screens/` and compares them; geometry of non-text elements can also be checked numerically.
2. **Development serial commands** on the device (SYS-06): set simulated time and speed, force a system state, load the test fixture, show a named screen. The owner photographs the panel only when a slice’s verification checklist asks for it.

## 8. Testing

| Level | What | Where |
|---|---|---|
| Unit | engine boundaries (ENG-07), formatting, validation, merge rules, calibration | host, every build |
| Parser | each provider against recorded real responses and malformed variants | host |
| Planner | refresh sequences with a scripted fake HTTP layer: all up, each provider down, rate-limited, bad data, year end | host |
| Snapshot | every fixture rendered and reviewed against the reference | host |
| Device smoke | boot, setup flow, real fetch, offline behaviour | on the board, owner-assisted, checklist per slice |
| Soak | 24 h run with memory and restart counters logged | on the board, Slice 9 |

Tests for `lib/core` are written before the code they test.

## 9. Enforcing silence

- `src/platform` exposes one function that puts the amplifier in shutdown and parks the I²S pins; it is the first call after reset (SIL-02) and the only code that touches those pins.
- A script in `tools/` runs before every build and in the test run. It fails if project sources (`src/`, `lib/`, `include/`, `sim/`) contain any of: `i2s`, `I2S`, `tone(`, `noTone(`, `ledcWriteTone`, `ledcWriteNote`, `dac_output`, `dacWrite`, `Audio.h`, `buzzer`, outside the single platform file and its test; or if the dependency list names an audio library. The allow-list is that one file.
- PWM is configured only by the backlight module, at ≥ 20 kHz, and only on the backlight pin.

## 10. Development commands

To be confirmed by the plan in Slice 1 and then recorded in the repository’s `README`:

| Purpose | Command (expected) |
|---|---|
| Host tests | `pio test -e native` |
| Render UI snapshots | `pio run -e sim -t snapshots` or a script in `tools/` |
| Build firmware | `pio run -e crowpanel7_basic` |
| Flash | `pio run -e crowpanel7_basic -t upload` |
| Serial monitor | `pio device monitor -b 115200` |
| Silence check | `python tools/check_silence.py` |
| Size report | part of the build output; fails over budget |

## 11. Definition of done for any slice

1. Spec Kit artefacts for the slice exist and `/speckit.analyze` reports no open critical findings.
2. Host tests pass. Silence check passes. Firmware builds for the target within the size budget.
3. Where the slice touches the UI: snapshots for affected fixtures have been regenerated and reviewed against the references.
4. The owner has completed the slice’s verification checklist on the device and the results are recorded in the feature directory.
5. `/speckit.converge` reports converged.
6. The repository `README` and this handover’s open questions are updated with anything learned.
