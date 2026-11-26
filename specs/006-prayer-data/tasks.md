---

description: "Task list for real prayer times (spec 006)"
---

# Tasks: Real prayer times

**Input**: Design documents from `specs/006-prayer-data/` ([plan.md](plan.md), [spec.md](spec.md), [research.md](research.md), [data-model.md](data-model.md), [contracts/](contracts/), [quickstart.md](quickstart.md))

**Tests**:
- **`lib/core` and `lib/providers`:** tests are written first (constitution VI), against the recorded answers in `test/fixtures/providers/` and altered copies of them made in memory. The fixture files are never edited.
- **Device behaviour:** the serial log and the owner checklist (constitution VII).

**Organization**: by user story. The brief's two stages:
- **Stage 1** (host only): Phase 1, Phase 2, and the host tasks of US3 and US4.
- **Stage 2** (device): everything else.

The firmware is always flashed with `--upload-port COM4`.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: can run in parallel (different files, no dependency on an unfinished task)
- **[Story]**: US1–US6 of spec.md
- Every task cites an FR/SC of spec 006, a research item (R#) or a requirement id (constitution X)

---

## Phase 1: Setup

- [X] T001 Create `lib/providers/library.json` (name `nvp_providers`, depends on `nvp_core`, srcDir and includeDir `src`, like `lib/core/library.json`). Keep `lib/providers/README.md`. Confirm the `native` and `crowpanel7_advance` environments pick it up, and that `[env:sim]` / `[env:engine_table]` ignore it if needed (`platformio.ini`).
- [X] T002 [P] Create the test runners `test/test_providers/test_main.cpp` and `test/test_store/test_main.cpp` (Unity, same pattern as `test/test_time/test_main.cpp`), plus a header-only host helper `test/support/fixture_file.h`, included as `"../support/fixture_file.h"` (PlatformIO compiles one test folder per program), that reads a fixture by its path relative to the project root (the cwd of `pio test`, as `test/test_screen/test_fixtures.cpp` already relies on) (analysis A1).

---

## Phase 2: Foundational (blocks every story)

### Tests first

- [X] T003 [P] Write `test/test_providers/test_json_stream.cpp`:
  - the tokenizer emits the expected key paths and values for a small document;
  - it gives the same events when the input is fed in chunks of 1, 7, 64 and 1 024 bytes (a token split across chunks);
  - strings with `\"`, `\\` and `ç` escapes are handled;
  - more than 64 bytes in a string, a depth over 8, a missing `}` and trailing garbage are errors.

  (R2, FR-003)
- [X] T004 [P] Write `test/test_providers/test_imsakiyem.cpp`:
  - on `v2_times_9206_2026-11-24_2026-12-31.json` the v2 parser yields 38 records; the first is 2026-11-24 `06:08 07:36 12:40 15:12 17:34 18:57` with Hijri 15/6/1448, and the last is 2026-12-31;
  - it reports district 9206, `source.type` `diyanet` and `success` true;
  - on the 128-record file it yields 128 records;
  - on `v2_times_district_not_found.json` it reports `success` false and `error.code` `DISTRICT_NOT_FOUND`;
  - the v1 parser on `v1_range_…json` yields 30 records equal to v2's first 30;
  - each parser fed 1-byte chunks gives the same output.

  (R1, R2, FR-001, FR-012)
- [X] T005 [P] Write `test/test_providers/test_validate.cpp`. Each altered copy of the v2 fixture, made in memory, gives its reason, and the valid file gives `Ok` with today = 2026-11-24:
  - truncated at 50 % → `Truncated` or `Malformed`;
  - two records swapped → still `Ok`, because order is not required but uniqueness is;
  - a date duplicated → `DuplicateDate`;
  - a date 401 days ahead → `DateTooFar`;
  - all dates before today − 2 → `NoFutureDate` or `DateTooOld`;
  - `ogle` before `gunes` → `BadRecord`;
  - `"times":[]` → `Empty`;
  - `district_id` `9205` → `WrongDistrict`;
  - `success:false` → `NotSuccess`;
  - an invalid time string `25:00` → `BadRecord`.

  (R4, FR-006, SC-005)
- [X] T006 [P] Write `test/test_store/test_store_format.cpp`:
  - save then load round-trips the header and 400 records;
  - a flipped byte in a record gives `BadCrc`;
  - a wrong magic gives `BadMagic`; version 2 gives `BadVersion`; a short file gives `Truncated`;
  - `find`/`covers` on the boundary dates;
  - a store that ends today: `covers(today)` is true and `find(tomorrow)` is null (spec edge case, analysis C2).

  (R5, FR-008, FR-009)
- [X] T007 [P] Write `test/test_store/test_merge.cpp` with the rules of 04 §4:
  - higher rank replaces;
  - equal rank with a later fetch replaces;
  - lower rank never replaces;
  - a rank-40 mismatch between two sources counts once with both time sets reported;
  - records before yesterday are pruned;
  - at most 400 records are kept (the oldest dropped).

  (R6, FR-008)
- [X] T008 [P] Write `test/test_store/test_power_loss.cpp`. With a fake `FileSystem` that fails (as a power cut) at call N for N = 1 … all calls of a save over an existing store, every remount loads either exactly the old store or exactly the new one, never an error and never a mix. A leftover `.new` is removed on load. (R5, FR-009, DATA-07, SC-004)
- [X] T009 [P] Extend `test/test_diag/test_command_args.cpp` with `data refresh`, `data show`, `data corrupt` and `data erase`, and `test/test_diag/test_selftest_report.cpp` with the two forms of the `data:` line (contracts/serial-console.md)

### Implementation

- [X] T010 Implement `lib/providers/src/nvp/providers/json_stream.h/.cpp` (R2); T003 passes
- [X] T011 Implement `lib/providers/src/nvp/providers/imsakiyem.h/.cpp` (v2 and v1 parsers to a `DayRecord` callback, plus `BatchInfo`); T004 passes
- [X] T012 Implement `lib/providers/src/nvp/providers/validate.h/.cpp` and `http_get.h` (the abstract interface); T005 passes
- [X] T013 Implement `lib/core/src/nvp/store/file_system.h`, `store.h/.cpp` (format, `load`, atomic `save`, `find`, `covers`) and `merge.h/.cpp`; T006–T008 pass
- [X] T014a Write `test/test_providers/test_refresh_flow.cpp` and implement `lib/providers/src/nvp/providers/refresh.h/.cpp`: a pure `refresh_with(HttpGet&, FileSystem&, district, today, now_utc, Store&)` that runs v2, then v1 for a v2 failure other than `DISTRICT_NOT_FOUND`, then parse, validate, merge and save. It returns a `RefreshResult` (records stored, provider, reason). Tested with a fake `HttpGet` returning the fixtures and altered copies: a rejection leaves the store and the file unchanged; `DISTRICT_NOT_FOUND` does not try v1; a v2 network error falls back to v1 (FR-006, FR-007, SC-005, analysis D1).
- [X] T014 Add the `data` commands and the `data:` report line to `lib/core/src/nvp/diag/command.h/.cpp` and `report.h/.cpp`; T009 passes
- [X] T015 Run `pio test -e native`; all suites pass. Record the list of new tests and the power-loss count in `specs/006-prayer-data/research.md` R5 (Stage 1 report).

**Checkpoint (Stage 1)**: parser, validation, merge, store and power loss proven on the host.

---

## Phase 3: User Story 1 — Real times (P1) 🎯 MVP

**Goal**: after setup, Diyanet's times for the district on screen within 20 s.

**Independent Test**: compare three dates with Diyanet's website (SC-001, SC-002).

- [X] T016 [P] [US1] Implement `src/platform/https_fetch.h/.cpp`, an `nvp::providers::HttpGet` on `esp_http_client`:
  - CA bundle, `User-Agent` `nvp::kUserAgent`, `Accept-Encoding: identity`;
  - 15 s timeout, no redirects;
  - streams 1 kB chunks to the callback;
  - refuses when the caller says the time is not valid.

  (R3, FR-002, FR-003)
- [X] T017 [P] [US1] Implement `src/platform/littlefs_fs.h/.cpp`, an `nvp::store::FileSystem` over the mounted `/littlefs` (stdio `fopen`/`fread`/`fwrite`/`fclose`/`rename`/`remove`) (R5)
- [X] T018 [US1] Implement `src/app/data_service.h/.cpp`, run on the network worker. It wires `nvp::providers::refresh_with` (T014a) to `https_fetch` and `littlefs_fs`; it does not repeat its logic (analysis D1):
  - `start()` loads the store for the stored district;
  - `refresh()` calls `refresh_with` (waiting for a running scan before the save, FR-011) and publishes;
  - `data refresh` and `data show` are handled here, with the replies of contracts/serial-console.md (FR-016, analysis C1);
  - `DataView` published under a lock with a version (R7);
  - log lines per contracts/serial-console.md, with the minimum free internal heap during the fetch (R3, FR-017).
- [X] T019 [US1] In `src/app/net_worker.cpp`, call `data_service::start()` at boot. Trigger `refresh()` when the mode is running, the link is up, `clock::time_valid()` holds, and either the store does not cover today or the one retry 60 s after a failure is due (R8, FR-005).
- [X] T020 [US1] In `src/app/bringup_app.cpp` `build_main_view`, take yesterday, today and tomorrow from the `DataView` (the UI's copy, refreshed when the version changes). Feed `today_present` and **`today_rank = engine::rank(today.source)`** (40 for İmsakiyem; the default 0 would choose `calculated`, analysis U1) to `select_state`; the Hijri date comes from the record. Add a host test to `test/test_screen/test_system_state.cpp`: clock valid, today present with rank 40 and Wi-Fi up → `ok`, not `calculated` (FR-012, FR-013, FR-015).
- [X] T021 [US1] Flash. Confirm from the log `data: imsakiyem v2 ok, N records` within 20 s of `wifi: connected`, and `data show`. Record the heap minimum in research R3. Pick three dates (today, about +10 days and about +40 days) and list the device's six times and Hijri date for each in the owner checklist (SC-001, SC-002).

---

## Phase 4: User Story 2 — Kept on the device (P1)

**Goal**: stored times shown after a restart without Wi-Fi; `offline` after 10 minutes.

**Independent Test**: router off, restart (SC-003).

- [X] T022 [US2] In `src/app/net_worker.cpp`, record the moment the link went down in running mode and publish it in `DataView`. In `src/app/bringup_app.cpp`, feed `wifi_down_minutes` to `select_state` (FR-013).
- [X] T023 [US2] Owner step (analysis U2): with the store filled, the owner switches the router off and restarts the device. The board clock gives the time; the stored times must appear within 2 s, and after 10 minutes the state must be `offline` (reference 10). The implementer reads `today yes` and the state from the serial report the owner sends, and records it in the owner checklist (SC-003).

---

## Phase 5: User Story 3 — Never half-written (P1)

**Goal**: a store write is atomic; a damaged store is detected and rebuilt.

**Independent Test**: T008 on the host; `data corrupt` on the device (SC-004, SC-006).

- [X] T024 [US3] In `src/app/data_service.cpp`, at load: a bad magic, version or CRC is logged once as `bad checksum, ignored` (or the reason), the file is deleted, nothing is shown, and the next refresh rebuilds it. A leftover `.new` is deleted (FR-009).
- [X] T025 [US3] Implement the `data corrupt` and `data erase` commands (`data refresh` and `data show` are in T018) in `src/app/data_service.cpp`, `src/app/serial_console.cpp` and `src/app/bringup_app.cpp` (requests to the worker; development builds) (FR-016)
- [X] T026 [US3] On the device: `data corrupt`, then restart with Wi-Fi. The log shows the bad file ignored, the screen shows `no_data`, and the real times return after the refresh within a minute (SC-006).

---

## Phase 6: User Story 4 — Nothing wrong gets in (P1)

**Goal**: a bad batch is rejected whole; the store is unchanged.

**Independent Test**: T005 on the host.

- [X] T027 [US4] In `src/app/data_service.cpp`, a rejected `RefreshResult` (T014a) leaves `DataView` untouched and logs `rejected (<reason>), store unchanged` (FR-006, FR-007, FR-017). The rejection logic itself is tested on the host in T014a.

---

## Phase 7: User Story 5 — Honest when empty (P2)

**Goal**: `no_data` without a record for today; normal again when data arrives.

**Independent Test**: `data erase` with Wi-Fi off, then on (SC-007).

- [X] T028 [US5] On the device: `data erase` with `ntp off` and Wi-Fi down. The screen shows `no_data` (the owner compares with reference 14). Then `data refresh` with Wi-Fi up, and the screen switches back without a restart (FR-013, FR-015, SC-007).

---

## Phase 8: User Story 6 — Visible provenance (P2)

**Goal**: the overlay shows source, last update and last stored date.

**Independent Test**: open the overlay after a refresh.

- [X] T029 [P] [US6] Extend `nvp::net::OverlayInput` and `make_overlay_view` in `lib/core/src/nvp/net/views.h/.cpp` with source, last refresh (local) and last day, plus their tests in `test/test_net/test_views.cpp`. Source names come from 02 §9.5 (`Diyanet (imsakiyem)`); formats are `DD.MM.YYYY HH:MM` and `DD.MM.YYYY`; the dash when absent (FR-014).
- [X] T030 [US6] Feed them from `DataView` in `src/app/bringup_app.cpp` `overlay_view()`, and the `Son güncelleme` status text from the last refresh (`HH:MM` today, else `DD.MM`) (FR-013, FR-014)

---

## Phase 9: Location change (DATA-08) and polish

- [X] T031 In `src/app/data_service.cpp`, when the stored district differs from the loaded store's: show nothing from the old store; after the first successful save for the new district, delete the old district's file and log it (FR-010, DATA-08)
- [X] T032 [P] Update the documents:
  - 04 §8 with the store format of research R5;
  - 05 §4 (`lib/providers` contents) and §5 (the store files);
  - 06 roadmap Slice 6 outcome;
  - README commands (`data …`);
  - `specs/006-prayer-data/contracts/serial-console.md` with the lines as implemented.
- [X] T033 Owner checklist `specs/006-prayer-data/owner-checklist.md`: the brief's nine questions plus the three dates and the device's values; answers recorded (constitution VII)
- [X] T034 Run `python tools/check.py`; report the image size; run `/speckit-converge`

---

## Dependencies & execution order

- **Phase 1 → Phase 2:** tests T003–T009 before T010–T014; T015 closes Stage 1.
- **US1** (T016–T021) needs Phase 2.
- **US2** needs T018 and T020.
- **US3:** T024 and T025 need T018.
- **T014a** needs T011–T013; **T018** needs T014a.
- **US4:** T027 needs T018.
- **US5** needs T025.
- **US6:** T029 can start any time; T030 needs T018.
- **T031** needs T018.
- **Polish** comes last.

## Parallel examples

- **Phase 2 tests:** T003–T009 together.
- **Phase 2 implementations:** T010/T011/T012 (providers) alongside T013 (store).
- **US1:** T016 and T017 together.
- **Any time:** T029.

## Implementation strategy

1. **Stage 1:** Phases 1 and 2 (host). Report the test list and results.
2. **Stage 2:**
   - US1, which is the MVP;
   - US3 and US4 device parts, US2, US5, US6;
   - location change;
   - polish;
   - owner checklist with the three dates;
   - converge.

---

## Phase 10: Convergence

- [X] T035 In `src/app/data_service.cpp`, call `t.before_save` (wait for a running Wi-Fi scan) before every flash write outside `refresh_with`'s save: the delete of a bad or leftover file in `load_store`, the delete in `erase()`, the byte flip in `corrupt()`, and the old district's delete after a location change; `load_store` on a location change can run while setup is scanning per FR-011 (partial)

---

## Phase 11: Convergence

- [X] T036 In `src/app/data_service.cpp` `show()`, call `before_write()` before `nvp::store::load(...)`: `load()` deletes a leftover `store_<district>.new`, which a failed save can leave behind when its own clean-up delete fails, so `data show` can write to flash during a scan per FR-011 (partial)
