# Implementation Plan: Main screen with sample data

**Branch**: `feat/003-main-screen` | **Date**: 2026-11-24 | **Spec**: [spec.md](spec.md)

**Input**: Feature specification from `specs/003-main-screen/spec.md`

## Summary

This slice draws the approved main screen on the panel from the Slice 2 engine's output and a small system-state view.

- **Fonts**: Barlow and Barlow Condensed, converted with `lv_font_conv` 1.5.3 using LVGL 9.6's own options.
- **Numbers**: a fixed-cell number widget, so ticking digits never move.
- **Timeline**: strip, hatch, leaders and marker drawn in draw events, with no canvas.
- **Redraws**: the screen compares each new state with the last one it drew and redraws only what changed.
- **States**: the six system states, with fourteen fixtures in one shared table.
- **Self-checking**: the host harness renders every fixture to PNG, and tools measure the geometry and build side-by-side comparisons with the references.
- **Clock**: the device reads real time from the board's BM8563 clock (owner decision OD3-1; verified against the V1.4 schematic and the PCF8563 data sheet). With no prayer times loaded it starts in the truthful `no_data` state, reference 14 (OD3-2 as revised after analysis finding K1); the sample day appears only on the fixture command.
- **Developer control**: serial commands drive a simulated clock, forced states and fixtures.
- **Diagnostics**: the Slice 1 screens become a hidden mode, reached by a two-finger 5-second hold or `diag`.

## Technical Context

**Language/Version**: C++17 (gnu++17) on ESP-IDF 5.5.5 (`espressif32@8.0.0`, decision 0002) and the native host; Python 3 for tools; Node 24 (npx) for font conversion only.

**Primary Dependencies**:
- LVGL 9.6.0 (pinned).
- `lv_font_conv@1.5.3` (npm, pinned, development machine only).
- Barlow and Barlow Condensed TTFs from google/fonts (OFL 1.1), pinned by commit in `assets/fonts/SOURCES.md`.
- No new firmware library.

**Storage**: none new. Fonts are in the image. The RTC is the only new persistent state, held by the clock chip itself.

**Testing**:
- **Unity on native**: RTC codec, sim clock, state choice, gesture, fixtures, `evaluate_no_time`, command parser additions. All are written first, as constitution VI requires.
- **Host harness**: snapshots with geometry checks, and flushed-area accounting for the redraw bound.
- **Owner checklist** on the panel.

**Target Platform**: ESP32-S3 CrowPanel Advance 7.0" V1.4 (800×480 RGB, GT911, BM8563); the development machine for the harness.

**Project Type**: embedded firmware with host-testable libraries.

**Performance Goals**:
- A normal one-second update redraws under 8 % of the screen.
- A period change redraws only the changed elements, without visible tearing.
- The UI task is idle most of each second; a development build logs its longest frame time per minute.

**Constraints**:
- Exact colours and strings.
- ±1 px geometry and ±2 px text (02 §12).
- Fonts ≤ 300 kB, image ≤ 2.9 MiB.
- No bitmaps except the future QR code.
- No network or flash work on the UI thread.
- The device stays silent.
- No flash write during a Wi-Fi scan.

**Scale/Scope**: one screen with fourteen fixtures; about 2 000 lines of UI and core code, plus fonts and tools.

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-checked after Phase 1 design.*

| Principle | Status | How this plan complies |
|---|---|---|
| I. Silence | Pass | No audio. State changes are visual only. The RTC's interrupt pin is not connected and nothing drives a pin with a tone. The image silence check still runs every build. |
| II. Truthful display | Pass | The clock is shown only when the board clock's integrity mark is clear, its fields are in range and the time is later than the build; otherwise `no_time`. With no prayer times loaded the device shows `no_data` (reference 14). Sample times appear only when a fixture is called up deliberately over the serial port (development builds). The earlier boot choice that showed sample times as today's was dropped after analysis finding K1. |
| III. Design contract | Pass | References 01–14 are reproduced and reviewed side by side, using only the font consolidations 02 §2.2 allows. Strings come from one table. The engineering `Main screen` button on the diagnostic screen is outside 02-ui-spec (spec 001 assumption). |
| IV. Offline first | Pass | The screen draws from local state only. The clock comes from the board's battery-backed RTC, not the network. |
| V. Verify, do not assume | Pass | The RTC part, pins and battery come from the V1.4 schematic; registers from the NXP data sheet Rev. 11.1; GT911 multi-touch registers from the vendor data sheet; the font tool version from npm, with LVGL 9.6's own font headers as evidence; font sources pinned by commit. |
| VI. Testable core | Pass | The RTC codec, clock, state choice, gesture, fixtures and no-time evaluation live in `lib/core`, tests first. `lib/ui` uses only LVGL and `lib/core`. The I²C, touch and serial code stays in `src/platform`. |
| VII. Owner in the loop | Pass | An owner checklist covers fidelity, legibility at 3 m, colours off-axis, the fast-forward day, the RTC time, the restart, the gesture and silence. |
| VIII. Small, private, polite | Pass | No network use is added. |
| IX. Resource discipline | Pass | Fonts are budgeted and reported. No heap in the per-second path. The RTC is written only on an explicit command. |
| X. Simplicity and scope | Pass | Draw events instead of a canvas; no new firmware library; overlay and setup screens stay out of scope. |

## Project Structure

### Documentation (this feature)

```text
specs/003-main-screen/
├── spec.md, plan.md, research.md (R1–R14), data-model.md, quickstart.md
├── contracts/serial-console.md   # mode, clock, state and fixture commands
├── contracts/screen-api.md       # screen functions, fonts, harness, geometry checks
├── checklists/requirements.md
└── tasks.md                      # next: /speckit-tasks
```

### Source Code (repository root)

```text
assets/fonts/
  src/ (TTFs, OFL.txt), SOURCES.md, gen_fonts.py → lib/ui/src/fonts/*.c   # R3
lib/core/src/nvp/
  time/  rtc_codec.h/.cpp, sim_clock.h/.cpp, zone.h          # R1, R2, R11
  screen/ system_state.h/.cpp, fixtures.h/.cpp, gesture.h/.cpp  # R7, R10, R13
  engine/engine.h/.cpp        # + evaluate_no_time()           # R7
  text/tr.h                   # + status strings of 02 §6
  diag/command.h/.cpp         # + mode, clock, state, fixture commands  # R12
lib/ui/src/
  main/ main_screen.h/.cpp, main_view.h, mono_text.h/.cpp,
        timeline.h/.cpp, columns.h/.cpp, icons.h/.cpp, fonts.h   # R4–R8
  diag/diag_screen.cpp        # + "Main screen" button          # R13
src/platform/
  rtc_bm8563.h/.cpp           # one 7-byte read / write on platform::i2c  # R1
  touch_gt911.h/.cpp          # + number of points              # R13
src/app/
  clock.h/.cpp                # RTC or simulated source         # R11
  bringup_app.cpp → app modes Main / Diagnostic / Settings, gesture, commands
sim/ui_render/main.cpp        # renamed from sim/diag_render; + fixtures 01–14  # R9
tools/
  pio_build_time.py, measure_snapshot.py, compare_reference.py,
  check_snapshots.py (+ geometry), size_by_module.py (+ fonts bucket)
test/test_screen/             # rtc codec, sim clock, state, gesture, fixtures, no-time, commands
```

**Structure Decision**: follow `05-architecture.md` §4 and §7.
- **Pure logic** (time, state, fixtures, gesture) goes into `lib/core`.
- **Drawing** goes into `lib/ui`, used unchanged by the device and the host harness.
- **Board access** (the RTC, touch point count) goes into `src/platform`.
- **Wiring and modes** go into `src/app`.

The Slice 1 harness is extended, not duplicated.

## Phase 0: Research

Complete: [research.md](research.md) R1–R14. No open questions remain. The owner decided the boot screen (OD3-1, OD3-2), and the RTC facts were verified before planning.

## Phase 1: Design

Complete: [data-model.md](data-model.md), [contracts/serial-console.md](contracts/serial-console.md), [contracts/screen-api.md](contracts/screen-api.md), [quickstart.md](quickstart.md).

Post-design constitution check: all Pass. After finding K1, OD3-2 was revised to the truthful `no_data` start, so no exception remains.

## Complexity Tracking

None. (An exception to Principle II was proposed and withdrawn after analysis finding K1; see OD3-2 in the spec.)
