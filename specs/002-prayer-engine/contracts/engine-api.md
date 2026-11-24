# Contract: engine API (`lib/core`, spec 002)

The interface Slice 3 (screen), the data slices and the host harness use. Standard C++17, no SDK headers, no heap (FR-019, FR-023). Types are in [data-model.md](../data-model.md).

## Headers

| Header | Namespace | Provides |
|---|---|---|
| `nvp/engine/calendar.h` | `nvp::engine` | `Date`, `DayNumber`, `LocalSeconds`, `Moment`, `valid(Date)`, `to_day_number(Date)`, `to_date(DayNumber)`, `weekday(DayNumber)`, `at(DayNumber, minutes)` |
| `nvp/engine/day_record.h` | `nvp::engine` | `DayRecord`, `HijriDate`, `SourceId`, `rank(SourceId)`, `valid(const DayRecord&)`, vakit index constants |
| `nvp/engine/engine.h` | `nvp::engine` | `Settings`, `Input`, `MainView`, `ColumnState`, `evaluate(const Input&, MainView&)` |
| `nvp/engine/fixture.h` | `nvp::engine::fixture` | `kYesterday`, `kToday`, `kTomorrow`, `kDate` (2026-11-17), expected values at 14:32:07 |
| `nvp/engine/table.h` | `nvp::engine` | `table_header(out, cap)`, `table_row(const MainView&, out, cap)`: the Markdown lines of [engine-table.md](engine-table.md) |
| `nvp/text/tr.h` | `nvp::text` | every Turkish string (research R4) |
| `nvp/text/format.h` | `nvp::text` | `format_hhmm`, `format_hhmmss`, `format_duration_note`, `format_date`, `format_hijri` |

## `evaluate(const Input& in, MainView& out)`

- **Pure**: reads only `in`; writes only `out`; same input, same output.
- **Never fails**: invalid or missing records are handled by the rules below; there is no error code.
- **Total over all inputs**: any `Moment` with a valid date and `second_of_day` 0–86 399. A `Moment` outside that range gives `has_data = false`.

Rules, in order:

1. `today` missing, invalid or not dated `now.date` → `has_data = false`, clock and date filled, all else cleared: text empty, `const char*` fields `""` (never null), numbers 0 (ENG-08).
2. `yesterday` missing → its Yatsı = today's Yatsı minute on `today − 1`; `tomorrow` missing → its İmsak = today's İmsak minute on `today + 1`; `approx_bounds = true` (ENG-04).
3. Period: before today's İmsak → Yatsı `[yesterday Yatsı, today İmsak)`; between vakits → that period; from today's Yatsı → Yatsı `[today Yatsı, tomorrow İmsak)`, `next_is_tomorrow = true` (ENG-03). Half-open intervals, boundaries at second 00 (ENG-02).
4. Remaining, elapsed, fraction, soon, entered (ENG-05, research R2).
5. Columns per `02-ui-spec.md` §4.5, including the after-Yatsı and before-İmsak cases (FR-011–FR-013).
6. Kerahat intervals (ENG-06, research R9).
7. Strings from `tr.h` and `format.h`.

## Text helpers (`format.h`)

Each writes into `char* out` with capacity `cap`, always terminates, never writes past `cap`, and returns the number of bytes written (without the terminator), or 0 when `cap` is too small (the output is then empty).

| Function | Input | Output |
|---|---|---|
| `format_hhmm(minutes)` | 0–1439 | `HH:MM` |
| `format_hhmmss(seconds)` | ≥ 0 | `HH:MM:SS`; hours may exceed 23 |
| `format_duration_note(seconds)` | ≥ 0 | `1 dk’dan az sonra`, `M dk sonra`, `H sa sonra`, `H sa M dk sonra` (02 §9.1) |
| `format_date(Date)` | valid date | `D MMMM YYYY dddd` |
| `format_hijri(HijriDate)` | present, month 1–12 | `D MMMM YYYY`; empty when not present |

## Compatibility

Slice 3 binds `MainView` fields directly. Adding fields later is compatible; renaming or changing meaning needs a spec change.
