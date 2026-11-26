# Data model: Real prayer times

The engine's types (`engine::DayRecord`, `Date`, `HijriDate`, `SourceId`, `rank()`) come from Slice 2 and are reused unchanged.

## Fetched batch (`lib/providers`)

```text
ParseEvent stream -> DayRecord callback
BatchResult { ok, reason, records_seen, district_seen, success_flag, error_code[32] }
Reason { Ok, Http(status), NotSuccess, ErrorCode, Malformed, Truncated, WrongDistrict, Empty, NoFutureDate,
         BadRecord(ENG-01), BadDate, DuplicateDate, DateTooFar, DateTooOld, TooMany }
```

- **Parsers:** `parse_imsakiyem_v2` and `parse_imsakiyem_v1` are fed chunks. They emit one `DayRecord` per completed element, with source `SourceId::Imsakiyem` and rank 40.
- **`validate_batch(records, n, requested_district, district_seen, today)`** applies research R4 and returns the first failing reason.
- **Limits:** at most 400 records per batch; more is `TooMany`.

## Store (`lib/core/src/nvp/store`)

```text
StoreFile (little-endian)
  header 32 B: 0 "NVPS" | 4 u16 version=1 | 6 u16 count | 8 u32 district_id | 12 u32 crc32(records)
               | 16 i64 last_refresh_utc | 24 i32 first_day | 28 i32 last_day
               (the planned two reserved u16 fields were dropped: with them the header was 36 B)
  records count x 20 B: u16 day_since_2020 | 6 x u16 minutes | u8 h_day | u8 h_month | u16 h_year
               | u8 source_id | u8 flags (bit0 hijri present)
Store { district_id, last_refresh_utc, records[<=400] sorted by date }
```

- **`load(FileSystem&, district_id, Store&)`** returns `Ok`, `Missing`, `BadMagic`, `BadVersion`, `BadCrc` or `Truncated`. A bad file is never returned as data.
- **`save(FileSystem&, const Store&)`** writes `store_<id>.new`, closes it, reads it back, checks the CRC, then renames it over `store_<id>.bin` (research R5). It returns `Ok` or the failing step; on failure the old file is untouched.
- **`merge(Store&, incoming, n, fetch_utc, today) -> MergeStats { added, replaced, kept, mismatches, pruned }`** applies the rules of 04 §4.
- **Fetch time** (analysis I1): the store keeps one time per file, the last successful refresh, not one per record. For 04 §4's "equal rank with a later fetch replaces", the incoming batch is always the later fetch, so an equal-rank incoming record replaces the stored one.
- **`covers(const Store&, Date)`**; **`find(const Store&, Date) -> const DayRecord*`**.
- **`FileSystem`** (the interface): `open(path, mode)`, `read`, `write`, `close`, `rename`, `remove`, `exists`. Its fake fails at the Nth call.

## Published data (worker → UI, research R7)

```text
DataView { version, district_id, count, records (PSRAM copy), last_refresh_utc, last_day,
           wifi_down_since_ms (0 = up), refreshing }
```

## State inputs (FR-013)

- `today_present` = the store has a record for today.
- `wifi_down_minutes` = minutes since the worker saw the link go down in running mode.
- `refresh_failed_or_wifi_down` = false and `stored_days_after_today` = large until Slice 7.

## Overlay (FR-014)

- `source` = the 02 §9.5 name for today's record's `SourceId`.
- `last_update` = `DD.MM.YYYY HH:MM` (local).
- `last_day` = `DD.MM.YYYY`.
- `—` when absent.
