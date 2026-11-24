# Hardware and architecture

This file fixes what is known about the hardware, recommends a software stack, and sets the code structure that the slices build on. Recommendations are a starting point for `/speckit.plan`; facts marked ⚠ must be confirmed there. Library and SDK versions move quickly: look up current documentation during planning and pin exact versions in the plan, rather than relying on memory.

**Updated after Slice 1 (2026-11-24, FR-033 of `specs/001-silent-board-bringup/spec.md`).** The target board is the Advance model DIS02170A, revision V1.4 (§1.2); the firmware is plain ESP-IDF 5.5.5 with `esp_lcd` and LVGL 9.6.0 (§3, `docs/decisions/0002-display-stack.md`). Facts verified in Slice 1 are marked ✓ with their date and the section of that feature’s `research.md` (cited as “R*n*”).

## 1. The board

### 1.1 Not the target: CrowPanel 7.0" basic, model DIS08070H

**Not a build target since 2026-11-20** (owner decision recorded in `specs/001-silent-board-bringup/spec.md`). The owner has no basic board, and the `crowpanel7_basic` environment with its board and partition files was removed in Slice 1 (git history keeps them). This section is kept for reference only; nothing in it was checked on hardware, and its ⚠ items stay open. Supporting this board again would be a new slice.

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

### 1.2 Target: CrowPanel Advance 7.0", model DIS02170A, revision V1.4

The project’s only build target since 2026-11-20 (owner decision, spec 001), environment `crowpanel7_advance`. Board definition `boards/crowpanel7_advance.json`, pins with citations in `src/platform/board.h`.

Facts below were read on 2026-11-20 from the vendor’s V1.4 schematic (Eagle, nets parsed), its V1.3–V1.5 examples, wiki, course and datasheets, cited in `specs/001-silent-board-bringup/research.md` R1–R6; owner confirmations on the board are dated where they apply. The vendor repository is `https://github.com/Elecrow-RD/CrowPanel-Advance-7-HMI-ESP32-S3-AI-Powered-IPS-Touch-Screen-800x480` (commit `70db9ed`, examples per hardware revision). Its readme’s “Pin definition” section belongs to another board and is not used (R1).

| Item | Value | Status |
|---|---|---|
| Revision | V1.4: electrically identical to V1.3 (only the BOOT/RESET key part changed); V1.5 changes only the touch connector footprint | ✓ R1 (2026-11-20); silkscreen V1.4 confirmed by the owner 2026-11-23 (I1) |
| Module | ESP32-S3-WROOM-1-N16R8: 16 MB flash (QIO, 80 MHz), 8 MB octal PSRAM (80 MHz) | ✓ R2; detected 16 MB flash and 8 MB PSRAM on the owner’s board 2026-11-23 (I2, esptool and report) |
| Serial | UART0, RX GPIO44 / TX GPIO43, through a CH340K USB bridge. GPIO19/20 (the S3’s native USB) carry the microphone or UART1 depending on the DIP switches, so USB-CDC is never used | ✓ R2 |
| I²C bus | SDA GPIO15, SCL GPIO16, shared by the auxiliary controller (0x30) and touch (0x5D) | ✓ R3, R5 |
| Auxiliary controller | U13, STC8H1K28, I²C address 0x30, single-byte writes with no register address, firmware not published, no read-back. It drives backlight enable (P1.1) and supply (P3.5), the amplifier control (P3.6 via Q9), the buzzer (P2.7 via Q8), touch reset (P1.7) and can hold the ESP32 in reset (P5.4) | ✓ R3; chip marking confirmed by the owner 2026-11-23 (I6) |
| Panel | 7.0" IPS, 800×480, 16-bit parallel RGB, driver SC7277 (datasheet DCLK 23–27 MHz) | ✓ R4 |
| RGB pins | B0…B4: 21, 47, 48, 45, 38. G0…G5: 9, 10, 11, 12, 13, 14. R0…R4: 7, 17, 18, 3, 46. DE 42, VSYNC 41, HSYNC 40, PCLK 39 | ✓ R4 |
| RGB timing | HSYNC and VSYNC each: front porch 8, pulse 4, back porch 8; polarities 0; data sampled on the falling PCLK edge. Vendor examples use 16 to 21 MHz; the project runs **14 MHz** (34 Hz), the lowest the panel accepted (12 MHz is rejected) | ✓ R4, stack decision log (2026-11-23) |
| Touch | GT911 at 0x5D, 400 kHz; interrupt GPIO1; reset through the auxiliary controller (byte 250, then INT held low while reset is released selects 0x5D). Bounded start-up, at most 10 attempts | ✓ R5; product ID `911` read and touch accepted by the owner 2026-11-23 (C1, C2) |
| Backlight | MT9201 boost LED driver, controlled only through the auxiliary controller. **The ESP32 has no backlight pin.** The controller’s dimming PWM frequency is undocumented | ✓ R6 |
| Amplifier | NS4168 (identity from the schematic; marking unreadable on the owner’s board, I5). CTRL low = shutdown (datasheet V1.2); CTRL has a pull-down and is fed only while the auxiliary controller pulls P3.6 low. I²S from the ESP32 on GPIO4 (data), GPIO5 (BCLK), GPIO6 (LRCLK), routed through the DIP switches | ✓ R3 (O3 resolved 2026-11-23) |
| Buzzer | passive, driven by the auxiliary controller (P2.7, Q8 with pull-down); off unless driven. Fitted on the owner’s board (I8) | ✓ R3 |
| DIP switches K1 (S1/S0) | route GPIO4/5/6 to the amplifier (0/0, “MIC&SPK”), the wireless module (0/1) or the SD card (1/1, “MIC&TF Card”). Any position other than 0/0 disconnects the ESP32 from the amplifier; they do not affect the buzzer. Recommended and set on the owner’s board: **1/1** (I3, 2026-11-23) | ✓ R3 |
| Strapping pins | GPIO3, 45 and 46 are LCD data lines, driven only after boot | ✓ R4 |
| Clock | on-board real-time clock with coin-cell backup (product page) | ⚠ not checked in Slice 1; confirm in the schematic before Slice 10 |
| Other | microSD, battery charging, a socket for add-on wireless modules, built-in microphone (product page) | not used |

Command bytes accepted by the auxiliary controller (V1.3 and later; R3):

| Byte | Meaning | Allowed by the firmware’s command gate |
|---|---|---|
| 0 | backlight brightest | **yes**: sent once after the silence step (FR-017, OD1) |
| 1…244 | backlight dimmer levels (244 dimmest); 2 and 3 are also used by a vendor example for the microphone path | no (OD1) |
| 245 | backlight off | no |
| 246 | buzzer on | **never** |
| 247 | buzzer off | **yes** (silence step) |
| 248 | amplifier on (“turn on the speaker”) | **never** |
| 249 | amplifier mute; the factory firmware sends it first | **yes** (silence step) |
| 250 | touch reset (“activate touch screen”) | **yes** (touch start-up) |
| 251…255 | undocumented | no |

Every byte to 0x30 passes through one pure gate function in `src/platform/audio_off.h`, unit-tested on the host (`test/test_audio_off/`). The wiki’s “Amp MUTE P3.7, SHUT P3.6” conflicts with the V1.4 netlist (P3.7 unconnected); the schematic wins (R3).

Consequences for this project:

- **Silence goes through the auxiliary controller.** The ESP32 cannot reach the amplifier control or the buzzer directly. The silence step drives GPIO4/5/6 low, then sends 249 and 247 to 0x30 (up to 20 tries, 50 ms apart). An I²C acknowledge proves the controller received the bytes, not that it acted, so the report says “acknowledged”, never “verified silent”; the owner’s listening checks are the evidence (§2).
- **No backlight dimming, ever** (owner decision OD1, 2026-11-20): the dimming frequency cannot be shown to be ≥ 20 kHz (SIL-04). SET-04 is dropped.
- **TIME-08 may come for free.** The on-board clock would remove the “no time after a power cut without Wi-Fi” gap without an external module; Slice 10 then targets the on-board chip once the ⚠ above is checked.
- **Flash is not a constraint.** 16 MB leaves room for a second application slot, so over-the-air update becomes possible after version 1. The 2.9 MB image budget stays as a discipline, not a necessity.
- **Better panel.** IPS removes the viewing-angle concern noted for the basic model’s TN panel.

### 1.3 How to tell which board it is

The model number is printed on the back label and silk-screen (`DIS08070H` or `DIS02170A`), and the hardware revision on the PCB (`V2.x`, `V3.0`, …). The owner confirms this in Slice 1 before any flashing.

Done on 2026-11-23: DIS02170A, silkscreen V1.4 (owner check I1). The firmware does not detect the board; the report prints `board: DIS02170A V1.4 (configured; not detected)`.

## 2. Hardware risks to settle in Slice 1

| Risk | Why it matters | How Slice 1 settles it | Outcome (Slice 1) |
|---|---|---|---|
| Picture instability on RGB panels while Wi-Fi and flash are active | The ESP32-S3 drives the panel from a frame buffer in PSRAM; cache and flash activity can disturb the pixel stream and show as horizontal shift or flicker. This class of problem is widely reported for ESP32-S3 RGB panels. | A stress screen with fine vertical lines while the firmware scans Wi-Fi, makes HTTPS requests and writes flash in a loop. The owner watches for 10 minutes. If unstable: lower the pixel clock, enable bounce buffers or the equivalent in the chosen driver, or change driver stack (§3). | **Settled 2026-11-24.** Every Arduino-ESP32 configuration (PCLK 14 MHz, bounce buffers, cache invalidation, writes deferred past scans) still drifted or garbled under Wi-Fi load. Plain ESP-IDF with four build options was stable for 10 minutes of stress (owner B5, serial log). Cause and ladder: research R8 and its “Stack decision log”; decision 0002. Rule kept for every later slice: **no flash write while a Wi-Fi scan is in progress.** |
| Touch start-up | on the basic board revision 3.0 it needs the expander sequence; on the Advance V1.4 the GT911 reset is behind the auxiliary controller | touch test screen; owner taps four corners | **Settled 2026-11-23.** Reset through byte 250 plus INT on GPIO1, bounded to 10 attempts (R5). All four corner targets passed within 10 px (C2); touch re-checked on the ESP-IDF firmware 2026-11-24 (C1). Two corners were at the 10 px limit; the touch scale (GT911 resolution registers 0x8146…0x8149) is still to be checked. |
| Backlight dimming | PWM may be audible or unsupported by the backlight driver | sweep duty at ≥ 20 kHz; owner listens in a quiet room and reports. If any noise or no dimming: backlight fixed on, brightness settings hidden (SET-04) | **Decided 2026-11-20 (OD1): no dimming, ever.** The backlight is dimmed only by the auxiliary controller at an undocumented frequency, so SIL-04 cannot be confirmed. The firmware sends byte 0 (full on) once and configures no PWM at all; the command gate refuses every other level. SET-04 is dropped. Owner: no whine with the backlight on (A3), backlight steady (D1). |
| Amplifier and, on the Advance model, buzzer | an enabled amplifier can hiss or pop; a floating buzzer pin can chirp at reset | drive every sound-capable control line to its inactive level first thing at boot; owner listens at the speaker connector or built-in speaker during boot and Wi-Fi activity | **Settled 2026-11-23/24.** Both sit behind the auxiliary controller (§1.2). The silence step mutes the amplifier (249) and turns the buzzer off (247) through the command gate, which never sends 246 or 248; the report shows `acknowledged`. Owner: silent at power-on, reset, stress, Wi-Fi setup and touch (A1–A6), with no speaker connected (I4) and DIP switches at 1/1 (I3), so the amplifier has neither input nor load. No sound heard at power-on with the factory firmware either (I7). |
| 4 MB flash | firmware with TLS, LVGL and fonts must fit without OTA | size report in every build; budget in §5 | **Not a constraint on the target** (16 MB). The 2.9 MB image budget is kept as a discipline and checked in every build. |

## 3. Stack

Chosen in Slice 1 on the evidence of the stress test (`docs/decisions/0002-display-stack.md`, 2026-11-24; research R8 “Stack decision log”). It replaces the recommendation of Arduino-ESP32 with LovyanGFX made before the board was known.

| Layer | Choice | Notes |
|---|---|---|
| Build | PlatformIO, driven from the command line | lets Claude Code build, flash, monitor and run host tests with one tool |
| Framework | **plain ESP-IDF 5.5.5**: `platform = espressif32@8.0.0`, `framework = espidf`, `platformio/framework-espidf@4.50505.0` pinned exactly | Decision 0002 supersedes the framework part of `docs/decisions/0001-platform.md`; the platform pin stays. Arduino-ESP32 3.3.12 was used until 2026-11-23. Its precompiled libraries fix build options the RGB panel needs under Wi-Fi load, and building Arduino as an ESP-IDF component would pull in `espressif/esp-sr` (audio) and cloud telemetry components (R8, L4 findings). Build options live in `sdkconfig.defaults`. Entry point `app_main()` in `src/main.cpp`. Always pin an exact release, never a moving `stable` link. |
| Panel driver | Espressif `esp_lcd` RGB panel | PCLK 14 MHz, vendor timing (§1.2), one frame buffer in PSRAM, bounce buffer 10 × 800 px in internal RAM, `dma_burst_size` 64. Depends on `CONFIG_SPIRAM_XIP_FROM_PSRAM`, `CONFIG_LCD_RGB_ISR_IRAM_SAFE`, `CONFIG_LCD_RGB_RESTART_IN_VSYNC` and `CONFIG_ESP32S3_DATA_CACHE_LINE_64B` (decision 0002). Closest to the vendor’s own ESP-IDF example for this board (R7). |
| Graphics | **LVGL 9.6.0** (`lvgl/lvgl@9.6.0`), RGB565, PARTIAL mode with one 800 × 40 draw buffer in internal RAM, on core 1 | UI code depends on LVGL only, never on the panel driver. Configured through `include/lv_conf.h` (R9). |
| Touch | own small GT911 reader in `src/platform/touch_gt911`, written against the GT911 datasheet | no third-party touch library: the reset is behind the auxiliary controller (R5) |
| I²C | ESP-IDF `i2c_master` driver (`src/platform/i2c_bus`) | one bus for the auxiliary controller and touch |
| Wi-Fi provisioning | own access point + HTTP server on `esp_wifi`/`esp_netif` and `esp_http_server`; DNS responder added in Slice 4 | a small custom portal, chosen in Slice 1 over a general-purpose Wi-Fi manager library because the page also selects the location and must match `02-ui-spec.md` §10. Slice 1’s paths and NVS keys are the base for Slice 4 (R11, `specs/001-silent-board-bringup/contracts/setup-portal.md`). |
| HTTP / TLS | `esp_http_client` with `crt_bundle_attach = esp_crt_bundle_attach` and the full built-in root-certificate bundle (`CONFIG_MBEDTLS_CERTIFICATE_BUNDLE_DEFAULT_FULL`) | certificate checks always on. `tools/check_tls.py` rejects `skip_cert_common_name_check`, an `esp_http_client` configuration without a CA, and insecure ESP-TLS options in `sdkconfig*` (decision 0002, SEC-01) |
| JSON | a streaming parser with field filtering | responses repeat large objects per record |
| Storage | ESP-IDF `nvs` for settings; LittleFS through the managed component `joltwallet/littlefs` **1.22.3** (`esp_littlefs`) for the day store | 1.22.3 is the version Arduino 3.3.12 used, so a partition formatted by the Arduino builds stays readable (decision 0002) |
| Prayer calculation | a vendored, pinned port of *adhan* | used only for fallbacks and plausibility checks |
| QR codes | a small QR generator library | setup and status screens |

Rejected in Slice 1 (R7, R8): LovyanGFX `Bus_RGB` (no bounce-buffer support; a reported blank-screen issue on an Elecrow 7" board was closed without a clear fix); `ESP32_Display_Panel` 1.0.4 (✓ checked 2026-11-20: **no** preset for the Advance board); Arduino-ESP32 as an ESP-IDF component (audio and telemetry dependencies); PSRAM at 120 MHz; two frame buffers.

The module boundaries in §4 kept the framework change local: only `src/platform` and `src/main.cpp` changed (decision 0002).

## 4. Code structure

```
lib/core/        Pure C++ (no ESP-IDF, FreeRTOS or LVGL headers).
                 Day records, validation, the prayer-time engine, text formatting
                 (durations, dates, period names), settings model, source ranks and
                 merge rules, calibration maths.
                 src/nvp/diag/: diagnostic logic from Slice 1 (command parser, report,
                 stress schedule, touch judge, backoff, AP password, JSON and form
                 helpers). Permanent product code.
lib/providers/   Pure C++. One parser per provider: bytes in, records out. The refresh
                 planner as a state machine that asks an abstract HTTP interface for
                 data, so it runs on a development machine with canned responses.
lib/ui/          LVGL only. Screens and widgets from 02-ui-spec.md. Takes a plain view
                 model struct; knows nothing about Wi-Fi, flash or time sources.
                 src/diag/: boot, diagnostic and settings screens from Slice 1.
                 Permanent product code.
src/platform/    Board bring-up (audio off, panel, touch, backlight), storage, Wi-Fi,
                 access point + DNS + HTTP server, HTTPS client, time, tasks, watchdog.
                 The only ESP-IDF code (decision 0002).
src/main.cpp     app_main(): the silence step first, then hands over to src/app.
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
- Only `src/platform` (and the entry point `src/main.cpp`) may include board, driver, ESP-IDF or FreeRTOS headers.
- No dynamic allocation in steady state inside the UI update path.
- **The diagnostic mode is permanent product code** (owner decision 2026-11-20, FR-007a of spec 001). The diagnostic and settings screens (`lib/ui/src/diag`) and their logic (`lib/core/src/nvp/diag`) stay in the firmware. From Slice 3 on the device boots to the product screen, and the diagnostics are reached only through a hidden touch gesture (proposed: two fingers held for 5 s, which `02-ui-spec.md` §8 does not use) or the serial command `diag`. Leaving the mode returns to the product screen and stops stress mode. Their flash cost counts against the image budget in every slice; `tools/check.py` reports it.

## 5. Flash and memory

Advance board (16 MB flash), no over-the-air update. Table `partitions/crowpanel7_advance.csv`, set in Slice 1:

| Partition | Label, type | Offset | Size |
|---|---|---|---|
| NVS | `nvs`, data/nvs | 0x9000 | 24 kB |
| PHY init | `phy_init`, data/phy | 0xF000 | 4 kB |
| Application | `factory`, app/factory | 0x10000 | 3 MB; the image must stay ≤ 2.9 MiB, kept as a discipline |
| LittleFS (day store, nothing else) | `littlefs`, data/spiffs | 0x310000 | 4 MB |
| Core dump | `coredump`, data/coredump | 0x710000 | 128 kB, the crash record written on a panic (ELF format) |
| Unallocated | — | 0x730000 | about 8.8 MB, left free on purpose |

NVS keys in use since Slice 1: namespace `wifi`, keys `ssid` and `pass` (the password is write-only, NET-08); namespace `ap`, key `pass` (eight digits, generated on first use, kept until factory reset). Slice 4 and later slices reuse them.

Fonts are the main controllable cost. The number fonts contain only digits, colon and hyphen; text fonts contain the Turkish set in `02-ui-spec.md` §2.2; 4 bits per pixel. Budget for all fonts: 300 kB.

Memory layout of the display (decision 0002): the frame buffer (800 × 480 × 2 B) lives in PSRAM; the bounce buffer (10 × 800 px) and the LVGL draw buffer (800 × 40 × 2 B = 64 000 B) live in internal RAM. Code also runs from PSRAM (`CONFIG_SPIRAM_XIP_FROM_PSRAM`). Keep internal RAM for Wi-Fi and TLS; a TLS handshake needs a few tens of kB of contiguous heap. With the Slice 1 firmware under stress, the internal heap had 56 467 B free with a largest block of 31 744 B (research, stack decision log, 2026-11-24); watch these numbers as product code is added. Fetches run one at a time (§10 of `04-data-sources.md`).

**No flash write while a Wi-Fi scan is in progress** (NVS, day store, any file). A write started during a scan broke the picture on the Arduino build; the ESP-IDF build passed the same case, and the rule is kept as a margin (decision 0002).

Over-the-air update is out of scope for version 1. The unallocated 8.8 MB leaves room for a second application slot afterwards.

## 6. Concurrency

- **UI task**: LVGL timer handler and screen updates. Reads an immutable snapshot of application state once per second (and on change notifications). Never blocks on I/O (UI-04).
- **App task**: mode state machine, Wi-Fi events, time sync, refresh planner, storage. Publishes snapshots.
- **HTTP server**: runs in its own context provided by the framework; handlers only read snapshots or enqueue commands for the app task.
- One mutex guards LVGL. One queue carries commands to the app task. No other shared mutable state.
- Cores (set in Slice 1, decision 0002): LVGL and the panel on core 1; Wi-Fi, TLS and flash work on core 0. FreeRTOS calls stay in `src/platform/tasks`.
- Watchdogs per SYS-01.

## 7. Seeing the UI without the hardware

Claude Code cannot look at the panel. Two mechanisms close that gap; build both in Slice 3 and use them in every later slice that touches the UI.

1. **Host UI harness.** Compile `lib/ui`, `lib/core` and LVGL for the development machine with a headless display driver that renders into an 800×480 buffer and writes PNG files. A command-line entry point renders a named fixture, for example time 14:32:07 with system state `ok`, to a file. A script renders all fixtures listed in `02-ui-spec.md` §11 into `out/snapshots/`. The agent opens each snapshot next to its reference render in `docs/design/screens/` and compares them; geometry of non-text elements can also be checked numerically.
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

- `src/platform` exposes one function that puts the amplifier in shutdown and parks the I²S pins; it is the first call after reset (SIL-02) and the only code that touches those pins. On the target board (§1.2) it drives GPIO4/5/6 low and sends 249 (amplifier mute) and 247 (buzzer off) to the auxiliary controller; it lives in `src/platform/audio_off.cpp`, and the command gate in `audio_off.h` refuses 246, 248 and every backlight level except 0.
- A script in `tools/` runs before every build and in the test run. It fails if project sources (`src/`, `lib/`, `include/`, `sim/`) contain any of: `i2s`, `I2S`, `tone(`, `noTone(`, `ledcWriteTone`, `ledcWriteNote`, `dac_output`, `dacWrite`, `Audio.h`, `buzzer`, outside the single platform file and its test; or if the dependency list names an audio library. The allow-list is that one file.
- Since the move to ESP-IDF the framework is no longer filtered through `lib_ignore`, so every device build also checks the linked image for I²S, DAC, LEDC and codec symbols (`tools/check_elf_silence.py`, decision 0002).
- PWM is configured only by the backlight module, at ≥ 20 kHz, and only on the backlight pin. On the target board the ESP32 has no backlight pin, so the firmware configures no PWM at all (R6, OD1).

## 10. Development commands

Confirmed in Slice 1 (`platformio.ini`, `tools/check.py`); the repository’s `README` lists them for daily use. PlatformIO environments: `crowpanel7_advance` (device, the default), `native` (host unit tests), `sim` (host UI harness). The basic board’s `crowpanel7_basic` was removed.

| Purpose | Command |
|---|---|
| Everything a slice must pass (tool tests, silence and TLS checks, host tests, firmware build, size report, image silence check, snapshots) | `python tools/check.py` (`--skip-firmware` for a host-only run) |
| Host tests | `pio test -e native` |
| Render UI snapshots | `pio run -e sim -t exec` (writes `out/snapshots/*.png`), then `python tools/check_snapshots.py` |
| Build firmware | `pio run -e crowpanel7_advance` |
| Flash | `pio run -e crowpanel7_advance -t upload` |
| Serial monitor | `pio device monitor -b 115200` |
| Silence check | `python tools/check_silence.py` (sources, before every build); `tools/check_elf_silence.py` (linked image, after every device build) |
| TLS check | `python tools/check_tls.py` |
| Size report | part of the build output (`tools/pio_size_budget.py`); fails over the 2.9 MiB budget. `tools/check.py` adds the size by module, including the diagnostic mode’s share |

The first ESP-IDF build of the framework takes about 10 minutes; later builds are quick (decision 0002).

## 11. Definition of done for any slice

1. Spec Kit artefacts for the slice exist and `/speckit.analyze` reports no open critical findings.
2. Host tests pass. Silence check passes. Firmware builds for the target within the size budget.
3. Where the slice touches the UI: snapshots for affected fixtures have been regenerated and reviewed against the references.
4. The owner has completed the slice’s verification checklist on the device and the results are recorded in the feature directory.
5. `/speckit.converge` reports converged.
6. The repository `README` and the open questions in `01-product-brief.md` are updated with anything learned.
