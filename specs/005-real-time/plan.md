# Implementation Plan: Real time

**Branch**: `feat/005-real-time` | **Date**: 2026-11-24 | **Spec**: [spec.md](spec.md)

**Input**: Feature specification from `specs/005-real-time/spec.md`

## Summary

The device gets the time from three SNTP servers as soon as it is connected in running mode, and resyncs every hour.
- **Validity:** every answer passes through one pure validity rule (`nvp::time::WallClock`), which refuses times before the build, applies corrections at once and logs them.
- **Board clock:** after each network sync the board's clock is written, so a start without Wi-Fi shows the right time (TIME-08).
- **Fallback:** if no SNTP answer arrives within 60 s, the device reads the `Date` header of a plain-HTTP answer from a host it already uses (OD5-1).
- **Certificate dates:** checked from now on, and HTTPS waits for a valid clock.
- **Time zone:** a POSIX TZ string, parsed and converted in `lib/core`, stored in NVS, default `<+03>-3`.
- **Seconds:** the panel shows each new second within 100 ms, by redrawing on the second change.
- **Screen states:** `no_time` and `no_data` are now chosen from real inputs.

## Technical Context

**Language/Version**: C++17 on ESP-IDF 5.5.5 (`espressif32@8.0.0`) and the native host; Python 3 for tools.

**Primary Dependencies**: none new.
- ESP-IDF `esp_netif_sntp` (lwIP SNTP), with the weak `sntp_sync_time` hook overridden (R1).
- `esp_http_client` for the fallback (R2).
- `mbedtls` with `CONFIG_MBEDTLS_HAVE_TIME_DATE=y` (R3).

**Storage**: NVS `set/tz` (new). The board clock BM8563 is now also written (Slice 3 driver).

**Testing**:
- **Unity on native**, tests first: `WallClock`, `Zone` (POSIX TZ including DST transitions), `parse_http_date`, state selection from real inputs, command parser additions.
- **Host harness**: a midnight redraw check.
- **Device**: serial log checks (`time jitter`, sync lines) and the owner checklist.

**Target Platform**: CrowPanel Advance 7.0" V1.4; the owner's network.

**Performance Goals**: clock valid ≤ 10 s after Wi-Fi; ≤ 1 s error over 24 h; new second on the panel ≤ 100 ms after the boundary (p99).

**Constraints**:
- No new host contacted (SEC-02).
- TLS date checks on, with HTTPS only once the clock is valid (OD5-1).
- No storage write during a Wi-Fi scan: the zone is written only on the `tz` command; the board clock is on I²C, not flash.
- Silent (SIL-01).
- Image ≤ 2.9 MiB.

**Scale/Scope**: about 900 lines of core and device code, plus tests.

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-checked after Phase 1 design.*

| Principle | Status | How this plan complies |
|---|---|---|
| I. Silence | Pass | No audio. The ELF silence check stays. |
| II. Truthful display | Pass | `no_time` until a checked source gives a time; times before the build are refused from every source; no sample day on the main screen (OD3-2 kept). |
| III. Design contract | Pass | `no_time` matches reference 13, which already rendered in Slice 3. No new strings on screen. |
| IV. Offline first | Pass | The clock keeps running without Wi-Fi, and the board's clock gives the time after a power cut without Wi-Fi. |
| V. Verify, do not assume | Pass | The SNTP API, Kconfig defaults, the weak hook and the mbedTLS date option come from the local 5.5.5 sources. The fallback hosts' `Date` headers were checked live. Crystal drift is a ⚠ measured on the device (R7). |
| VI. Testable core | Pass | Validity, zones and date parsing are pure `lib/core` with tests first. SNTP, HTTP, the system clock and NVS stay in `src/platform`. |
| VII. Owner in the loop | Pass | Six-question checklist (quickstart §3). |
| VIII. Small, private, polite | Pass | TLS gets stronger (dates checked, OD5-1). The fallback uses hosts already contacted, sends only the request line, `Host` and `User-Agent`, and runs at most once a minute and only while SNTP fails. NTP traffic is one request per hour to three operators. |
| IX. Resource discipline | Pass | No per-second allocation. NVS is written only on `tz`. |
| X. Simplicity and scope | Pass | One clock model and one hook. The zone parser covers POSIX TZ only, no zone database. |

## Project Structure

### Documentation (this feature)

```text
specs/005-real-time/
├── spec.md (OD5-1), plan.md, research.md (R1–R11), data-model.md, quickstart.md
├── contracts/serial-console.md
├── checklists/requirements.md
└── tasks.md                     # next: /speckit-tasks
```

### Source Code (repository root)

```text
lib/core/src/nvp/time/
  wall_clock.h/.cpp    # validity, sources, corrections, RTC write policy (R4)
  zone.h/.cpp          # POSIX TZ parser and conversion; replaces the fixed UTC+3 zone.h (R5)
  http_date.h/.cpp     # IMF-fixdate parser (R2)
lib/core/src/nvp/diag/command.h/.cpp   # tz, time sync, ntp on|off, clock invalid, time jitter
lib/core/src/nvp/diag/report.h/.cpp    # time: line
src/platform/
  time_sync.h/.cpp     # SNTP setup, weak sntp_sync_time override -> locked sample (R1)
  http_date.cpp/.h     # plain-HTTP Date fetch, redirects off (R2, OD5-1)
  systime.h/.cpp       # gettimeofday/settimeofday wrappers
  settings_store       # + set/tz
  https_probe.cpp      # refuses while time is not valid (R3)
src/app/
  clock.h/.cpp         # system clock + WallClock + zone; RTC write after sync; sim unchanged
  net_worker.cpp       # starts SNTP in running mode; 60 s fallback timer; samples to the app
  bringup_app.cpp      # redraw on the second change (R6); state from real inputs (R8)
sdkconfig.defaults     # LWIP_SNTP_MAX_SERVERS=3, MBEDTLS_HAVE_TIME_DATE=y
tools/check_tls.py     # + date-check rule, + plain-HTTP allowance for http_date.cpp only
sim/ui_render/redraw_check.cpp  # + midnight run (R9)
test/test_time/        # wall clock, zone, http date
```

**Structure Decision**: the layering of 05-architecture §4.
- The validity and conversion logic is pure, in `lib/core`.
- The SNTP and HTTP clients and the system clock are in `src/platform`.
- The app wires them through one `clock::now()`.

Slice 3's clock module is extended, not replaced. Its simulated clock still overrides real time (FR-011).

## Phase 0: Research

Complete: [research.md](research.md) R1–R11. Owner decision OD5-1 (certificate dates checked; plain-HTTP fallback) is recorded in the spec.

## Phase 1: Design

Complete: [data-model.md](data-model.md), [contracts/serial-console.md](contracts/serial-console.md), [quickstart.md](quickstart.md). Post-design constitution check: all pass.

## Complexity Tracking

None.
