# Data model: Main screen with sample data (spec 003)

Builds on spec 002 (`nvp::engine::MainView`, `DayRecord`, `Settings`). New types are plain structs, no heap.

## RTC reading (`lib/core/src/nvp/time/rtc_codec.h`)

| Field | Type | Notes |
|---|---|---|
| `raw[7]` | uint8 | registers 02h..08h as read in one transaction (research R1) |
| `valid` | bool | VL = 0, C = 0, all BCD digits ≤ 9, fields in range, not before the build time |
| `reason` | enum `RtcInvalid` | `None`, `NoAnswer`, `IntegrityLost` (VL), `Century`, `BadField`, `BeforeBuild` |
| `utc` | `LocalSeconds` (seconds since 1970 UTC) | only meaningful when valid |

Functions: `decode(const uint8_t raw[7], int64_t build_utc) → RtcReading`; `encode(int64_t utc, uint8_t raw[7])` (VL = 0, C = 0, weekday from the date). Years 2000–2099 only.

## Clock (`lib/core/src/nvp/time/`)

| Type | Fields | Rules |
|---|---|---|
| `SimClock` | `base_local` (LocalSeconds), `base_ms` (uint32 monotonic), `speed` (uint16, 1–3600), `paused` | `now(ms) = base_local + (ms − base_ms) × speed / 1000`; changing speed rebases first so time never jumps |
| `kDefaultUtcOffsetMinutes` | int16 = 180 | TIME-04 default `<+03>-3` |
| `to_moment(LocalSeconds)` | → `engine::Moment` | date and second of day |

## System state (`lib/core/src/nvp/screen/system_state.h`)

`enum class SystemState { NoTime, NoData, Calculated, CacheEnding, Offline, Ok }`, in the order of 02 §6.

| Input (`StateInputs`) | Type |
|---|---|
| `clock_valid` | bool |
| `today_present` | bool |
| `today_rank` | uint8 (source rank of today's record; < 40 = calculated) |
| `refresh_failed_or_wifi_down` | bool |
| `stored_days_after_today` | int16 |
| `wifi_down_minutes` | int32 |
| `forced` | optional state (development command) |

`select_state(const StateInputs&) → SystemState`: forced state if set, otherwise the first match top to bottom (FR-012).

## Screen view (`lib/ui/src/main/main_view.h`)

| Field | Type | Example (fixture 01) |
|---|---|---|
| `state` | `SystemState` | `Ok` |
| `city` | char[48] | `İstanbul` |
| `last_update_text` | char[8] | `03:00` (`DD.MM` when not today) |
| `days_left` | int16 | used by `CacheEnding` |
| `kerahat_enabled` | bool | true |

The screen function: `ui::main_screen_update(const engine::MainView&, const SystemView&)`. It compares with what it last drew and updates only what changed (research R8).

## Fixture (`lib/core/src/nvp/screen/fixtures.h`)

| Field | Type |
|---|---|
| `number` | uint8 1–14 |
| `reference` | `const char*`, e.g. `01-main-ogle-1432.png` |
| `second_of_day` | int32 (−1 for fixture 13, no clock) |
| `state` | `SystemState` |
| `days_left` | int16 (5 for fixture 11) |

All fixtures use `engine::fixture::kYesterday/kToday/kTomorrow`, city `İstanbul`, last update `03:00` (research R10).

## Gesture (`lib/core/src/nvp/screen/gesture.h`)

`TwoFingerHold`: `feed(uint32_t ms, uint8_t points) → bool fired`. It fires once when ≥ 2 points have been reported continuously for 5000 ms; it resets when fewer than 2 are reported; it does not fire again until fingers are lifted (FR-022).

## App mode (`src/app`)

`Mode { Main, Diagnostic, Settings }`. Start: `Main`. `diag` / gesture → `Diagnostic`; `main` / the on-screen button → `Main` and stress off (FR-023). Clock source `{ Rtc, Simulated }`; start `Rtc`.
