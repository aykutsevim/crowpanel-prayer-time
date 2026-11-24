# Research: Prayer-time engine (spec 002)

Date: 2026-11-24. No third-party code is added in this slice. Sources: `docs/specification/03-functional-spec.md` §3 and §8, `02-ui-spec.md` §3–§5 and §9, `04-data-sources.md` §2 and §9, `05-architecture.md` §4, the constitution, and the slice prompt `docs/development/prompts/slice-02-prayer-engine.md`.

## R1. Time representation

**Decision**: a calendar date is converted once to a **day number** (days since 1970-01-01, signed 32-bit). A moment is `day number × 86 400 + second of day`, held as a signed 64-bit **local second count** (`LocalSeconds`). Periods, bounds and differences are plain integer arithmetic on that count. Dates are converted back only for display.

**Rationale**:
- Midnight, month end and year end disappear as special cases. "Yesterday's Yatsı" is `(today − 1) × 86 400 + 19:15`, and 23:59:59 → 00:00:00 is just `+1`.
- Remaining and elapsed seconds are subtractions; continuity across midnight follows from using the same instants on both sides (spec US3).
- 64 bits avoid the 2038 limit of a 32-bit second count; day numbers fit 32 bits for any realistic date.
- The weekday is `(day + 3) mod 7` with Monday = 0 (1970-01-01, day 0, is a Thursday = 3; the modulo is taken as non-negative for dates before 1970), so no table of years is needed.

**Algorithm**: `days_from_civil` and `civil_from_days` by Howard Hinnant ("chrono-Compatible Low-Level Date Algorithms", public domain, howardhinnant.github.io/date_algorithms.html). They are exact for the proleptic Gregorian calendar, use only integer arithmetic, and need no tables. Verified by tests against known dates: 1970-01-01 = 0 (Thursday), 2026-11-17 = Tuesday (the design's fixture date), 2026-12-31 = Thursday, 2027-01-01 = Friday, 2024-02-29 valid, 2026-02-29 invalid, 2000-02-29 valid, 2100-02-29 invalid.

**Alternatives considered**:
- Plain fields (Y, M, D, h, m, s) everywhere: every comparison across midnight needs carry logic; error-prone at year end.
- `std::chrono` calendar types (C++20): the project builds with C++17 (gnu++17) for both targets.
- `time_t` / `mktime`: depends on the C library's time zone state, which the engine must never read (FR-019).
- The storage format's day number since 2020-01-01 (`04-data-sources.md` §9, u16): fine for storage, but the engine should not inherit a storage limit; converting between the two epochs is one subtraction in the storage slice.

## R2. Boundary rules (from the slice prompt's suggested answers, recorded in the spec's Assumptions)

- A vakit at `HH:MM` begins at `HH:MM:00` (ENG-02).
- Period *k* is the half-open interval `[start, end)`: at the exact second of a vakit the new period runs, elapsed is 0 and remaining is its full length.
- Durations in notes use `floor(seconds / 60)` minutes (02 §9.1).
- The countdown is the remaining seconds, shown `HH:MM:SS`; at a boundary it shows the new period's length.
- "Entered" = elapsed < 300 s. "Soon" = threshold > 0 and remaining ≤ threshold × 60.

## R3. Output shape and text

**Decision**: one plain result struct (`nvp::engine::MainView`), filled by one function `evaluate(const Input&, MainView&)`. It carries both numbers (period index, bounds, remaining, fraction, flags, column states, kerahat intervals) and the ready Turkish strings the main screen draws (clock, date, Hijri date, period name, countdown label, countdown, period bounds, column times and notes). The caller owns the struct; `evaluate` writes into it.

**Rationale**: Slice 3 can bind the screen to one struct and redraw by field (UI-03) without any time logic of its own (FR-020). Writing into a caller-owned struct needs no heap (FR-023). Strings are fixed-size char arrays sized for the longest Turkish text plus margin (data-model.md).

**Alternatives considered**: a separate formatting pass in `lib/ui`: it would spread text rules across two libraries and could not be tested without LVGL; returning by value: a few hundred bytes per call on the stack every second is acceptable on the device, but out-parameters make the no-allocation rule obvious.

## R4. Turkish strings

**Decision**: every user-visible string the engine emits is in one header, `lib/core/src/nvp/text/tr.h`, as UTF-8 `const char*` constants: six vakit names, six period names, six countdown labels, the notes `vakit girdi`, `şu an`, `yarın`, ` sonra`, the duration parts ` dk`, ` sa`, `1 dk’dan az`, twelve Gregorian months, seven weekdays, twelve Hijri months. Source files are UTF-8. The apostrophe is U+2019 (bytes E2 80 99); a test asserts those bytes so an editor cannot silently replace it with U+0027.

**Rationale**: constitution III (byte for byte) and the slice prompt ("one table in lib/core"). GCC on both targets reads UTF-8 source as-is.

## R5. Number formatting without the C library

**Decision**: small in-house helpers write zero-padded two-digit fields and unsigned integers into a buffer, with an explicit capacity, and return the length. No `snprintf`.

**Rationale**: `snprintf` is safe on both targets, but its allocation behaviour differs between C libraries (newlib may allocate on first use of some paths) and the no-allocation test (R7) would not see it. The helpers are a few lines and fully tested.

## R6. Fraction

**Decision**: `float`, computed as `elapsed / length`, clamped to `[0, 1)`. The ESP32-S3 has a single-precision FPU; the screen needs it only for a 132-pixel progress bar. Tests compare with a tolerance of 0.0005 (fixture 0.641 = 5887 / 9180).

## R7. Proving "no allocation" (FR-023, SC-007)

**Decision**: the engine test program replaces the global `operator new` / `operator delete` with counting versions, runs `evaluate` for every second of a day, and asserts zero allocations. The engine uses no containers, strings or `new`; the replacement makes that a test rather than a promise.

## R8. Sources and records

**Decision**: a `SourceId` enumeration in `lib/core` with the five providers of `04-data-sources.md` §2 (`DIYANET` 50, `IMSAKIYEM` 40, `EMUSHAF` 40, `ALADHAN` 20, `LOCAL` 10) and a `rank()` helper. The engine passes the source through unchanged; it does not decide the `calculated` state (spec Assumptions). A record holds its six times as minutes after midnight (u16), matching the storage layout of `04-data-sources.md` §9.

## R9. Kerahat

**Decision**: three intervals in seconds of today, `[gunes, gunes + a)`, `[ogle − b, ogle)`, `[aksam − c, aksam)`, lengths limited to 0–90 minutes; zero or layer off omits the interval; each is clipped to `[0, 86 400]`. With valid records Güneş ≥ 00:01 and Akşam ≤ 23:58, so clipping matters only for extreme lengths, but it is applied uniformly. Defaults 45/45/45 from `03-functional-spec.md` §8.

## R10. Output table for the owner (FR-022)

**Decision**: a host program `sim/engine_table/main.cpp`, built by a new environment `engine_table` (native, `lib/core` only, no LVGL), built with `pio run -e engine_table` and run by `tools/check_engine_table.py`, which prints the table and compares it with the contract (`-t exec` works too where the console is UTF-8). It prints one Markdown table row per moment listed in the slice prompt. `tools/check.py` runs both, so the table is always reproducible.

## R11. Shared fixture (FR-021)

**Decision**: `lib/core/src/nvp/engine/fixture.h` defines the yesterday, today and tomorrow records of `03-functional-spec.md` §3 (date 2026-11-17, Hijri 7 Cemaziyelahir 1448, source `IMSAKIYEM`) as `constexpr` data. The tests, the table program and later the UI harness include it. Unused, it costs nothing in the firmware image.

## R12. Device build (SC-006)

`lib/core` is already compiled into the device firmware because `src/app` uses `nvp/diag`. PlatformIO compiles all of a library's sources once it is a dependency, so every `python tools/check.py` run builds the engine for the ESP32-S3 with the ESP-IDF warning flags (`-Werror=all` subset, including `format-truncation`). No extra step is needed.

## R13. Result: printed table against the reference renders (2026-11-24, SC-005)

`pio run -e engine_table` followed by `tools/check_engine_table.py` produces the table of `contracts/engine-table.md` exactly (17 lines). Each reference render was opened next to its row and compared on period name, bounds, countdown label, countdown value, column states and every column note:

| Render | Moment | Result |
|---|---|---|
| `01-main-ogle-1432.png` | 14:32:07 | match (Öğle vakti 12:54–15:27, `00:54:53`, past/past/current `şu an`/`54 dk sonra`/`3 sa 18 dk sonra`/`4 sa 41 dk sonra`) |
| `02-main-gece-0330.png` | 03:30:00 | match (Yatsı vakti 19:15–06:16, `02:46:00`, no current column, `2 sa 46 dk sonra` … `15 sa 44 dk sonra`) |
| `03-main-imsak-0650.png` | 06:50:00 | match (İmsak vakti 06:16–07:46, `00:56:00`, `56 dk sonra` … `12 sa 24 dk sonra`) |
| `04-main-kusluk-1000.png` | 10:00:00 | match (Kuşluk vakti 07:46–12:54, `02:54:00`, Güneş current) |
| `05-main-vakit-girdi-1255.png` | 12:55:00 | match (`vakit girdi`, `02:32:00`, `2 sa 32 dk sonra`, `4 sa 56 dk sonra`, `6 sa 19 dk sonra`) |
| `06-main-ikindi-1620.png` | 16:20:00 | match (İkindi vakti 15:27–17:51, `01:31:00`, `1 sa 31 dk sonra`, `2 sa 54 dk sonra`) |
| `07-main-az-kaldi-1742.png` | 17:42:00 | match (`00:09:00`, soon = yes, shown in the alert colour; `9 dk sonra`, `1 sa 32 dk sonra`) |
| `08-main-aksam-1820.png` | 18:20:00 | match (Akşam vakti 17:51–19:14, `00:54:00`, `54 dk sonra`) |
| `09-main-yatsi-2130.png` | 21:30:00 | match (Yatsı vakti 19:14–06:17, `08:47:00`, İmsak `06:17` `yarın`, Yatsı current) |

The hatched kerahat bands in the renders sit at the 45-minute intervals the engine reports (07:46–08:31, 12:09–12:54, 17:06–17:51).
