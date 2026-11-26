# Implementation Plan: Real prayer times

**Branch**: `feat/006-prayer-data` | **Date**: 2026-11-24 | **Spec**: [spec.md](spec.md)

**Input**: Feature specification from `specs/006-prayer-data/spec.md`

## Summary

The device fetches the owner's district from İmsakiyem v2 over HTTPS, from today up to a year ahead, with v1 `range` as the in-provider fallback.
- **Parsing:** the answer is parsed while it streams in, by a small field-filtering JSON tokenizer.
- **Validation:** the whole batch is checked (ENG-01, dates, district, emptiness).
- **Merging:** the rules of 04 §4.
- **Storage:** one file per district on LittleFS, with a versioned header and a CRC-32, replaced by write-new, verify, atomic rename.
- **At run time:** the network worker owns the store and publishes a RAM copy. The UI looks up yesterday, today and tomorrow for the engine, and the screen states and overlay come from it.
- **Live check:** both İmsakiyem interfaces and emushaf agreed for 27–30 days. The 2027 calendar is already published, which corrects 04 §3.1.

## Technical Context

**Language/Version**: C++17 on ESP-IDF 5.5.5 and the native host; Python 3 for tools.

**Primary Dependencies**: none new.
- `esp_http_client` with the full CA bundle and certificate date checks (Slices 1 and 5).
- `joltwallet/littlefs` 1.22.3 (littlefs 2.11): atomic rename, commit on close.
- A JSON tokenizer of our own in `lib/providers` (R2).

**Storage**: LittleFS `/littlefs/store_<district>.bin`, 32 B header + 20 B per record, at most 400 records (8 kB).

**Testing**:
- **Unity on native**, tests first:
  - tokenizer;
  - parsers on the recorded fixtures (`test/fixtures/providers/`) and altered variants;
  - validation reasons;
  - merge;
  - store format;
  - save with a failure injected at every file-system call.
- **Device:** serial log and the owner checklist, including a comparison of three dates with Diyanet's website.

**Target Platform**: CrowPanel Advance 7.0" V1.4; the owner's district (Ankara 9206).

**Performance Goals**:
- Real times within 20 s of the Wi-Fi connection.
- The UI never touches flash.
- A store update is visible within 1 s.

**Constraints**:
- TLS with dates on; HTTPS only with a valid clock.
- One request at a time; no write during a scan.
- The answer is never held whole.
- Image ≤ 2.9 MiB.
- Internal heap during a fetch watched (⚠ measured).

**Scale/Scope**: about 1 500 lines of core and provider code with tests, about 400 lines of device code.

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-checked after Phase 1 design.*

| Principle | Status | How this plan complies |
|---|---|---|
| I. Silence | Pass | No audio. |
| II. Truthful display | Pass | Only Diyanet-derived records (`source.type diyanet`, rank 40) are stored and shown. A bad or partial batch is rejected whole. A bad store file is never shown. `no_data` when today is missing. The overlay shows the real source and update time. |
| III. Design contract | Pass | `ok`, `offline` and `no_data` match references 01, 10 and 14. The overlay rows use 02 §7.4 formats and 02 §9.5 names. No new strings. |
| IV. Offline first | Pass | The screen is served from the store. A restart without Wi-Fi shows stored times at once. |
| V. Verify, do not assume | Pass | The live check of 04 §13 was done and recorded as fixtures; 04 is corrected where the service differs (2027 published, v2 404, emushaf starts 5 days back). littlefs atomicity is cited from its own README and header. Heap peak ⚠ measured. |
| VI. Testable core | Pass | Tokenizer, parsers and validation are in `lib/providers`; store, merge and the file-system interface in `lib/core`. All are host-tested first, power loss included. HTTPS and LittleFS stay in `src/platform`. |
| VII. Owner in the loop | Pass | The brief's nine questions plus three dates to compare with Diyanet. |
| VIII. Small, private, polite | Pass | One host (04 §11), one request per refresh, User-Agent with project and version, no identifiers, TLS with dates. |
| IX. Resource discipline | Pass | Store writes only on a successful refresh, atomic. No flash on the UI thread. No body buffering. |
| X. Simplicity and scope | Pass | One provider. The scheduler, the second mirror and calculation are left to Slice 7. |

## Project Structure

### Documentation (this feature)

```text
specs/006-prayer-data/
├── spec.md, plan.md, research.md (R1–R10), data-model.md, quickstart.md
├── contracts/serial-console.md
├── checklists/requirements.md
└── tasks.md                     # next: /speckit-tasks
test/fixtures/providers/         # recorded answers (R1), README
```

### Source Code (repository root)

```text
lib/providers/                   # new library (pure C++, no SDK), library.json
  src/nvp/providers/json_stream.h/.cpp     # streaming tokenizer with key paths (R2)
  src/nvp/providers/imsakiyem.h/.cpp       # v2 and v1 parsers -> DayRecord callback
  src/nvp/providers/validate.h/.cpp        # batch validation reasons (R4)
  src/nvp/providers/http_get.h             # abstract fetch interface (R3)
lib/core/src/nvp/store/
  file_system.h                            # interface (R5)
  store.h/.cpp                             # format, load, save (atomic), find, covers
  merge.h/.cpp                             # 04 §4 (R6)
lib/core/src/nvp/diag/command, report      # data refresh|show|corrupt|erase; data: line
src/platform/
  https_fetch.h/.cpp                       # HttpGet on esp_http_client, streaming (R3)
  littlefs_fs.h/.cpp                       # FileSystem on the mounted LittleFS
src/app/
  data_service.h/.cpp                      # refresh, store ownership, DataView publish (R7, R8)
  net_worker.cpp                           # calls data_service on the worker; Wi-Fi-down time
  bringup_app.cpp                          # engine input from DataView; states; overlay; commands
test/test_providers/                       # tokenizer, parsers, validation (fixtures)
test/test_store/                           # format, merge, power-loss
docs/specification/04-data-sources.md      # corrected (done in planning)
```

**Structure Decision**: `lib/providers` is created as 05 §4 describes: pure, with an abstract HTTP interface.
- The store logic is in `lib/core` behind a file-system interface.
- The worker owns I/O.
- The UI reads a published copy.

## Phase 0: Research

Complete: [research.md](research.md) R1–R10. The live check found the mirrors in agreement, so planning was not stopped. 04-data-sources §3.1 and §3.2 are corrected.

## Phase 1: Design

Complete: [data-model.md](data-model.md), [contracts/serial-console.md](contracts/serial-console.md), [quickstart.md](quickstart.md). Post-design constitution check: all pass.

## Complexity Tracking

| Item | Why | Simpler alternative rejected because |
|---|---|---|
| Own streaming JSON tokenizer | A year of v1 is about 300 kB and the RAM budget is tight; 05 §3 asks for streaming with field filtering. | `cJSON` needs the whole body; ArduinoJson would be a new dependency that still builds a document. |
