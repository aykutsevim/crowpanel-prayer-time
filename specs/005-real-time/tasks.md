---

description: "Task list for real time (spec 005)"
---

# Tasks: Real time

**Input**: Design documents from `specs/005-real-time/` ([plan.md](plan.md), [spec.md](spec.md), [research.md](research.md), [data-model.md](data-model.md), [contracts/](contracts/), [quickstart.md](quickstart.md))

**Tests**: every `lib/core` addition has its tests written first (constitution VI). The device behaviour is verified from the serial log and by the owner checklist (constitution VII).

**Organization**: by user story (US1–US5 of spec.md). The firmware is always flashed with `--upload-port COM4`.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: can run in parallel (different files, no dependency on an unfinished task)
- **[Story]**: US1–US5 of spec.md
- Every task cites an FR/SC of spec 005, a research item (R#) or a requirement id (constitution X)

---

## Phase 1: Setup

- [X] T001 Set `CONFIG_LWIP_SNTP_MAX_SERVERS=3` and `CONFIG_MBEDTLS_HAVE_TIME_DATE=y` in `sdkconfig.defaults`, with comments citing R1, R3 and OD5-1
- [X] T002 [P] Create `test/test_time/test_main.cpp` (Unity runner, same pattern as `test/test_net/test_main.cpp`) with an empty run list
- [X] T003 [P] Extend `tools/check_tls.py` with `tools/test_check_tls.py` cases: fail when `sdkconfig.defaults` lacks `CONFIG_MBEDTLS_HAVE_TIME_DATE=y`; allow an `esp_http_client_config_t` without a CA only in `src/platform/http_date.cpp`, and only when every URL literal in it starts with `http://` (R3, OD5-1, FR-016)

---

## Phase 2: Foundational (blocks every story)

### Tests first (`lib/core`)

- [X] T004 [P] Write `test/test_time/test_wall_clock.cpp`:
  - an Rtc sample makes the clock valid with source rtc;
  - an Ntp sample before the build is refused and leaves `valid` unchanged;
  - an Ntp sample after an Rtc start gives the signed correction (sample advanced by the uptime since it was taken);
  - an Http sample is ignored while the Ntp time is younger than 6 h, and accepted after 6 h or after `forget_ntp_freshness()`;
  - `write_rtc` is true on every accepted Ntp or Http sample and false for Rtc (I1);
  - a correction that crosses local midnight (23:59:58 → 00:00:03) is applied as one step (C2);
  - `invalidate()` makes the clock not valid until the next accepted sample;
  - valid stays true with no samples for days.

  (FR-003..FR-006, R4)
- [X] T005 [P] Write `test/test_time/test_zone.cpp`:
  - parse `<+03>-3` (UTC+3, no DST), `UTC0`, `CET-1CEST,M3.5.0,M10.5.0/3` and `EST5EDT,M3.2.0,M11.1.0`, plus `J` and `n` rule forms;
  - `utc_to_local` one second either side of both 2026 transitions of CET and EST;
  - invalid strings (``, `+3`, `<+03`, `CET-1CEST,M13.5.0,M10.5.0`, `ABC25`) are refused and leave the zone unchanged.

  (FR-012, TIME-04, R5)
- [X] T006 [P] Write `test/test_time/test_http_date.cpp`:
  - `Tue, 24 Nov 2026 11:41:17 GMT` parses to its UTC seconds;
  - wrong weekday, lower-case month, missing `GMT`, the RFC 850 form and an out-of-range field are refused.

  (FR-002, R2)
- [X] T007 [P] Extend `test/test_diag/test_command_parser.cpp` and `test/test_diag/test_command_args.cpp` for `tz`, `tz <string>`, `time sync`, `ntp on`, `ntp off`, `clock invalid` and `time jitter N` (10–3600; 9 and 3601 invalid) (contracts/serial-console.md)
- [X] T008 [P] Extend `test/test_diag/test_selftest_report.cpp` with the three forms of the `time:` line of contracts/serial-console.md (FR-015)

### Implementation (`lib/core`)

- [X] T009 Implement `lib/core/src/nvp/time/wall_clock.h/.cpp` (`TimeSource`, `TimeSample`, `SyncResult`, `WallClock::accept/invalidate/forget_ntp_freshness`) per data-model.md; T004 passes
- [X] T010 [P] Implement `lib/core/src/nvp/time/posix_zone.h/.cpp` (`Zone`, `parse_zone`, `utc_to_local`, `local_to_utc`, `kDefaultZone = "<+03>-3"`); keep `nvp/time/zone.h`'s inline UTC+3 helpers for the fixtures and Slice 3 tests; T005 passes
- [X] T011 [P] Implement `lib/core/src/nvp/time/http_date.h/.cpp`; T006 passes
- [X] T012 Add the commands to `lib/core/src/nvp/diag/command.h/.cpp` and the `time:` line (`ReportInput.time_line`) to `lib/core/src/nvp/diag/report.h/.cpp`; T007 and T008 pass
- [X] T013 Run `pio test -e native` and confirm all suites pass

**Checkpoint**: the clock model, zones and date parsing are proven on the host.

---

## Phase 3: User Story 1 — Correct clock (P1) 🎯 MVP

**Goal**: the panel shows network time within 10 s of Wi-Fi, accurate to 1 s, new seconds within 100 ms, and the board clock follows.

**Independent Test**: start with Wi-Fi, compare with a phone; `time jitter 600` (SC-001..SC-003).

- [X] T014 [P] [US1] Implement `src/platform/systime.h/.cpp`: `set_utc_us(int64_t)` via `settimeofday`, `utc_us()` via `gettimeofday` (R4)
- [X] T015 [US1] Implement `src/platform/time_sync.h/.cpp`:
  - `start()` (three servers, immediate mode, `esp_netif_sntp_init` with `start = true`);
  - `restart()`, `stop()`, `running()`;
  - the strong `extern "C" void sntp_sync_time(struct timeval*)` that stores the sample (UTC µs, uptime µs and the server's name, when lwIP gives one) under a lock and does not set the clock;
  - `take_sample(TimeSample&)`.

  (R1, FR-001)
- [X] T016 [US1] Rework `src/app/clock.h/.cpp`:
  - the real source is `platform::systime` plus `WallClock` plus the stored `Zone`;
  - `start()` sets the system clock from a valid board-clock reading, or to `NVP_BUILD_UTC` when none (R3);
  - `apply_pending()` takes samples from `time_sync` and the fallback, calls `WallClock::accept`, sets the system clock, writes the board clock when `write_rtc`, and logs one line per result in the format of contracts/serial-console.md;
  - `now()` keeps the simulated clock first (FR-011);
  - `describe_time()` for the `time:` report line;
  - a failing board-clock write is logged once per boot, not on every sync (spec edge case, C1);
  - publishes `valid` in one `std::atomic<bool>` (`clock::time_valid()`), which the network worker reads (U1).

  (FR-003..FR-006, FR-014, FR-015, TIME-08)
- [X] T017 [US1] In `src/app/net_worker.cpp`:
  - call `platform::time_sync::start()` on the first `JoinOk` or `TryOk` (running mode only);
  - call `restart()` on every later `JoinOk`;
  - publish `ip_up_since_ms` for the fallback timer.

  Starting at `TryOk` is intended: the owner's network has internet there, and the success screen's `Saat ve vakitler alınıyor` becomes true (analysis I2).

  (R1)
- [X] T018 [US1] In `src/app/bringup_app.cpp`, call `clock::apply_pending()` on every loop pass. When the displayed second differs from `clock::now()`'s, update the product screen at once and call `platform::display::refresh_now()` (R6, FR-009). Add the `time:` line to `print_report()`.
- [X] T019 [US1] Implement `time jitter N` in `src/app/bringup_app.cpp`: record the millisecond within the real second (`platform::systime`) at which each new second's flush returned, then print the count, median, p99 and max per contracts/serial-console.md (R6, SC-003)
- [X] T020 [US1] Build and flash on COM4. From the log, confirm the `time: ntp … ok` line within 10 s of `wifi: connected`, the `rtc written` mark and the report's `time: valid, source ntp`. Run `time jitter 600` and record the result in `specs/005-real-time/research.md` R6. Report the image size (SC-001, SC-003).

---

## Phase 4: User Story 2 — Honest waiting (P1)

**Goal**: `no_time` whenever the time is not valid; the board clock covers a start without Wi-Fi.

**Independent Test**: `clock invalid` with the router off shows reference 13; network time ends it within 1 s.

- [X] T021 [P] [US2] Extend `test/test_screen/test_system_state.cpp` with real inputs: clock not valid → `no_time`; clock valid and `today_present=false` → `no_data` (FR-007)
- [X] T022 [US2] In `src/app/bringup_app.cpp` `build_main_view`, feed `select_state` from `clock::now().valid` and `today_present=false` instead of the current inline choice; keep `state …` forcing (FR-007, R8)
- [X] T023 [US2] Implement `clock invalid` in `src/app/clock.cpp` and `src/app/bringup_app.cpp` (`WallClock::invalidate`, system clock to `NVP_BUILD_UTC`) per contracts/serial-console.md (R11)
- [X] T024 [US2] On the device: `clock invalid` with `ntp off`. Confirm `screen: main (state no_time)` and `time: not valid` in the report. Then `ntp on` and confirm the time is valid again within 10 s. Ask the owner to compare with reference 13 (FR-008, SC-005).

---

## Phase 5: User Story 3 — Stubborn about getting the time (P2)

**Goal**: the HTTP `Date` fallback after 60 s without SNTP; SNTP replaces it later.

**Independent Test**: `ntp off` → `time: http … ok` after 60 s, within 2 s of a phone (SC-004).

- [X] T025 [US3] Implement `src/platform/http_date.h/.cpp`:
  - `fetch(const char* url, int64_t* utc_us, uint32_t* rtt_ms, const char** why)` over plain HTTP, redirects off, `User-Agent` from `nvp::kUserAgent`;
  - reads only the `Date` header with `nvp::time::parse_http_date`;
  - the sample is `date + 500 ms + rtt / 2`;
  - refuses an rtt over 2000 ms.

  (R2, OD5-1)
- [X] T026 [US3] In `src/app/net_worker.cpp`, add the fallback timer: 60 s after the address with no accepted Ntp sample (`clock` tells the worker through a flag), fetch from `http://ezanvakti.imsakiyem.com/`, else `http://ezanvakti.emushaf.net/`; retry every 60 s; hand samples to `clock` (FR-002)
- [X] T027 [US3] Implement `ntp off` / `ntp on` and `time sync` in `src/app/serial_console.cpp`, `src/app/bringup_app.cpp` and `src/app/net_worker.cpp`. `ntp off` stops SNTP, calls `WallClock::forget_ntp_freshness`, and restarts the fallback timer (R11).
- [X] T028 [US3] Change the signature to `platform::https::probe(bool time_valid)` in `src/platform/https_probe.h/.cpp`; it refuses with "time not valid" when the argument is false. `src/app/net_worker.cpp` passes `clock::time_valid()` (the atomic of T016), so `src/platform` includes no app header (R3, U1).
- [X] T029 [US3] On the device: `ntp off`. Confirm `time: http ezanvakti.imsakiyem.com ok` after about 60 s and compare with a phone. Then `ntp on` and confirm the next `time: ntp … ok`. Record the result in research R2 (SC-004).

---

## Phase 6: User Story 4 — Midnight (P2)

**Goal**: the date changes once at midnight, without a glitch.

**Independent Test**: the host midnight check, and `clock sim … 23:59:50` on the device (SC-006).

- [X] T030 [P] [US4] Extend `sim/ui_render/redraw_check.cpp` with a midnight run on the no-data path (real date, no records). 23:59:59 → 00:00:00 must change the date line exactly once, and every flushed frame must hold one day's date only. Add a second case where a clock jump from 23:59:58 to 00:00:03 changes the date once (FR-010, R9, C2).
- [X] T031 [US4] On the device: `clock sim 2026-11-24 23:59:50` at speed 1, then `clock speed 60`. The owner watches the change; then `fixture off` / `clock rtc` (SC-006).

---

## Phase 7: User Story 5 — Time zone (P3)

**Goal**: a stored POSIX zone, default `<+03>-3`, effective at once.

**Independent Test**: `tz UTC0` moves the clock 3 h at once and survives a restart (SC-007).

- [X] T032 [US5] Add `load_tz(char*, size_t)` and `store_tz(const char*)` (NVS `set/tz`) to `src/platform/settings_store.h/.cpp` (FR-012)
- [X] T033 [US5] Handle `tz` and `tz <string>` in `src/app/serial_console.cpp` and `src/app/bringup_app.cpp` (replies per contracts/serial-console.md) (U2). In `src/app/clock.cpp`, load the zone at start (default `<+03>-3`), and apply `tz <string>` after `parse_zone` succeeds, storing it only then. Replace every `nvp::time::utc_to_local`/`local_to_utc` use in `src/app` with the stored zone; `time set` keeps local-time input in the stored zone (FR-012, FR-013).
- [X] T034 [US5] On the device: `tz UTC0` (the clock moves 3 h at once), restart (it is kept), `tz <+03>-3`, `tz bad` (refused) (SC-007)

---

## Phase 8: Polish & cross-cutting

- [X] T035 [P] Update the documents:
  - 03 §2 TIME-08 note (the writing half is done);
  - 04 §10 (date checks on, HTTPS waits for a valid clock, the plain-HTTP `Date` fallback; OD5-1) and §12 row "Power cut, no Wi-Fi" (the board clock covers it);
  - 05 §3 (SNTP, the mbedTLS date option) and §5 (NVS `set/tz`);
  - 06 roadmap Slice 5 outcome;
  - README commands.
- [X] T036 Owner checklist in `specs/005-real-time/owner-checklist.md` from quickstart §3, answers recorded; the overnight midnight and 24 h accuracy items, with the hourly corrections from the log recorded in research R7 (constitution VII, SC-002)
- [X] T037 Run `python tools/check.py`; report the image size; run `/speckit-converge`

---

## Dependencies & execution order

- **Phase 1 → Phase 2** (tests T004–T008 before T009–T012; T013 closes) → stories.
- **US1** (T014–T020) needs T009–T012.
- **US2** needs T016 and T022.
- **US3** needs T016 and T017.
- **US4** needs nothing beyond Phase 2 for T030.
- **US5** needs T010 and T016.
- **Polish** comes last.

## Parallel examples

- **Phase 2 tests:** T004–T008 can run in parallel.
- **Phase 2 implementations:** T010 and T011 alongside T009.
- **US1:** T014 alongside T015.
- **Host checks:** T021 and T030 can run at any time after Phase 2.

## Implementation strategy

1. **MVP:** Phases 1–3, network time with board-clock write and second alignment.
2. **Then:** US2 and US3 (honesty and fallback), US4 (midnight), US5 (zone).
3. **Last:** Polish, owner checklist, `check`, converge.

## Phase 9: Convergence

- [X] T038 While the time servers stay silent, repeat the HTTP Date fallback every 6 hours after an accepted fallback sample (the age at which `WallClock` lets an Http sample replace an Ntp one), so the clock does not drift for days on a network that blocks NTP; disarm it as soon as an SNTP sample is accepted. `src/app/net_worker.cpp` `drive_fallback`, `src/app/clock.h/.cpp` (`last_source()`) per FR-002, FR-006, TIME-02 (partial)
