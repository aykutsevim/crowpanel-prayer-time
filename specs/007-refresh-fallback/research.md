# Research: Automatic refresh and fallback (spec 007)

Live checks of 04 §13 items 2, 4 and 6, done on 2026-11-26 for the owner's district (Ankara, 9206), with the User-Agent `NamazVaktiPanel/0.7 (ESP32-S3; personal device)`. Recorded answers are in `test/fixtures/providers/` (README lists each request). About 20 requests in total, at most one per second.

## R1. The second mirror, emushaf (04 §13 item 2)

**Decision.** Parse `GET https://ezanvakti.emushaf.net/vakitler/{district}` as a top-level JSON array; per element take `MiladiTarihKisa` (`DD.MM.YYYY`), `Imsak`, `Gunes`, `Ogle`, `Ikindi`, `Aksam`, `Yatsi` (`HH:MM`) and `HicriTarihKisa` (`D.M.YYYY`, for example `10.6.1448`). Ignore the ISO fields (04 §3.2 quirk). Source id `Emushaf`, rank 40.

**Found.**
- The OpenAPI document (`/openapi.json`, version 0.7.0) has four paths (`/ulkeler`, `/sehirler/{ulke}`, `/ilceler/{sehir}`, `/vakitler/{ilce}`). The `Vakit` schema has 19 required string fields; `HicriTarihKisa` is the easiest Hijri field to parse.
- **The answer is cached for up to five days.** `cache-control: public, max-age=432000`, `cf-cache-status: HIT`. On 26 November it returned the same bytes as on 24 November: 32 records from 19.11 to 20.12. From today the answer therefore covers **between about 24 and 31 days ahead**, depending on the cache's age. So emushaf alone may not reach the "30 days ahead" goal (affects R6).
- An unknown district gives **502** with `text/plain` `error code: 502` (a Cloudflare page, not JSON). It counts as a failure of the provider, not as "district not found".
- No rate limit is published. The page says "Muhabbetle yapılmıştır" and names its author; no terms of use. One call per refresh, as 04 §3.2 says.
- The 27 overlapping days with İmsakiyem were identical in all six times and the Hijri date (spec 006 R1).

## R2. The online calculation service, Aladhan (04 §13 item 4)

**Decision.** Calendar per month, `GET https://api.aladhan.com/v1/calendar/{year}/{month}?latitude={lat}&longitude={lon}&method=13&school=0&timezonestring=Europe/Istanbul`. Take `data[].date.gregorian.date` (`DD-MM-YYYY`) and the first five characters of `Fajr`, `Sunrise`, `Dhuhr`, `Asr`, `Maghrib`, `Isha` (they carry ` (+03)`). **Do not take Aladhan's Hijri date** (below). Source id `Aladhan`, rank 20. **Do not use the address lookup** for coordinates (R4).

**Found.**
- The answer is `{code:200, status:"OK", data:[30 or 31 days]}`, 40 kB a month. `meta` repeats the method (`params {Fajr:18, Isha:17}`) and the offsets it applied (`Sunrise −7, Dhuhr +5, Asr +4, Maghrib +7`), the same as 04 §3.5.
- Rate-limit headers: `RateLimit-Limit: 12`, `X-RateLimit-Limit-Second: 12` (12 a second). `Cache-Control: public,max-age=3600`.
- **Distance from the published times**, 38 days (24.11.2026–31.12.2026): median 0 for each of the six times; every difference within ±1 minute.
- **Hijri date differs**: Aladhan says 14 Cemaziyelahir 1448 for 24.11.2026, Diyanet 15. Different on 16 of the 38 days. Calculated records get their Hijri date from R8 instead.
- `timingsByAddress` for "Ankara, Ankara, Türkiye" answered 200 with `meta.latitude 8.8888888, meta.longitude 7.7777777` (placeholder values); an unknown address answered **503** "Geocoding is temporarily unavailable". The geocoding is not dependable (R4).

## R3. Calculation on the device (04 §3.5)

**Decision.** A C++17 port of the astronomical core of **adhan-js v4.4.6** (MIT, Batoul Apps, tag commit `b6e04cd9da28fffc66773404a0a2e9c739370baf`) into `lib/core/src/nvp/calc/`, with the MIT notice. Turkey method: Fajr 18°, Isha 17°, standard Asr, adjustments sunrise −7, Dhuhr +5, Asr +4, Maghrib +7 (`src/CalculationMethod.ts` lines 88–96 at that tag, read 2026-11-26). Results in minutes after local midnight, no heap, no exceptions. Files to port: `Astronomical.ts`, `SolarCoordinates.ts`, `SolarTime.ts`, `PrayerTimes.ts`, `CalculationParameters.ts`, `CalculationMethod.ts` (Turkey only), `Madhab.ts`, `HighLatitudeRule.ts`, `MathUtils.ts`, `DateUtils.ts`, `TimeComponents.ts`, `Rounding.ts`. Not ported: Qibla, Sunnah times, Shafaq, polar-circle resolution (Türkiye is far from the polar circle).

**Test oracle.** `Shared/Times/Ankara-Turkey.json` from adhan-js at the same tag (24 dates in 2019, method Turkey, `variance` 2 minutes), plus the other seven files of that folder for wider coverage. In addition our own fixture: the 128 published days for Ankara (R3 measurement below), each time within ±2 minutes.

**Alternatives considered.**
- **adhan-cpp** (Lotconn, MIT, v1.0.2): needs C++20 calendar types (the project builds with `gnu++17`), throws exceptions, one author, a few months old. Used only as a cross-check.
- **Adhan C** (radcheb, MIT): last commit 2019, uses global `localtime()`, no Turkey method.
- **PrayTimes**: JavaScript only now; the old C++ was LGPL.
- **AthanLib**: GPL-3.0, would make the firmware GPL.
- Writing the astronomy ourselves: forbidden by 04 §3.5 ("use a maintained, tested implementation").

**Measured for Ankara (proxy until the port exists).** The same formulas (solar declination and equation of time, hour angles for 18°, 0.833°, 17°, standard Asr) with the Turkey adjustments, compared with the 128 published days 24.11.2026–31.03.2027. Calibrated on the first 30 days, tested on the next 98:

| Time | Offset learned | Before: median (range) | After: median (range) |
|---|---|---|---|
| İmsak | 0 | 0 (0..+1) | 0 (0..+1) |
| Güneş | 0 | 0 (0..+1) | 0 (0..+1) |
| Öğle | 0 | 0 (−1..+1) | 0 (−1..+1) |
| İkindi | 0 | 0 (−1..0) | 0 (−1..0) |
| Akşam | 0 | −1 (−1..0) | −1 (−1..0) |
| Yatsı | 0 | −1 (−1..0) | −1 (−1..0) |

(published minus calculated, minutes). For Ankara the calculation is already within a minute; calibration matters for districts that share their centre's table (OD4-3), where the coordinates are not the centre's. The port must reproduce this table in its own host test (Stage 2 reports it again).

**Stage 2 results (2026-11-26, host; T024–T027, T034).** The port (`lib/core/src/nvp/calc/`) against adhan's shared test data, every time within the file's variance:

| File | Days | Worst difference | Variance |
|---|---|---|---|
| Ankara-Turkey | 24 | 2 min | 2 |
| Doha-Qatar | 12 | 0 | 0 |
| Dubai-Gulf | 12 | 1 | 1 |
| Kuwait City-Kuwait | 12 | 2 | 2 |
| Makkah-UmmAlQura | 12 | 0 | 0 |
| Singapore-Singapore | 366 | 2 | 2 |
| Tehran-Tehran | 12 | 1 | 1 |

London (Moonsighting Committee) is not used: its special cases are not ported.

The port against the 128 published days for Ankara (with İmsakiyem's coordinates for 9206), published minus calculated, every time within ±2 minutes:

| Time | Median | Range |
|---|---|---|
| İmsak | 0 | −1..+1 |
| Güneş | 0 | −1..+1 |
| Öğle | 0 | −1..+1 |
| İkindi | −1 | −2..+1 |
| Akşam | 0 | −1..+1 |
| Yatsı | −1 | −1..+1 |

Calibrated on the first 30 days and tested on the next 98: all six offsets come out 0 for Ankara, so before and after are the same: medians 0, 0, 0, −1, −1, −1 (ranges within −2..+1), each within the 1 minute of SC-004. The proxy measurement above differs by a minute in a few ranges because the proxy rounded differently from adhan.

**Hijri date for calculated days.** The tabular calendar gives 13/6/1448 for 24.11.2026 (Diyanet 15/6/1448); the learned offset is +2. Predicting the 7 days after each published day: 742 of 840 equal to Diyanet's (88 %). The misses are month ends: Diyanet's Cemaziyelahir 1448 has 30 days, the tabular 29. Continuing Diyanet's month day by day (instead of the offset) gave exactly the same 742, so the simple offset stays. A computed Hijri date is shown only in the `calculated` state (02 §8), where every time is marked approximate.

## R4. Coordinates (04 §6, LOC-03)

**Decision.** Coordinates come from a **table bundled with the firmware**, generated with the place list. Resolution order: (1) owner override (Slice 8; nothing yet); (2) the bundled table; (3) only if the place is missing from the table, one `GET /api/v2/locations/districts/{id}` call, accepted when `coordinates.confidence` is `high` or `medium` and the point lies in Türkiye's box (04 §6 step 3). The Aladhan address lookup is dropped.

**Found.**
- The İmsakiyem v2 district list that `tools/gen_places.py` already reads (3 pages of 500) carries `coordinates{lat, lon, confidence}` for **all 1003 entries, all `confidence:"high"`, all inside Türkiye's box**. Kadıköy (100059), an entry that shares its centre's table, has its own coordinates.
- The district detail for 9206 answers `{lat 39.92063, lon 32.85403, confidence "high"}`; 999999 answers 404 `DISTRICT_NOT_FOUND`.
- Aladhan's address lookup gave placeholder coordinates (R2).

**Rationale.** Coordinates available at once, offline, for every place in the list; no request; no dependence on a geocoder. 12 bytes per entry (about 12 kB of flash). The entry is identified by its Diyanet (or centre) id and its district name, because the stored location keeps the centre's id for shared entries (OD4-3).

**Built (T031–T033).** `data/places_coords.json` (1003 rows, generated 2026-11-26 with 3 requests); `tools/gen_web.py` turns it into `src/generated/places_coords.cpp` (rows `{id, fnv1a32(NAME), lat_e5, lon_e5}`, sorted); `nvp::net::lookup_coords` finds a row by id and name, or by id when only one row has it. The district-detail parser accepts `high` and `medium` inside Türkiye's box.

**Consequence for the spec.** FR-015's second step (address lookup) is replaced; spec FR-015 and 04 §6 are updated in this planning step.

## R5. The refresh planner (DATA-03, 04 §5.1)

**Decision.** A pure, deterministic state machine `nvp::providers::Planner` in `lib/providers`. Inputs are events with the time given by the caller: `boot`, `link_up`, `link_down`, `tick` (UTC seconds and the local date and minute of day), `setup_done`, `location_changed`, `manual`, and `refresh_finished(outcome)`. Output: "start a refresh now, because …" or nothing, and the next scheduled time for display. It knows nothing of Wi-Fi, tasks or HTTP.

- **Night minute**: `180 + fnv1a32(station MAC) mod 120`, so 03:00–04:59 local time (FR-005).
- **Weekly**: due at the night minute once `now ≥ last_success + 7 days`.
- **Low coverage**: fewer than 30 days ahead → once per night at the night minute.
- **Boot and reconnection**: today without a Diyanet-derived record, or under 30 days ahead, or last success more than 7 days old; reconnection at most once per 10 minutes.
- **Manual**: at most once per minute.
- **Retry** after a refresh that did not reach its goal (R6): 5 min, 15 min, 1 h, 3 h, then every 6 h; one extra attempt per reconnection. **Hourly** instead while today's record is calculated (FR-003).
- A refresh never starts without a valid clock, a location and the link up.

**Alternatives.** Timers in the device code: not testable on the host, against constitution VI.

## R6. One refresh (04 §5.2, owner direction)

**Decision.** `refresh_with` grows into a step runner over a fixed provider list `[Diyanet (empty slot), İmsakiyem, Emushaf, Aladhan, Local]`, with a request counter (cap 12), per-provider cool-down and skip checks, and a software block list (development). Steps as 04 §5.2. Changes found by research:

- **Goal reached** = Diyanet-derived records today..today+30, **or** every Diyanet-derived provider that was asked answered validly (nothing more is available, for example emushaf's cached month or the year end). Only a provider failure makes a refresh "not reached", which starts the retry schedule. Without this, a year end or emushaf's cache would retry every 6 hours for nothing.
- **Outcomes**: `complete` (goal reached), `partial` (something stored, a provider failed), `failed` (nothing stored, a provider failed). "Last successful refresh" is the end of the last `complete` or `partial` refresh that stored Diyanet-derived records.
- **Diyanet first** (FR-001, FR-002): the calculated steps run only after both mirrors, and Aladhan is asked only for months that hold a day without a Diyanet-derived record. Merge keeps rank above fetch time (spec 006).
- **Plausibility exception** (FR-016): if İmsakiyem's batch fails only the plausibility check, it is kept aside; if emushaf's batch also fails only that check, and both agree on every overlapping day, İmsakiyem's batch is accepted, the coordinates are marked suspect (persisted), and `coordinates suspect` is logged. Otherwise both are rejected.
- **v1 fallback** stays one 30-day request (spec 006); 90-day windows are not needed for the 30-day goal.
- The store is written once at the end, if anything changed (FR-008).

**Request budget.** Normal week: **1** (one İmsakiyem v2 call at the weekly night minute). A refresh: at most İmsakiyem v2 + v1 + emushaf + 2 Aladhan months + 1 district detail = **6**, under the cap of 12. Worst week (everything failing, today calculated): the hourly retry of FR-003, which is exempt from the 24-hour skip (FR-013 exception, analysis C1), asks only İmsakiyem v2 and emushaf: 168 × 2 = 336 requests, plus the Aladhan months (asked only while a month has a day without any record). Without a calculated today, the retry schedule, the 24-hour skip and the 30-minute cool-downs keep it far lower. The planner tests fail above **400 requests a week** (about one every 25 minutes; İmsakiyem allows 100 per 5 minutes).

**Stage 1 results (2026-11-26, host).** 35 scenario tests in `test/test_planner` against scripted sources, all passing; `pio test -e native` 348/348; firmware 1 745 120 B. Measured by the budget test: a normal week **1** request (the weekly refresh at 03:41 of day 7); a week with every source failing and today calculated **360** requests in 169 refreshes, at most 5 in one; both mirrors failing without coordinates (ordinary backoff and the 24 h skip) **33** requests. Year end (İmsakiyem to 31.12, emushaf to 20.01): `imsakiyem 2026-12-20..2027-12-20 ok`, `emushaf 2026-12-20..2027-01-19 ok`, outcome complete, no retry; then one `low coverage` refresh each night while under 30 days. Found and fixed: a retry that fell in the night window was followed a minute later by the night refresh; any refresh that starts inside the window now counts as that night's.

## R7. Persistent refresh state (FR-014, 04 §8)

**Decision.** A small file `/littlefs/refresh.bin` (120 bytes as implemented: a 12-byte header and a 108-byte payload with magic `NVPR`, version 1, CRC-32, then last success, last attempt, last outcome, retry step, next retry, per-provider failure count and cool-down deadline, calibration offsets with the day count and the district they belong to, Hijri offset, coordinates-suspect flag), saved by the same write-new, verify, rename path as the store, generalised from `store.cpp`. Written at the end of each refresh and when calibration changes, never more often than a refresh. A bad or missing file means "never refreshed": the boot rules then refresh at once.

**Alternatives.** In the store header (store format 2): every failure would rewrite 8 kB; and the counters are not per district. NVS: possible, but the atomic LittleFS path is already proven with a power-loss test.

## R8. Calculated records and their Hijri date (02 §8 line "compute only as a fallback")

**Decision.**
- Aladhan records are stored (they cost requests); the device's own calculation is **not stored**: the worker computes today..today+7 into the RAM copy when today has no record, coordinates exist and the clock is valid, also with no network (US4). It never replaces a stored record (rank 10).
- Calibration (04 §3.5): offsets recomputed after each merge when at least 14 Diyanet-derived records exist (median over the most recent 30); ±10 limit; persisted (R7) so that they survive when the store has no Diyanet-derived records left.
- **Hijri for calculated records**: the arithmetic (tabular) Islamic calendar, shifted by a day offset learned from the latest Diyanet-derived record (persisted). Shown only in the `calculated` state, as 02 allows. Aladhan's Hijri is not used (R2).

## R9. System states and screen

**Decision.** `select_state` already evaluates the full order of 02 §6 (spec 003); this feature feeds its real inputs:
- `today_rank` from today's record (10, 20 or 40);
- `refresh_failed_or_wifi_down` = last outcome not `complete`, or Wi-Fi down;
- `stored_days_after_today` = consecutive days after today with any record (it was fixed at 400).

The `≈` prefix and the 44 px size of reference 12, and the status texts of 11 and 12, already exist (spec 003, snapshots 11 and 12). The status panel names come from 02 §9.5 (`tr.h` has all five).

## R10. Development commands

**Decision.** Extend the `data` family (contracts/serial-console.md): `data block <imsakiyem|emushaf|aladhan>`, `data unblock <name|all>`, `data plan` (planner state, next scheduled refresh in local time, retry step, cool-downs, failure counters, blocked sources), `data calib` (coordinates with origin, offsets, days used, Hijri offset), and the mismatch count in `data show`. Blocks are kept in RAM only, so a restart clears them.

## R12. Device integration (T036..T041, 2026-11-26)

**HTTP timeouts (FR-010).** ESP-IDF 5.5.5 `esp_http_client_config_t` has a single `timeout_ms` ("Network timeout in milliseconds", `components/esp_http_client/include/esp_http_client.h` line 196); `esp_http_client.c` passes it to `esp_transport_connect()` (line 1629), and `esp_http_client_set_timeout_ms()` (header line 592) changes it on an open client. `src/platform/https_fetch.cpp` therefore opens with 10 s (connect and TLS handshake) and sets 15 s right after the connection is open (headers and body reads).

**On the panel (COM4, firmware 1 809 472 B; log lines):**
- First start of this firmware: `refresh state missing`, `night minute 03:35`, `store_9206.bin loaded, 366 records`, `coordinates 39.92063 32.85403 (table)`; 2 s after `wifi: connected`: `refresh started (boot: never refreshed)`, `imsakiyem ok, 366 records, 1 requests, 2.1 s`, `calibration 0 0 0 0 0 0 from 30 days, hijri +1`, `refresh complete ... heap min 22 kB`. Report: `next 04.12 03:35`.
- `data block imsakiyem`, `data refresh`: `imsakiyem blocked`, `emushaf ok, 26 records, 1 requests, 1.6 s`, `refresh complete` (the store still reached 30 days); screen unchanged.
- `data block emushaf`, `data erase`: at once `screen: main (state calculated)`, `8 records 2026-11-26..2026-12-03, source local` (the device's own days, no network needed).
- `data refresh` with both blocked: `aladhan ok, 37 records, 1 requests, 2.6 s` (November and December, 2 requests in all), `refresh failed ... retry 26.11.2026 12:18 (today calculated, hourly)`. TLS to `api.aladhan.com` works with the bundled CA store.
- `data unblock all`, `data refresh`: `imsakiyem ok`, `refresh complete`, `screen: main (state ok)`.
- Free internal heap during a refresh: the lowest since start stays 22 kB (as in spec 006, mbedTLS in PSRAM).

**Found:** after a calculated period, yesterday's record stays Aladhan's (İmsakiyem is asked from today on), so between midnight and İmsak the night's "yesterday Yatsı" comes from a calculated record for one night. **Fixed by T045:** İmsakiyem is asked from yesterday (`from` = today - 1, `to` = from + 365, still within its 366-day limit). On the panel: `data show` went from `(imsakiyem 366, aladhan 1)` to `(imsakiyem 367)` after one refresh, so the service answers a start date in the past.

**An unexplained crash (open).** On the first start after flashing the T045 build, the network task stopped with `Guru Meditation Error: Core 0 panic'ed (IllegalInstruction)` about 2 s after start, right after `data show`. The core dump (decoded with `esp-coredump` and `xtensa-esp32s3-elf-gdb`) puts it in `make_view` (`src/app/data_service.cpp:435`, reading the store), task `net`, 3 568 B of 8 704 B stack used. That function only reads memory, so an illegal instruction there suggests the code itself was read wrongly; this build runs its code from PSRAM (`CONFIG_SPIRAM_XIP_FROM_PSRAM=y`, `CONFIG_SPIRAM_FETCH_INSTRUCTIONS=y`, octal PSRAM at 80 MHz). Not reproduced in 9 further starts, 6 of them with `data show` and `data plan` during start-up. The cause is not known; the report line `reset: panic, core dump saved` shows a repeat, and the dump can be read with the same tools.

## R13. The 04 §12 table, row by row (SC-007, T044)

| Row | Situation | Host test |
|---|---|---|
| 1 | Everything works: `ok` | `test_planner` `test_all_up_only_imsakiyem`; `test_screen` `test_ok` |
| 2 | Wi-Fi down, store covers today: `offline` | `test_screen` `test_each_row`; `test_planner` `test_no_refresh_without_clock_location_or_link` |
| 3 | Wi-Fi down long, store nearly exhausted: `cache_ending` | `test_screen` `test_each_row`, `test_first_match_wins`; `test_store` `test_consecutive_days_after` |
| 4 | One mirror down: nothing | `test_planner` `test_imsakiyem_down_emushaf_covers`, `test_malformed_mirror_other_used` |
| 5 | Both mirrors down, store covers today: nothing yet, retries | `test_planner` `test_both_mirrors_down_store_covers_today` |
| 6 | Both down, today not covered: `calculated`, back by itself | `test_planner` `test_calculated_then_published_returns_within_an_hour`, `test_calculated_mirrors_never_skipped`; `test_calc` `test_fill_local_never_replaces` |
| 7 | No source and no coordinates: `no_data` | `test_planner` `test_no_coordinates_no_data`, `test_no_coordinates_no_aladhan` |
| 8 | Power cut, no Wi-Fi, no RTC: `no_time` | `test_screen` `test_each_row`; `test_planner` `test_no_refresh_without_clock_location_or_link` |
| 9 | Board clock valid | spec 005 `test_time` (board-clock and correction tests) |

## R11. Terms of use (04 §13 item 6)

- **İmsakiyem**: the project's code is GPL-3.0; the data is published as an open data set; 100 requests / 5 min; a contact e-mail is given. We use only the API, no code.
- **emushaf**: no terms or limit published; author named.
- **Aladhan**: no terms read beyond the rate-limit headers (12/s).
- The firmware is for personal use; 04 §13's note stands (contact the maintainers before any wider distribution).
