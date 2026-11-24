# Implementation Plan: Prayer-time engine

**Branch**: `feat/002-prayer-engine` | **Date**: 2026-11-24 | **Spec**: [spec.md](spec.md)

**Input**: Feature specification from `specs/002-prayer-engine/spec.md`

## Summary

A pure C++17 engine in `lib/core` that, given the local moment and the day records for yesterday, today and tomorrow, returns everything the main screen shows: the running period with its bounds, countdown and progress, the six columns with states and notes, the kerahat intervals, and the exact Turkish strings. Times are handled as one 64-bit local second count derived from a day number (Hinnant's civil-date algorithms), so midnight and year end are ordinary arithmetic. Tests are written first, cover every boundary of ENG-07 plus a second-by-second sweep of a whole day, prove there is no heap use, and compare every string with `02-ui-spec.md` §9. A host program prints the engine's output for fifteen moments, which matches an independently computed table and the reference renders.

## Technical Context

**Language/Version**: C++17 (gnu++17), the same sources for the host (`native@1.2.1`) and the device (ESP-IDF 5.5.5 via `espressif32@8.0.0`, decision 0002).

**Primary Dependencies**: none. Standard library headers only (`<cstdint>`, `<cstddef>`, `<cstring>`). No `snprintf` in the engine (research R5).

**Storage**: N/A. The engine receives records; storing them is a later slice. `DayRecord` mirrors the 20-byte storage layout of `04-data-sources.md` §9 (six u16 minutes, Hijri, source id).

**Testing**: Unity on `native` (`pio test -e native`), new suite `test/test_engine/`. Tests first (constitution VI). A counting global `operator new` proves no allocation (research R7). The table program is checked against `contracts/engine-table.md`.

**Target Platform**: development machine (Windows host GCC/MinGW) and the ESP32-S3 firmware; `lib/core` is already part of the firmware build.

**Project Type**: embedded firmware with host-testable libraries (`05-architecture.md` §4).

**Performance Goals**: one `evaluate` per second on the device, well under a millisecond; a full-day sweep (86 400 calls) in the host test runs in seconds.

**Constraints**: no heap, no global mutable state, no clock or time-zone access (FR-019, FR-023); UTF-8 sources with U+2019 kept exact (constitution III); no SDK headers in `lib/core` (constitution VI).

**Scale/Scope**: about 600–900 lines of engine and text code, about as much test code; one new PlatformIO environment.

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-checked after Phase 1 design.*

| Principle | Status | How this plan complies |
|---|---|---|
| I. Silence | Pass | No hardware code. No text suggests sound; state changes are visual only (02 §5). |
| II. Truthful display | Pass | `approx_bounds` is set whenever a neighbour day is substituted (ENG-04); "no data" is explicit (ENG-08); the Hijri line stays empty rather than computed (spec Assumptions). |
| III. Design contract | Pass | All strings in one table, compared byte for byte with 02 §9, U+2019 asserted as bytes. The output table is checked against the reference renders `01`–`09`. |
| IV. Offline first | Pass | Pure function of local inputs; no network or storage. |
| V. Verify, do not assume | Pass | No libraries. The date algorithm is cited (Hinnant) and verified by tests against known weekdays and leap years. Expected values were computed by an independent model and checked against two reference renders. |
| VI. Testable core | Pass | `lib/core`, standard C++ only, tests written first, built for host and device from the same sources. |
| VII. Owner in the loop | Pass | No hardware question. The owner's part is to accept the printed table; no checklist is needed. |
| VIII. Small, private, polite | Pass | Nothing leaves the device; no secrets involved. |
| IX. Resource discipline | Pass | No heap (tested); per-call work is constant; the image size is reported by `check.py` as usual. |
| X. Simplicity and scope | Pass | One function, one result struct, no new dependency. Every task will cite ENG-01…ENG-08 or a spec section. |

No violations; Complexity Tracking is empty.

## Project Structure

### Documentation (this feature)

```text
specs/002-prayer-engine/
├── spec.md
├── plan.md               # this file
├── research.md           # R1–R12 decisions
├── data-model.md         # types and buffer sizes
├── quickstart.md         # how to validate
├── contracts/
│   ├── engine-api.md     # headers, evaluate() rules, text helpers
│   └── engine-table.md   # expected printed table (15 moments)
├── checklists/requirements.md
└── tasks.md              # next: /speckit-tasks
```

### Source Code (repository root)

```text
lib/core/src/nvp/
  engine/
    calendar.h / calendar.cpp    # Date, day numbers (Hinnant), weekday, LocalSeconds
    day_record.h / day_record.cpp# DayRecord, HijriDate, SourceId + rank, validation (ENG-01)
    engine.h / engine.cpp        # Settings, Input, MainView, evaluate() (ENG-02..ENG-08)
    fixture.h                    # shared fixture of 03 §3 (constexpr)
    table.h / table.cpp          # one Markdown row per moment (FR-022), caller's buffer
  text/
    tr.h                         # every Turkish string (02 §9)
    format.h / format.cpp        # HH:MM, HH:MM:SS, duration notes, dates, Hijri dates
test/test_engine/
  test_main.cpp                  # runner, counting operator new
  test_calendar.cpp              # day numbers, weekdays, leap years, month/year ends
  test_text.cpp                  # every string byte for byte, durations, dates
  test_records.cpp               # validation, wrong neighbour dates
  test_period.cpp                # US1, ENG-02/03/05, soon, entered
  test_columns.cpp               # US2, 02 §4.5 special cases
  test_days.cpp                  # US3, midnight, missing records, ±2 min, no data
  test_view_text.cpp             # US4, clock, dates and strings in MainView
  test_kerahat.cpp               # US5
  test_sweep.cpp                 # SC-003 invariants over 86 400 s, SC-007 no allocation
  test_table.cpp                 # program output == contracts/engine-table.md
sim/engine_table/main.cpp        # prints the table rows from table.h (FR-022)
platformio.ini                   # new env engine_table (native, lib/core only)
tools/check.py                   # runs the table program
```

**Structure Decision**: follow `05-architecture.md` §4. The engine and its text live in `lib/core` beside the Slice 1 diagnostics; the table program lives in `sim/` beside the UI harness, built by its own native environment so it needs no LVGL. The table rows are produced by `nvp/engine/table.h` in `lib/core`, so `test_table.cpp` compares them with the contract without running a process; the program in `sim/` only prints them. Unused in the firmware, the linker drops it.

## Phase 0: Research

Complete: [research.md](research.md). No open questions; the slice prompt's suggested answers fixed the boundary rules (spec Assumptions).

## Phase 1: Design

Complete: [data-model.md](data-model.md), [contracts/engine-api.md](contracts/engine-api.md), [contracts/engine-table.md](contracts/engine-table.md), [quickstart.md](quickstart.md).

Post-design constitution check: unchanged, all Pass.

## Complexity Tracking

None.
