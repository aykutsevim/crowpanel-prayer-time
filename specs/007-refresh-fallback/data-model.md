# Data model: Automatic refresh and fallback (spec 007)

Existing entities (`DayRecord`, `Store`, the store file) are unchanged; see spec 006 `data-model.md`. `SourceId` already lists all five providers with ranks 50, 40, 40, 20, 10.

## Provider (lib/providers)

| Field | Type | Notes |
|---|---|---|
| id | `ProviderId` | `Diyanet` (empty slot, never enabled here), `Imsakiyem`, `Emushaf`, `Aladhan`, `Local` |
| order | fixed | the list order above (FR-001) |
| rank | from `engine::rank(SourceId)` | ≥ 40 Diyanet-derived |
| blocked | bool, RAM only | development command (R10) |
| failures | u8 | refreshes in a row in which this provider failed; reset by a success |
| cool_down_until | i64 UTC s | 30 min after 429/503 (FR-011); 24 h after the fifth failure (FR-013), not applied to Diyanet-derived providers while today is calculated |

## Refresh outcome

`complete` (goal reached, R6) · `partial` (Diyanet-derived records stored, a provider failed) · `failed` (nothing Diyanet-derived stored and a provider failed) · `skipped` (no provider could be asked: all blocked or cooling down).

## Planner state (persisted, `refresh.bin`, R7)

| Field | Type | Rule |
|---|---|---|
| last_success_utc | i64 | end of the last `complete`/`partial` refresh that stored Diyanet-derived records |
| last_attempt_utc | i64 | |
| last_outcome | u8 | the outcome above |
| retry_step | u8 | 0 = none; 1..4 = 5 min, 15 min, 1 h, 3 h; 5 = every 6 h |
| next_retry_utc | i64 | 0 when none |
| providers[4] | failures u8, cool_down_until i64 (30 min after 429/503), skip_until i64 (24 h) | in `ProviderId` order: Diyanet, İmsakiyem, Emushaf, Aladhan |
| calibration | 6 × i8 offsets, u8 days used, u32 district, i8 hijri_offset | valid only for its district |
| coords_suspect | bool | FR-016 exception; cleared on location change |

RAM only: `night_minute` (from the MAC), `last_reconnect_refresh_utc`, `last_manual_utc`, `blocked` flags, `mismatches_since_boot`.

**Transitions.** `refresh_finished(complete)` → retry_step 0, next_retry 0, failures of answering providers 0. `partial`/`failed` → retry_step+1 (max 5), next_retry = now + step delay, or now + 1 h when today is calculated. `location_changed` → calibration and coords_suspect cleared, retry_step 0, immediate refresh.

## Coordinates

| Field | Type | Notes |
|---|---|---|
| lat_e5, lon_e5 | i32 | degrees × 100 000 |
| origin | enum | `table` (bundled), `detail` (İmsakiyem district detail), `override` (Slice 8) |
| valid | bool | inside 35.8…42.2, 25.6…44.9 |

Bundled table `places_coords` (generated with the place list): rows `{district_id u32, name_hash u32 (FNV-1a of the upper-case district name), lat_e5 i32, lon_e5 i32}`, sorted, 1003 rows, 16 B each. Lookup by (stored district id, stored district name); fallback by id alone.

## Calibration

Six offsets in minutes (published − calculated, median over the most recent ≤ 30 Diyanet-derived days, at least 14), rejected if any |offset| > 10 (logged). Hijri offset: days between the tabular Islamic date and the latest Diyanet-derived record's Hijri date.

## Mismatch

As spec 006 (`MergeStats`): count since boot, the first pair kept for the log; today's mismatch over 2 minutes flagged for Slice 8.

## Batch (incoming)

As spec 006, plus `origin` (which provider) and `plausible` (result of the 04 §7 check). Scratch adds a second buffer of 40 records for emushaf, kept while deciding the FR-016 exception.
