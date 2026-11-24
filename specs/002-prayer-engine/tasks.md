---

description: "Task list for the prayer-time engine (spec 002)"
---

# Tasks: Prayer-time engine

**Input**: Design documents from `specs/002-prayer-engine/` ([plan.md](plan.md), [spec.md](spec.md), [research.md](research.md), [data-model.md](data-model.md), [contracts/](contracts/), [quickstart.md](quickstart.md))

**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/

**Tests**: Required. The constitution (Principle VI) says tests for `lib/core` are written before the code, and spec SC-001 asks for an automated test per scenario and boundary. In every phase the test task comes first and must fail before the code task.

**Organization**: Tasks are grouped by user story. No device, owner or network is needed in this slice.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: can run in parallel (different files, no dependency on an unfinished task)
- **[Story]**: the user story of spec.md (US1–US5)
- Every task names its files and cites requirement ids (constitution X)

## Path conventions

- Engine: `lib/core/src/nvp/engine/`, text: `lib/core/src/nvp/text/` (namespaces `nvp::engine`, `nvp::text`)
- Tests: `test/test_engine/` (one Unity program; each file exposes a `run_*_tests()` function, `test_main.cpp` owns `main`)
- Host table program: `sim/engine_table/`
- Standard C++17 only: no SDK, FreeRTOS or LVGL headers in `lib/core` (constitution VI); no `snprintf`, no heap in the engine (research R5, R7)

---

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: the empty test program and folders, so every later task can add a test file.

- [X] T001 Create `test/test_engine/test_main.cpp`: Unity runner calling `run_calendar_tests()`, `run_text_tests()`, `run_record_tests()`, `run_period_tests()`, `run_column_tests()`, `run_day_tests()`, `run_view_text_tests()`, `run_kerahat_tests()`, `run_sweep_tests()`, `run_table_tests()` (declare each; add the call as each file appears). It also replaces global `operator new` / `operator new[]` / `operator delete` (all forms) with versions that count allocations in a global counter exposed as `extern std::size_t g_allocations` (research R7, FR-023). Confirm `pio test -e native -f test_engine` builds and runs with zero tests.
- [X] T002 [P] Add two lines to `lib/core/README.md` describing the new folders `src/nvp/engine/` (prayer-time engine, spec 002) and `src/nvp/text/` (Turkish strings and formatting); the folders themselves appear with their first files in Phase 2 (05-architecture §4).

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: dates, records, Turkish strings and number formatting. Every user story needs them.

**⚠️ CRITICAL**: no user-story work starts until this phase is complete.

### Calendar (research R1)

- [X] T003 [P] Write `test/test_engine/test_calendar.cpp` (`run_calendar_tests`): `to_day_number(1970-01-01) == 0`; round trip `to_date(to_day_number(d)) == d` for every day from 1999-12-25 to 2101-01-05; `weekday`: 1970-01-01 Thursday, 2026-11-17 Tuesday (Salı), 2026-12-31 Thursday, 2027-01-01 Friday, a date before 1970 (1969-12-31 Wednesday); `valid`: 2024-02-29 true, 2026-02-29 false, 2000-02-29 true, 2100-02-29 false, month 0/13 false, day 0/32 false, 2026-04-31 false; `at(day, 774)` equals `day × 86400 + 774 × 60`. Must fail (no code yet).
- [X] T004 Implement `lib/core/src/nvp/engine/calendar.h` and `calendar.cpp`: `Date {int16_t year; uint8_t month; uint8_t day;}` with `==`; `using DayNumber = int32_t; using LocalSeconds = int64_t;`; `Moment {Date date; int32_t second_of_day;}`; `valid(Date)`, `to_day_number(Date)` (Hinnant `days_from_civil`), `to_date(DayNumber)` (`civil_from_days`), `weekday(DayNumber)` → 0 = Monday … 6 = Sunday using a non-negative `(d + 3) mod 7`, `at(DayNumber, uint16_t minutes)`, `kSecondsPerDay = 86400`. Cite the algorithm source in a comment. T003 passes.

### Day records (ENG-01, FR-001 to FR-003, research R8)

- [X] T005 [P] Write `test/test_engine/test_records.cpp` (`run_record_tests`): the fixture's three records are valid; equal times invalid (ENG-01); decreasing times invalid; a time of 1440 invalid; 0 and 1439 valid at the ends; invalid date invalid; `rank(SourceId)` = 50, 40, 40, 20, 10 for `Diyanet`, `Imsakiyem`, `Emushaf`, `Aladhan`, `Local`. Must fail.
- [X] T006 Implement `lib/core/src/nvp/engine/day_record.h` and `day_record.cpp`: vakit index constants `kImsak … kYatsi` (0–5) and `kVakitCount = 6`; `enum class SourceId : uint8_t`; `rank()`; `HijriDate {uint8_t day; uint8_t month; uint16_t year; bool present;}`; `DayRecord {Date date; uint16_t minutes[6]; HijriDate hijri; SourceId source;}`; `valid(const DayRecord&)`. T005 passes.
- [X] T007 [P] Create `lib/core/src/nvp/engine/fixture.h` (`nvp::engine::fixture`, all `constexpr`): `kDate` 2026-11-17; `kToday` {06:16, 07:46, 12:54, 15:27, 17:51, 19:14}, Hijri 7/6/1448, source `Imsakiyem`; `kYesterday` dated 2026-11-16 {06:15, 07:45, 12:54, 15:28, 17:52, 19:15}; `kTomorrow` dated 2026-11-18 {06:17, 07:47, 12:54, 15:26, 17:50, 19:13} (November trend: İmsak later, Yatsı earlier each day); expected values at 14:32:07 from 03 §3 (`kExpectedRemaining = 3293`, `kExpectedFractionTimes1000 = 641`). Comment that only yesterday's Yatsı and tomorrow's İmsak are fixed by 03 §3 (FR-021, research R11).

### Turkish strings and formatting (FR-015 to FR-018, 02 §9, research R4, R5)

- [X] T008 [P] Write `test/test_engine/test_text.cpp` (`run_text_tests`):
  - every constant in `tr.h` equals the 02 §9 text byte for byte (vakit names, period names §9.2, countdown labels §9.2, notes §9.3, months, weekdays, Hijri months);
  - the U+2019 in `1 dk’dan az` is the three bytes E2 80 99;
  - `format_hhmm`: 0 → `00:00`, 774 → `12:54`, 1439 → `23:59`;
  - `format_hhmmss`: 0, 3293 → `00:54:53`, 39780 → `11:03:00`, 86399 → `23:59:59`;
  - `format_duration_note`: 0 and 59 → `1 dk’dan az sonra`, 60 → `1 dk sonra`, 3599 → `59 dk sonra`, 3600 → `1 sa sonra`, 3660 → `1 sa 1 dk sonra`, 36300 → `10 sa 5 dk sonra`, 86340 → `23 sa 59 dk sonra`;
  - `format_date`: 2026-11-17 → `17 Kasım 2026 Salı`, 2026-12-31 → `31 Aralık 2026 Perşembe`, 2027-01-01 → `1 Ocak 2027 Cuma`, 2024-02-29 → `29 Şubat 2024 Perşembe`;
  - `format_hijri`: 7/6/1448 → `7 Cemaziyelahir 1448`, not present → empty, month 0 or 13 → empty;
  - every helper with a too-small capacity writes an empty, terminated string and returns 0.
  Must fail.
- [X] T009 [P] Create `lib/core/src/nvp/text/tr.h`: all strings of research R4 as UTF-8 `inline constexpr const char*` arrays indexed by vakit (0–5), month (1–12, index 0 unused) and weekday (0 = Monday). The file is UTF-8 without BOM; the apostrophe is U+2019 (FR-015, constitution III).
- [X] T010 Implement `lib/core/src/nvp/text/format.h` and `format.cpp`: `format_hhmm`, `format_hhmmss`, `format_duration_note`, `format_date`, `format_hijri` per [contracts/engine-api.md](contracts/engine-api.md), each `(…, char* out, size_t cap) → size_t`, built on two private helpers (append string, append zero-padded unsigned) with no C library formatting. T008 passes.

**Checkpoint**: `pio test -e native -f test_engine` passes for calendar, records and text.

---

## Phase 3: User Story 1 - Running period and countdown (Priority: P1) 🎯 MVP

**Goal**: for any second, the running period, its bounds, next vakit, remaining, elapsed, fraction, soon and entered (ENG-02, ENG-03, ENG-05, FR-004 to FR-007).

**Independent Test**: `test_period.cpp` with the fixture and hand-computed values ([contracts/engine-table.md](contracts/engine-table.md) gives the expected remaining for 15 moments).

- [X] T011 [P] [US1] Write `test/test_engine/test_period.cpp` (`run_period_tests`), all with the fixture and default settings:
  - 14:32:07 → period `kOgle`, start/end = 12:54:00/15:27:00 of 2026-11-17 as `LocalSeconds`, `next == kIkindi`, `next_is_tomorrow == false`, remaining 3293, elapsed 5887, fraction within 0.0005 of 0.6413, `period_name == "Öğle vakti"`, `countdown_label == "İkindiye kalan süre"`, `start_text "12:54"`, `end_text "15:27"`, `countdown_text "00:54:53"` (spec US1 scenario 1);
  - for each vakit *k*: one second before → previous period, remaining 1; exactly at it → period *k* (or İmsak), elapsed 0, remaining = full length (US1 2–3, ENG-02);
  - 03:30:00 → Yatsı, start 2026-11-16 19:15:00, end 2026-11-17 06:16:00, next İmsak today (US1 4);
  - 21:30:00 → Yatsı, start 19:14:00 today, end 2026-11-18 06:17:00, `next_is_tomorrow == true`, remaining 31620 (`08:47:00`) (US1 5);
  - soon: 17:36:00 true, 17:35:59 false; threshold 0 never true; threshold 30 at 17:21:00 true (US1 6, FR-007);
  - entered: 12:58:59 true, 12:59:00 false (US1 7);
  - `has_data` true and `approx_bounds` false for the full fixture.
  Must fail.
- [X] T012 [US1] Create `lib/core/src/nvp/engine/engine.h` with `Settings` (defaults 15, true, 45, 45, 45), `Input`, `ColumnState`, `Column`, `KerahatInterval` and `MainView` exactly as [data-model.md](data-model.md) (buffer sizes included), and `void evaluate(const Input&, MainView&)`.
- [X] T013 [US1] Implement the period part of `lib/core/src/nvp/engine/engine.cpp`: clear `out`; validate `today` against `now.date`; compute today's day number and the six `LocalSeconds`; yesterday's Yatsı and tomorrow's İmsak (from the records when valid and correctly dated, else substituted; full substitution logic is US3); choose the period per ENG-03 with half-open intervals; fill `period`, `start`, `end`, `next`, `next_is_tomorrow`, `remaining_s`, `elapsed_s`, `fraction` (clamped to [0,1)), `soon`, `entered`, `period_name`, `countdown_label`, `start_text`, `end_text`, `countdown_text`, `now_second_of_day`. No heap, no `snprintf`. T011 passes.

**Checkpoint**: US1 tests pass; the hero of the main screen can be driven from `MainView`.

---

## Phase 4: User Story 2 - Six columns in the right state (Priority: P1)

**Goal**: each column's displayed time, state, note and seconds-until per 02 §4.5 (FR-010 to FR-013).

**Independent Test**: `test_columns.cpp` against the column cells of [contracts/engine-table.md](contracts/engine-table.md).

- [X] T014 [P] [US2] Write `test/test_engine/test_columns.cpp` (`run_column_tests`):
  - 14:32:07: İmsak, Güneş `Past` with empty notes; Öğle `Current` `şu an`; İkindi `Next` `54 dk sonra` seconds_until 3293; Akşam `Later` `3 sa 18 dk sonra`; Yatsı `Later` `4 sa 41 dk sonra` (US2 1);
  - 12:55:00: Öğle `Current` `vakit girdi` (US2 2);
  - 03:30:00: no `Current`; İmsak `Next` `2 sa 46 dk sonra`; the rest `Later`; all six show today's times (US2 3, FR-012);
  - 21:30:00: Yatsı `Current` `şu an`; İmsak `Next`, `time_text "06:17"`, `minutes 377`, note `yarın`; the others `Past` (US2 4, FR-012);
  - 19:14:00: Yatsı `Current` `vakit girdi`, İmsak `yarın`;
  - for every vakit, the states one second before and at it differ only as the period change requires (US2 5);
  - `name` of each column equals the `tr.h` vakit name; `seconds_until` is 0 for `Past` and `Current`.
  Must fail.
- [X] T015 [US2] Implement the column part of `evaluate` in `lib/core/src/nvp/engine/engine.cpp`: normal case, after-Yatsı case (İmsak shows tomorrow's İmsak, `yarın`), before-İmsak case (no current); notes via `format_duration_note`, `tr::kVakitGirdi` / `tr::kSuAn` / `tr::kYarin`; `time_text` via `format_hhmm`. T014 passes.

**Checkpoint**: US1 and US2 together give everything the hero and the columns of the main screen need.

---

## Phase 5: User Story 3 - Midnight and missing days (Priority: P2)

**Goal**: midnight continuity, substitution with `approx_bounds`, "no data" (ENG-04, ENG-07, ENG-08, FR-003, FR-008, FR-009).

**Independent Test**: `test_days.cpp`.

- [X] T016 [P] [US3] Write `test/test_engine/test_days.cpp` (`run_day_tests`):
  - 23:59:59 on 2026-11-17 and 00:00:00 on 2026-11-18 (today = previous tomorrow, with a further tomorrow record): identical `start` and `end`, remaining drops by 1, period Yatsı both times; column times switch to the new day's (US3 1, TIME-06);
  - yesterday missing, 03:30:00: start = 2026-11-16 at today's Yatsı 19:14, `approx_bounds` true (US3 2);
  - tomorrow missing, 21:30:00: end = 2026-11-18 at today's İmsak 06:16, İmsak column `06:16` `yarın`, `approx_bounds` true (US3 3);
  - today missing → `has_data` false, `period_name` and `countdown_label` are the empty string `""` (never null), columns cleared, `clock_*` and `date_text` still filled (US3 4, ENG-08);
  - today invalid (equal times) → same as missing; yesterday dated 2026-11-15 (wrong) → treated as missing (FR-003); "today" dated another day → no data (US3 5);
  - three identical records → normal results, `approx_bounds` false (US3 6);
  - tomorrow's İmsak 2 minutes earlier and 2 minutes later than today's: continuity across midnight as in scenario 1 (US3 7, ENG-07);
  - month end 2026-11-30 → 2026-12-01 and year end 2026-12-31 → 2027-01-01 midnight crossings keep continuity (ENG-07);
  - `second_of_day` 86400 or −1 → `has_data` false.
  Must fail where behaviour is not yet implemented.
- [X] T017 [US3] Complete `lib/core/src/nvp/engine/engine.cpp`: neighbour date checks (FR-003), substitution and `approx_bounds` (ENG-04), "no data" path that fills only the clock and date (ENG-08), out-of-range `Moment`. T016 passes; T011 and T014 still pass.

---

## Phase 6: User Story 4 - Exact Turkish text on the screen (Priority: P2)

**Goal**: the hero's clock, Gregorian and Hijri dates and all strings in `MainView` are exactly the design's text (FR-015 to FR-018, SC-004).

**Independent Test**: `test_view_text.cpp` on `MainView` fields.

- [X] T018 [P] [US4] Write `test/test_engine/test_view_text.cpp` (`run_view_text_tests`): at 14:32:07 `clock_hhmm "14:32"`, `clock_ss ":07"`, `date_text "17 Kasım 2026 Salı"`, `hijri_text "7 Cemaziyelahir 1448"`; with the Hijri date absent `hijri_text` empty; all six period names and countdown labels by placing `now` inside each period; date text on 2026-12-31 and 2027-01-01 with matching records; `countdown_text` above ten hours (`11:03:00` at 19:14:00). Must fail.
- [X] T019 [US4] Fill `clock_hhmm`, `clock_ss`, `date_text` (from `now.date`), `hijri_text` (from today's record) in `lib/core/src/nvp/engine/engine.cpp`. T018 passes.

---

## Phase 7: User Story 5 - Kerahat intervals (Priority: P3)

**Goal**: three optional intervals on the day strip (ENG-06, FR-014).

**Independent Test**: `test_kerahat.cpp`.

- [X] T020 [P] [US5] Write `test/test_engine/test_kerahat.cpp` (`run_kerahat_tests`): fixture with 45/45/45 → [07:46, 08:31), [12:09, 12:54), [17:06, 17:51) in seconds of the day (US5 1); one length 0 → that interval not shown (US5 2); `kerahat_enabled` false → none shown (US5 3); a record with Güneş 00:30 and noon length 90 is clipped at 0, and an Akşam interval never ends after 86 400 (US5 4); lengths above 90 are limited to 90; intervals are the same at any `now` of the day. Must fail.
- [X] T021 [US5] Fill `kerahat[3]` in `lib/core/src/nvp/engine/engine.cpp`. T020 passes.

---

## Phase 8: Polish & Cross-Cutting Concerns

**Purpose**: the whole-day proof, no-allocation proof, the printed table, the device build, and records.

- [X] T022 [P] Write `test/test_engine/test_sweep.cpp` (`run_sweep_tests`) (SC-003, SC-007): for every second of 2026-11-17 and the first 7 hours of 2026-11-18 (fixture plus a record for 2026-11-19), check `start <= now < end`, `remaining + elapsed == end − start`, exactly one `Next` and at most one `Current`, and that `period`, `next` and every column `state` change only at a vakit second or at midnight; evaluate each moment twice and require byte-identical `MainView` results (FR-019, analysis C1); record `g_allocations` before and after the loop and assert it is unchanged. Must pass with the code of Phases 3–7 (fix any defect it finds in `engine.cpp`).
- [X] T023 [P] Write `test/test_engine/test_table.cpp` (`run_table_tests`): build the 15 rows for the moments of [contracts/engine-table.md](contracts/engine-table.md) with `table_header` / `table_row` and compare the joined text with the table block in that contract file byte for byte. The test reads the contract file relative to the project directory (`pio test` runs there); if the file cannot be opened the test **fails** with a message naming the path, it never skips (analysis U1). Must fail.
- [X] T024 Implement `lib/core/src/nvp/engine/table.h` and `table.cpp`: `table_header(char*, size_t)` (header and separator lines) and `table_row(const MainView&, char*, size_t)` in the exact format of the contract: time `HH:MM:SS`, period name, `start–end` with U+2013, countdown label, remaining, `yes`/`no` for soon and entered, then each column as `state` or `state note`. T023 passes (FR-022).
- [X] T025 Create `sim/engine_table/main.cpp` (prints the header and the 15 rows for the fixture via `table.h`) and add `[env:engine_table]` to `platformio.ini`: `platform = native@1.2.1`, `build_src_filter = -<*> +<../sim/engine_table/>`, `lib_ignore = nvp_ui, lvgl`, `build_flags = -std=gnu++17 -Wall -Wextra -Werror`. Run `pio run -e engine_table -t exec` and compare with the contract (FR-022, quickstart step 2). *Done 2026-11-24: identical (17 lines). `-t exec` fails to echo Turkish letters on a cp1252 console, so the program is run by `tools/check_engine_table.py` instead.*
- [X] T026 Add an `engine table` step to `tools/check.py` that runs `pio run -e engine_table -t exec` after the host tests, and extend `tools/test_repository.py` if it checks the environment list (FR-022). *Done: `check.py` builds the env and runs `tools/check_engine_table.py` (tested by `tools/test_check_engine_table.py`), which prints the table and fails on any difference from the contract; `test_repository.py` does not list environments.*
- [X] T027 Run `python tools/check.py`: all steps pass, the firmware builds with `lib/core` including the engine under the ESP-IDF flags (SC-006), image size and diagnostic-mode size are reported. Confirm by search that `lib/core/src/nvp/engine` and `nvp/text` include no SDK, FreeRTOS or LVGL header, no `<cstdio>`, and no clock access (`<ctime>`, `<chrono>`, `time(`, `clock(`) (constitution VI, research R5, FR-019).
- [X] T028 Compare the printed table with the reference renders `docs/design/screens/01` to `09` row by row (period name, bounds, countdown label and value, column states and notes) and record the result in `specs/002-prayer-engine/research.md` as a dated note (SC-005).
- [X] T029 [P] Update `README.md` (Status: Slice 2 done; Commands: `pio run -e engine_table`, then `python tools/check_engine_table.py`), `lib/core/README.md`, and `docs/specification/05-architecture.md` §4 if the engine's file layout differs from what it says (FR-020, 05 §4).
- [X] T030 Run `/speckit-converge` and resolve any appended task.

---

## Dependencies & Execution Order

### Phase dependencies

- **Setup (Phase 1)**: none.
- **Foundational (Phase 2)**: after Setup. Blocks all stories.
- **US1 (Phase 3)**: after Foundational. Creates `engine.h` and the period logic every other story extends.
- **US2 (Phase 4)**: after US1 (same `engine.cpp`, needs the period).
- **US3 (Phase 5)**: after US1; independent of US2 except that its column assertions use US2's rules (run after US2 to keep `engine.cpp` edits sequential).
- **US4 (Phase 6)**: after US1.
- **US5 (Phase 7)**: after US1.
- **Polish (Phase 8)**: after all stories.

### Within a phase

- The test task first; it must fail before its code task.
- Code tasks that touch `engine.cpp` (T013, T015, T017, T019, T021) run one at a time.

### Parallel opportunities

- Phase 2: T003, T005, T007, T008, T009 in parallel (different files); then T004, T006, T010.
- Story test files T011, T014, T016, T018, T020 can all be written in parallel once `engine.h` (T012) exists; their code tasks are sequential.
- Phase 8: T022, T023 and T029 in parallel.

---

## Parallel Example: Foundational

```text
Task: "Write test/test_engine/test_calendar.cpp (T003)"
Task: "Write test/test_engine/test_records.cpp (T005)"
Task: "Create lib/core/src/nvp/engine/fixture.h (T007)"
Task: "Write test/test_engine/test_text.cpp (T008)"
Task: "Create lib/core/src/nvp/text/tr.h (T009)"
```

---

## Implementation Strategy

### MVP first (US1)

1. Phases 1 and 2.
2. Phase 3 (US1): the hero values are right at every second.
3. **Stop and validate**: `pio test -e native -f test_engine`.

### Incremental delivery

1. US2: columns. With US1 this covers the main screen's time logic for a normal day.
2. US3: midnight and missing days.
3. US4: dates and the remaining text.
4. US5: kerahat.
5. Polish: sweep, no-allocation proof, printed table, check, reference comparison.

---

## Notes

- Each task cites ENG-01…ENG-08, an FR or SC of spec 002, or a 02-ui-spec section.
- Commit after each phase or logical group; commit messages cite requirement ids.
- No owner checklist in this slice: no hardware question; the owner accepts the printed table.
