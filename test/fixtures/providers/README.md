# Recorded provider answers (spec 006 research R1)

Real answers, saved byte for byte on 2026-11-24 with the User-Agent `NamazVaktiPanel/0.1.0 (development)`,
for Ankara (district 9206). Tests make altered copies of them in memory (truncated, reordered, corrupted);
the files themselves are never edited. Regenerate only by fetching again and recording the date here.

| File | Request |
|---|---|
| `imsakiyem/v2_times_9206_2026-11-24_2026-12-31.json` | `GET https://ezanvakti.imsakiyem.com/api/v2/times?district=9206&from=2026-11-24&to=2026-12-31` (200, 38 records, 8 088 B) |
| `imsakiyem/v2_times_9206_2026-11-24_2027-03-31.json` | same with `to=2027-03-31` (200, 128 records: 2027 is already published) |
| `imsakiyem/v2_times_district_not_found.json` | `district=999999` (404, `DISTRICT_NOT_FOUND`) |
| `imsakiyem/v1_range_9206_2026-11-24_2026-12-23.json` | `GET https://ezanvakti.imsakiyem.com/api/prayer-times/9206/range?startDate=2026-11-24&endDate=2026-12-23` (200, 30 records, 24 437 B) |
| `emushaf/vakitler_9206_2026-11-24.json` | `GET https://ezanvakti.emushaf.net/vakitler/9206` (200, 32 records from 2026-11-19; for Slice 7) |

## Added for Slice 7 (spec 007 research R1), recorded 2026-11-26

User-Agent `NamazVaktiPanel/0.7 (ESP32-S3; personal device)`. Ankara (district 9206), coordinates from İmsakiyem.

| File | Request |
|---|---|
| `imsakiyem/v2_district_9206.json` | `GET https://ezanvakti.imsakiyem.com/api/v2/locations/districts/9206` (200, `coordinates{lat, lon, confidence:"high"}`) |
| `imsakiyem/v2_district_100059.json` | same for Kadıköy, an entry with its own id copied from its centre (200) |
| `imsakiyem/v2_district_not_found.json` | `districts/999999` (404, `DISTRICT_NOT_FOUND`) |
| `emushaf/vakitler_999999_502.txt` | `GET https://ezanvakti.emushaf.net/vakitler/999999` (502, `text/plain`, `error code: 502`) |
| `aladhan/calendar_2026_11_9206.json` | `GET https://api.aladhan.com/v1/calendar/2026/11?latitude=39.92063&longitude=32.85403&method=13&school=0&timezonestring=Europe/Istanbul` (200, 30 days) |
| `aladhan/calendar_2026_12_9206.json` | same for month 12 (200, 31 days) |
| `aladhan/timings_by_address_ankara.json` | `GET https://api.aladhan.com/v1/timingsByAddress/26-11-2026?address=Ankara, Ankara, Türkiye&method=13` (200, but `meta.latitude` 8.8888888, `meta.longitude` 7.7777777: placeholder values) |
| `aladhan/timings_by_address_503.json` | the same form with an unknown address (503, `Geocoding is temporarily unavailable`) |

`GET https://ezanvakti.emushaf.net/vakitler/9206` on 2026-11-26 returned the same bytes as `emushaf/vakitler_9206_2026-11-24.json` (served from a cache, `max-age=432000`), so it was not saved again.

## Added for Slice 7 (spec 007 research R1), recorded 2026-11-26

User-Agent `NamazVaktiPanel/0.7 (ESP32-S3; personal device)`. Ankara (district 9206), coordinates from İmsakiyem.

| File | Request |
|---|---|
| `imsakiyem/v2_district_9206.json` | `GET https://ezanvakti.imsakiyem.com/api/v2/locations/districts/9206` (200, `coordinates{lat, lon, confidence:"high"}`) |
| `imsakiyem/v2_district_100059.json` | same for Kadıköy, an entry with its own id copied from its centre (200) |
| `imsakiyem/v2_district_not_found.json` | `districts/999999` (404, `DISTRICT_NOT_FOUND`) |
| `emushaf/vakitler_999999_502.txt` | `GET https://ezanvakti.emushaf.net/vakitler/999999` (502, `text/plain`, `error code: 502`) |
| `aladhan/calendar_2026_11_9206.json` | `GET https://api.aladhan.com/v1/calendar/2026/11?latitude=39.92063&longitude=32.85403&method=13&school=0&timezonestring=Europe/Istanbul` (200, 30 days) |
| `aladhan/calendar_2026_12_9206.json` | same for month 12 (200, 31 days) |
| `aladhan/timings_by_address_ankara.json` | `GET https://api.aladhan.com/v1/timingsByAddress/26-11-2026?address=Ankara, Ankara, Türkiye&method=13` (200, but `meta.latitude` 8.8888888, `meta.longitude` 7.7777777: placeholder values) |
| `aladhan/timings_by_address_503.json` | the same form with an unknown address (503, `Geocoding is temporarily unavailable`) |

`GET https://ezanvakti.emushaf.net/vakitler/9206` on 2026-11-26 returned the same bytes as `emushaf/vakitler_9206_2026-11-24.json` (served from a cache, `max-age=432000`), so it was not saved again.
