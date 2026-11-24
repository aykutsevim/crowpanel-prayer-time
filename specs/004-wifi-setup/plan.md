# Implementation Plan: Wi-Fi setup from the phone

**Branch**: `feat/004-wifi-setup` | **Date**: 2026-11-24 | **Spec**: [spec.md](spec.md)

**Input**: Feature specification from `specs/004-wifi-setup/spec.md`

## Summary

This slice turns Slice 1's engineering hotspot into the product setup flow.
- **Device network.** The device starts in setup when the Wi-Fi details or the location are missing. The setup screen shows its network, password and QR code.
- **Phone page.** A wildcard DNS responder and a catch-all `302` bring the phone's sign-in sheet up. There the owner picks a network and a district from the bundled place list.
- **Connection attempt.** The attempt runs for up to 20 s with the device's network still up. Failures are classed from the driver's disconnect reasons. On success the details and the location are stored, and the network closes after 5 s.
- **Running mode.** Reconnects with backoff and never reopens the device's network on its own. A tap opens the status overlay; holding it for 5 s offers `Baştan kur`.

**Building blocks:**
- One pure, table-driven mode machine in `lib/core`, evolved from Slice 1's machine.
- A pure DNS-reply builder and a failure classifier in `lib/core`.
- The page as plain HTML, CSS and JS in `data/portal`, gzipped into a C++ array.
- A generator for the place list. It found 134 alias districts, now mapped to their centre ids (OD4-3).
- The three screens and the touch classifier in `lib/ui` and `lib/core`, checked against references 15 to 17.

## Technical Context

**Language/Version**: C++17 (gnu++17) on ESP-IDF 5.5.5 (`espressif32@8.0.0`, `framework-espidf@4.50505.0`) and the native host. Python 3 for the tools. Plain ES2017 JavaScript for the page. Node 24 for the page tests and the font conversion, on the development machine only.

**Primary Dependencies**: none new.
- LVGL 9.6.0 with `LV_USE_QRCODE 1`, using its bundled Nayuki `qrcodegen` (MIT, error correction M) (R11).
- ESP-IDF `esp_wifi`, `esp_netif`, `esp_http_server` and lwIP sockets, already in `REQUIRES`.
- `lv_font_conv@1.5.3` for the three new fonts.

**Storage**: NVS. The Slice 1 keys (`wifi/ssid`, `wifi/pass`, `ap/pass`) are kept. New keys: `loc/district_id`, `loc/province`, `loc/district`, `sec/pin` (R10). LittleFS is not touched.

**Testing**:
- **Unity on native**, tests first (constitution VI):
  - `mode_step` over the full grid;
  - `classify_failure`, `dns_reply`, `auth_class`, `merge_scan`, `signal_class`;
  - `wifi_qr_payload`, `place_lookup`, `title_case_tr`, `glyph_filter`;
  - `TouchClassifier`, and the form and submission validation.
- **`node --test`** for the page logic.
- **Python unit tests** for `gen_places.py` (canned responses, no network), `gen_web.py` and `scan_log_secrets.py`.
- **Host harness**: fixtures 15 to 17 compared with the references.
- **Owner checklist** on two phones.

**Target Platform**: CrowPanel Advance 7.0" V1.4 (ESP32-S3-WROOM-1-N16R8). The owner's iPhone and Android phone.

**Project Type**: embedded firmware with host-testable libraries and an embedded web page.

**Performance Goals**:
- The setup page loads in under 2 s on a joined phone (about 25 kB of page plus about 10 kB of place list, gzipped).
- The scan list is available as soon as the page asks, because it is cached before the network starts.
- From power-on to the main screen takes under 3 minutes for a first-time user (SC-001).

**Constraints**:
- Exact strings and the 02 §12 tolerances.
- The page ≤ 25 600 B gzipped, with no external requests.
- No storage write during a scan.
- No secrets in logs or responses.
- Fonts ≤ 300 kB, image ≤ 2.9 MiB.
- Internal heap: watch the free and largest-block figures with the HTTP server, the DNS socket and the access point up together (05 §5).
- Silent throughout.

**Scale/Scope**:
- Three screens and one overlay; one page; a place list of 1003 entries.
- About 2 500 lines of C++ and 700 lines of JS/CSS/HTML, plus tools and tests.

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-checked after Phase 1 design.*

| Principle | Status | How this plan complies |
|---|---|---|
| I. Silence | Pass | No audio. Touch gives no feedback (SIL-06). The ELF silence check runs on every build. Nothing new drives a pin. |
| II. Truthful display | Pass | The overlay shows dashes for data that does not exist yet. The settings-page QR stays hidden until the page exists (OD4-2). `Saat ve vakitler alınıyor` lasts only for the 5 s grace, and then the truthful `no_data` / `no_time` main screen appears. Alias districts show their own name and use the centre's Diyanet times, which is exactly what Diyanet publishes for them (OD4-3). |
| III. Design contract | Pass | Screens 15 to 17 follow 02 §7 and are compared with the references. Strings are byte for byte. The "not supported" mark that NET-06 needs, and 02 §10 lacks, was approved by the owner as `desteklenmiyor` (OD4-4). The page's rules for re-enabling the form add no text. |
| IV. Offline first | Pass | The main screen never waits for the network. Setup has no timeout, and the running mode keeps the main screen up while it reconnects. |
| V. Verify, do not assume | Pass | Captive-portal behaviour comes from AOSP source, Apple's documentation and Espressif's example (R1). AP+STA and scanning behaviour comes from the ESP-IDF 5.5 guide and `manager.c` (R2, R3). Reason and auth codes come from the local 5.5.5 headers (R3, R4). The place list's shape was inspected live, which is how the alias ids were found (R6). The QR ECC level comes from the LVGL source (R11). The unverified phone behaviours are owner-checklist items. |
| VI. Testable core | Pass | All decisions are pure functions in `lib/core`: modes, classification, DNS, scan, QR payload, lookup, touch, text. They are tested first. `src/platform` holds only the sockets, Wi-Fi, NVS and httpd calls. The page logic has its own tests. |
| VII. Owner in the loop | Pass | The brief's 13 questions, plus iOS camera-join, Android stay-joined and the hidden-network check (quickstart §3). |
| VIII. Small, private, polite | Pass | The secrets rules are in FR-023, R14 and SC-008, checked by a log scan that never prints secrets. TLS is untouched. The place-list generator calls two listed hosts, slowly, only on the development machine, with the project's User-Agent. |
| IX. Resource discipline | Pass | Flash writes happen only after a successful attempt, on `Baştan kur`, and once for the PIN. They are ordered so a power cut is safe (R10). Size is reported for the image, the fonts and the page. Nothing new runs on the UI thread. |
| X. Simplicity and scope | Pass | One machine instead of two. No Wi-Fi manager library. No JS framework or minifier dependency. The place-name check streams the list instead of keeping a database. |

## Project Structure

### Documentation (this feature)

```text
specs/004-wifi-setup/
├── spec.md, plan.md, research.md (R1–R15), data-model.md, quickstart.md
├── contracts/portal-http.md     # device network, DNS, HTTP paths, page behaviour
├── contracts/serial-console.md  # wifi setup|cancel|forget, report lines, fixtures 15–17
├── contracts/screen-api.md      # screens, overlay, gestures, harness
├── checklists/requirements.md
└── tasks.md                     # next: /speckit-tasks
```

### Source Code (repository root)

```text
tools/
  gen_places.py (+ test_gen_places.py with canned responses)     # R6, R8
  gen_web.py    (+ test)  → src/generated/web_assets.{h,cpp}      # R7 (git-ignored output)
  pio_gen_web.py           # pre-script that runs gen_web.py before the firmware build
  scan_log_secrets.py (+ test)                                     # SC-008
  check.py                 # + node --test, page size, new fonts in size report
data/
  places_tr.json           # generated, reviewed, committed (date + sha256)
  portal/ index.html, style.css, ui.js (shared components), app.js, test/*.test.mjs
assets/fonts/gen_fonts.py  # + nvp_cond_600_38, nvp_txt_600_19, nvp_txt_400_18; Â Î Û added (R13)
lib/core/src/nvp/
  net/  mode.h/.cpp (replaces diag/wifi_states), failure.h/.cpp, dns_reply.h/.cpp,
        scan.h/.cpp (auth_class, merge, signal_class), wifi_qr.h/.cpp,
        places.h/.cpp (place_lookup), submission.h/.cpp, views.h (SetupView, ConnectingView, OverlayView)
  screen/ gesture.h/.cpp   # + TouchClassifier (Tap, Hold5s)
          fixtures.h/.cpp  # + fixtures 15–17
  text/ tr.h               # + setup, connecting, overlay, confirmation strings
        title_case.h/.cpp, glyph_filter.h/.cpp
  diag/ command.h/.cpp     # wifi setup|cancel|forget mapped to the new events
        report.h/.cpp      # + mode, phones, last result, location lines
lib/ui/src/
  setup/ setup_screen, connecting_screen, status_overlay, qr_box (rounded ink square + lv_qrcode)
  fonts/ + three generated fonts
src/platform/
  wifi_link   # + scan config (grouped), auth raw value, disconnect reasons, AP client events,
              #   station config per R4, DHCP option 114
  dns_responder.h/.cpp (UDP 53 on the AP interface, uses nvp::net::dns_reply)
  setup_portal # + paths of NET-04, catch-all 302, gzip bodies, socket settings
  settings_store.h/.cpp (loc/*, sec/pin; ordered writes)
src/app/
  net_worker.cpp  # drives nvp::net::mode_step; timers → events; no write during scan
  bringup_app.cpp # → app modes: Setup / Connecting / Main (+ overlay) / Diagnostic; touch classifier
include/lv_conf.h # LV_USE_QRCODE 1
sdkconfig.defaults # LWIP_MAX_SOCKETS 16
sim/ui_render/main.cpp # + fixtures 15–17
test/test_net/    # mode grid, failure, dns, scan, qr, places, submission
test/test_screen/ # + touch classifier, fixtures 15–17, title case, glyph filter
```

**Structure Decision**: the layering of 05-architecture §4.
- **Logic:** every decision is in `lib/core`.
- **Drawing:** `lib/ui` draws from plain views.
- **SDK calls:** `src/platform` is a thin wrapper over the SDK. It reports raw values (auth mode and reason numbers), and `lib/core` classifies them.
- **Wiring:** `src/app` turns timers into events and connects the parts.

Slice 1's portal, Wi-Fi wrapper and machine are extended rather than duplicated (spec 001 FR-025).

## Phase 0: Research

Complete: [research.md](research.md), R1–R15. No `NEEDS CLARIFICATION` remains. Owner decisions OD4-1, OD4-2 and OD4-3 are recorded in the spec.

## Phase 1: Design

Complete:
- [data-model.md](data-model.md)
- [contracts/portal-http.md](contracts/portal-http.md)
- [contracts/serial-console.md](contracts/serial-console.md)
- [contracts/screen-api.md](contracts/screen-api.md)
- [quickstart.md](quickstart.md)

Post-design constitution check: everything passes. The only wording gap (W1) was approved as OD4-4.

## Implementation stages (from the brief)

1. **Stage 1 (host only):**
   - the place-list generator and its reviewed output;
   - the mode machine and all pure functions, tests first;
   - the fonts;
   - the three screens with fixtures 15 to 17 compared with the references.
2. **Stage 2:**
   - device network, DNS, HTTP server and the page, up to the point where a phone joins, the page opens, and the network and province/district lists work;
   - the first half of the owner checklist.
3. **Stage 3:**
   - the attempt, failure classes, storage, reconnect, the overlay with tap and hold, and start over;
   - the rest of the checklist.

## Open items

- **W1, wording (constitution III): resolved.** OD4-4 in the spec: the network name is shown greyed with `desteklenmiyor` after it.
- **Risks for the owner checklist (R1):**
  - iOS may not show the sign-in sheet after a camera QR join. The typed address is the fallback (User Story 6).
  - The Android private-IP flag.
  - Phones following the channel switch.
  - Connecting to a hidden network.

## Complexity Tracking

None.
