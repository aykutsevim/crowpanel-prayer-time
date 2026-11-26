# Implementation Plan: Automatic refresh and fallback

**Branch**: `feat/007-refresh-fallback` | **Date**: 2026-11-26 | **Spec**: [spec.md](spec.md)

**Input**: Feature specification from `specs/007-refresh-fallback/spec.md`

## Summary

The device keeps its own data current and falls back in a fixed order, with Diyanet data always first (owner direction).
- **Planner:** a pure state machine in `lib/providers` decides when to refresh (setup, location change, boot, reconnection, weekly at a per-device night minute, nightly while under 30 days, retries with backoff, hourly while calculated, manual). Host-tested with scripted time and responses.
- **Runner:** `refresh_with` becomes a step runner over İmsakiyem → emushaf → Aladhan, with a request cap of 12, cool-downs, a 24 h skip after five failed refreshes, a software block list, the plausibility check and its two-mirror exception. The device's own calculation fills today..+7 in RAM when nothing else covers today, even offline.
- **Calculation:** a C++17 port of adhan-js v4.4.6 (MIT), Turkey method, with self-calibration and a tabular Hijri date for calculated days.
- **Coordinates:** a table bundled with the place list (all 1003 entries have high-confidence coordinates); the İmsakiyem district detail only if a place is missing. Aladhan's geocoding is dropped (placeholder results).
- **Live check:** Aladhan is within ±1 min of the published times for Ankara but its Hijri date differs on 16 of 38 days; emushaf is served from a 5-day cache, so alone it may stop short of 30 days.
- **Screen:** the real inputs for `calculated` and `cache_ending`; the rendering exists since spec 003.

## Technical Context

**Language/Version**: C++17 on ESP-IDF 5.5.5 and the native host; Python 3 for tools.

**Primary Dependencies**:
- New, vendored: the astronomical core of **adhan-js v4.4.6** (MIT, commit `b6e04cd9`), ported to C++17 (R3).
- Existing: `esp_http_client` with the CA bundle, mbedTLS in PSRAM (spec 006 R3), LittleFS, the JSON tokenizer of spec 006.

**Storage**:
- `store_<district>.bin` unchanged (format 1).
- New `/littlefs/refresh.bin`, 64 B, CRC, atomic replace (R7).
- Coordinates table in flash, generated (about 16 kB).

**Testing**:
- **Unity on native**, tests first: planner scenarios with a scripted clock and fetch layer; parsers on the new fixtures; plausibility; calculation against the adhan test data and the 128 published Ankara days; calibration; merge ranks; persistence with power cuts; the weekly request budget.
- **Device:** serial log and the owner checklist with the block commands.

**Target Platform**: CrowPanel Advance 7.0" V1.4; Ankara 9206.

**Performance Goals**:
- A normal week: 1 request.
- Calculated times on screen within 1 s of the day starting without a record.
- `calculated` left within 70 min of a mirror returning.

**Constraints**:
- At most 12 requests per refresh; one at a time; never during a scan's flash write rule (spec 006).
- Image ≤ 2.9 MiB (now 1.664 MiB).
- No heap in the calculation; no exceptions.
- TLS checks on.

**Scale/Scope**: about 2 000 lines of core and provider code with tests (planner, runner, two parsers, calculation port, calibration, Hijri), about 400 lines of device code, one generator change.

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-checked after Phase 1 design.*

| Principle | Status | How this plan complies |
|---|---|---|
| I. Silence | Pass | No audio. |
| II. Truthful display | Pass | Diyanet-derived first and never replaced by calculation; `calculated` whenever today's record is below rank 40, with `≈`, 44 px and the source named in the panel; computed Hijri only in that state; `cache_ending` counts the real days. |
| III. Design contract | Pass | References 11 and 12 already implemented and snapshot-tested (spec 003); no new strings (all five source names exist in `tr.h`). |
| IV. Offline first | Pass | The device's own calculation works with no network; the store and planner state survive restarts. |
| V. Verify, do not assume | Pass | 04 §13 items 2, 4, 6 checked live and recorded as fixtures; the library chosen from its repository and pinned to a commit; 04 corrected (§3.2 cache, §3.4 Hijri and geocoding, §6 coordinates). |
| VI. Testable core | Pass | Planner, runner, parsers, calculation and calibration are pure and host-tested first; the device only feeds events and executes fetches. |
| VII. Owner in the loop | Pass | The brief's eight checks with exact commands (quickstart). |
| VIII. Small, private, polite | Pass | 1 request a normal week; cap 12 per refresh; backoff, cool-downs, 24 h skip, night minute spread by MAC; no identifiers sent; hosts of 04 §11 only (fewer: no geocoding). |
| IX. Resource discipline | Pass | `refresh.bin` written at most once per refresh; the device's own calculation kept in RAM; coordinates table 16 kB of flash. |
| X. Simplicity and scope | Pass | The official API is an empty slot; no settings UI; the calculation is a port, not new astronomy. |

## Project Structure

### Documentation (this feature)

```text
specs/007-refresh-fallback/
├── spec.md, plan.md, research.md (R1–R11), data-model.md, quickstart.md
├── contracts/serial-console.md
├── checklists/requirements.md
└── tasks.md                     # next: /speckit-tasks
test/fixtures/providers/          # + aladhan/, emushaf 502, imsakiyem district detail (R1, R2, R4)
```

### Source Code (repository root)

```text
lib/core/src/nvp/calc/            # adhan-js port (R3): astronomy, solar time, prayer times (Turkey), MIT notice
lib/core/src/nvp/calc/calibrate.h/.cpp   # offsets, ±10 limit, tabular Hijri + offset (R8)
lib/core/src/nvp/store/atomic_file.h/.cpp # write-new/verify/rename, shared by store and refresh state (R7)
lib/core/src/nvp/store/refresh_state.h/.cpp # refresh.bin format (R7)
lib/core/src/nvp/net/coords.h/.cpp        # lookup in the generated table (R4)
lib/providers/src/nvp/providers/
  emushaf.h/.cpp                  # parser (R1)
  aladhan.h/.cpp                  # calendar parser (R2)
  plausible.h/.cpp                # 04 §7 check against the uncalibrated calculation
  planner.h/.cpp                  # when to refresh (R5)
  refresh.h/.cpp                  # step runner, cap, cool-downs, blocks, two-mirror exception (R6)
src/generated/places_coords.h     # generated table (R4)
tools/gen_places.py               # also writes the coordinates table
src/app/data_service.*            # planner events, runner, local fill, calibration, commands
src/app/net_worker.*, bringup_app.cpp  # events, MAC, state inputs (R9)
lib/core/src/nvp/diag/command.*, report.*   # data block|unblock|plan|calib; report line
test/test_planner/                # scenarios, budget
test/test_calc/                   # port, calibration, Hijri
test/test_providers/, test/test_store/  # new parsers, plausibility, ranks, refresh.bin power loss
docs/specification/04-data-sources.md   # corrected in planning
```

**Structure Decision**: the planner and runner stay in `lib/providers` (05 §4: the refresh planner is a pure state machine there); the calculation goes in `lib/core` with the engine it feeds; the device code only translates events and executes fetches.

## Phase 0: Research

Complete: [research.md](research.md) R1–R11. Decisions that change the spec or 04:
- **FR-015:** the coordinates come from the bundled table; the Aladhan address step is dropped (R4). Spec FR-015 and 04 §6 updated.
- **Goal reached** includes "every Diyanet-derived provider answered" (R6), because emushaf's cached month and the year end can make 30 days unreachable without any failure.
- **Calculated Hijri** is tabular plus a learned offset, not Aladhan's (R2, R8).

## Phase 1: Design

Complete: [data-model.md](data-model.md), [contracts/serial-console.md](contracts/serial-console.md), [quickstart.md](quickstart.md). Post-design constitution check: all pass.

## Complexity Tracking

| Item | Why | Simpler alternative rejected because |
|---|---|---|
| Porting adhan-js instead of vendoring a C++ library | No maintained C++17, exception-free, permissively licensed library exists (R3). | adhan-cpp needs C++20 and exceptions; others are unmaintained or GPL. |
| A separate `refresh.bin` | Counters change on failures, which do not rewrite the store; calibration must outlive the Diyanet records it came from. | Store format 2 would rewrite 8 kB on every failure; NVS would need its own power-loss proof. |
| Two-mirror plausibility exception | Owner direction: Diyanet data has priority over the calculation; wrong coordinates must not block correct published times. | Plain rejection (04 §7) would show calculated times while two published sources agree. |
