# Data model: Real time

All types are pure C++ in `lib/core` (constitution VI) unless a type is named in `src/`.

## Time sample (`nvp::time::TimeSample`)

| Field | Meaning |
|---|---|
| `source` | `Rtc`, `Ntp`, `Http` |
| `utc_us` | the time the source gave, in UTC microseconds since 1970 |
| `at_uptime_us` | the device's uptime when the sample was taken (so later processing does not add delay) |
| `server` | short name for the log (`time.google.com`, `ezanvakti.imsakiyem.com`, `bm8563`) |

## Wall clock (`nvp::time::WallClock`, research R4)

**State:**
- `valid` (bool);
- `source` (`None`, `Rtc`, `Ntp`, `Http`);
- `last_sync_uptime_us`;
- `last_correction_us` (signed);
- `rtc_write_failed_logged` (bool; a failing board-clock write is logged once per boot).

**`accept(sample, system_utc_us_now, build_utc)`** returns a `SyncResult`:
- `Refused::BeforeBuild`, when `sample.utc` is earlier than `build_utc` (TIME-03);
- `Ignored::NtpFresh`, for an `Http` sample while the source is `Ntp` and the last sync is younger than 6 h;
- otherwise `Accepted { correction_us = sample_now − system_now, write_rtc }`:
  - the sample is first advanced by the uptime since it was taken;
  - `write_rtc` is true for every accepted `Ntp`/`Http` sample (FR-004, analysis I1).

**Rules:**
- `valid` becomes true on the first accepted sample and stays true until restart (FR-006).
- `Rtc` is accepted only at start, from a reading that passed Slice 3's checks.

## Time zone (`nvp::time::Zone`, research R5)

```text
Zone { std_offset_s, has_dst, dst_offset_s, start: Rule, end: Rule }
Rule { kind: MonthWeekDay | Julian1 | Julian0, month, week, weekday, day, time_s }
parse_zone(const char*, Zone&) -> bool        // invalid -> false, Zone unchanged
utc_to_local(const Zone&, utc_s) -> local_s
```

- The stored setting is NVS `set/tz`, a string of at most 63 bytes. Default `<+03>-3`.

## HTTP date (`nvp::time::parse_http_date`, research R2)

- `parse_http_date("Tue, 24 Nov 2026 11:41:17 GMT", &utc_s) -> bool` accepts the IMF-fixdate form only.
- Day and month names are English and case-sensitive; the day-of-week must match the date; `GMT` is required.

## System state inputs (research R8)

- `StateInputs.clock_valid` comes from the wall clock (or `true` while the simulated clock is on).
- `today_present = false` until Slice 6. The other inputs are unchanged.

## Second alignment measurement (research R6)

```text
JitterStats { count, median_ms, p99_ms, max_ms }   // ms within the real second at which the new second was flushed
```
