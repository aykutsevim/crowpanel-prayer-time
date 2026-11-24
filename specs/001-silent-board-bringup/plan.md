# Implementation Plan: Silent Board Bring-up

**Branch**: `feat/001-silent-board-bringup` | **Date**: 2026-11-20 | **Spec**: [spec.md](spec.md)

**Input**: Feature specification from `specs/001-silent-board-bringup/spec.md`, prompt Step 3 of `docs/development/prompts/slice-01-board-bringup.md`

## Summary

This slice brings up the CrowPanel Advance 7.0" (DIS02170A, V1.4) silently. It covers display, touch and backlight, a permanent diagnostic mode, a stress mode, and a minimal Wi-Fi setup through the device's hotspot. It produces the evidence and the owner checklist that settle the display stack and the backlight behaviour.

Research changed the silence design. On V1.4 the amplifier control, the buzzer and the backlight driver are all reachable **only** through an auxiliary microcontroller (STC8H1K28 at I²C 0x30), using single-byte commands (research R3).

- **Silence function**: parks the ESP32 ends of the I²S lines, then sends "amplifier mute" (249) and "buzzer off" (247) with bounded retries.
- **Command gate**: every byte to that controller goes through one gate that cannot send "amplifier on" (248), "buzzer on" (246) or any dimming value. The backlight is always fully on (owner decision OD1, 2026-11-20).
- **Display**: Espressif's `esp_lcd` RGB driver with the vendor's timing from its own ESP-IDF example, with LVGL 9.6.0 on top. A documented mitigation ladder decides on evidence whether that stack holds under Wi-Fi, TLS and flash load (R7, R8).

## Technical Context

**Language/Version**: C++17 (gnu++17). Firmware uses plain **ESP-IDF 5.5.5** (`framework = espidf`, `framework-espidf@4.50505.0`) via PlatformIO `espressif32@8.0.0`, pinned exactly (R9). Until 2026-11-24 it used Arduino-ESP32 3.3.12; the display needed build options the precompiled Arduino libraries cannot have (research R8 L4, [decision 0002](../../docs/decisions/0002-display-stack.md)). Host code uses the same C++17 on `native@1.2.1`. Python 3 for tools.

**Primary Dependencies**:

- `lvgl/lvgl@9.6.0`
- ESP-IDF components: `esp_lcd` RGB panel, `i2c_master`, UART driver, `esp_wifi`/`esp_netif`, `esp_http_server`, `esp_http_client` with `esp_crt_bundle_attach`, `nvs_flash`
- Managed component `joltwallet/littlefs ==1.22.3` (`src/idf_component.yml`; the version Arduino 3.3.12 used, so the partition stays readable)
- No other third-party libraries. A GT911 reader is written in-house against the vendor-supplied datasheet (R5).

**Storage**:

- **NVS**:
  - `wifi/ssid`, `wifi/pass`: read by Slice 4 too (FR-025)
  - `ap/pass`
- **LittleFS** (partition label `littlefs`): stress scratch file `/scratch.bin`, at most 4 kB, rewritten in place (FR-013)

**Testing**:

- Unity on `native`, test-first for `lib/core`:
  - command gate, serial command parser, corner-touch judging, stress counters, report formatting, JSON escaping, reconnect backoff, AP password formatting
- Host snapshots of the diagnostic screens (env `sim`, R12)
- Device verification: the owner checklist

**Target Platform**: Elecrow CrowPanel Advance 7.0", DIS02170A rev V1.4, ESP32-S3-WROOM-1-N16R8 (16 MB flash, 8 MB octal PSRAM), 800×480 RGB565 panel (SC7277), GT911 touch.

**Project Type**: embedded firmware with host-testable core and host UI harness.

**Performance Goals**:

- Diagnostic screen within 5 s of power-on (SC-006)
- At least one stress cycle per minute (SC-003). Target: one scan plus one write about every 10 s.
- Encrypted request at most every 30 s
- UI stays responsive during stress (UI-04)

**Constraints**:

- No sound (SIL-01…06)
- TLS verification always on
- No secrets in output
- Image ≤ 2.9 MiB, reported after each build, with the diagnostic mode's share reported separately (FR-032)
- Internal RAM kept for Wi-Fi and TLS: LVGL draw buffer 64 kB internal, frame buffer 768 kB PSRAM

**Scale/Scope**: one device, three engineering screens (boot, diagnostic, settings), one setup page, 8 serial commands.

## Constitution Check

*GATE: checked before Phase 0 and re-checked after Phase 1.*

| Principle | Status | How this plan complies |
|---|---|---|
| I. Silence | Pass | `audio_off()` is the first call in `app_main()`. It parks GPIO 4/5/6 and sends 249 and 247 to the auxiliary controller with retries (R3). The gate rejects 246 and 248. No I²S, no tone, no LEDC. No audio component is required (`src/CMakeLists.txt` lists the components used), and every device build checks the linked image for I2S, DAC, LEDC and codec symbols (`tools/check_elf_silence.py`). Under Arduino, `ESP_I2S` and `ESP_SR` were in `lib_ignore`. The silence check allow-list stays `src/platform/audio_off.*` and `test/test_audio_off/*`. FR-001a blocks stress and Wi-Fi when the silence step is not acknowledged. The backlight is never dimmed (OD1 = C): the auxiliary controller's PWM frequency is undocumented, so the gate allows only "full on" (byte 0) and the ESP32 generates no PWM at all. Whether the auxiliary controller still switches the backlight driver at byte 0 is also undocumented (R6), so inaudibility is shown by owner checks A3 and D1, not claimed from paper (SIL-04). |
| II. Truthful display | Pass | No times or clock shown. The report says "acknowledged" (not "silent"), "skipped" (not "passed"), and "configured / not detected" for the revision. |
| III. Design contract | Pass | Only engineering screens, in English and outside `02-ui-spec.md` (spec assumptions). The hidden-mode gesture must not collide with `02-ui-spec.md` §8 (FR-007a). It is defined when the product screen exists (Slice 3). Proposed: two fingers held for 5 s, which is not used by §8. |
| IV. Offline first | Pass | Boot never waits for the network. Wi-Fi and stress run in a background task. |
| V. Verify, do not assume | Pass | All pins and commands cited from vendor sources (research R2–R6). Undocumented points listed as owner questions I1…I8 and items O3, O4. All versions exact (R9), including the `framework-espidf` package via `platform_packages` and the managed component in `src/idf_component.yml`. |
| VI. Testable core | Pass | Pure logic in `lib/core/src/nvp/diag/` with tests written first. The command gate is a header-only pure function in `src/platform/audio_off.h`, tested on the host. `lib/ui` diagnostic screens depend only on LVGL and `lib/core`, and render on the host. Only `src/platform` (and the entry point `src/main.cpp`) includes ESP-IDF or FreeRTOS headers: serial byte I/O (`console_io`) and task creation (`tasks`) live there, and `src/app` calls them. |
| VII. Owner in the loop | Pass | [owner-checklist.md](owner-checklist.md) is created now. Answers are recorded per stage. |
| VIII. Small, private, polite | Pass | One allowed host, at most 1 request every 30 s, User-Agent set. Built-in CA bundle (`esp_crt_bundle_attach`); `tools/check_tls.py` rejects a client configuration without a CA, `skip_cert_common_name_check`, insecure ESP-TLS options, and (from the Arduino period) `setInsecure` and one-argument `HTTPClient::begin("https…")`. Passwords are never printed or served. The AP password appears only on the device screen. |
| IX. Resource discipline | **Justified deviation** | Stress mode writes flash about every 10 s while it runs. That conflicts with "flash writes are rare" (SYS-03). See Complexity Tracking. Size is reported every build. |
| X. Simplicity and scope | **Justified deviation** | Wi-Fi provisioning comes forward from Slice 4, and the diagnostics stay in the product. Both are owner decisions recorded in the spec (2026-11-20). FR-033 corrects the roadmap, the architecture doc and the constitution in this branch. |

**Constitution amendment in this branch** (FR-033), version 1.0.0 → 1.1.0, MINOR:

- "Hardware and Product Constraints" names the Advance (DIS02170A) as the target.
- The partition line changes for 16 MB.

Principles I–III are untouched, so no special approval is needed. OD1 = C keeps Principle I as written.

**Post-design re-check (after Phase 1, and again after OD1 = C)**: passes. The contracts add no new hosts, sounds, PWM, or secrets in output. No open owner decisions remain; the board inspection answers I1–I8 are due before flashing.

## Project Structure

### Documentation (this feature)

```text
specs/001-silent-board-bringup/
├── spec.md
├── plan.md               # this file
├── research.md           # R1–R12, owner questions, stack decision log
├── data-model.md
├── quickstart.md
├── owner-checklist.md    # yes/no questions with answer columns (FR-030)
├── contracts/
│   ├── aux-controller.md   # command bytes and the gate
│   ├── serial-console.md   # commands and the report format
│   ├── setup-portal.md     # hotspot and HTTP paths
│   └── diag-screens.md     # layout of boot, diagnostic and settings screens
├── checklists/requirements.md
└── tasks.md              # /speckit-tasks
```

### Source code (repository root)

```text
boards/crowpanel7_advance.json        # 16 MB, QIO flash, octal PSRAM (R2)
sdkconfig.defaults                    # ESP-IDF options: PSRAM, XIP from PSRAM, IRAM-safe RGB ISR, 64 B cache line (R8 L4)
CMakeLists.txt, src/CMakeLists.txt    # ESP-IDF project and main component; src/idf_component.yml pins littlefs
partitions/crowpanel7_advance.csv     # NVS 24k, PHY 4k, app 3 MB, LittleFS 4 MB, rest unallocated
include/lv_conf.h                     # LVGL 9.6 config, shared by device and sim
platformio.ini                        # envs: native, sim, crowpanel7_advance (default)

src/main.cpp                          # app_main(): audio_off() first, then the UI task on core 1 runs app::start() and app::loop()
src/platform/
  audio_off.h / audio_off.cpp         # silence step + aux-controller gate (pure gate in header)
  board.h                             # pins and timing for DIS02170A V1.4, with citations
  display.h / display.cpp             # esp_lcd RGB panel, LVGL display + flush, set_pclk hook
  touch_gt911.h / touch_gt911.cpp     # bounded vendor start-up sequence, point reading
  backlight.h / backlight.cpp         # full on via the gate; no other level exists (OD1)
  storage.h / storage.cpp             # LittleFS mount, bounded scratch write
  wifi_link.h / wifi_link.cpp         # STA connect, backoff, async scan, AP
  i2c_bus.h / i2c_bus.cpp             # the shared I2C bus (aux controller, GT911)
  setup_portal.h / setup_portal.cpp   # esp_http_server on the AP; embedded HTML page
  https_probe.h / https_probe.cpp     # one GET with the CA bundle
  sysinfo.h / sysinfo.cpp             # heap, PSRAM, MAC, reset reason
  console_io.h / console_io.cpp       # UART0 line reader and writer (the only serial code)
  tasks.h / tasks.cpp                 # worker task on a given core, Lock type (FreeRTOS stays here)
src/app/
  bringup_app.h / bringup_app.cpp     # wiring, self-tests, stress logic (worker from platform/tasks, core 0), UI loop (core 1)
  serial_console.h / serial_console.cpp  # command dispatch; bytes come from platform/console_io

lib/core/src/nvp/diag/                # pure C++, host-tested first
  command.h/.cpp                      # serial command parser
  stress.h/.cpp                       # counters and schedule (scan/write every cycle, request every 30 s)
  touch_judge.h/.cpp                  # corner targets, pass within 10 px
  report.h/.cpp                       # report lines, password-free by construction
  json.h/.cpp                         # string escaping for /api/scan and /api/status
  backoff.h/.cpp                      # 5 s → 5 min
  ap_password.h/.cpp                  # 8 digits from a random source
  selftest.h                          # result types
lib/ui/src/diag/                      # LVGL only
  view_model.h                        # plain struct filled by src/app
  boot_screen.cpp, diag_screen.cpp, settings_screen.cpp

sim/diag_render/main.cpp              # renders view-model fixtures to out/snapshots/*.png (R12)
test/test_audio_off/test_audio_off.cpp
test/test_diag/…                      # one file per lib/core/nvp/diag unit
tools/check.py                        # env renamed; adds the TLS source check and the sim render
tools/check_tls.py                    # fails on setInsecure / one-argument https begin (SEC-01)
```

**Structure decision**:

- The layering of `05-architecture.md` §4 is kept.
  - **Diagnostic logic** is permanent product code (clarification 2026-11-20), so its pure parts live in `lib/core/src/nvp/diag/`.
  - **Diagnostic screens** live in `lib/ui/src/diag/`.
  - Only `src/platform` includes board, SDK or FreeRTOS headers. `src/app` uses only `src/platform`, `lib/core` and `lib/ui`. `src/main.cpp` is the entry point (`app_main`) and calls only those two.
- **The basic-board environment** `crowpanel7_basic` and its board/partition files are **removed**. The owner has no basic board. Git history keeps them, and a future basic profile is a new slice.

### Startup order (MODE-01, FR-001, FR-005, SC-006)

1. `audio_off()`:
   1. GPIO 4/5/6 driven low.
   2. I2C master bus on SDA 15, SCL 16 (`i2c::begin()`).
   3. Send 249 and 247 to 0x30, at most 20 tries × 50 ms. Result kept, including any byte the gate refused.
2. UART0 driver at 115200, then the first report line `audio-off: acknowledged | failed (reason)`, followed by any refusal recorded in step 1.
3. Backlight byte 0 (full) through the gate.
4. Display: `esp_lcd` RGB panel, LVGL, boot screen.
5. Touch start-up sequence: bounded, at most 10 tries (about 2.5 s worst case, while the boot screen shows).
6. Diagnostic screen. Log `[ms] diag screen shown` and keep the time for the report line `boot-to-diag` (SC-006).
7. LittleFS mount (format only the `littlefs` partition on failure; FR-013). It runs after the screen is up because a first-boot format can take several seconds.
8. NVS read: stored Wi-Fi details.
9. Full report on serial (FR-026).
10. Start the background worker on core 0: Wi-Fi join if details are stored and the silence step passed, the stress scheduler, the portal while the AP is up.

### Staging (from the slice prompt)

- **Stage 1**: steps 1–3, 7 and 9 (silence, serial, backlight full on, storage, report; `boot-to-diag: n/a (no display)`), board files, partitions, `lib/core/nvp/diag` units used by the report and parser, and serial commands `report` and `help`. No display. The owner flashes and compares against the expected serial output in [quickstart.md](quickstart.md).
- **Stage 2**: display + boot and diagnostic screens + sim snapshots; then touch. The owner answers the matching checklist part after each.
- **Stage 3**: Wi-Fi setup (settings screen, AP, portal, stored details), stress mode, and the remaining checklist. Mitigation ladder if needed.
- **Close-out**: project-document corrections and constitution amendment (FR-033, including SET-04 dropped), the stack decision recorded (FR-031), `/speckit-converge`.

## Complexity Tracking

| Violation | Why needed | Simpler alternative rejected because |
|---|---|---|
| Stress mode writes flash about every 10 s (SYS-03 "rare writes") | The display risk under flash writes is the main thing this slice must test (`05-architecture.md` §2). | Testing without writes would leave the riskiest case unverified. Writes are owner-started, bounded to one 4 kB file rewritten in place, and stop when stress stops. About 60 writes per 10 minutes is negligible wear for a 4 MB LittleFS. |
| Minimal Wi-Fi provisioning in Slice 1 (roadmap scope) | The owner decided (2026-11-20) that the encrypted request must really run. | Serial-entered credentials were offered and declined by the owner. The provisioning uses the Slice 4 paths and NVS keys, so nothing is thrown away. |
| Diagnostic mode ships in the product (adds flash, a hidden entry) | Owner decision (clarification 2026-11-20): the hardware can be re-checked at any time. | A separate test build was offered and declined. Its size is reported separately (FR-032). |
