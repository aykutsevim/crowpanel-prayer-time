---

description: "Task list for automatic refresh and fallback (spec 007)"
---

# Tasks: Automatic refresh and fallback

**Input**: Design documents from `specs/007-refresh-fallback/` ([plan.md](plan.md), [spec.md](spec.md), [research.md](research.md), [data-model.md](data-model.md), [contracts/](contracts/), [quickstart.md](quickstart.md))

**Tests**:
- **`lib/core` and `lib/providers`:** tests are written first (constitution VI). Recorded answers in `test/fixtures/providers/` are never edited; tests alter copies in memory.
- **Device behaviour:** the serial log and the owner checklist (constitution VII).

**Organization**: by user story. The brief's three stages:
- **Stage 1** (host): Phase 1, Phase 2, US1 and the scenario part of US2, US3, US6 and US8, all against **scripted sources** (a `Source` interface that returns prepared batches or failures), so no parser is needed yet.
- **Stage 2** (host): the real sources (emushaf and Aladhan parsers, the İmsakiyem adapter), the calculation port, calibration, coordinates and plausibility (US4, US7 and the parser tasks of US2 and US3).
- **Stage 3** (device): wiring, states, commands, owner checklist.

The firmware is always flashed with `--upload-port COM4`.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: can run in parallel (different files, no dependency on an unfinished task)
- **[Story]**: US1–US8 of spec.md
- Every task cites an FR/SC of spec 007, a research item (R#) or a requirement id

---

## Phase 1: Setup

- [X] T001 Create the test runners `test/test_planner/test_main.cpp` and `test/test_calc/test_main.cpp` (Unity, same pattern as `test/test_store/test_main.cpp`), and a header-only `test/support/scripted_source.h`: a `Source` fake that answers each call from a script (`records`, `fail(Reason)`, `http(status)`, `rate_limited`, `timeout`) and records every call (provider, from, to, the simulated time) for assertions (plan, R5, R6). *Done: `scripted_source.h` also holds `Sim`, a minute-by-minute device in miniature (planner + runner + store + FakeFs). The `test_calc` runner arrives with T024.*
- [X] T002 [P] Create `lib/core/src/nvp/calc/LICENSE-adhan.txt` with adhan-js's MIT notice (Copyright (c) 2016 Batoul Apps) and a `README.md` naming the source: `https://github.com/batoulapps/adhan-js`, tag `v4.4.6`, commit `b6e04cd9da28fffc66773404a0a2e9c739370baf`, and the files ported (R3).

---

## Phase 2: Foundational (blocks every story)

### Tests first

- [X] T003 [P] Extend `test/test_store/test_merge.cpp` with the rank rules of FR-002 and SC-006, over every pair of the ranks 10, 20, 40: a lower rank never replaces a higher one, whatever the fetch time; a higher rank always replaces; equal ranks follow spec 006. Include a stored rank-40 record and a newer rank-10 one: the stored record stays.
- [X] T004 [P] Write `test/test_store/test_refresh_state.cpp`: save then load round-trips every field of data-model "Planner state"; wrong magic, version or CRC → `Missing` behaviour (defaults, "never refreshed"); with the fake file system of spec 006, a cut at every call of a save over an existing `refresh.bin` loads exactly the old or the new state (R7, FR-014, DATA-07).

### Implementation

- [X] T005 Move the write-new, close, read back, rename sequence out of `lib/core/src/nvp/store/store.cpp` into `lib/core/src/nvp/store/atomic_file.h/.cpp` (`atomic_write(fs, name, bytes, len)`), used by `save`; spec 006's `test_power_loss` must still pass unchanged (R7).
- [X] T006 Implement `lib/core/src/nvp/store/refresh_state.h/.cpp` (`RefreshState`, `load_refresh_state`, `save_refresh_state`, 64 B, magic `NVPR`, version 1, CRC-32) on `atomic_write`; T004 passes (R7).
- [X] T007 Confirm `lib/core/src/nvp/store/merge.cpp` already satisfies T003 (rank before fetch time); fix it if a case fails. T003 passes (FR-002).
- [X] T008 Define the provider model in `lib/providers/src/nvp/providers/source.h`: `ProviderId { Diyanet, Imsakiyem, Emushaf, Aladhan, Local }` in that order, `kProviderOrder`, `source_id(ProviderId)` (to `engine::SourceId`), and the abstract `Source { virtual Fetch fetch(ProviderId, Date from, Date to, RecordSink&) }` returning an outcome (`ok`, `failed(reason)`, `http(status)`, `rate_limited`, `timeout`) and records parsed (data-model "Provider", FR-001).

**Checkpoint**: ranks proven; persistence proven against power cuts.

---

## Phase 3: User Story 1 — Looks after itself (P1) 🎯 MVP

**Goal**: refreshes happen at the specified moments and nowhere else.

**Independent Test**: simulated weeks in `test_planner` (SC-001).

- [X] T009 [US1] Write `test/test_planner/test_triggers.cpp` against `nvp::providers::Planner` with a scripted clock (UTC seconds plus local date and minute, time zone `<+03>-3`). Cases:
  - setup done and location changed → a refresh at once;
  - boot with Wi-Fi and a valid clock: refresh when today has no Diyanet-derived record, when fewer than 30 days ahead are stored, when the last success is more than 7 days old; none otherwise;
  - no refresh while the clock is invalid, without a location, or with the link down;
  - weekly: exactly one refresh at the night minute 7 days after the last success, none at other minutes of the week;
  - low coverage: one refresh per night at the night minute until coverage ≥ 30 days;
  - reconnection after an outage: only if a boot condition holds, at most once per 10 minutes;
  - manual: at most once per minute;
  - night minute: `180 + fnv1a32(mac) mod 120`, two different MACs give two minutes in 03:00–04:59 (FR-004, FR-005, FR-006, SC-001, R5).
- [X] T010 [US1] Implement `lib/providers/src/nvp/providers/planner.h/.cpp`: events `boot`, `link_up`, `link_down`, `tick`, `setup_done`, `location_changed`, `manual`, `refresh_finished(outcome, today_calculated)`; outputs `should_start()` with a reason text and `next_scheduled()`; state as data-model "Planner state" plus the RAM fields; loads from and saves to `RefreshState`. T009 passes (R5).

---

## Phase 4: User Story 6 — Published times always win (P1)

**Goal**: Diyanet-derived first; calculated never replaces published.

**Independent Test**: runner scenarios with scripted sources (SC-006).

- [X] T011 [US6] Write `test/test_planner/test_runner_order.cpp` for `refresh_run(Source&, …)`:
  - all sources up: only İmsakiyem is asked (one request), outcome `complete`;
  - İmsakiyem gives under 30 days: emushaf is asked next; Aladhan and Local never while today and tomorrow have rank-40 records;
  - both mirrors down: Aladhan is asked only after both, and only for the months with a day still lacking a rank-40 record;
  - a stored rank-40 day is never overwritten by Aladhan's day;
  - in every scenario of this file the call log shows each Diyanet-derived provider before any calculated one (FR-001, FR-002, FR-007, SC-006, R6).
- [X] T012 [US6] Rewrite `lib/providers/src/nvp/providers/refresh.h/.cpp` as the step runner of R6 over `Source`: steps İmsakiyem → emushaf (if under 30 days or İmsakiyem failed) → Aladhan (current month, and the next within 7 days of month end, if today or tomorrow lacks a rank-40 record; needs coordinates); merge into the scratch copy; one atomic save at the end if anything changed; a `should_continue()` callback checked before each request (a location change abandons the run without storing, analysis U7); result with outcome `complete`/`partial`/`failed`/`skipped`, per-provider results, requests made, merge stats. **Keep the build green (analysis I3):** in the same task write the İmsakiyem `Source` adapter over `HttpGet` (v2 then v1, as spec 006) in `lib/providers/src/nvp/providers/http_source.h/.cpp`, and keep spec 006's `refresh_with(HttpGet&, …)` as a thin wrapper over the runner with only that adapter, so `test_refresh_flow.cpp` and `src/app/data_service.cpp` build and pass unchanged until T036. T011 passes.
  *Done as a new `runner.h/.cpp` (`refresh_run`) beside spec 006's `refresh.h/.cpp`, which is left untouched, so `test_refresh_flow.cpp` and the device build stay green until T036 (analysis I3 met without a wrapper). The İmsakiyem `Source` over `HttpGet` therefore moves to T030.*

---

## Phase 5: User Story 2 — One source down goes unnoticed (P1)

**Goal**: the second mirror covers for the first.

**Independent Test**: runner scenarios; on the device `data block imsakiyem` (SC-002).

- [X] T013 [US2] Write `test/test_planner/test_runner_mirrors.cpp`:
  - İmsakiyem down → emushaf's records stored, outcome `partial`, store ≥ 30 days when emushaf supplies them;
  - **year end**: on 2026-12-20 İmsakiyem answers only to 2026-12-31 and emushaf to 2027-01-20 → the store reaches 2027-01-20, outcome `complete` because both answered (goal rule of FR-012, R6); log the request sequence in the test output;
  - emushaf answers its cached month (24 days ahead) and İmsakiyem is up → `complete`, no retry;
  - the two mirrors disagree on a day → the newer kept, one mismatch counted with both time sets (US2 AC3, 04 §4).
- [X] T014 [US2] Make T013 pass in `lib/providers/src/nvp/providers/refresh.cpp` (goal rule, mismatch reporting through `MergeStats`).
- [X] T015 [P] [US2] Write `test/test_providers/test_emushaf.cpp` on `emushaf/vakitler_9206_2026-11-24.json`: 32 records from 2026-11-19 to 2026-12-20; 2026-11-24 equals İmsakiyem's six times and Hijri 15/6/1448 (from `HicriTarihKisa`); 1-byte chunks give the same output; `vakitler_999999_502.txt` gives a parse failure, not "district not found"; an empty array is `Empty` (R1).
- [X] T016 [US2] Implement `lib/providers/src/nvp/providers/emushaf.h/.cpp` (streaming parser over `JsonStream`, top-level array, `MiladiTarihKisa`, six times, `HicriTarihKisa`); T015 passes (R1, FR-009). The answer starts 5–7 days before today, so the source drops records before yesterday **before** validating, or the batch fails `DateTooOld`.

---

## Phase 6: User Story 3 — Approximate, and says so (P1)

**Goal**: calculated times when no Diyanet-derived source can supply today, and back to published by themselves.

**Independent Test**: runner and planner scenarios; on the device both mirrors blocked, then unblocked (SC-003).

- [X] T017 [US3] Write `test/test_planner/test_calculated.cpp`:
  - both mirrors down, empty store, coordinates known → Aladhan's month stored with rank 20, outcome `failed` (no Diyanet-derived data), the planner schedules the next attempt **1 h** later (not 5 min) because today is calculated;
  - one hour later İmsakiyem answers → its records replace the rank-20 days, outcome `complete`, retry cleared;
  - no coordinates → Aladhan not asked, nothing stored, `no_data` stays (04 §12 row 7) (FR-003, FR-007, FR-012).
- [X] T018 [US3] Make T017 pass in `planner.cpp` and `refresh.cpp`.
- [X] T019 [P] [US3] Write `test/test_providers/test_aladhan.cpp` on `aladhan/calendar_2026_11_9206.json` and `calendar_2026_12_9206.json`: 30 and 31 records; 2026-11-24 gives `06:08 07:36 12:40 15:13 17:34 18:56` (the ` (+03)` suffix stripped, `Fajr` used, not `Imsak`); **no Hijri date** taken; records outside the requested month dropped, and records before yesterday dropped before validation (the month starts before today; otherwise `DateTooOld`); `{code:503}` and a non-JSON body are failures; every time within ±1 minute of the İmsakiyem fixture for the same 38 days (R2).
- [X] T020 [US3] Implement `lib/providers/src/nvp/providers/aladhan.h/.cpp` and `aladhan_url(year, month, lat_e5, lon_e5, …)`; T019 passes (R2, FR-009).

---

## Phase 7: User Story 8 — Gentle on the services (P2)

**Goal**: rare, serial, backing-off requests.

**Independent Test**: planner scenarios with failures and reconnect storms.

- [X] T021 [US8] Write `test/test_planner/test_politeness.cpp`:
  - 429 or 503 from İmsakiyem → not called again for 30 minutes, even by a manual refresh;
  - five failed refreshes in a row for one provider → skipped 24 h, then tried again;
  - retries after 5 min, 15 min, 1 h, 3 h, then every 6 h; a reconnection allows one extra attempt;
  - Wi-Fi reconnecting every 5 s for an hour → at most 6 refreshes;
  - a refresh never makes more than 12 requests;
  - **calculated today**: both mirrors failing for 30 hours → still retried every hour (no 24 h skip for Diyanet-derived providers), each retry asks only İmsakiyem v2 and emushaf; when one answers, the next refresh restores them (FR-003, FR-013 exception, analysis C1);
  - **budget**: a scripted normal week costs ≤ 2 requests; a week with every source failing and today calculated costs ≤ 400; the test fails above either (FR-008, FR-010–FR-013, SC-001, R6).
- [X] T022 [US8] Make T021 pass in `planner.cpp` and `refresh.cpp` (cool-downs and failure counters in `RefreshState`, the cap counter in the runner).
- [X] T023 [US8] Write `test/test_planner/test_edge_cases.cpp`: location change while a refresh runs (`should_continue()` turns false between two requests; nothing stored for the old district; a new refresh starts for the new one); malformed data from one mirror (rejected, the other used); a slow source (timeout counted as a failure); persistence across a restart in the middle of the retry schedule (the step continues). Make it pass (spec edge cases, FR-014).

**Checkpoint (Stage 1)**: planner, runner, ranks and persistence proven on the host. Report the scenario list with pass or fail and the year-end request sequence (T013).

---

## Phase 8: User Story 4 — Works with no internet at all (P2)

**Goal**: calculation on the device, calibrated.

**Independent Test**: `test_calc`; on the device router off and store erased (SC-004).

- [X] T024 [P] [US4] Copy `Shared/Times/*.json` from adhan-js at commit `b6e04cd9` into `test/fixtures/calc/adhan/` (with a README naming the commit) and write `test/test_calc/test_prayer_times.cpp`: for `Ankara-Turkey.json` (method Turkey) every time within the file's `variance`; the other seven files with their methods where the port supports them; for the 128 published Ankara days of `imsakiyem/v2_times_9206_2026-11-24_2027-03-31.json` with the coordinates of `v2_district_9206.json`, every time within ±2 minutes, and print the per-prayer median and range (R3).
- [X] T025 [US4] Port the adhan-js core into `lib/core/src/nvp/calc/` (`astronomical`, `solar_coordinates`, `solar_time`, `prayer_times` with the Turkey method; no heap, no exceptions, results in minutes after local midnight for a UTC offset); T024 passes (R3, FR-017).
- [X] T026 [P] [US4] Write `test/test_calc/test_calibrate.cpp`: offsets are the per-prayer median over the most recent ≤ 30 rank-40 days; none with 13 days, offsets with 14; an offset beyond ±10 rejects the set (reason names the prayer); the tabular Hijri date of 2026-11-24 plus the learned offset gives 15/6/1448; the Ankara 128-day split of research R3 (calibrate on 30, test on 98) gives a median of at most 1 minute for each time (FR-018, SC-004, R8).
- [X] T027 [US4] Implement `lib/core/src/nvp/calc/calibrate.h/.cpp` (offsets, Hijri tabular calendar and offset) and the "local fill" function `fill_local(store_copy, today, coords, calib, days=8)` that adds rank-10 records only where no record exists; T026 passes (R8).

---

## Phase 9: User Story 7 — Wrong data is caught (P2)

**Goal**: implausible batches rejected; the two-mirror exception.

**Independent Test**: altered copies of the fixtures (SC-008).

- [X] T028 [P] [US7] Write `test/test_providers/test_plausible.cpp` on the İmsakiyem and emushaf fixtures with Ankara's coordinates: unchanged → plausible; every time +60 min (shifted hour) → rejected, reason names date and prayer; a shifted day (each record carries the times of the record 60 days later, so the dates no longer match the times) → rejected; another district's times (Ankara's times checked against the coordinates of a district more than 20 minutes away, for example Edirne or Hakkari) → rejected; no coordinates or invalid clock → check skipped. Runner case: both mirrors fail only this check and agree on every overlapping day → İmsakiyem's batch stored, `coords_suspect` set and persisted, log line; if they disagree → both rejected (FR-016, SC-008, R6).
- [X] T029 [US7] Implement `lib/providers/src/nvp/providers/plausible.h/.cpp` (first, middle, last record against the **uncalibrated** calculation, 20 minutes) and the exception in `refresh.cpp`; T028 passes.

---

## Phase 10: Real sources and coordinates (Stage 2, serves US2, US3, US4, US7)

- [X] T030 Write `lib/providers/src/nvp/providers/http_source.h/.cpp`, the `Source` over `HttpGet` for İmsakiyem (v2, then v1 unless `allow_fallback` is off, reusing spec 006's parser and validation), emushaf and Aladhan per month: build the URL, feed the matching parser, validate (spec 006 rules), map status 429/503 to `rate_limited` and a body over 512 kB to a failure; tests in `test/test_providers/test_refresh_flow.cpp` with the new fixtures (FR-009, FR-010).
- [X] T031 [P] Extend `tools/gen_places.py` to write `data/places_coords.json` from the district list it already reads: one row per bundled entry `[district_id_as_stored, "UPPER NAME", lat_e5, lon_e5]`; fail if any entry lacks coordinates, has a confidence other than `high`/`medium`, or lies outside Türkiye's box. Regenerate (3 requests, throttled) and commit the file (R4). *Done as `python tools/gen_places.py --coords`, which maps the district pages onto the committed list without regenerating it; the full run writes both files.*
- [X] T032 Extend `tools/gen_web.py` (pre-build) to emit `src/generated/places_coords.cpp/.h` (sorted rows `{district_id, fnv1a32(name), lat_e5, lon_e5}`) from `data/places_coords.json`, and write `lib/core/src/nvp/net/coords.h/.cpp` (`lookup(table, n, district_id, name)` → coordinates or none; fallback by id) with tests in `test/test_net/test_coords.cpp` (Ankara 9206 → 39.92063, 32.85403; Kadıköy under centre 9541 → its own coordinates) (R4, FR-015). *Done: the fallback by id alone applies only when exactly one row has that id; otherwise the device asks the district detail.*
- [X] T033 [P] Write `test/test_providers/test_district_detail.cpp` and implement the district-detail parser in `lib/providers/src/nvp/providers/imsakiyem.cpp` (`v2_district_9206.json` → coordinates, `high`; `v2_district_not_found.json` → not found; `low` confidence rejected; a point outside Türkiye rejected) (R4, FR-015).
- [X] T034 Run `pio test -e native`; all suites pass. Record in `specs/007-refresh-fallback/research.md` R3 the per-prayer differences measured with the port before and after calibration (Stage 2 report).

**Checkpoint (Stage 2)**: parsers, calculation, calibration, coordinates and plausibility proven on the host.

---

## Phase 11: User Story 5 — Warned in time (P2)

**Goal**: `cache_ending` with the right number of days.

**Independent Test**: date moved near the end of the store with sources blocked (SC-005).

- [X] T035 [US5] Add `stored_days_after(store, today)` (consecutive days after today with any record) to `lib/core/src/nvp/store/store.h/.cpp` with tests in `test/test_store/test_store_format.cpp`, and a host test in `test/test_screen/test_system_state.cpp`: last outcome `failed`, 5 days left → `cache_ending` with 5; 0 → `… bugün bitiyor`; with today calculated → `calculated` wins (02 §6 order) (FR-019, R9).

---

## Phase 12: Device integration (Stage 3)

- [X] T036 In `src/app/data_service.h/.cpp`: own the `Planner`, the `RefreshState` (loaded at start from `/littlefs/refresh.bin`) and an `HttpSource` over `https_fetch`; feed it the events from `worker_tick` (boot, link up/down, tick with the local date and minute from the UI's `note_today`, setup done, location changed, manual); run the runner when `should_start()`; save the state at the end; wait for scans before every write (spec 006 FR-011); `should_continue()` false when the location changed (FR-004, FR-006, R5, R6). In `src/platform/https_fetch.cpp`, set a 10 s connect and a 15 s read timeout (FR-010, analysis G5): read the `esp_http_client` documentation of ESP-IDF 5.5.5 for how the two are configured, and record it in research.md. Remove the spec 006 `refresh_with` wrapper once nothing calls it.
- [X] T037 In `src/app/data_service.cpp`, after each merge recompute the calibration (persist when it changes) and, whenever today has no record and coordinates and a valid clock exist, call `fill_local` on the published copy (not saved), also without a network (FR-017, FR-018, US4, R8).
- [X] T038 [P] Pass the station MAC to the planner for the night minute (`src/platform/sysinfo.h`) and the coordinates from the bundled table at location load, with the district-detail fetch only when the table has no row (`src/app/data_service.cpp`) (FR-005, FR-015).
- [X] T039 In `src/app/bringup_app.cpp` `build_main_view`: `today_rank` from today's record (10, 20, 40), `refresh_failed_or_wifi_down` from the last outcome and the link, `stored_days_after_today` from `stored_days_after` (it was 400); the overlay's `Vakit kaynağı` from today's record's source (02 §9.5; all five names exist in `tr.h`); for any record without a Hijri date (Aladhan and device records), compute it from the tabular calendar plus the learned offset (T027) and show it only in the `calculated` state (02 §8; analysis U4) (FR-019, FR-020, R8, R9).
- [X] T040 Add `data block`, `data unblock`, `data plan`, `data calib` and the mismatch count of `data show` to `lib/core/src/nvp/diag/command.h/.cpp` (tests in `test/test_diag/test_command_args.cpp`), the report line of contracts/serial-console.md to `report.h/.cpp` (tests in `test_selftest_report.cpp`), and their handling in `src/app/serial_console.cpp` and `src/app/data_service.cpp`; log lines as in the contract (FR-022, FR-023).
- [X] T041 Flash (COM4). From the log confirm: the boot decision, the night minute, one İmsakiyem request, `refresh complete`, `data plan` and `data calib`. Record the free internal heap during a refresh in research.md.

---

## Phase 13: Polish

- [X] T042 [P] Update the documents: 04 §5 with the goal rule (R6), the hourly-retry exemption from the 24 h skip and the 400-a-week ceiling, §7 with the two-mirror plausibility exception (FR-016, analysis I6), and §8 with `refresh.bin`; 05 §4 (`lib/providers` planner and runner, `lib/core/calc`) and §5 (`refresh.bin`, the coordinates table); 06 roadmap Slice 7 outcome; README commands (`data block|unblock|plan|calib`); `specs/007-refresh-fallback/contracts/serial-console.md` as implemented.
- [X] T043 Owner checklist `specs/007-refresh-fallback/owner-checklist.md`: the brief's eight checks with exact commands (quickstart): normal operation; first mirror blocked; both blocked with the store erased (compare with reference 12 and Diyanet's website); unblocked (marking gone within 70 min); Wi-Fi off and store erased (device calculation); cache ending (reference 11, `clock sim`); next scheduled refresh between 03:00 and 04:59; no sound. Record the answers (constitution VII; SC-002, SC-003, SC-004, SC-005, SC-009, SC-010).
- [X] T044 Confirm that every row of the 04 §12 table has a named host test (list them in research.md; SC-007), run `python tools/check.py`, report the image size, run `/speckit-converge`.

---

## Dependencies & execution order

- **Phase 1 → Phase 2.** T005 before T006; T003 before T007.
- **US1** (T009–T010) needs T006 and T008.
- **US6** (T011–T012) needs T008 and T007; **US2**, **US3**, **US8** runner tasks need T012.
- **Parsers** T015–T016, T019–T020, T033 are independent of the runner ([P]).
- **US4** (T024–T027) is independent of the runner; **US7** T029 needs T025 and T012.
- **T030** needs T012, T016, T020; **T032** needs T031.
- **US5** T035 needs nothing from the runner.
- **Device** T036–T041 need all host phases.
- **Polish** last.

## Parallel examples

- **Phase 2 tests:** T003 and T004 together.
- **Stage 2 parsers:** T015, T019, T024, T026, T028, T031, T033 together.
- **Device:** T038 alongside T036.

## Implementation strategy

1. **Stage 1 (host):** Phases 1–7 against scripted sources. Report the scenario list and the year-end request sequence.
2. **Stage 2 (host):** Phases 8–10. Report the per-prayer differences measured for Ankara.
3. **Stage 3 (device):** Phases 11–12, then the owner checklist and converge.

---

## Phase 14: Convergence

- [X] T045 CRITICAL: ask İmsakiyem from yesterday (v2 `from` = today − 1, `to` = from + 365; v1 likewise) so that after a calculated period yesterday's record is replaced by the published one; the engine takes yesterday's Yatsı as the start of the night period (`lib/core/src/nvp/engine/engine.cpp`), and today a calculated yesterday stays for one night (research R12). Add a runner scenario in `test/test_planner/` (calculated period, then İmsakiyem returns: yesterday is rank 40) per Constitution II (contradicts)
- [X] T046 Remove spec 006's `refresh_with` and `RefreshScratch` from `lib/providers/src/nvp/providers/refresh.h/.cpp` (the device no longer calls it; keep `v2_url`, `v1_url`, `http_outcome_text`, `provider_text` where still used) and move any check of `test/test_providers/test_refresh_flow.cpp` that `test_http_source.cpp` does not already cover onto `HttpSource`, per T036 (partial)

## Phase 15: Convergence

- [X] T047 Update the Slice 6 line of `docs/specification/05-architecture.md` §4 (lib/providers) so `refresh` is described as the İmsakiyem URL builders only (`v2_url`, `v1_url`, `http_outcome_text`), not `refresh_with`, per T046 (partial)
- [X] T048 Add to the "As implemented in Slice 7" note of `docs/specification/04-data-sources.md` §5.2 that İmsakiyem is asked from yesterday to yesterday + 365 (v1 likewise from yesterday), so yesterday's record is Diyanet-derived after a calculated period, per FR-012 / T045 (partial)
