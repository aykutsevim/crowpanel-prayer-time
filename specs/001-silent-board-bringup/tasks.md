---
description: "Task list for Slice 1: Silent Board Bring-up"
---

# Tasks: Silent Board Bring-up

**Input**: Design documents from `specs/001-silent-board-bringup/`

**Prerequisites**: [plan.md](plan.md), [spec.md](spec.md), [research.md](research.md), [data-model.md](data-model.md), [contracts/](contracts/), [quickstart.md](quickstart.md), [owner-checklist.md](owner-checklist.md)

**Tests**:

- Included. Constitution Principle VI requires tests for `lib/core` to be written **before** the code they test, so each test task precedes its implementation task and must fail first.
- Device behaviour is verified by the owner checklist.

**Organization**:

- Tasks are grouped by user story (US1…US7 = spec Stories 1…7).
- Each phase also names the **stage** from the plan (Stage 1 = silence and report, Stage 2 = display and touch, Stage 3 = Wi-Fi and stress), because the owner flashes and checks after each stage.

**Rules for every task**:

- Cite the requirement ids in the commit message.
- Run `python tools/check.py` before committing (`--skip-firmware` for host-only tasks).
- Never report a hardware behaviour as verified without an owner answer or a log line (CLAUDE.md).
- The words `i2s`, `I2S` and `buzzer` and the audio APIs may appear **only** in `src/platform/audio_off.{h,cpp}` and `test/test_audio_off/` (the silence check enforces this).

## Format: `[ID] [P?] [Story] Description`

- **[P]**: can run in parallel (different files, no dependency on an unfinished task)
- **[Story]**: US1…US7

---

## Phase 1: Setup (shared infrastructure, Stage 1)

**Purpose**: amend the constitution for the Advance board first, then switch the project to it, pin every dependency and report the diagnostic-mode size from the first build (FR-032).

- [X] T001 Create `boards/crowpanel7_advance.json`: ESP32-S3, `memory_type` `qio_opi`, `flash_mode` `qio`, `f_flash` 80 MHz, 16 MB flash, `-DBOARD_HAS_PSRAM`, `-DARDUINO_USB_CDC_ON_BOOT=0` (GPIO19/20 carry mic/UART1, research R2), name "Elecrow CrowPanel Advance 7.0\" (DIS02170A, ESP32-S3-WROOM-1-N16R8)", url = REPO (research R2).
- [X] T002 [P] Create `partitions/crowpanel7_advance.csv`: nvs 0x9000/0x6000, phy_init 0xF000/0x1000, factory 0x10000/0x300000, `littlefs` (data, spiffs) 0x310000/0x400000. The remainder is unallocated, with a header comment citing plan.md and `05-architecture.md` §5.
- [X] T003 Amend `.specify/memory/constitution.md` to 1.1.0 (MINOR):
  - "Hardware and Product Constraints" names the target DIS02170A V1.4 (ESP32-S3-WROOM-1-N16R8)
  - the 16 MB partition layout of plan.md, with the 2.9 MiB image budget kept
  - a Sync Impact Report at the top
  - check `CLAUDE.md` for the same lines (FR-033)
  - done before T004 removes the basic-board env, because the constitution governs until it is amended
- [X] T004 Rewrite `platformio.ini`:
  - **Env `crowpanel7_advance`** (`default_envs`):
    - `platform = espressif32@8.0.0`
    - `platform_packages = platformio/framework-arduinoespressif32@4.30312.261116, platformio/framework-arduinoespressif32-esp32s3-libs@3.3.12`
    - `board = crowpanel7_advance`, `board_build.partitions = partitions/crowpanel7_advance.csv`, `board_build.filesystem = littlefs`
    - `lib_deps = lvgl/lvgl@9.6.0`
    - `lib_ignore = ESP_I2S, ESP_SR`
    - `build_flags = -DLV_CONF_INCLUDE_SIMPLE -Iinclude -DNVP_BOARD_NAME=\"crowpanel7_advance\"`
    - the existing silence and size scripts
  - **Remove** env `crowpanel7_basic` and update the header comment (plan, Structure decision).
  - Delete `boards/crowpanel7_basic.json` and `partitions/crowpanel7_basic.csv`.
- [X] T005 Add env `sim` to `platformio.ini`:
  - `platform = native@1.2.1`, `lib_deps = lvgl/lvgl@9.6.0`
  - `build_src_filter` limited to `sim/diag_render/`, with `lib/ui` and `lib/core` as libraries
  - `-DLV_CONF_INCLUDE_SIMPLE -Iinclude -DNVP_SIM=1`
  - Make sure `pio test -e native` does not compile LVGL (`lib_ignore = nvp_ui, lvgl` in `native`).
- [X] T006 [P] Create `include/lv_conf.h` for LVGL 9.6.0 (research R9):
  - `LV_COLOR_FORMAT_DEFAULT LV_COLOR_FORMAT_RGB565`
  - `LV_MEM_SIZE` 64 kB in bytes
  - `LV_USE_OS LV_OS_NONE`, `LV_DEF_REFR_PERIOD 33`
  - fonts Montserrat 14 and 20 only
  - `LV_USE_LOG 0` on the device
  - under `#if NVP_SIM`: `LV_USE_TEST 1`, `LV_USE_TEST_SCREENSHOT_COMPARE 1`, `LV_USE_LODEPNG 1`
- [X] T007 [P] Create `lib/ui/library.json` (name `nvp_ui`, depends on `nvp_core` and LVGL; LVGL only, no Arduino). In `lib/ui/README.md`, add after the layering rule: "Exception: `src/diag/` holds the engineering screens (boot, diagnostic, settings). They are in English and outside `02-ui-spec.md` (spec 001, Assumptions)."
- [X] T008 [P] Update `tools/check.py`:
  - `FIRMWARE_ENV = "crowpanel7_advance"`
  - add the step `tls check` (`python tools/check_tls.py`) after the silence check
  - add the step `ui snapshots` (`pio run -e sim -t exec`) when not `--skip-firmware`
  - update `PARTITION_BYTES` in `tools/pio_size_budget.py` to reference `partitions/crowpanel7_advance.csv`
- [X] T009 [P] Write `tools/test_check_tls.py` first: it must fail on fixtures containing `setInsecure(`, `.begin("https` and `begin(String("https`, and pass on `begin(client, url)`. Then implement `tools/check_tls.py`, scanning `src/`, `lib/` and `sim/` (SEC-01, research R10).
- [X] T010 [P] Add `tools/size_by_module.py`. It reads `.pio/build/crowpanel7_advance/firmware.map` and prints the bytes contributed by `lib/ui/src/diag`, `lib/core/src/nvp/diag` and `src/app` (the diagnostic mode, which ships with the product). Call it from `tools/pio_size_budget.py` after the total (FR-032).

**Checkpoint**: `python tools/check.py --skip-firmware` passes. `pio run -e crowpanel7_advance` builds the Slice 0 `main.cpp`, and the size and the diagnostic-mode share (zero for now) are reported.

---

## Phase 2: Foundational (blocking prerequisites, Stage 1)

**Purpose**: the pieces every story uses: board constants, self-test types, the report, the serial command parser, the console, the app skeleton and storage.

**⚠️ No user story starts before this phase is complete.**

- [X] T011 Create `src/platform/board.h` with every pin and timing constant from research R2–R5, each with its source comment:
  - RGB pins and timing, `kPclkHz = 16000000`, `pclk_active_neg`
  - I²C 15/16
  - GT911 0x5D and INT GPIO1
  - auxiliary-controller address 0x30
  - I²S-side GPIOs 4/5/6, named `kAmpDataPins` (no banned token)
  - `kModel = "DIS02170A"`, `kRevision = "V1.4"`
- [X] T012 [P] Write `test/test_diag/test_selftest_report.cpp`. Its tests must fail first:
  - the report has the exact keys and order of `contracts/serial-console.md`
  - Skipped and Failed results carry their reasons
  - `board: DIS02170A V1.4 (configured; not detected)`
  - `backlight: full (dimming disabled)`
  - `memory: flash 16 MB, psram 8 MB (detected)` from `flash_bytes` and `psram_bytes`
  - `boot-to-diag: N ms`, or `n/a (no display)` when `boot_to_diag_ms` is 0 (SC-006)
  - the `audio-off` line names no individual part (FR-026, SIL-05)
  - `test_report_never_contains_secret`: feeds an SSID that looks like a password and checks that no `ReportInput` field can carry a password (FR-024, FR-026)
- [X] T013 [P] Write `test/test_diag/test_command_parser.cpp`. Its tests must fail first:
  - every command in `contracts/serial-console.md` parses
  - case and whitespace are tolerated
  - `\r` is ignored
  - `diag` parses
  - unknown input and the absent `bl`/`dim` commands give `Unknown` (FR-028)
- [X] T014 Implement `lib/core/src/nvp/diag/selftest.h` (types from data-model.md) and `lib/core/src/nvp/diag/report.h/.cpp` (`report::format(const ReportInput&)` into a caller buffer, no heap) until T012 passes.
- [X] T015 Implement `lib/core/src/nvp/diag/command.h/.cpp` (`parse_command`) until T013 passes.
- [X] T016 [P] Implement `src/platform/sysinfo.h/.cpp`: internal free and largest block, PSRAM free, detected flash chip size and PSRAM size (report line `memory`, owner answer I2), station MAC, reset reason, uptime.
- [X] T017 [P] Implement `src/platform/storage.h/.cpp`:
  - `LittleFS.begin(true, "/littlefs", 4, "littlefs")`, formatting only that partition on failure
  - a self-test that writes 16 bytes to `/selftest.bin`, reads them back and compares them (data-model SelfTestResult Storage)
  - mounted after the diagnostic screen is shown (plan startup step 7), because a first-boot format can take seconds (SC-006)
- [X] T018 Implement `src/platform/console_io.h/.cpp` (non-blocking line reader on UART0 at 115200 and a line writer; the only serial code) and `src/app/serial_console.h/.cpp` (dispatch of parsed commands to callbacks, `help` text, `unknown command, type help`, and `diag` answering `already in diagnostic mode` in this slice, FR-007a). `src/app` includes no Arduino, ESP-IDF or FreeRTOS header (constitution VI; contracts/serial-console.md).
- [X] T019 Implement `src/app/bringup_app.h/.cpp` skeleton. It holds the self-test results, prints the report at the end of start-up and on `report` (FR-026), and exposes `start()` and `loop()`. Like all of `src/app`, it includes only `src/platform`, `lib/core` and `lib/ui` headers (constitution VI).
- [X] T020 Rewrite `src/main.cpp` with the startup order of plan.md: `audio_off()` is the first statement of `setup()`, then `Serial.begin`, then `app::start()`. The `audio_off` call is a stub until T025 replaces it. The comment cites SIL-02 and MODE-01.

**Checkpoint**: host tests pass. The firmware builds and prints the report with silence `pending`.

---

## Phase 3: User Story 1, Silence (P1) 🎯 MVP, Stage 1

**Goal**: the amplifier is muted and the buzzer held off before anything else. No byte that can make sound can ever be sent (SIL-01…05, FR-001, FR-001a, FR-002).

**Independent test**: flash, power-cycle and reset in a quiet room. The report shows `audio-off: acknowledged first`. The owner answers A1 and A2.

### Tests for User Story 1 (write first, must fail)

- [X] T021 [US1] Write `test/test_audio_off/test_audio_off.cpp`:
  - `aux_byte_allowed` checked for all 256 values against `contracts/aux-controller.md`: only 0, 247, 249 and 250 are allowed
  - 246, 248 and 1…245 are rejected
  - the silence sequence is exactly `{249, 247}`
  - `kSilenceMaxAttempts == 20`, `kSilenceRetryMs == 50`
  - the native env reaches the header through `-Isrc/platform`, with no Arduino include

### Implementation for User Story 1

- [X] T022 [US1] Implement the pure part of `src/platform/audio_off.h`: `constexpr bool aux_byte_allowed(uint8_t)`, the sequence and retry constants, the `SilenceState` struct, and declarations of `audio_off()`, `aux_send(uint8_t)` and `silence_state()`. No Arduino include in the pure section; platform declarations sit behind `#ifdef ARDUINO`. T021 must pass.
- [X] T023 [US1] Implement `src/platform/audio_off.cpp`, following research R3:
  1. Drive GPIO 4, 5 and 6 as outputs, low.
  2. `Wire.begin(15, 16, 100000)`.
  3. Up to 20 attempts, 50 ms apart: write 249, then 247, to 0x30, each checked with `endTransmission() == 0`. Record attempts and acknowledgement.
  4. `aux_send()` refuses any byte the gate rejects and records it in `SilenceState.rejected_count` and `last_rejected`. After `Serial.begin` it logs `aux: rejected byte N` at once; refusals recorded earlier are printed with the silence line (plan startup step 2).
  5. No other file may write to 0x30 (FR-001, FR-002).
- [X] T024 [US1] Make `ran_first` truthful. `audio_off()` sets it only if it runs before any other peripheral init. Debug builds `assert` that no other platform init ran before it (a static counter in `sysinfo`), and the report shows `audio-off: acknowledged first (attempts N)` or `failed (reason)` (FR-026). The line names no individual part: `audio-off` stands for the whole sequence (249, then 247), and the silence check (SIL-05) keeps part names out of `lib/core`.
- [X] T025 [US1] Replace the stub in `src/main.cpp` with the real `audio_off()`. Add a one-line serial print of the silence result right after `Serial.begin` (quickstart §2).
- [X] T026 [US1] Add the FR-001a gate in `src/app/bringup_app.cpp`. While `silence_state().acknowledged` is false, the start functions for stress, Wi-Fi setup and Wi-Fi join return `refused: silence step not acknowledged`. Use one helper, `can_use_radio_and_load()`, that Phase 5 and Phase 9 call.
- [X] T027 [US1] Verify the silence check allow-list in `tools/check_silence.py` still matches `src/platform/audio_off.{h,cpp}` and `test/test_audio_off/test_audio_off.cpp`, and that `lib_ignore` blocks `ESP_I2S` and `ESP_SR`. Add a tool test in `tools/test_check_silence.py` that fails if `platformio.ini` drops either from `lib_ignore`.
- [X] T028 [US1] Fetch the NS4168 datasheet (manufacturer Nsiway), confirm the CTRL-pin truth table, and record it under open item O3 in `specs/001-silent-board-bringup/research.md`. Done before the Stage 1 owner check, because the silence step relies on it. If the datasheet contradicts research R3, stop and ask the owner.
- [X] T029 [US1] **Stage 1 owner check**:
  - Before the first flash: confirm the esptool command names against the installed version, add `backup/` to `.gitignore` if missing, and give the owner quickstart §2 step 0 (chip read and factory firmware backup). Record the flash and PSRAM sizes under I2.
  - **Do not hand over the flash command until I7a and I7b are answered** (I7 is "yes"), because flashing removes the factory firmware they are about. If the sound starts before the screen lights, stop and discuss with the owner: a sound from the auxiliary controller before the ESP32 runs is outside the firmware's control.
  - Build, report the size, and give the owner the flash command and the expected serial output from quickstart §2.
  - Ask the board inspection I1–I8 if they are still unanswered, then A1, A2 and F1.
  - Record the answers and the pasted report in `specs/001-silent-board-bringup/owner-checklist.md`.

**Checkpoint**: the MVP is silent and reports it. Do not continue to Stage 2 until A1 and A2 are "yes", or the problem has been discussed with the owner.

---

## Phase 4: User Story 2, Stable picture (P2), Stage 2

**Goal**: boot screen, then the diagnostic screen with colour bars, border, line field and "800 x 480" (FR-005…FR-008, MODE-01).

**Independent test**: render the snapshots on the host and compare them with `contracts/diag-screens.md`. The owner answers B1–B4.

- [X] T030 [P] [US2] Create `lib/ui/src/diag/view_model.h`: `DiagViewModel` with the self-tests, Wi-Fi line, backlight line, heap, stress counters, touch state and corner results, heartbeat, active screen, and confirmation state (data-model.md). Plain data only.
- [X] T031 [P] [US2] Implement `lib/ui/src/diag/boot_screen.h/.cpp`: the boot screen per `contracts/diag-screens.md`.
- [X] T032 [US2] Implement `lib/ui/src/diag/diag_screen.h/.cpp` per `contracts/diag-screens.md`:
  - the border at x=0, x=799, y=0 and y=479
  - the eight labelled bars in the given order
  - tick marks every 10 px
  - the 1-px alternating line field, drawn with an LVGL draw-event callback, not 800 objects
  - the `800 x 480` label
  - the info panel and the controls area (`Stress`, `Settings`)
  - the orientation arrow
  - the heartbeat
  - an `update(const DiagViewModel&)` that changes only the labels that differ
- [X] T033 [US2] Implement `sim/diag_render/main.cpp`. It creates `lv_test_display_create(800, 480)` and renders fixtures `boot`, `diag_idle`, `diag_failures` and `diag_stress` (`contracts/diag-screens.md`) to `out/snapshots/*.png` (research R12). Add `out/` to `.gitignore`.
- [X] T034 [US2] Review the snapshots yourself against `contracts/diag-screens.md`. Check numerically from the PNGs:
  - the border pixels on all four edges are white
  - the line field alternates exactly
  - the bar colours are correct

  Fix and re-render until all three hold, and note the result in the commit message.
- [X] T035 [US2] Implement `src/platform/display.h/.cpp` (research R7, R8 L0):
  - `esp_lcd_new_rgb_panel` with the pins and timing from `board.h`, PCLK 16 MHz, `pclk_active_neg = 1`, `num_fbs = 1`, `fb_in_psram = 1`, no bounce buffer
  - LVGL display with `lv_display_set_buffers` on one 800×40 RGB565 buffer from `heap_caps_malloc(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)`, PARTIAL mode
  - a flush callback that copies the area into the panel's frame buffer (`esp_lcd_rgb_panel_get_frame_buffer`) and calls `lv_display_flush_ready`
  - `lv_tick_set_cb` on `millis`
  - `display_set_pclk(uint32_t)` wrapping `esp_lcd_rgb_panel_set_pclk`, for ladder L2
  - initialisation on core 1
  - the self-test Display result
- [X] T036 [US2] In `src/platform/board.h`, set the orientation side constant from the vendor board drawing `Datasheet/LI070DE42Y_0812.dwg` or the product photos, with a citation comment. If the vendor material does not settle it, ask the owner which edge the USB-C connector is on and record the answer (contracts/diag-screens.md).
- [X] T037 [US2] Wire the display into `src/app/bringup_app.cpp` in the plan startup order: boot screen right after display init and before touch start-up and storage mount, diagnostic screen as soon as touch start-up ends (≤ 5 s, SC-006), log `[ms] diag screen shown` and keep the time for the report line `boot-to-diag`, `lv_timer_handler()` in `loop()`, the view model refreshed once per second. If display init fails, everything else still runs and the report says so (spec edge case).
- [X] T038 [US2] **Stage 2a owner check**: give the owner B1–B4 (and D1 and A3, since the backlight is on, see Phase 7). Record the answers. Add the first row to the "Stack decision log" in `specs/001-silent-board-bringup/research.md` (L0, idle).

**Checkpoint**: the picture is confirmed idle. Touch and stress build on it.

---

## Phase 5: User Story 3, Stability under load (P3), Stage 3

**Goal**:

- Stress mode runs Wi-Fi scans and scratch writes every cycle (about 10 s), plus an HTTPS request at most every 30 s, while the picture stays put (FR-009…FR-013).
- Counters and timestamped log lines are shown and printed.

**Independent test**:

- Start stress without Wi-Fi details: HTTPS shows `skipped (no Wi-Fi set up)`.
- With Wi-Fi details (US7): HTTPS shows successes. The owner answers B5 and A4.

### Tests for User Story 3 (write first, must fail)

- [ ] T039 [P] [US3] Write `test/test_diag/test_stress.cpp` for `StressSchedule::next(now_ms, connected, ap_up)` and `StressCounters` (data-model.md):
  - scan and write every 10 s
  - HTTPS due only when 30 s or more have passed since the last attempt
  - a skip is counted only when HTTPS was due and the device was not connected
  - nothing runs while the AP is up (`paused`)
  - counters never wrap in 10 minutes

### Implementation for User Story 3

- [ ] T040 [US3] Implement `lib/core/src/nvp/diag/stress.h/.cpp` until T039 passes.
- [ ] T041 [P] [US3] Add `storage_scratch_write()` to `src/platform/storage.cpp`. It rewrites `/scratch.bin` in place with 4096 bytes of a counter pattern, so the file never grows (FR-013).
- [ ] T042 [P] [US3] Implement `src/platform/https_probe.h/.cpp` (research R10):
  - `NetworkClientSecure` with `useBuiltinCACertBundle()`
  - `HTTPClient::begin(client, "https://ezanvakti.imsakiyem.com/api/v2/locations/countries/2")`
  - header `User-Agent: NamazVaktiPanel/<kFirmwareVersion>`
  - 10 s timeout
  - success is HTTP 200 with a body of 4 kB or less
  - the result carries status, bytes, duration and a short reason

  `tools/check_tls.py` must pass.
- [ ] T043 [P] [US3] Add the async scan to `src/platform/wifi_link.h/.cpp`: `scan_start()` and `scan_poll()` using `WiFi.scanNetworks(true)`, `scanComplete()` and `scanDelete()`. Zero networks counts as success.
- [ ] T044 [US3] Implement the stress task in `src/app/bringup_app.cpp`:
  - a worker on core 0, created through `src/platform/tasks.h/.cpp` (new: `start_worker(core, fn)` and a `Lock` type; FreeRTOS stays in `src/platform`, constitution VI), driven by `StressSchedule`
  - log lines `[  ms] stress: …` per contracts/serial-console.md
  - counters copied to the view model under the platform `Lock`
  - start and stop from `stress on|off` and the on-screen button
  - refuses via `can_use_radio_and_load()` (FR-001a)
  - stops when the diagnostic mode is left (FR-007a)
- [ ] T045 [US3] Show the stress status and counters in `lib/ui/src/diag/diag_screen.cpp`. Re-render the `diag_stress` snapshot and review it.
- [ ] T046 [US3] **Stage 3 owner check (stress)**, after US7 is done so that HTTPS runs:
  - The owner runs stress for 10 minutes and answers B5 and A4, pasting the final report.
  - Record a row in the Stack decision log.
  - If B5 is "no", follow research R8: apply L1, then L2, and so on, one change per commit, repeating B4 and B5 each time, and logging every attempt.

**Checkpoint**: the stack decision is backed by evidence.

---

## Phase 6: User Story 4, Touch (P4), Stage 2

**Goal**: a marker under the finger, live coordinates, and four corner targets judged by the device (FR-014, FR-014a, FR-015).

**Independent test**: touch the four corners. All of them turn green, and the report shows `touch corners: … passed`.

### Tests for User Story 4 (write first, must fail)

- [X] T047 [P] [US4] Write `test/test_diag/test_touch_judge.cpp`:
  - target centres (20,20), (779,20), (20,459), (779,459)
  - pass when |dx| ≤ 10 and |dy| ≤ 10, fail at 11
  - assignment only within 60 px of a target
  - the latest touch wins
  - signed dx and dy

### Implementation for User Story 4

- [X] T048 [US4] Implement `lib/core/src/nvp/diag/touch_judge.h/.cpp` until T047 passes.
- [X] T049 [US4] Implement `src/platform/touch_gt911.h/.cpp` (research R5):
  - **Start-up**: up to 10 attempts. Probe 0x30 and 0x5D. When either is missing: `aux_send(250)`, GPIO1 output low for 120 ms, back to input, wait 100 ms.
  - **Self-test**: product ID registers read `911`.
  - **Polling**: `read_point()` reads status 0x814E, takes the first point from 0x8150 when the buffer-ready bit is set, then writes 0 to 0x814E.
  - No interrupt is required. Polling runs at about 30 Hz from the LVGL input device.
- [X] T050 [US4] Register an LVGL pointer input device in `src/platform/display.cpp`, fed by `touch_gt911`; `src/app` receives raw points through a callback. Feed raw points to `touch_judge`. Show the ring marker and coordinates, or `no touch`, in `lib/ui/src/diag/diag_screen.cpp`, and colour the corner targets with `+dx,+dy`. No sound or vibration: there is none to give (SIL-06).
- [X] T051 [US4] Add the corner results to the report (`report.cpp` and `contracts/serial-console.md`, line `touch corners:`). Extend T012 with that line. Render the `diag_touch` snapshot and review it.
- [X] T052 [US4] **Stage 2b owner check**: C1, C2 and A6. Record the answers.

---

## Phase 7: User Story 5, Backlight fully on (P5), Stage 1

**Goal**: full brightness right after the silence step and no way to dim it (FR-016…FR-018, OD1).

**Independent test**: the backlight is on at boot. The report and screen say `backlight: full (dimming disabled)`. No backlight command exists.

- [X] T053 [US5] Implement `src/platform/backlight.h/.cpp`: the single function `backlight_full_on()`, which calls `aux_send(0)` and records the Backlight self-test (acknowledged or failed). Call it in `bringup_app::start()` right after the silence result line (plan startup step 3). No other backlight API.
- [X] T054 [US5] Confirm that T021 covers rejection of 1…245, and that the parser test T013 rejects `bl` and `dim`. Show `Backlight: full (dimming disabled)` on the diagnostic screen (T032 view model) and in the report (T012).
- [X] T055 [US5] Owner check D1 and A3, asked together with Stage 2a (T038). Record the answers.

---

## Phase 8: User Story 6, Evidence (P6), Stages 1–3

**Goal**: one complete report on serial and on screen, printed at boot and on demand, with no secrets (FR-026, FR-027, FR-024).

**Independent test**:

- Restart and compare the report with the board label.
- Unplug the panel cable: the report still prints in full.

- [ ] T056 [US6] Fill every `ReportInput` field from live state in `src/app/bringup_app.cpp`: firmware, board, silence, all self-tests, Wi-Fi line, backlight, corners, heap, PSRAM, stress, uptime. Print it at the end of start-up and on `report`.
- [X] T057 [US6] Show the report lines on the diagnostic screen's info panel, with failed results in the alert colour (FR-027). Re-render the `diag_failures` snapshot and review it.
- [ ] T058 [US6] Make every self-test independent. A throwing or timing-out test is caught and recorded as Failed with a reason, and the next test still runs. Display failure must not stop the serial report (spec edge case).
- [ ] T059 [US6] Owner check F1 (Stage 1), and F2 after Stage 3. Record the pasted reports.

---

## Phase 9: User Story 7, Wi-Fi setup from another phone (P7), Stage 3

**Goal**: a settings screen starts a WPA2 hotspot (after confirmation). A phone page submits the SSID and password. On success they are stored in NVS `wifi/*` and the device rejoins after a restart. "Forget Wi-Fi" asks for confirmation (FR-019…FR-025, FR-019a, NET-01, NET-02, NET-08).

**Independent test**: quickstart §4 step 1. The owner answers E1–E3 and A5.

### Tests for User Story 7 (write first, must fail)

- [ ] T060 [P] [US7] Write `test/test_diag/test_json.cpp`: escapes `"`, `\`, control characters and non-ASCII bytes (passed through as UTF-8), a 32-byte SSID, and an empty string.
- [ ] T061 [P] [US7] Write `test/test_diag/test_backoff.cpp`: 5 s, 10 s, 20 s … capped at 300 s; reset on success (NET-01).
- [ ] T062 [P] [US7] Write `test/test_diag/test_ap_password.cpp`: always 8 decimal digits, including leading zeros, from injected random words.
- [ ] T063 [P] [US7] Write `test/test_diag/test_wifi_states.cpp` for the transition table in data-model.md WifiState:
  - forget erases and goes to Idle
  - cancel returns to the previous state and keeps the details
  - a failed try discards the submitted details and stays in SetupAP
  - with silence not acknowledged, every start is refused
  - credential validation: SSID of 1…32 bytes, password of 0…63 bytes

### Implementation for User Story 7

- [ ] T064 [P] [US7] Implement `lib/core/src/nvp/diag/json.h/.cpp`, `backoff.h/.cpp`, `ap_password.h/.cpp` and the pure transition table in `lib/core/src/nvp/diag/wifi_states.h/.cpp` until T060–T063 pass.
- [ ] T065 [US7] Complete `src/platform/wifi_link.h/.cpp`:
  - STA join from NVS `wifi/ssid` and `wifi/pass` with backoff
  - AP+STA hotspot `NamazVakti-XXXX`, WPA2, at most 2 clients, 192.168.4.1
  - `ap/pass` generated with `esp_random()` on first use (NET-02)
  - a try of up to 20 s; store on success and close the AP after a 5 s grace period; discard on failure (MODE-04, FR-022)
  - forget, cancel
  - the password is never logged (FR-024)
- [ ] T066 [US7] Implement `src/platform/setup_portal.h/.cpp` per `contracts/setup-portal.md`:
  - `WebServer` on port 80, running only while the AP is up
  - `GET /`: inline page under 4 kB with network list, SSID field, password field and status polling
  - `GET /api/scan`: at most 20 entries, deduplicated, JSON-escaped
  - `POST /api/setup`: 202 or 400
  - `GET /api/status`: never a password
  - 404 otherwise
- [ ] T067 [US7] Implement `lib/ui/src/diag/settings_screen.h/.cpp` per `contracts/diag-screens.md`:
  - Wi-Fi state line, `Start Wi-Fi setup` and `Forget Wi-Fi` behind `Yes`/`No` confirmations that close after 15 s (FR-019a)
  - `Cancel setup`, which acts at once
  - the hotspot name, password and address shown while the hotspot is open
  - `Back`
  - the `Settings` button on the diagnostic screen opens it
- [ ] T068 [US7] Render the `settings_none`, `settings_confirm` and `settings_hotspot` snapshots and review them against `contracts/diag-screens.md`.
- [ ] T069 [US7] Wire Wi-Fi into `src/app/bringup_app.cpp`:
  - serial `wifi setup|cancel|forget`
  - join on boot in the background, only if `can_use_radio_and_load()` (FR-001a, FR-023)
  - stress pauses while the AP is up
  - Wi-Fi state shown in the view model and the report
  - while the AP is up, the serial Wi-Fi line reads `hotspot NamazVakti-XXXX at 192.168.4.1`, never the password (FR-024, spec edge case)
- [ ] T070 [US7] Check with a test in `test/test_diag/test_selftest_report.cpp`, extended, and by inspecting the `/api/status` handler code, that no response or log path can contain `wifi/pass` or `ap/pass` (FR-024, NET-08).
- [ ] T071 [US7] **Stage 3 owner check (Wi-Fi)**: E1, E2, E3 and A5. Record the answers. Then run T046.

---

## Phase 10: Polish and cross-cutting

- [ ] T072 [P] Update `docs/specification/05-architecture.md` (FR-033):
  - §1.2 with the verified V1.4 facts and the auxiliary-controller command table (research R2–R6), dated 2026-11-20
  - §1.1 marked as the non-target board
  - §2 risks updated with the backlight decision and silence through the auxiliary controller
  - §3 with the chosen display stack (`esp_lcd` + LVGL 9.6.0) and why
  - §4 noting that the diagnostic mode is permanent product code (`lib/core/src/nvp/diag`, `lib/ui/src/diag`)
  - §5 with the 16 MB partitions
  - §10 with env names `crowpanel7_advance` and `sim`
  - ⚠ items verified so far marked with the date
- [ ] T073 [P] Update the rest of the project documents for the owner decisions (FR-033):
  - `docs/specification/06-roadmap.md`: Slice 1 scope gains Wi-Fi setup and permanent diagnostics; Slice 4 builds on the stored keys; Slice 8 has no brightness or night dimming
  - `docs/specification/03-functional-spec.md`: SET-04 dropped, with a note
  - `docs/specification/01-product-brief.md`: open questions 1 and 4 answered
  - `docs/development/prompts/slice-03-main-screen.md`: the hidden diagnostic gesture per FR-007a, proposal two fingers held for 5 s, not colliding with `02-ui-spec.md` §8
  - `docs/development/prompts/slice-04-wifi-setup.md`: reuse NVS `wifi/*`, `ap/pass` and the portal paths
  - `docs/development/prompts/slice-08-settings-page.md`: no brightness settings
- [ ] T074 [P] Update `README.md` with the development commands (`python tools/check.py`, `pio run -e crowpanel7_advance -t upload`, `pio device monitor -b 115200`, `pio run -e sim -t exec`), the DIP switch recommendation, and the serial command list.
- [ ] T075 Record the final stack decision (FR-031) in `specs/001-silent-board-bringup/research.md` and copy a summary into `docs/decisions/0002-display-stack.md`. Confirm that every row of `specs/001-silent-board-bringup/owner-checklist.md` is answered and dated.
- [ ] T076 Run `python tools/check.py`: all steps pass, and the size and diagnostic-mode size are reported. Confirm by search that `src/app/` and `lib/` include no `Arduino.h`, `esp_*`, `freertos/*` or `driver/*` header (constitution VI). Then run `/speckit-converge`.

---

## Dependencies and execution order

### Phase dependencies

- **Setup (Phase 1)** comes first.
- **Foundational (Phase 2)** depends on Setup and blocks all stories.
- **US1 (Phase 3)** depends on Foundational. It is the MVP and also provides the command gate used by US4, US5 and US7.
- **US5 (Phase 7)** depends on US1 (gate). It is tiny and belongs to Stage 1, so do it right after US1.
- **US6 (Phase 8)**: the report skeleton is in Foundational. T056/T059 grow as other stories land, and F1 is asked in Stage 1.
- **US2 (Phase 4)** depends on Foundational. It starts Stage 2.
- **US4 (Phase 6)** depends on US2 (display and LVGL input) and on US1 (gate, for command 250).
- **US7 (Phase 9)** depends on US2 (settings screen) and US1 (FR-001a gate).
- **US3 (Phase 5)**: T039–T045 depend on US2. The owner check T046 depends on US7 so that HTTPS actually runs.
- **Polish (Phase 10)** comes after all stories. T072–T074 can start earlier. The constitution amendment (T003) and the module-size script (T010) sit in Phase 1, and the NS4168 check (T028) in Phase 3.

### Delivery order by stage (what the owner flashes)

1. **Stage 1**: Phases 1–3, then Phase 7, then T056/T059 (F1). Owner: I1–I8, A1, A2, F1.
2. **Stage 2**: Phase 4, then Phase 6. Owner: B1–B4, D1, A3, C1, C2, A6.
3. **Stage 3**: Phase 9, then Phase 5. Owner: E1–E3, A5, B5, A4, F2.
4. **Close-out**: Phase 10.

### Within each story

- Tests first, and they must fail. Then the pure `lib/core` code, then platform code, then app wiring, then UI and snapshots, then the owner check.

### Parallel opportunities

| Phase | Tasks that can run in parallel |
|---|---|
| Setup | T002, T006, T007, T008, T009, T010, after T003, T001 and T004 |
| Foundational | T012 ∥ T013; T016 ∥ T017 |
| US2 | T030 ∥ T031 |
| US3 | T041 ∥ T042 ∥ T043 (different platform files), after T040 |
| US7 | T060 ∥ T061 ∥ T062 ∥ T063 (tests), then T064 |
| Polish | T072 ∥ T073 ∥ T074 |

## Parallel example: User Story 7

```text
# Tests first, all in parallel (different files):
T060 test/test_diag/test_json.cpp
T061 test/test_diag/test_backoff.cpp
T062 test/test_diag/test_ap_password.cpp
T063 test/test_diag/test_wifi_states.cpp
# Then T064 makes them pass. T065 (wifi_link) and T067 (settings screen) can proceed side by side.
```

## Implementation strategy

### MVP first (User Story 1)

1. Phases 1–2, then Phase 3 (silence and report).
2. **Stop and validate**: the owner flashes and confirms A1, A2 and F1 from the serial report. No display yet.

### Incremental delivery

- Each stage ends with an owner check whose answers are recorded before the next stage starts (constitution VII).
- The picture (US2) is confirmed idle before touch, Wi-Fi or stress are added. Any later instability can then be attributed to load.
- Commits are small and build, with requirement ids in each message (for example `US1: aux command gate rejects 246/248 (SIL-02, FR-001)`).

## Notes

- `[P]` means different files and no dependency on an unfinished task.
- If a vendor fact turns out wrong on the device, correct `research.md` and `05-architecture.md` in the same branch.
- Never weaken a test or a check to make it pass. Never call `setInsecure`. Never print a password.
