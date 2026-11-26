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
