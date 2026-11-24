# Data model: Prayer-time engine (spec 002)

All types live in `lib/core/src/nvp/engine/` (namespace `nvp::engine`) and `lib/core/src/nvp/text/` (namespace `nvp::text`). Plain structs, no heap, no pointers that outlive a call except the caller's record pointers in `Input`.

## Date and time

| Type | Fields | Rules |
|---|---|---|
| `Date` | `year` (int16), `month` (1–12), `day` (1–31) | `valid(Date)`: month 1–12, day within the month, leap years per the Gregorian rule. |
| `DayNumber` | int32 | days since 1970-01-01 (`to_day_number`, `to_date`; research R1). |
| `LocalSeconds` | int64 | `day_number × 86 400 + second_of_day`; the only representation used for comparisons. |
| `Moment` | `date` (`Date`), `second_of_day` (0–86 399) | the current local time, given by the caller (FR-019). |

Helpers: `weekday(DayNumber)` → 0 = Monday … 6 = Sunday; `at(DayNumber, minutes)` → `LocalSeconds` of `HH:MM:00`.

## Day record

| Field | Type | Rules |
|---|---|---|
| `date` | `Date` | must be valid |
| `minutes[6]` | uint16 each | İmsak, Güneş, Öğle, İkindi, Akşam, Yatsı; minutes after local midnight; each 0–1439 and strictly increasing (ENG-01, FR-002) |
| `hijri` | `HijriDate {day, month, year, present}` | optional; month 1–12; `present = false` when the source gave none |
| `source` | `SourceId` | `DIYANET`, `IMSAKIYEM`, `EMUSHAF`, `ALADHAN`, `LOCAL` (rank 50, 40, 40, 20, 10; research R8); passed through |

`valid(const DayRecord&)` → bool. An invalid record is treated exactly like a missing one.

Index constants: `kImsak = 0, kGunes = 1, kOgle = 2, kIkindi = 3, kAksam = 4, kYatsi = 5` (identifiers per CLAUDE.md).

## Settings

| Field | Type | Default | Range |
|---|---|---|---|
| `soon_threshold_min` | uint8 | 15 | used as given; 0 disables "soon" |
| `kerahat_enabled` | bool | true | |
| `kerahat_sunrise_min`, `kerahat_noon_min`, `kerahat_sunset_min` | uint8 | 45, 45, 45 | limited to 0–90; 0 omits the interval |

## Input

| Field | Type | Notes |
|---|---|---|
| `now` | `Moment` | |
| `yesterday`, `today`, `tomorrow` | `const DayRecord*` | any may be null; a non-null record that is invalid, or whose date is not today − 1 / today / today + 1 relative to `now.date`, counts as missing (FR-003) |
| `settings` | `Settings` | |

`today` is matched against `now.date`: a "today" record with another date is missing, which gives "no data".

## Result: `MainView`

Filled by `evaluate(const Input&, MainView&)`. Every field is overwritten on every call.

### Status

| Field | Type | Meaning |
|---|---|---|
| `has_data` | bool | false → "no data" (FR-009); only `clock_hhmm`, `clock_ss`, `date_text` are filled; text fields are empty strings and `const char*` fields point to `""` (never null), numbers are 0 |
| `approx_bounds` | bool | a missing neighbour was substituted (FR-008) |

### Hero (02 §4.1 M5–M15, §5)

| Field | Type | Example at 14:32:07 |
|---|---|---|
| `clock_hhmm`, `clock_ss` | char[6], char[4] | `14:32`, `:07` |
| `date_text` | char[48] | `17 Kasım 2026 Salı` |
| `hijri_text` | char[40] | `7 Cemaziyelahir 1448` (empty when the record has none) |
| `period` | uint8 | `kOgle` |
| `period_name` | `const char*` into `tr.h`, never null | `Öğle vakti` |
| `countdown_label` | `const char*` into `tr.h`, never null | `İkindiye kalan süre` |
| `start`, `end` | `LocalSeconds` | 12:54:00, 15:27:00 of 2026-11-17 |
| `start_text`, `end_text` | char[6] | `12:54`, `15:27` |
| `next` | uint8 | `kIkindi` |
| `next_is_tomorrow` | bool | false (true after Yatsı) |
| `remaining_s`, `elapsed_s` | int32 | 3293, 5887 |
| `fraction` | float | 0.6413 |
| `countdown_text` | char[10] | `00:54:53` |
| `soon`, `entered` | bool | false, false |

### Columns (02 §4.5), `columns[6]`

| Field | Type | Example (İkindi at 14:32:07) |
|---|---|---|
| `name` | `const char*` | `İkindi` |
| `minutes` | uint16 | 927 |
| `time_text` | char[6] | `15:27` |
| `state` | `ColumnState` {`Past`, `Current`, `Next`, `Later`} | `Next` |
| `seconds_until` | int32 | 3293 (0 for past and current) |
| `note` | char[32] | `54 dk sonra` |

Invariant (SC-003): exactly one column is `Next`; at most one is `Current`.

### Day strip

| Field | Type | Notes |
|---|---|---|
| `now_second_of_day` | int32 | position of the now marker, 02 §3 |
| `kerahat[3]` | `{bool shown; int32 start_s; int32 end_s}` | seconds of today, half-open, clipped to 0–86 400 (FR-014) |

### Buffer sizes

Longest strings, in UTF-8 bytes: date `29 Ağustos 2026 Cumartesi` = 26; Hijri `30 Cemaziyelevvel 1448` = 22; note `23 sa 59 dk sonra` = 17, `1 dk’dan az sonra` = 19. The sizes above leave room; the formatting helpers never write past capacity and always terminate (tested).

## State transitions

The engine has no state between calls. The period moves `İmsak → Güneş → Öğle → İkindi → Akşam → Yatsı → (midnight, same period) → İmsak` purely as a function of `now` and the records.
