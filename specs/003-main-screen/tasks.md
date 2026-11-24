---

description: "Task list for the main screen with sample data (spec 003)"
---

# Tasks: Main screen with sample data

**Input**: Design documents from `specs/003-main-screen/` ([plan.md](plan.md), [spec.md](spec.md), [research.md](research.md), [data-model.md](data-model.md), [contracts/](contracts/), [quickstart.md](quickstart.md))

**Tests**: Required for every `lib/core` addition, written first (constitution VI). The screen itself is verified by the host harness:
- snapshots of fixtures 01–14;
- geometry checks;
- flushed-area accounting;
- the implementer's side-by-side review before any owner check (constitution III).

**Organization**: by user story.
- **Stage 1** of the slice prompt: Setup, Foundational, US1 and US2 (fixture 01).
- **Stage 2**: US3 (fixtures 02–09).
- **Stage 3**: US4, US5, US8 and US7, then the device and the owner checklist.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: can run in parallel (different files, no dependency on an unfinished task)
- **[Story]**: US1–US8 of spec.md
- Every task cites an FR/SC of spec 003, a 02-ui-spec section or a requirement id (constitution X)

## Path conventions

| What | Where |
|---|---|
| Pure logic | `lib/core/src/nvp/{time,screen,engine,text,diag}/` |
| Screen | `lib/ui/src/main/` |
| Board access | `src/platform/` |
| Wiring and modes | `src/app/` |
| Host harness | `sim/ui_render/` |
| New host tests | `test/test_screen/` |
| Fonts | `assets/fonts/` |

---

## Phase 1: Setup

- [X] T001 Rename `sim/diag_render/` to `sim/ui_render/`, and update `[env:sim]` `build_src_filter` in `platformio.ini` and the references in `tools/check.py`, `tools/check_snapshots.py` and `README.md`. Confirm `pio run -e sim -t exec` still writes the Slice 1 snapshots (FR-020).
- [X] T002 [P] Create `test/test_screen/test_main.cpp`: a Unity runner in the style of `test/test_engine/test_main.cpp`, with `run_*_tests()` declarations added as files appear. Confirm `pio test -e native -f test_screen` builds.
- [X] T003 [P] Add `tools/pio_build_time.py`, a pre-script for `env:crowpanel7_advance` only. It appends `-DNVP_BUILD_UTC=<int seconds since 1970 UTC from the host clock>` to the build flags (research R2, TIME-03). *Done: rounded down to 00:00 UTC of the build day, because a define that changes on every build forces a full rebuild.*
- [X] T004 [P] Add `-DNVP_DEV_TOOLS=1` to `env:crowpanel7_advance` build flags, with a comment that the release build of Slice 9 drops it (SYS-06, FR-018).

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: fonts, tokens, strings, the fixed-cell widget, the view types and the fixture table. Every story needs them.

### Fonts (FR-003, research R3)

- [X] T005 Create `assets/fonts/gen_fonts.py`. It:
  - downloads the four TTFs and `OFL.txt` from google/fonts (`ofl/barlow`, `ofl/barlowcondensed`) into `assets/fonts/src/`, only if they are missing;
  - records the google/fonts commit and the date in `assets/fonts/SOURCES.md`;
  - converts the 14 fonts of research R3 into `assets/fonts/out/<name>.c` with `npx --yes lv_font_conv@1.5.3 --no-compress --no-prefilter --bpp 4 --format lvgl --force-fast-kern-format --lv-font-name <name>`. The Turkish set ranges are listed in the script, with U+2019, U+2026 and, for `nvp_txt_500_21`, U+2248.
  - prints the size of each output and the total.

  Run it and commit the outputs. *Done 2026-11-24: google/fonts commit `06bfe0d`; output deterministic; 84.6 kB of glyph bitmaps. The outputs go straight to `lib/ui/src/fonts/` (T006); downloads use certifi's CA bundle, verification on.*
- [X] T006 Make the fonts compile into `lib/ui`: put the generated files in `lib/ui/src/fonts/` with `fonts.h` declaring the 14 `lv_font_t`. Each generated file includes `lvgl.h`; on the device the `LV_CONF_PATH` define from `tools/pio_lv_conf.py` already points LVGL at `include/lv_conf.h`, and in the `sim` env `LV_CONF_INCLUDE_SIMPLE` with `-Iinclude` does the same, so no per-file include change is needed (confirm both builds). Add a `fonts` bucket to `tools/size_by_module.py`, and report it in `tools/check.py`, failing above 300 kB (FR-003, FR-025, SC-008).

### Tokens and strings (02 §2.1, §6, §9)

- [X] T007 [P] Create `lib/ui/src/main/tokens.h`: the colours of 02 §2.1 as `lv_color_hex` constants, plus the layout numbers of 02 §3–§4.5 (X(t), Cx(k), element positions M1–M21) from `docs/design/tokens.json`, each with its source (FR-001, FR-002, FR-005).
- [X] T008 [P] Write `test/test_screen/test_state_text.cpp` asserting the 02 §6 status strings byte for byte, including U+2019 where used:
  - `Saat alınamadı, Wi-Fi bekleniyor`, `Vakitler alınamadı`, `Yaklaşık vakitler, hesaplama ile`;
  - `Kayıtlı vakitler N gün sonra bitiyor` and `Kayıtlı vakitler bugün bitiyor`;
  - `Çevrimdışı, kayıtlı vakitler kullanılıyor` and `Son güncelleme `;
  - the no-time and no-data hero texts `Saat eşitleniyor`, `Saat bekleniyor`, `Wi-Fi bağlantısı bekleniyor`, `Wi-Fi ve konum ayarını kontrol edin`, and `Kerahat vakti`.

  Then add them to `lib/core/src/nvp/text/tr.h` (FR-011).

### Pure screen logic (tests first; FR-012, FR-017, research R7, R10)

- [X] T009 [P] Write `test/test_screen/test_system_state.cpp`. It checks each row of 02 §6 in order, with first match winning; a forced state overriding; `cache_ending` only when stored days ≤ 7; and `offline` only after more than 10 minutes. Then implement `lib/core/src/nvp/screen/system_state.h/.cpp` (`SystemState`, `StateInputs`, `select_state`).
- [X] T010 [P] Write `test/test_screen/test_no_time.cpp` for `engine::evaluate_no_time(const DayRecord*, const Settings&, MainView&)`:
  - with a record: columns show its times as `Later` with empty notes; the strip times and kerahat are filled; there is no period or countdown text;
  - without a record: columns are empty;
  - `has_data` is false either way.

  Then implement it in `lib/core/src/nvp/engine/engine.h/.cpp` (research R7, 02 §6 row 1).
- [X] T011 [P] Write `test/test_screen/test_fixtures.cpp`. It checks: 14 rows numbered 1–14; reference names equal the files in `docs/design/screens/` 01–14; the times and states of research R10; fixture 11 has 5 days; fixture 13 has no clock. Then implement `lib/core/src/nvp/screen/fixtures.h/.cpp` (FR-017).

### Screen building blocks

- [X] T012 Implement `lib/ui/src/main/mono_text.h/.cpp`: a widget drawing each character centred in a fixed cell (digit cell `round(0.475 × px)`, colon `round(0.252 × px)`, others the digit cell), with set-text and colour, and an invalidate-only-on-change behaviour (FR-004, research R4).
- [X] T013 Create `lib/ui/src/main/main_view.h` (`ui::SystemView`) and `lib/ui/src/main/main_screen.h` (`main_screen_create`, `main_screen_update`) per [contracts/screen-api.md](contracts/screen-api.md).
- [X] T014 Extend `sim/ui_render/main.cpp` to render `fixture N` (all 14) through `main_screen_create` and `main_screen_update`. It writes `out/snapshots/<reference name>.png` from the fixture table and runs the engine (or `evaluate_no_time`) for each fixture's moment and state (FR-020).

**Checkpoint**: fonts generated and within budget; the state, no-time and fixture tests pass; the harness writes 14 (still mostly empty) images.

---

## Phase 3: User Story 1 - Glance from across the room (Priority: P1) 🎯 MVP

**Goal**: the hero (clock, date, Hijri, period name and bounds, progress, countdown label and countdown) and the status row for fixture 01 (02 §4.1 M1–M15).

**Independent Test**: the fixture 01 snapshot matches `01-main-ogle-1432.png` in the hero and status row within 02 §12.

- [X] T015 [US1] Implement the hero and status row in `lib/ui/src/main/main_screen.cpp`:
  - **Status row**: M1 pin icon and M2 city. The city is truncated with `…` above 300 px (FR-011). M3 status text and M4 icon are right-aligned at 776.
  - **Hero**: M5/M6 clock and seconds (`mono_text` 96 and 46); M7 date; M8 Hijri; M9 kerahat legend.
  - **Period**: M10 period name; M11–M13 bounds (`mono_text` 25) and the progress track and fill; M14 label; M15 countdown (`mono_text` 114, `now`; `alert` when soon).

  Baselines and anchors come from 02 §4.1 (FR-001, FR-010).
- [X] T016 [P] [US1] Implement `lib/ui/src/main/icons.h/.cpp`: pin, Wi-Fi, Wi-Fi-off and approx in a 16 × 16 box from LVGL lines and arcs, stroke 1.6 px, round caps, in a given colour (research R6, M1, M4).
- [X] T017 [US1] Render fixture 01. Open `out/snapshots/01-main-ogle-1432.png` and `docs/design/screens/01-main-ogle-1432.png` side by side, compare the hero and status row, and fix differences until within 02 §12. Record the remaining differences in `specs/003-main-screen/research.md` (R15 review log) (SC-001).

---

## Phase 4: User Story 2 - The whole day at a glance (Priority: P1)

**Goal**: hour scale, strip with separators and hatch, marker, leader lines and the six columns (02 §3, §4.2–§4.5).

**Independent Test**: measured geometry within ±1 px of the formulas, and fixture 01 matches its reference in the lower half.

- [X] T018 [P] [US2] Write `tools/measure_snapshot.py` (standard library PNG decoding, as in `tools/check_snapshots.py`). It reports JSON with the strip bounding box (`yatsi` and segment colours at y 264–294), the current column box (`now` fill at y 336–464), and the marker (`now` 3 px line at y 259–299). Write `tools/test_measure_snapshot.py` on synthetic PNGs (FR-021).
- [X] T019 [US2] Implement `lib/ui/src/main/timeline.h/.cpp` with draw events, per research R5:
  - the scale (M16, M17);
  - the strip: 7 segments with the outer ends rounded at radius 7, 6 separators of 2 px `bg`, and the hatch at 45°, 3 px stripes, 6 px period, clipped to each kerahat interval;
  - the leader polylines (M20; current in `now` 2.5 px, others `line` 1.5 px);
  - the marker as a separate 14 × 48 object (gap 9 px `bg`, 3 px `now` line, triangle with a 2 px `bg` outline).

  It covers FR-005 to FR-008.
- [X] T020 [US2] Implement `lib/ui/src/main/columns.h/.cpp`: six boxes of 125.33 × 128 at y 336. Each has the name (500 19), the time (`mono_text` 48), and the note (400 15; the current note uses Regular 15, an allowed consolidation). States per 02 §4.5: `current` fill `now` radius 12 with `bg` text; `past` `dim`; `next` note `now`; `later` note `muted` (FR-009).
- [X] T021 [US2] Extend `tools/check_snapshots.py` with geometry checks for every fixture that shows the element. The strip box must be 24..776 × 264..294; the marker x must equal X(now) ± 1; the current column box must match Cx(k) ± 1. It fails on a missing snapshot (FR-021, SC-002).
- [X] T022 [US2] Render fixture 01 and compare the lower half with the reference as in T017; fix until within tolerance and `check_snapshots.py` passes. Log the result in research R15 (SC-001, SC-002).

**Checkpoint (Stage 1 of the slice prompt)**: fixture 01 matches its reference; report both image paths and the remaining differences to the owner.

---

## Phase 5: User Story 3 - The screen follows time (Priority: P1)

**Goal**: correct output at every moment, with bounded redraws (FR-015, FR-016, 02 §5).

**Independent Test**: fixtures 02–09 match their references, and redraw accounting stays within its bound.

- [X] T023 [US3] Implement change-driven updates in `lib/ui/src/main/main_screen.cpp`. Keep the last drawn `MainView`/`SystemView`, and per call update only changed objects:
  - a new second: clock, seconds, countdown, progress fill;
  - a new minute: marker and notes;
  - a new period: column states, leaders, hero texts, bounds;
  - a new day or settings: everything.

  This covers UI-03 and FR-015.
- [X] T024 [US3] Add flushed-area accounting to `sim/ui_render` (count pixels in the flush callback per update). Add a harness mode that steps fixture 01 by one second, one minute, across a vakit and across midnight, and asserts:
  - a one-second step flushes < 8 % of 384 000 px;
  - the midnight step flushes the whole changed area;
  - a backward jump of one hour and a forward jump of several hours redraw everything for the new moment;
  - the image after each step equals a full redraw of the same moment (no leftovers, FR-016).

  Run it from `tools/check.py` (research R8).
- [X] T025 [US3] Render fixtures 02–09; compare each with its reference; fix; log in research R15 (SC-001).

**Checkpoint (Stage 2)**: fixtures 01–09 within tolerance.

---

## Phase 6: User Story 8 - Real time from the board's clock (Priority: P1)

**Goal**: start on the real time from the BM8563 (OD3-1, FR-026 to FR-030).

**Independent Test**: codec tests on the host; `time get`/`time set` on the board; the clock survives a power cut.

- [X] T026 [P] [US8] Write `test/test_screen/test_rtc_codec.cpp` (research R1, data-model). Cases:
  - decode a valid raw image of 2026-11-24 09:30:05 UTC;
  - VL set → `IntegrityLost`;
  - C set → `Century`;
  - BCD digit > 9, month 13, day 32 and Feb 29 in 2026 → `BadField`;
  - time before the build → `BeforeBuild`;
  - encode/decode round trip for every day from 2000 to 2099 at noon;
  - encode clears VL and C and sets the weekday from the date.

  Then implement `lib/core/src/nvp/time/rtc_codec.h/.cpp`.
- [X] T027 [P] [US8] Write `test/test_screen/test_sim_clock.cpp`: speed 1 and 60; a speed change rebases with no jump; pause and run; millisecond counter wrap. Then implement `lib/core/src/nvp/time/sim_clock.h/.cpp` and `lib/core/src/nvp/time/zone.h` (`kDefaultUtcOffsetMinutes = 180`, UTC ↔ local) (research R2, R11).
- [X] T028 [US8] Implement `src/platform/rtc_bm8563.h/.cpp`:
  - `read(uint8_t raw[7]) → bool`: one `write_read` of register 0x02 for 7 bytes at 0x51 on `platform::i2c`;
  - `write(const uint8_t raw[7]) → bool`: one write of 0x02 plus 7 bytes.

  Cite the schematic and the data sheet in comments. Silence first: it is called only after `audio_off()` (research R1).
- [X] T029 [US8] Implement `src/app/clock.h/.cpp`:
  - **at start**: read and decode the RTC (build time from `NVP_BUILD_UTC`), and log `rtc: … valid` or `rtc: not valid (<reason>)`;
  - **sources**: `Rtc` (base + `esp_timer`), `Simulated` (SimClock);
  - **output**: `now() → {valid, LocalSeconds}`;
  - **setter**: `set_rtc_local(...)`, which encodes and writes the chip.

  It covers FR-026 to FR-030.
- [X] T030 [US8] Wire the clock into `src/app` so that after start the main screen shows `no_data` for `clock::now()` (reference 14, no records loaded; Hijri empty), or `no_time` with no stored times when the clock is not valid (OD3-2 as revised, FR-019, constitution II). Sample records are used only by the fixture command. Add the report lines `clock:` and `screen:` (contracts/serial-console.md). Depends on T031 (the state rendering).

---

## Phase 7: User Story 4 - Honest states (Priority: P2)

**Goal**: the six states and fixtures 10–14 (FR-012, FR-013).

**Independent Test**: fixtures 10–14 match references 10–14.

- [X] T031 [US4] Implement the state variants in `lib/ui/src/main/main_screen.cpp` per 02 §6 and the references (FR-012, FR-013):
  - **`no_time`**: `--:--` clock, no seconds, `Saat bekleniyor`, empty Hijri, `Saat eşitleniyor`, bounds hidden, label `Wi-Fi bağlantısı bekleniyor`, `--:--:--` in `dim`, no marker; stored times as `later` without notes.
  - **`no_data`**: clock and date normal; the Hijri date from the newest stored record if any (fixture 14: the sample day), else empty; one `yatsi` strip without separators, leaders or hatch; legend hidden; columns `--:--` in `dim`.
  - **`calculated`**: the `≈` prefix (500 21, raised 8 px, 3 px gap) and 44 px times.
  - **`cache_ending`**, **`offline`**, **`ok`**: status text, icon and colour.
- [X] T032 [US4] Render fixtures 10–14; compare with references; fix; log in research R15 (SC-001).

**Checkpoint**: fixtures 01–14 within tolerance and reviewed.

---

## Phase 8: User Story 5 - Developer control from the serial port (Priority: P2)

**Goal**: the commands of [contracts/serial-console.md](contracts/serial-console.md) (FR-018, FR-029, FR-030).

**Independent Test**: parser tests on the host; each command on the board.

- [X] T033 [P] [US5] Write `test/test_diag/` parser tests for the new commands and their arguments:
  - `diag`, `main`;
  - `time get`, `time set YYYY-MM-DD HH:MM:SS`;
  - `clock rtc|sim …|speed N|pause|run`;
  - `state <name> [N]`, `state auto`;
  - `fixture N|off`;
  - invalid arguments: speed 0, speed 3601, fixture 15, month 13, a malformed time.

  Then extend `lib/core/src/nvp/diag/command.h/.cpp` with `parse_command_line()` returning the command and parsed arguments (research R12).
- [X] T034 [US5] Implement the handlers in `src/app/serial_console.cpp` and the app. Replies are exactly as the contract states, the help text is updated, and everything sits behind `#if NVP_DEV_TOOLS` except `diag`/`main` (SYS-06).

---

## Phase 9: User Story 6 - Self-checking without the device (Priority: P2)

**Goal**: comparison images and the full harness in `check.py` (FR-020, FR-021).

- [X] T035 [P] [US6] Write `tools/compare_reference.py`. It writes `out/compare/<name>.png` (reference \| snapshot \| difference) and prints a summary per fixture: the share of differing pixels and the largest difference box. Write `tools/test_compare_reference.py` on synthetic images.
- [X] T036 [US6] Run all fixture renders, geometry checks and redraw accounting from `tools/check.py`, and document the commands in `quickstart.md` and the README (SC-001, SC-002).

---

## Phase 10: User Story 7 - Diagnostics become hidden (Priority: P3)

**Goal**: start on the main screen; reach diagnostics by the gesture or `diag`; leave by a button or `main` (FR-022, FR-023).

- [X] T037 [P] [US7] Write `test/test_screen/test_gesture.cpp` (research R13). Cases:
  - two points for 4999 ms → no fire; for 5000 ms → fire once;
  - a gap of one report with 1 point → reset;
  - one finger for 10 s → no fire;
  - no second fire without a release.

  Then implement `lib/core/src/nvp/screen/gesture.h/.cpp`.
- [X] T038 [US7] Extend `src/platform/touch_gt911.h/.cpp` to report the number of points (0x814E bits 3–0) with point 1, keeping the hand-back write of 0x814E (spec 001 research R5).
- [X] T039 [US7] Implement the app modes in `src/app/bringup_app.cpp`:
  - start in `Main`;
  - feed the gesture from the touch driver while in `Main`; on fire, or on `diag`, switch to the diagnostic screen;
  - add a `Main screen` button to `lib/ui/src/diag/diag_screen.cpp` (engineering English);
  - the button or `main` returns to `Main` and stops stress mode;
  - a single tap on the main screen does nothing.

  Update the Slice 1 snapshots of the diagnostic screen.

---

## Phase 11: Polish, device and owner checks

- [X] T040 Run `python tools/check.py`: everything passes; image size, font total (≤ 300 kB) and diagnostic-mode size are reported (FR-025, SC-008). Confirm by search that `lib/ui/src/main` includes only LVGL and `lib/core` headers, and nothing for storage, Wi-Fi, files or the network (constitution VI, FR-014).
- [X] T041 Flash the board and check the boot log. Then, from the serial port: `time get`; `time set <now>`; `fixture 1` … `fixture 14`; `clock sim` with `clock speed 3600` across midnight; the restart; `diag`/`main`. Record the log evidence in research R15.
- [X] T042 Write `specs/003-main-screen/owner-checklist.md` with numbered yes/no questions:
  - **picture**: fixture 01 matches its reference; readable from 3 m; strip colours distinguishable from the side and from below; fixtures 02–09 match; fixtures 10–14 match;
  - **fast-forward day**: no flicker, leftovers or moving digits;
  - **clock**: the time on screen is right after `time set`, and after a power cut of several minutes;
  - **restart**: shows the main screen in the `no_data` state with the right time and date (reference 14);
  - **hidden mode**: the two-finger 5 s hold opens diagnostics; a tap does not; `Main screen` returns;
  - **silence**: no sound.

  Ask the owner, and record the answers (constitution VII, SC-003 to SC-007, SC-009, SC-010).
- [X] T043 [P] Update the docs:
  - `docs/specification/06-roadmap.md`: TIME-08's read part moves to Slice 3 (OD3-1);
  - `docs/specification/03-functional-spec.md`: a note on TIME-08;
  - `docs/specification/05-architecture.md`: §1.2, RTC verified (BM8563 at 0x51); §4, the new folders;
  - `README.md`: status and commands.
- [X] T044 Run `/speckit-converge` and resolve any appended task.

---

## Dependencies & Execution Order

- **Setup → Foundational**: Foundational blocks every story.
- **US1 → US2 → US3**: they share `main_screen.cpp`; one after another.
- **US8**: its codec, sim clock and driver tasks (T026–T029) can start after Foundational. T030 needs US1–US3 and T031 (US4).
- **US4**: after US2; it shares `main_screen.cpp`.
- **US5**: the parser (T033) any time after Setup; the handlers (T034) after US8.
- **US6**: T035 any time; T036 after US4.
- **US7**: T037 and T038 any time; T039 after US5.
- **Polish**: last.

### Parallel opportunities

- **Setup**: T002, T003, T004.
- **Foundational**: T007, T008, T009, T010, T011 (different files).
- **Tools and core tests**: T016, T018, T026, T027, T033, T035 and T037 can be written while the screen work goes on.

## Parallel Example: Foundational

```text
Task: "tokens.h (T007)"
Task: "status strings test + tr.h (T008)"
Task: "system_state test + code (T009)"
Task: "evaluate_no_time test + code (T010)"
Task: "fixtures test + code (T011)"
```

## Implementation Strategy

| Stage | Tasks | Result |
|---|---|---|
| 1 (MVP) | Setup, Foundational, US1, US2 | fixture 01 within tolerance, shown to the owner as image paths and a difference list |
| 2 | US3 | fixtures 02–09, bounded redraws |
| 3 | US8, US4, US5, US6, US7 | then the device, the owner checklist and converge |

## Notes

- Each UI task ends with a side-by-side review before it is ticked (constitution III).
- Commit after each phase. Commit messages cite FR ids and requirement ids.

---

## Phase 12: Convergence

- [X] T045 Render a long city name (wider than 300 px in Barlow 600 18) in `sim/ui_render` and assert the label ends with `…` (U+2026) and is at most 300 px wide per FR-011 (partial)
- [X] T046 Render the main screen at 00:00:00 and 23:59:59 of the sample day in `sim/ui_render` and assert the marker is drawn and lies fully inside the 800-px canvas per spec 003 edge case "Marker at the edges" (partial)
- [X] T047 On the device, send `diag`, `stress on`, `main` and record from the log that stress mode stopped on leaving the diagnostic mode per FR-023 (partial)
