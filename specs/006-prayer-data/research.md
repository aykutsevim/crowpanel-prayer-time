# Research: Real prayer times

Live requests were made on 2026-11-24 with the User-Agent `NamazVaktiPanel/0.1.0 (development)`, for the owner's district, Ankara centre (9206). Library facts come from the locally installed, pinned sources. Items marked ⚠ need a measurement on the device.

## R1. Live verification (04 §13 items 1, 3, 6)

**What was fetched.** All answers are recorded byte for byte under `test/fixtures/providers/` (README there).
- **İmsakiyem API description** (`/api-docs/json`): OpenAPI 3.0.0, API version **1.1.0**, unchanged since 20 November.
- **İmsakiyem v2** `GET /api/v2/times?district=9206&from=2026-11-24&to=2026-12-31`:
  - status 200, 8 088 bytes, 38 records;
  - envelope `{success, data:{location, source:{type:"diyanet", provider}, times:[…]}, meta:{version, count, from, to, timezone:"Europe/Istanbul"}}`;
  - each record `{date:"2026-11-24", hijri:{day, month, month_name, year}, imsak:"06:08", gunes, ogle, ikindi, aksam, yatsi, flags:[]}`;
  - headers `X-API-Version: 2`, `RateLimit-Policy: 100;w=300`, `Cache-Control: public, max-age=3600`.
- **İmsakiyem v2 to 2027-03-31**: 200, **128 records**, all with `source.type "diyanet"` and empty `flags`. **The 2027 calendar is already published on 24 November 2026.** 04 §3.1's "expect the data to end on 31 December" is therefore wrong at this time of year and is corrected (T-task).
- **İmsakiyem v2, unknown district** (`999999`): **404** `{"success":false,"error":{"code":"DISTRICT_NOT_FOUND",…},"meta":{"version":"2"}}`. The third-party report of "success with an empty list" (04 §3.1) does not hold for v2.
- **İmsakiyem v1** `GET /api/prayer-times/9206/range?startDate=2026-11-24&endDate=2026-12-23`:
  - 200, 30 records, 24 437 bytes (about 815 B per record, because each repeats the district object);
  - envelope `{success, code, message, data:[…], meta:{totalCount}, requestId, timestamp}`.
- **emushaf** `GET /vakitler/9206`:
  - 200, 32 records starting **2026-11-19** (five days before today, not today as 04 §3.2 says);
  - fields as in 04 §3.2.

**Comparison (04 §13 item 3).**
- **v2 and emushaf:** 27 overlapping days (24 Nov – 20 Dec), **0 differences** in the six times and the Hijri date.
- **v2 and v1:** 30 days, 0 differences.

The mirrors agree, so planning continues.

**Decision.**
- **v2 is primary** within İmsakiyem: 213 B per record instead of 815, one flat record per day, the source type in the answer, and a proper 404 for unknown districts.
- **v1 `range` is the in-provider fallback**: on a v2 error other than `DISTRICT_NOT_FOUND`, or a v2 answer that fails validation. Windows of at most 30 days (one page; 04 §3.1 advises not relying on paging metadata).

**Range requested:** from today to today + 365 days (the v2 limit is 366 days). The source decides how far it goes; on 24 November that is into 2027.

**04 §13 item 6 (terms):** the İmsakiyem README asks for fair use and states its rate limit (100 per 5 min). One refresh is one request, so no further action is needed for personal use (04 §13 note kept).

## R2. Parsing without holding the answer (FR-003)

**Decision.** A small streaming JSON tokenizer in `lib/providers` (`nvp::providers::JsonStream`), written for this project. It is a SAX-style state machine fed with arbitrary chunks, so a token may split across chunks.
- **What it tracks:** the current key path and depth.
- **What it emits:** values only for the paths a parser asks for:
  - v2: `success`; `error.code`; `data.location.district_id`; `data.source.type`; and per element of `data.times`: `date`, `hijri.day`, `hijri.month`, `hijri.year`, and the six time keys.
  - v1: the equivalents under `data[]`: `date`, `hijri_date.*`, `times.*`, `district_id._id`.
- **No heap:** fixed buffers, a 64-byte string limit, depth ≤ 8.
- **Errors:** malformed or truncated JSON ends the parse with an error.
- **The provider parsers** (`imsakiyem_v2`, `imsakiyem_v1`) map those events to `engine::DayRecord`s through a callback, so not even the record list must be held. The caller collects at most 400 records.
- **Host tests** feed the recorded fixtures in chunks of 1, 7, 64 and 1 024 bytes. Each also gets truncated, reordered, corrupted and wrong-district variants.

**Rationale.**
- 05 §3 asks for "a streaming parser with field filtering".
- A year of v2 is about 78 kB, and v1 is about four times that.
- ESP-IDF's `cJSON` and `jsmn`-style tokenizers need the whole body.

**Alternatives considered.**
- ArduinoJson 7 with a filter: it streams, but builds a document of the filtered result (about 30 kB for a year), and it would be a new dependency for an ESP-IDF project.

## R3. HTTPS client (FR-002, SEC-01..SEC-03)

**Decision.**
- **Client:** `esp_http_client` with `crt_bundle_attach = esp_crt_bundle_attach`. The full bundle is already in use (`CONFIG_MBEDTLS_CERTIFICATE_BUNDLE_DEFAULT_FULL`, Slice 1, about 64 kB of flash, measured in Slice 1's size report).
- **Certificate dates** are checked (OD5-1). Requests run only when `clock::time_valid()`.
- **Interface:** an abstract `nvp::providers::HttpGet` (`get(url, on_status, on_chunk)`) in `lib/providers`. `src/platform/https_fetch` implements it on top of `esp_http_client_open`/`read` in 1 kB chunks, so the body streams into the parser.
- **Request:**
  - `User-Agent: NamazVaktiPanel/<version>` (`nvp::kUserAgent`, SEC-03);
  - no cookies, no other headers;
  - `Accept-Encoding: identity`, because gzip would need the whole body or an inflater;
  - timeout 15 s, redirects not followed.
- **Rules of 04 §10:** one request at a time, on the network worker only.
- **Peak memory:** measured on the device during a fetch with `heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL)` before and after (⚠). For reference, Slice 1's stress HTTPS ran with 55 kB internal free and a 31.7 kB largest block.

**Measured on the device (T021, 2026-11-24).**
- **First attempt failed, then fixed.** With Wi-Fi connected the report showed 41 487 B internal free, largest block 22 528 B (Slice 4 measured 51–55 kB). Every handshake, including Slice 1's `test https`, failed: `esp-x509-crt-bundle: PK verify failed with error 0x4290`, which is `MBEDTLS_ERR_RSA_PUBLIC_FAILED` (0x4280) plus `MBEDTLS_ERR_MPI_ALLOC_FAILED` (0x0010): mbedTLS could not allocate. ESP-IDF 5.5.5 offers the choice `MBEDTLS_MEM_ALLOC_MODE` (`components/mbedtls/Kconfig`); `CONFIG_MBEDTLS_EXTERNAL_MEM_ALLOC=y` (needs `SPIRAM_USE_MALLOC`, which is on) puts mbedTLS's allocations in PSRAM. Set in `sdkconfig.defaults`; certificate checks are unchanged.
- **After the change:** `imsakiyem v2 ok, 366 records stored (2026-11-24..2027-11-24), 2.1 s`, 2.2 s after `wifi: connected`; the lowest internal free heap since start 22 kB; report afterwards 41 395 B free, largest 27 648 B. The picture during a refresh, with TLS now in PSRAM, is an owner check (E9).
- The year asked for (to today + 365) came back whole: 366 records to 24 November 2027, about 78 kB streamed through a 1 kB buffer.

## R4. Validation (FR-006, DATA-06, 04 §7)

**Decision.** `nvp::providers::validate_batch` in `lib/providers`. It is pure and gives one reason code. Whole-batch rejection if:
- any record fails `engine::valid` (ENG-01);
- a date is missing, malformed, or duplicated;
- a date is more than 400 days after or more than 2 days before today;
- the batch is empty, or has no date ≥ today;
- `data.location.district_id` (v2) or `district_id._id` (v1) is not the requested id;
- `success` is not true;
- the JSON ended early.

The comparison with calculated times (04 §7, last bullet) arrives in Slice 7.

## R5. Store and atomic replace (FR-008..FR-011, DATA-07, DATA-08)

**Decision.**
- **One file per district:** `/littlefs/store_<district_id>.bin`.
- **Header (32 bytes, little-endian):**
  - magic `NVPS`, format version `1`;
  - district id (u32), record count (u16), reserved (u16);
  - last successful refresh (UTC seconds, i64);
  - first and last date as day numbers (2 × i32);
  - CRC-32 (IEEE) of the records.
- **Record (20 bytes, 04 §8):**
  - day number since 2020-01-01 (u16);
  - six times in minutes (6 × u16);
  - Hijri day (u8), month (u8), year (u16);
  - source id (u8);
  - Hijri-present flag (u8).

  400 records take 8 kB.
- **Replace procedure:**
  1. Write `store_<id>.new` completely.
  2. Close it; littlefs commits on close.
  3. Read it back and check the CRC.
  4. `rename()` it over `store_<id>.bin`.

  littlefs: "All POSIX operations, such as remove and rename, are atomic, even in event of power-loss. Additionally, file updates are not actually committed to the filesystem until sync or close is called on the file." (`managed_components/joltwallet__littlefs/src/littlefs/README.md` line 112, littlefs 2.11 inside `joltwallet/littlefs` 1.22.3). `lfs_rename` replaces an existing destination file (`lfs.h` line 514).

  `esp_littlefs`'s `vfs_littlefs_rename` refuses while either file is open (`esp_littlefs.c` line 2411), so the store keeps no file open between operations.
- **At start:**
  - a leftover `.new` is deleted;
  - `store_<id>.bin` with a bad magic, an unknown version or a bad CRC is ignored and deleted; the next refresh rebuilds it (FR-009).
- **Location change (DATA-08):** the screen uses only the store for the stored district. The old district's file is deleted only after the first successful write for the new one.
- **Testing:** the format, merge and replace logic live in `lib/core` (`nvp::store`) behind `nvp::store::FileSystem`, a small interface: open, read, write, close, rename, remove. A fake file system in the host tests fails at every single write, close and rename in turn. After each simulated power cut it remounts and checks that the store reads as exactly the old or exactly the new version (SC-004).
- **Scans:** the store write waits for or stops a running Wi-Fi scan, as Slice 4's writes do (FR-011).

**Header as implemented:** 32 bytes, `0 "NVPS" | 4 u16 version | 6 u16 count | 8 u32 district | 12 u32 CRC-32 | 16 i64 last refresh | 24 i32 first day | 28 i32 last day`. The two reserved u16 fields of the first draft were dropped, because with them the header was 36 bytes.

**Stage 1 report (T015, 2026-11-24).** `pio test -e native`: **303 of 303 pass** (9 suites). New in this slice:

| Suite | File | Tests |
|---|---|---|
| test_providers | test_json_stream.cpp | paths and values; chunk sizes 1/7/64/1024; escapes (`\"`, `\\`, `ç`, raw UTF-8, surrogate pair); a value over 64 bytes flagged, not kept; errors (key over 64 bytes, depth 9, missing `}`, unterminated string, empty input, trailing data, syntax); top-level scalars |
| test_providers | test_imsakiyem.cpp | v2 38 records with the first record and Hijri date checked; v2 128 records to 2027-03-31; v2 `DISTRICT_NOT_FOUND`; v1 30 records equal to v2; 1-byte chunks; v1 mixed districts; unreadable fields make invalid records; `parse_hhmm`/`parse_iso_date` |
| test_providers | test_validate.cpp | valid answers (v2 short and long, v1, today + 2); truncated; reordered is ok; duplicate; 401 days ahead (400 ok); all old; no date from today; bad record (`ogle` before `gunes`, `25:00`, a missing time); bad date; empty; wrong district (and none); `success:false` and the error code; not Diyanet; too many; a needed field too long |
| test_providers | test_refresh_flow.cpp | URLs; v2 ok stores; a rejection leaves the store and the file byte for byte unchanged (wrong district, bad record, truncated); `DISTRICT_NOT_FOUND` does not try v1; a v2 TLS error falls back to v1; a v2 503 falls back; v1 wrong district rejected; a save failure leaves the store; a next-day refresh keeps yesterday |
| test_store | test_store_format.cpp | round trip of 400 records; header layout; CRC-32 check value; bad CRC, magic, version, short file, missing, wrong district; `find`/`covers`/`days_after`; a store that ends today; file names; an empty store is not saved |
| test_store | test_merge.cpp | into an empty store; higher rank replaces (even from an older fetch); equal rank with a later fetch replaces; equal rank with an older fetch is kept; lower rank never replaces; rank-40 mismatches counted with both time sets; pruning before yesterday; at most 400 kept, oldest dropped |
| test_store | test_power_loss.cpp | a cut at each of the 59 calls of a 400-record save over an existing store; a cut on a first save; a single I/O error at each call |
| test_diag | test_command_args.cpp, test_selftest_report.cpp | the four `data` commands; both forms of the `data:` line and its place after `location:` |

**Power loss (SC-004):** `power loss: 59 calls per save, cut at each: 59 runs gave the old store, 1 (uncut) the new, 0 errors`. The rename is the last call and the commit point, so every cut, including one at the rename itself, leaves exactly the old store; a leftover `.new` is removed on load. A single failing call (not a power cut) also always leaves the old store, with no file left open.

## R6. Merge (FR-008, 04 §4)

**Decision.** `nvp::store::merge(stored, incoming, fetch_time, today)` in `lib/core`:
- one record per date;
- a higher rank replaces;
- an equal rank with a later fetch replaces;
- a lower rank never replaces;
- a rank-40 record replacing another source's rank-40 record with different times counts a `MISMATCH` and logs both;
- records before yesterday are pruned.

Fully tested, although only İmsakiyem exists in this slice.

## R7. Where the data lives at run time (FR-012, FR-015, constitution IX)

**Decision.**
- **The network worker owns the store** (core 0). It loads it at start, merges, writes, and then publishes a copy of the record array (at most 400 × 20 B, in PSRAM) with a version number under its lock.
- **The UI** (core 1) looks up yesterday, today and tomorrow by date under the lock on each second's update; it does no file I/O (constitution IX). A new version is drawn within the next second (FR-015).
- **Also published:** the last refresh time, the last stored date, the source of today's record, and the time Wi-Fi went down (for `offline`, FR-013).

## R8. Refresh triggers in this slice (FR-005)

The worker refreshes when the mode is running, Wi-Fi is up, the clock is valid, and one of these holds:
- the store does not cover today; this covers both the start and the end of setup, because after setup there is no store for the new district;
- `data refresh` was typed;
- one minute has passed since a failed attempt (once only).

Slice 7 replaces this with its scheduler.

## R9. States and overlay (FR-013, FR-014)

- **State inputs:** `select_state` (Slice 3) gets:
  - `today_present` from the store;
  - `wifi_down_minutes` from the worker's link-lost time;
  - `refresh_failed_or_wifi_down` and `stored_days_after_today` stay neutral until Slice 7, so `cache_ending` is never chosen.
- **Status text:** `Son güncelleme` with `HH:MM` when the last refresh was today, else `DD.MM` (02 §6).
- **Overlay:**
  - `Vakit kaynağı`: the 02 §9.5 name of today's record's source (`Diyanet (imsakiyem)`);
  - `Son güncelleme`: `DD.MM.YYYY HH:MM`;
  - `Kayıtlı son gün`: `DD.MM.YYYY`;
  - the grey dash when there is no store.

## R10. Development commands and logging (FR-016, FR-017)

**Commands** (development builds):
- **`data refresh`:** refresh now.
- **`data show`:** district, first and last date, count, source, CRC state, last refresh.
- **`data corrupt`:** flips a byte in the middle of the store file.
- **`data erase`:** deletes the store for the current district.

**Log lines:**

```text
[ 21345] data: imsakiyem v2 ok, 403 records stored (2026-11-24..2027-12-31), 2.3 s, heap min 28 kB
[ 21345] data: imsakiyem v2 rejected (wrong district 9205), store unchanged
[ 21345] data: imsakiyem v2 failed (timeout), retry in 60 s
[   612] data: store_9206.bin bad checksum, ignored
```
