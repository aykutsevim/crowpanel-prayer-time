# Contract: serial console additions (FR-022, FR-023, SYS-05, SYS-06)

Development-only commands (`NVP_DEV_TOOLS`), carried out by the network worker, which prints the reply as a log line. The `data:` report line and all log lines appear in every build. Spec 006's `data refresh|show|corrupt|erase` stay as they are; `data refresh` now goes through the planner (manual trigger, at most once per minute: `data: refused (once per minute)`).

| Command | Effect | Reply |
|---|---|---|
| `data block <imsakiyem\|emushaf\|aladhan>` | the runner treats that provider as failing without contacting it (RAM only, cleared by a restart) | `data: emushaf blocked` |
| `data unblock <name\|all>` | removes the block | `data: all unblocked` |
| `data plan` | planner state | see below |
| `data calib` | coordinates and calibration | see below |
| `data show` | as spec 006, plus `, mismatches N` and the source of each part of the range | `data: district 9206, 366 records 2026-11-26..2027-11-26 (imsakiyem 366), source imsakiyem, crc ok, last refresh 2026-11-26 03:41 UTC, mismatches 0` |

`data plan` (times in local time; the worker gets the time-zone offset from the tick):

```text
data: plan last success 26.11.2026 03:41 (complete), next weekly 03.12.2026 03:41, night minute 03:41
data: plan retry none, today diyanet, 366 days ahead
data: plan imsakiyem ok, emushaf ok, aladhan ok; blocked none
data: plan imsakiyem failures 2, cool-down until 26.11.2026 14:10; retry step 2 at 26.11.2026 14:25
```

`data calib`:

```text
data: calib coords 39.92063 32.85403 (table)
data: calib offsets 0 0 0 0 0 0 from 30 days, hijri +1
data: calib none (13 diyanet days, 14 needed)
data: calib coords suspect (two mirrors agreed, both implausible)
```

**As implemented (T040, device log 2026-11-26):** `data block imsakiyem` answers `data: imsakiyem blocked (development)`; `data unblock all` answers `data: all unblocked`; `data refresh` within a minute of the last one answers `data: refused (once per minute)`. `data plan` prints `plan last success 26.11.2026 11:16 (complete), next weekly 04.12.2026 03:35, night minute 03:35`, `plan retry none (step 0), today diyanet, 365 days ahead` and one `plan <provider> failures N[, cool-down until ..][, skipped until ..][, blocked]` line per provider. `data calib` prints `calib coords 39.92063 32.85403 (table)` and `calib offsets 0 0 0 0 0 0 from 30 days, hijri +1`. `data show` adds the count per source, for example `(imsakiyem 366, aladhan 1)`, and `mismatches N`.

## Report line

```text
data: 366 records 2026-11-26..2027-11-26, today yes, source imsakiyem, last refresh 03:41 (612 min ago), next 03.12 03:41
data: 38 records 2026-11-26..2027-01-02, today calculated (aladhan), last refresh failed, retry 14:25
```

As implemented (device): `data: 367 records 2026-11-25..2027-11-26, today yes, source imsakiyem, last refresh 11:19 (0 min ago), last outcome complete, next 04.12 03:35`; when today is calculated the source key says so (`source local`, `source aladhan`).

## Log lines

As implemented (device, 2026-11-26): `refresh started (boot: never refreshed)`, `imsakiyem ok, 366 records, 1 requests, 2.1 s`, `imsakiyem blocked`, `emushaf ok, 26 records, 1 requests, 1.6 s`, `aladhan ok, 37 records, 1 requests, 2.6 s`, `calibration 0 0 0 0 0 0 from 30 days, hijri +1`, `refresh complete, 367 records (2026-11-25..2027-11-26), 1 requests, 2.1 s, heap min 22 kB`, `refresh failed, 37 records (2026-11-25..2026-12-31), 2 requests, retry 26.11.2026 12:18 (today calculated, hourly)`. Planned forms (unchanged):

One line per provider call (FR-023), one per refresh outcome, one per mismatch:

```text
[  t] data: refresh started (boot: under 30 days ahead)
[  t] data: imsakiyem v2 ok, 366 records, 2.1 s
[  t] data: emushaf blocked (development)
[  t] data: emushaf failed (http 502), failures 3
[  t] data: emushaf rejected (implausible: 2026-11-26 ogle 60 min from calculation), store unchanged
[  t] data: aladhan 2026-11 ok, 30 records, 1.2 s
[  t] data: imsakiyem cooling down until 14:10 (http 429)
[  t] data: imsakiyem skipped for 24 h (5 failed refreshes)
[  t] data: refresh complete, 366 records stored (2026-11-26..2027-11-26), 3 requests
[  t] data: refresh partial (emushaf failed), 38 records stored, retry in 5 min
[  t] data: refresh failed, nothing stored, retry in 1 h (today calculated)
[  t] data: MISMATCH 2026-11-28: imsakiyem 368 456 760 912 1054 1137 / emushaf 368 456 761 912 1054 1137
[  t] data: coordinates suspect (two mirrors agreed, both implausible)
[  t] data: calibration 0 0 0 0 0 -1 from 30 days, hijri +1
[  t] data: calibration rejected (yatsi +14 > 10 min)
[  t] data: local times for 2026-11-26..2026-12-03 (no record for today)
```
