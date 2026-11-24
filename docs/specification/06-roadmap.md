# Roadmap

The product is built in ten slices. Each slice is one Spec Kit feature: it gets its own specification, plan, task list and branch, ends in something the owner can see working, and leaves the main branch releasable. Prompts for every slice are in `docs/development/prompts/`.

Order matters. The early slices remove the largest risks first: can the board show a stable picture silently (1), is the time logic right (2), does the approved design render correctly (3). Networking and data come after there is a correct screen to show them on.

## Overview

| # | Slice | The owner can see | Main requirements | Needs |
|---|---|---|---|---|
| 0 | Bootstrap | a repository with Spec Kit, a constitution and passing empty builds | process | — |
| 1 | Silent board bring-up | a stable test picture, working touch, no sound; Wi-Fi details entered from a phone | SIL-01…06, MODE-01, hardware risks | 0 |
| 2 | Prayer-time engine | test results on the development machine | ENG-01…08 | 0 |
| 3 | Main screen with sample data | Design A on the panel, every time-of-day state, driven by a simulated clock | UI-01…06, `02-ui-spec.md` §1–6, 9, 11–12, SYS-06 | 1, 2 |
| 4 | Wi-Fi setup from the phone | QR setup screen, captive portal, connection, status overlay, restart-setup | MODE-02…06, NET-01…08, LOC-01…02, `02-ui-spec.md` §7, 8, 10 | 3 |
| 5 | Real time | the real clock and date; “waiting for time” state | TIME-01…07 | 4 |
| 6 | Real prayer times | Diyanet times for the chosen district, kept across restarts | DATA-01, 02, 06, 07, 08; first provider; store | 5 |
| 7 | Automatic refresh and fallback | the device keeps itself up to date and survives outages, with honest labelling | DATA-03…05, LOC-03, all providers except the official API, `04-data-sources.md` §4–7 | 6 |
| 8 | Settings page | change location and display options from a browser on the home network | SET-01…03 (SET-04 dropped, OD1), NET-07, SEC-01…05 | 7 |
| 9 | Hardening and release | a version 1.0.0 that has run for 24 hours unattended | SYS-01…05, targets in `03-functional-spec.md` §12 | 8 |
| 10 | Optional extras | official Diyanet API with the owner’s credentials (the battery-backed clock moved to Slices 3 and 5) | `04-data-sources.md` §3.3 | 9 |

## Slice 0: Bootstrap

**Goal.** A repository Claude Code can work in productively.
**Delivers.** Git repository; Spec Kit initialised for Claude Code; project constitution capturing the hard constraints; `CLAUDE.md`; PlatformIO project skeleton with a host test environment and a device environment that both build; the silence check wired into the build; continuous checks runnable with one command.
**Done when.** `/speckit.constitution` output is committed, an empty host test passes, an empty firmware builds for the board, and the silence check runs.

## Slice 1: Silent board bring-up

**Goal.** Prove the hardware basics before any product code depends on them.
**Delivers.** Firmware that silences the audio path first, then brings up panel, touch and backlight, shows the boot screen and a diagnostic screen: colour bars, a one-pixel border, fine vertical lines, live touch coordinates, free memory. A stress mode exercises Wi-Fi scan, HTTPS and flash writes while the picture is shown. Board revision handling. A recorded stack decision.
**Delivers, added by owner decisions on 2026-11-20** (`specs/001-silent-board-bringup/spec.md`):
- A minimal Wi-Fi setup so that stress mode joins the home network and the HTTPS request really runs: an on-device settings screen starts the device’s own access point, a setup page on `http://192.168.4.1/` (`GET /`, `GET /api/scan`, `GET /api/status`, `POST /api/setup`) takes the network name and password, and the details are stored in NVS (namespace `wifi`, keys `ssid` and `pass`; the access-point password in namespace `ap`, key `pass`).
- The diagnostic and settings screens are permanent product code (`lib/core/src/nvp/diag`, `lib/ui/src/diag`). From Slice 3 on they are a hidden mode, reached by a hidden gesture or the serial command `diag` (FR-007a).
- The backlight is always fully on; there is no dimming (owner decision OD1).
**Done when.** The owner confirms: correct colours and orientation, border visible on all four edges, no shift or flicker during ten minutes of stress, touch accurate at the four corners, backlight fully on and steady, and no sound at any point.
**Out of scope.** Any product UI; the full setup flow of Slice 4 (QR code, captive-portal DNS, location, classified errors).
**Outcome.** Board DIS02170A V1.4. The firmware moved from Arduino-ESP32 to plain ESP-IDF 5.5.5 with `esp_lcd` and LVGL 9.6.0, because only that build kept the picture stable under Wi-Fi load (`docs/decisions/0002-display-stack.md`). Rule for every later slice: no flash write (NVS or cache files) while a Wi-Fi scan is in progress.

## Slice 2: Prayer-time engine

**Goal.** Correct time logic, proven without hardware.
**Delivers.** `lib/core`: day record and validation, period computation, column states, countdown, kerahat intervals, Turkish formatting of durations, dates and period names, with the full boundary test suite.
**Done when.** All tests in ENG-07 and the fixture expectations pass on the development machine.
**Out of scope.** Anything that touches the device, the network or LVGL.

## Slice 3: Main screen with sample data

**Goal.** The approved design, pixel-faithful, on the real panel.
**Delivers.** Converted fonts; `lib/ui` main screen; the host UI harness that renders every fixture to PNG; development serial commands to set simulated time, speed and system state; the six system-state variants rendered from forced state.
**Done when.** Snapshots for fixtures 01–14 have been reviewed against the references within the tolerances of `02-ui-spec.md` §12, and the owner confirms on the panel that the screen is legible from three metres and matches the reference renders for the listed times of day.
**Out of scope.** Network time, real data, Wi-Fi.
**Added in Slice 3 (owner decision, spec 003 OD3-1).** The time comes from the board's battery-backed BM8563 clock at every start (TIME-08, read half); development commands set it. Without loaded prayer times the device starts in the truthful `no_data` state.

## Slice 4: Wi-Fi setup from the phone

**Goal.** A stranger can put the device on their Wi-Fi with only a phone.
**Delivers.** Access point with generated password; setup screen with QR code; captive portal page with network scan, password entry and province/district selection from the bundled list; connection attempt with classified errors; stored credentials and location; reconnect with backoff; status overlay; restart-setup flow; the place-list generator and the generated list. It builds on Slice 1’s Wi-Fi setup rather than replacing it: the same NVS keys (`wifi/ssid`, `wifi/pass`, `ap/pass`) and the same portal paths, extended to the full NET-04 set (FR-025 of spec 001).
**Done when.** The owner completes setup from an iPhone and from an Android phone, sees each error message by provoking it, power-cycles the device and sees it reconnect, and returns to setup through the overlay.
**Out of scope.** Using the location for anything yet; the settings page.

## Slice 5: Real time

**Goal.** The clock on screen is the real local time, or the screen says it is not.
**Delivers.** SNTP with multiple servers, the HTTP `Date` fallback, time-zone setting, validity rule, the `no_time` state, midnight rollover on the real clock. The main screen now runs on real time with the sample day.
**Done when.** The clock matches a reference clock to the second; blocking NTP on the router still yields a correct clock; pulling the plug and booting without Wi-Fi shows `no_time`.

## Slice 6: Real prayer times

**Goal.** Today’s Diyanet times for the chosen district, stored on the device.
**Delivers.** HTTPS client with certificate verification; the İmsakiyem provider with streaming parse; batch validation; the persistent day store with atomic replace; the main screen fed from the store; `ok`, `offline` and `no_data` states; Hijri date from the data.
**Done when.** Times on screen equal Diyanet’s website for the district on three different days of the stored range; after a restart with Wi-Fi off the same times appear from the store; a corrupted store file is detected and rebuilt.
**Out of scope.** Scheduling, other providers, calculated fallbacks.

## Slice 7: Automatic refresh and fallback

**Goal.** The device looks after its own data and tells the truth when it cannot.
**Delivers.** Refresh triggers and night-window scheduling; the emushaf provider and coverage extension; coordinate resolution; the Aladhan provider; on-device calculation with self-calibration; rank-based merging; plausibility check; backoff and provider cool-downs; the `calculated` and `cache_ending` states; mismatch logging.
**Done when.** In the planner’s host tests every scenario in `04-data-sources.md` §12 produces the stated outcome; on the device, blocking each mirror host on the router in turn has no visible effect, blocking both with an emptied store yields `calculated`, and unblocking restores `ok` without intervention.

## Slice 8: Settings page

**Goal.** Everything adjustable is adjustable from a browser on the home network.
**Delivers.** Settings page with PIN; location change with safe store hand-over; kerahat options; soon threshold; manual refresh; advanced options; factory reset; mDNS name; overlay shows the PIN and address QR. No brightness or night-dimming settings: the backlight is always fully on (owner decision OD1, 2026-11-20; SET-04 dropped).
**Done when.** The owner changes district and sees new times within a minute, toggles each display option and sees it take effect, fails the PIN five times and sees the lock-out, and performs a factory reset.

## Slice 9: Hardening and release

**Goal.** Leave it running and forget about it.
**Delivers.** Watchdogs; power-loss tests on store and settings writes; memory and restart counters in the status API; 24-hour soak with a log summary; final size report; boot time measurement; user-facing `README` with setup instructions in Turkish; version 1.0.0 tag.
**Done when.** The targets in `03-functional-spec.md` §12 are measured and met, and the soak log shows no restart and flat memory.

## Slice 10: Optional extras

Only on the owner’s request.

- **Official Diyanet API provider**, when the owner has credentials: login, monthly fetch, quota-aware scheduling, comparison against the mirrors.
- **Battery-backed clock** (built into the Advance board, the project’s target since 2026-11-20), so the device shows times after a power cut without Wi-Fi.

## After version 1

Ideas that were deliberately left out: monthly timetable screen, Qibla direction, Ramadan view, religious-day list, more languages, locations outside Türkiye, over-the-air update on boards with enough flash. None of them may add sound.
