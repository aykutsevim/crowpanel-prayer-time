# Data sources, cache, refresh and fallback

Everything in this file about third-party services was checked against their public documentation on **17 November 2026** and re-checked on **20 November 2026**, when the İmsakiyem API description was read in full (it now has version 2 endpoints, see §3.1). Items marked ⚠ were not verified with a live call and must be confirmed during planning (§13). Services change; when the documentation and this file disagree, the live documentation wins and this file gets corrected.

## 1. Principles

1. **Diyanet is the authority.** The times people in Türkiye check against are the ones Diyanet publishes per district. The device shows those whenever it can get them.
2. **Fetch rarely, store a lot.** One successful refresh should cover months. The screen is always drawn from the local store.
3. **Fail over silently, degrade loudly.** Switching between equivalent sources is invisible. Dropping to calculated times is clearly labelled.
4. **Be a polite client.** These are free services, two of them run by volunteers. Low request rates, a descriptive `User-Agent`, no retries in tight loops.

## 2. Providers at a glance

| Rank | Id | What it is | Auth | One fetch covers | Published limits | Role |
|---|---|---|---|---|---|---|
| 50 | `DIYANET` | Diyanet’s own “Awqat Salah” REST API | account (application form), bearer token | a month, or a date range | per-service quotas, see §3.3 | optional; used first when the owner has credentials |
| 40 | `IMSAKIYEM` | community API republishing Diyanet tables | none | up to 366 days in one request | 100 requests / 5 min | default first choice; also the source of district coordinates and the place list |
| 40 | `EMUSHAF` | community API republishing Diyanet’s monthly table | none | about 30 days from today | none on its site; a third-party project reports about 100 requests / 15 min ⚠ | default second choice; extends coverage |
| 20 | `ALADHAN` | Aladhan API, calculated with its Diyanet method | none | a calendar month | none stated ⚠ | first calculated fallback |
| 10 | `LOCAL` | astronomical calculation on the device | n/a | any date | n/a | last resort, works offline |

Rank ≥ 40 is “Diyanet-derived”: the UI treats it as official. Rank < 40 is “calculated”: the UI enters the `calculated` state when today’s record has such a rank.

`IMSAKIYEM` and `EMUSHAF` republish the same Diyanet data and have equal rank. `IMSAKIYEM` is tried first because one fetch covers the rest of the year, which gives the longest offline autonomy. Neither has a service-level guarantee, which is why both are used and two calculated fallbacks sit behind them.

## 3. Provider details

### 3.1 İmsakiyem (`IMSAKIYEM`)

- Documentation: `https://ezanvakti.imsakiyem.com/` (README, dated 3 January 2026). Swagger UI at `/api-docs/`, machine-readable description at `/api-docs/json`. The description read on 20 November 2026 is **API version 1.1.0** and states that version 1 endpoints (`/api/…`) keep working unchanged and new endpoints live under `/api/v2/…`. The README does not mention version 2 yet; trust the API description.
- Base URL: `https://ezanvakti.imsakiyem.com`
- Rate limit: 100 requests per 5 minutes; HTTP 429 when exceeded.
- District ids are Diyanet’s own: the same numbers appear in the URLs of Diyanet’s prayer-times website and in `EMUSHAF`.
- Yearly data is loaded by the maintainer once per year (the 2026 set was added on 3 January 2026). **Expect the data to end on 31 December until the next year is loaded.** This is the main reason coverage must be extended from `EMUSHAF` (§5.2).

**Version 2 endpoints (preferred).** Envelope: `{ success, data, meta: { version } }` on success and `{ success: false, error: { code, message, details }, meta }` on failure, with the request id in the `x-request-id` header. Documented error codes include `VALIDATION_ERROR`, `INVALID_DATE`, `RANGE_TOO_LARGE`, `DISTRICT_NOT_FOUND`, `NO_TIMES_FOR_RANGE`, `LOCATION_REQUIRED`, `AMBIGUOUS_LOCATION`, `NO_DISTRICT_NEARBY`.

| Endpoint | Use in this project |
|---|---|
| `GET /api/v2/times?district={id}&from=YYYY-MM-DD&to=YYYY-MM-DD` | prayer times for a range of **at most 366 days** in one request. Also accepts `date` with `period` = `daily`, `weekly`, `monthly`, `yearly`, or `lat` + `lon` instead of `district`. The description says the `times` field of the answer is always an array. |
| `GET /api/v2/locations/districts/{id}` | district detail **with coordinates, a confidence level, the time zone, region and a display name**. Used for §6. |
| `GET /api/v2/locations/districts?countryId=2&page=N&limit=500` | paged district list (limit 1…500, default 50), filterable by `stateId`, `region`, `q`. Used to generate the bundled place list (§9). |
| `GET /api/v2/locations/countries`, `/countries/{id}` | country list and detail |
| `GET /api/v2/locations/search?q=…`, `/nearest?lat=…&lon=…` | not needed |
| `GET /api/v2/times/next` | not needed; the device computes this itself |

⚠ The version 2 **response bodies were not seen**; only the endpoint list and parameters are documented. Record real answers of `times` and `locations/districts/{id}` as fixtures before writing the parser (field names for the date, the six times, the Hijri date, coordinates and confidence).

**Version 1 endpoints (fallback inside this provider).**

- Times: `GET /api/prayer-times/{districtId}/{period}` with `period` one of `daily`, `weekly`, `monthly`, `yearly`, and according to the README also `range`. Query: `startDate`, `endDate` (`YYYY-MM-DD`), `page`, `limit` (README default 100).
- Locations: `GET /api/locations/countries`, `/states?countryId=2`, `/districts?stateId={id}`; items carry `_id`, `name`, `name_en` and parent ids; the list answer is `{ success, message, data: [...] }`.

Verified answer of `GET /api/prayer-times/9549/daily` (trimmed):

```json
{
  "success": true, "code": 200,
  "data": [{
    "district_id": { "_id": "9549", "name": "SULTANBEYLİ",
                     "state_id": { "_id": "539", "name": "İSTANBUL", "country_id": "2" } },
    "date": "2026-10-12T00:00:00.000Z",
    "hijri_date": { "day": 1, "month": 5, "month_name": "Cemaziyelevvel", "year": 1448,
                    "full_date": "1 Cemaziyelevvel 1448" },
    "meta": { "source": "Diyanet İşleri Başkanlığı" },
    "times": { "imsak": "05:40", "gunes": "07:04", "ogle": "12:55",
               "ikindi": "16:02", "aksam": "18:35", "yatsi": "19:54" }
  }],
  "meta": { "totalCount": 1 }
}
```

Version 1 mapping: `times.*` → the six times; calendar date = the first ten characters of `date` (ignore the time and zone); Hijri = `hijri_date.day`, `.month`, `.year`. Each record repeats the whole district object, so parse as a stream and keep only `date`, `hijri_date` and `times`.

Known rough edges, all reasons to validate strictly (§7):

- The live version 1 answer carries `meta.totalCount`, while the API description declares a `pagination` object (`currentPage`, `totalPages`, `totalItems`, `itemsPerPage`) and names the district field `districtId`. Do not depend on paging metadata; request windows small enough to fit one page, or page until an empty page.
- A third-party client reports that an unknown district id yields `success: true` with an empty `data` array. **An empty result is a failure**, not “no times”.

Plan for a refresh: one version 2 `times` call with `from` = today and `to` = 31 December. If version 2 is unavailable or its answer does not validate, fall back to version 1 `range` in windows of at most 90 days.

### 3.2 EzanVakti / emushaf (`EMUSHAF`)

- Documentation: `https://ezanvakti.emushaf.net/` (version 0.7.0). OpenAPI/Swagger at `/docs`, ReDoc at `/redoc`.
- Base URL: `https://ezanvakti.emushaf.net`
- Endpoints: `GET /ulkeler`, `GET /sehirler/{countryId}`, `GET /ilceler/{provinceId}`, `GET /vakitler/{districtId}`.
- `/vakitler/{districtId}` returns a JSON array of about 30 daily records starting today.
- Fields confirmed in use: `MiladiTarihKisa` (`DD.MM.YYYY`), `Imsak`, `Gunes`, `Ogle`, `Ikindi`, `Aksam`, `Yatsi` (`HH:MM`), `HicriTarihUzun` (for example `7 Cemaziyelahir 1448`). ⚠ Read the complete schema from `/docs`.
- Documented quirk: `MiladiTarihUzunIso8601` always carries Türkiye’s UTC offset regardless of location. **Take the calendar date from `MiladiTarihKisa` and ignore offsets.**
- Province list sample: `{"SehirAdi":"TEKİRDAĞ","SehirAdiEn":"TEKIRDAG","SehirID":"572"}`.
- District ids are the ones in the URLs of Diyanet’s own prayer-times site (a third-party project notes that `/en-US/9206/prayer-time-for-ankara` is Ankara and 9206 is its district id here). This gives a simple manual cross-check: open `https://namazvakitleri.diyanet.gov.tr/` for the district and compare the number in the address.
- Rate limit: none is documented; a third-party project reports about 100 requests per 15 minutes. ⚠ Design for one call per refresh.

### 3.3 Diyanet official API (`DIYANET`, optional)

- Home and documentation: `https://awqatsalah.diyanet.gov.tr/` (interactive reference under `/scalar`).
- Access requires an account. The owner applies with the form linked on that page and receives a user name and password. Without credentials this provider is skipped.
- Authentication: `POST /Auth/Login` with the credentials returns an access token and a refresh token; `GET /Auth/RefreshToken/{refreshToken}` renews. The older PDF guide states a token lifetime of 45 minutes. ⚠ Confirm request and response shapes in the live reference.
- Times: `GET /api/PrayerTime/Daily/{cityId}`, `/Weekly/{cityId}`, `/Monthly/{cityId}`, and `POST /api/PrayerTime/DateRange`. “City” in this API means the district-level place.
- Places: country, state and city list endpoints under `/api/Place/…`, plus `CityDetail`; `v2` variants return parents inline.
- Quota: `GET /api/Quota/My` reports remaining quota.
- **Quotas are tight.** The documentation states a limit of 10 calls per service, counted separately per parameter value (100 during the first 15 days of an account), and 10 calls per month per place for `DateRange`. The counting period for the general limit is not spelled out; treat it as per day at best and design for far fewer.

Rules for this provider: at most one `Monthly` call per refresh and one refresh per week; no polling of `Daily`; log in only when a fetch is due; never retry a failed login more than once per day; store the password only in device settings. ⚠ Confirm that `cityId` equals the district id used by the mirrors for the configured district (compare names) before trusting it.

Response fields to expect per day (from client libraries, ⚠ confirm): `fajr`, `sunrise`, `dhuhr`, `asr`, `maghrib`, `isha`, `gregorianDateShort`, `gregorianDateLongIso8601`, `hijriDateShort`, `hijriDateLong`. Mapping: `fajr` → İmsak, `sunrise` → Güneş, `dhuhr` → Öğle, `asr` → İkindi, `maghrib` → Akşam, `isha` → Yatsı.

### 3.4 Aladhan (`ALADHAN`)

- Documentation: `https://aladhan.com/prayer-times-api`; methods at `https://aladhan.com/calculation-methods`.
- Base URL: `https://api.aladhan.com/v1`. No key.
- Method `13` is “Diyanet İşleri Başkanlığı, Turkey (experimental)”. The service applies built-in minute offsets for this method to approximate Diyanet’s table. Results usually agree with Diyanet within a few minutes; they are not the published values.
- Calendar for a month: `GET /calendar/{year}/{month}?latitude={lat}&longitude={lon}&method=13&school=0&timezonestring=Europe/Istanbul`. The `timezonestring` parameter name is used by several open-source clients of this API. A date-range form also exists, `GET /calendar/from/{DD-MM-YYYY}/to/{DD-MM-YYYY}`; a user reported it returning one extra day before the requested start, so drop records outside the requested range.
- By address (used once to learn coordinates, §6): `GET /timingsByAddress/{DD-MM-YYYY}?address={text}&method=13`.
- Response: `data[]` (calendar) or `data` (single day) with `timings` (`Fajr`, `Sunrise`, `Dhuhr`, `Asr`, `Maghrib`, `Isha`, and others), `date.gregorian.date` (`DD-MM-YYYY`), `date.hijri`, and `meta.latitude`, `meta.longitude`. `meta` also reports `timezone`, `method`, `school` and the per-prayer `offset` minutes the service applied. ⚠ Timings may carry a suffix such as `06:16 (+03)`; parse the first five characters. (An `iso8601=true` parameter switches to full timestamps; it is not needed.)

Mapping: **`Fajr` → İmsak** (do not use Aladhan’s `Imsak` field, which is an earlier precautionary time), `Sunrise` → Güneş, `Dhuhr` → Öğle, `Asr` → İkindi, `Maghrib` → Akşam, `Isha` → Yatsı. Do not pass `tune`. `school=0` selects the standard Asr used by Diyanet.

### 3.5 On-device calculation (`LOCAL`)

- Standard solar-position prayer-time calculation for the stored coordinates and date. Use a maintained, tested implementation (for example the C or C++ port of Batoul Apps’ *adhan*) rather than writing the astronomy from scratch; pin its version.
- Parameters matching the Diyanet method as implemented by common open-source libraries (⚠ confirm against the library you vendor): Fajr angle 18°, Isha angle 17°, standard Asr (shadow factor 1), minute adjustments sunrise −7, Dhuhr +5, Asr +4, Maghrib +7, Fajr 0, Isha 0.
- **Self-calibration.** Whenever at least 14 Diyanet-derived records are in the store, compute for each of the six times the median difference (official minus calculated, in whole minutes) over the most recent 30 such days and save the six offsets. Apply them to `LOCAL` results. This keeps the last-resort fallback within about a minute of the official table for the district. Offsets beyond ±10 minutes are rejected as a sign of wrong coordinates.
- `LOCAL` needs valid time and coordinates, nothing else.

## 4. Merge rules

- The store holds at most one record per date.
- An incoming record replaces a stored one if its rank is higher, or equal with a later fetch time. A lower-ranked record never replaces a higher-ranked one.
- When a rank-40 record arrives for a date that already has a rank-40 record from the other mirror with different times, store the newer one and log a `MISMATCH` line with both sets of times. The count of mismatches since boot is exposed in the status API. A mismatch of more than 2 minutes on any time for today is worth the owner’s attention and is shown in the settings page.
- Records for dates before yesterday are pruned on every write.

## 5. Refresh policy

### 5.1 Triggers

| Trigger | Condition |
|---|---|
| Setup completed or location changed | immediately |
| Boot, after Wi-Fi and time are up | if today has no Diyanet-derived record, or fewer than 30 days ahead are stored, or the last successful refresh is more than 7 days old |
| Weekly | 7 days after the last successful refresh, at a device-specific minute between 03:00 and 04:59 local time (derived from the MAC address, so devices do not all call at once) |
| Low coverage | fewer than 30 days ahead stored: once per day in the night window until fixed |
| Wi-Fi reconnect after an outage | only if one of the boot conditions holds, and at most once per 10 minutes |
| Manual | `Şimdi güncelle` in the settings page; at most once per minute |

### 5.2 One refresh

Goal of a refresh: the store holds Diyanet-derived records from today through at least today + 30 days, and as far ahead as the first provider offers.

1. If credentials exist: `DIYANET`, one `Monthly` call.
2. `IMSAKIYEM`: one version 2 `times` call from today to 31 December; on failure, version 1 `range` in windows of ≤ 90 days (§3.1).
3. If coverage is still under 30 days ahead, or step 2 failed: `EMUSHAF`, one call.
4. If today or tomorrow still has no Diyanet-derived record: `ALADHAN` for the current month, and the next month when today is within 7 days of month end. Requires coordinates.
5. If today still has no record at all: `LOCAL` for today through today + 7 days. Requires coordinates.

Steps 4 and 5 fill gaps only (§4). Each step validates before storing (§7). The store is written once at the end of the refresh if anything changed.

Ordinary cost: 1 to 2 requests per week. Hard cap: 12 requests per refresh.

### 5.3 Failure handling

- A provider call fails on timeout (10 s connect, 15 s read), non-2xx status, a body over 512 kB, a parse error, or validation failure. On HTTP 429 or 503 do not call that provider again for 30 minutes.
- If a refresh ends without reaching its goal, retry after 5 min, 15 min, 1 h, 3 h, then every 6 h. A Wi-Fi reconnect allows one extra attempt.
- A provider that fails five refreshes in a row is skipped for 24 hours, then tried again.
- When calculated records are in use for today, retry the Diyanet-derived providers hourly instead of on the slower schedule.

## 6. Coordinates

`EMUSHAF` and version 1 of `IMSAKIYEM` do not supply coordinates. After the location is set and the network is up, resolve them once:

1. `IMSAKIYEM` version 2 district detail, `GET /api/v2/locations/districts/{id}`: take the coordinates. It also reports a confidence level and the district’s time zone; ⚠ record a real answer to learn the field names and the possible confidence values, and decide in the plan which confidence levels are acceptable.
2. If that fails or the confidence is too low: `ALADHAN` `timingsByAddress` with `address` = `<district>, <province>, Türkiye`; take `meta.latitude` and `meta.longitude`. Then retry with `<province>, Türkiye`.
3. Accept a result only if it lies inside Türkiye’s bounding box (latitude 35.8…42.2, longitude 25.6…44.9).
4. Store the coordinates with the location. The owner can override them in the settings page.

Without coordinates, steps 4 and 5 of a refresh and the plausibility check in §7 are skipped, and the settings page asks for latitude and longitude.

## 7. Validation of a fetched batch

Reject the whole batch if any of these fails:

- every record passes ENG-01 (six strictly increasing times within the day);
- dates parse, are unique, and none is more than 400 days ahead or more than 2 days behind today;
- the batch is not empty and contains today or a later date (an empty list with a success flag is a failure);
- for mirrors and the official API: the place in the response, where present, matches the configured district id;
- if coordinates and a valid clock are available: for the first, middle and last record, each time differs from the uncalibrated `LOCAL` result for that date by no more than 20 minutes. This catches wrong districts, wrong time zones and shifted columns.

Log the reason for any rejection (SYS-05).

## 8. Store

- One store per district on a wear-levelled flash file system, about 400 records at most (well under 16 kB).
- Suggested record (20 bytes, little-endian): day number since 2020-01-01 (u16), six times in minutes (6 × u16), Hijri day (u8), month (u8), year (u16), source id (u8), reserved (u8). Suggested header: magic, format version, district id, record count, write time, CRC-32 of the records. The plan may refine this; DATA-07 (atomic replace, corruption detected by checksum, previous version used if the new one is bad) is the requirement.
- Also persisted with the store: time of the last successful refresh, per-provider failure counters, calibration offsets.
- A store with an unknown format version is ignored and rebuilt.

## 9. Bundled place list

- `places_tr`: the 81 provinces of Türkiye and their districts with Diyanet district ids, embedded in the firmware and served to the portal as `/places.json`.
- Generated on a development machine by a script kept in the repository, from `IMSAKIYEM`: preferably the version 2 district list (`/api/v2/locations/districts?countryId=2&limit=500`, a few pages), otherwise the version 1 endpoints (`states?countryId=2`, then `districts?stateId=…` for each province). Throttled to one request per second. The script records the generation date and a checksum, and fails if the result does not contain exactly 81 provinces or if any province has no districts.
- Compact form, for example `[[provinceId, "İSTANBUL", [[9541, "İSTANBUL"], [9549, "SULTANBEYLİ"], …]], …]`, gzip-compressed in flash. Display names are converted to Turkish title case in the portal (`SULTANBEYLİ` → `Sultanbeyli`), using Turkish casing rules for `I/ı` and `İ/i`.
- Ids are Diyanet’s and are shared between the mirrors (Türkiye = country 2, İstanbul = province 539, Sultanbeyli = district 9549). ⚠ Confirm for a handful of districts against `EMUSHAF` when generating. The İmsakiyem open-data set is reported to hold 880 districts for Türkiye and Northern Cyprus together; a result far from that count is a warning sign.
- **Checked live on 2026-11-24 (spec 004 research R6).** The version 2 district list for country 2 holds 1003 entries in 81 provinces: 869 with Diyanet ids (which agrees with the 880 above) and 134 with ids from 100000 up (for example Kadıköy 100059, Çankaya), whose `times` answer says `"source":{"type":"copied_from_center","center_district_id":"9541"}`. Diyanet does not publish them separately and `EMUSHAF` does not know those ids. The bundled list keeps them under their own names with their centre's Diyanet id (owner decision OD4-3), so both mirrors serve every entry. Province names come from the version 1 `states` list, because the version 2 `state.name` is inconsistent (`Samsun İli`, `Kastamonu`). Names contain `Â` (`KÂĞITHANE`) besides the Turkish set. Generator: `tools/gen_places.py`; output `data/places_tr.json` (81 provinces, 1003 entries, 7.9 kB gzipped).

## 10. HTTP client rules

- HTTPS with certificate verification against a bundled root store (SEC-01), including the certificate's validity dates (`CONFIG_MBEDTLS_HAVE_TIME_DATE=y`, owner decision OD5-1 of spec 005). A request is made only once the clock is valid. The time fallback of TIME-02 therefore reads only the `Date` header of a plain-HTTP answer (`http://ezanvakti.imsakiyem.com/`, else `http://ezanvakti.emushaf.net/`; both answer port 80 with a redirect and a `Date` header, checked 2026-11-24) and uses nothing else from it.
- `User-Agent: NamazVaktiPanel/<version> (ESP32-S3; personal device)`. `Accept: application/json`.
- One request at a time. No parallel connections.
- Stream-parse JSON with a filter for the needed fields. Never buffer more than 64 kB of a response.
- Follow at most two redirects, same host only.

## 11. Hosts contacted

`ezanvakti.imsakiyem.com`, `ezanvakti.emushaf.net`, `api.aladhan.com`, `awqatsalah.diyanet.gov.tr` (only with credentials), and the configured time servers. Nothing else (SEC-02).

## 12. What the owner sees

| Situation | Screen |
|---|---|
| Everything works | `ok`, `Son güncelleme …` |
| Wi-Fi down, store covers today | `offline`; times unchanged |
| Wi-Fi down for long, store nearly exhausted | `cache_ending` with days left |
| One mirror down | nothing; the other is used |
| Both mirrors down, store covers today | nothing yet; retries continue |
| Both mirrors down, store does not cover today | `calculated`, times marked `≈`; back to normal on its own when a mirror returns |
| No source at all and no coordinates | `no_data` |
| Power cut, no Wi-Fi afterwards, no RTC module | `no_time` until Wi-Fi returns |
| Power cut, no Wi-Fi afterwards, the board's clock valid (DIS02170A V1.4) | the time from the board's clock; corrected when Wi-Fi returns (spec 003 OD3-1, spec 005) |

## 13. Verify before coding

Do these during `/speckit.plan` for Slice 6 and Slice 7 and record the results in the feature’s `research.md`:

1. Fetch the İmsakiyem API description again and check its version. Make one version 2 `times` call with `from`/`to` and one `locations/districts/{id}` call for the configured district; record both answers as fixtures and write down the field names. Make one version 1 `range` call as well, for the fallback path.
2. Fetch the emushaf OpenAPI schema; confirm field names for places and times.
3. For the configured district, fetch 30 days from both mirrors and diff them. If they differ, stop and report to the owner before going further.
4. Call Aladhan for the same district and month; confirm parameter names, the timing string format, and how far the results are from the mirrors.
5. If the owner has Diyanet credentials: read the live reference, confirm login and `Monthly` shapes and the quota endpoint, and compare one month with the mirrors.
6. Check current terms or usage notes on each service’s site. These are volunteer-run or free services; if this firmware is ever distributed beyond personal use, contact the maintainers first.
