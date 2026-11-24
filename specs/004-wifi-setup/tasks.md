---

description: "Task list for Wi-Fi setup from the phone (spec 004)"
---

# Tasks: Wi-Fi setup from the phone

**Input**: Design documents from `specs/004-wifi-setup/` ([plan.md](plan.md), [spec.md](spec.md), [research.md](research.md), [data-model.md](data-model.md), [contracts/](contracts/), [quickstart.md](quickstart.md))

**Tests**:
- **`lib/core`:** every addition gets its tests first (constitution VI).
- **Page logic:** `node --test`.
- **Tools:** Python unit tests with canned responses, no network.
- **Screens:** fixtures 15 to 17 in the host harness, compared with the references, then reviewed side by side before any owner check (constitution III).
- **Phone and device behaviour:** the owner checklist.

**Organization**: phases follow the user stories. The brief's three implementation stages are mapped onto them:
- **Stage 1** (host only): Phase 1, Phase 2, and the screen tasks of US1 and US4 (fixtures 15 to 17).
- **Stage 2**: the remaining US1 tasks up to "the page opens and the lists work", plus US6. Then the first half of the owner checklist.
- **Stage 3**: the attempt and success path of US1, then US2, US3, US4 (device), US5 and the Polish phase. Then the rest of the checklist.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: can run in parallel (different files, no dependency on an unfinished task)
- **[Story]**: US1–US6 of spec.md
- Every task cites an FR/SC of spec 004, a research item (R#), a 02-ui-spec section or a requirement id (constitution X)

## Path conventions

| What | Where |
|---|---|
| pure logic + tests | `lib/core/src/nvp/{net,screen,text,diag}/`, `test/test_net/`, `test/test_screen/`, `test/test_diag/` |
| screens | `lib/ui/src/setup/`, fonts in `lib/ui/src/fonts/` |
| SDK wrappers | `src/platform/` |
| wiring | `src/app/` |
| page | `data/portal/`, generated array in `src/generated/` (git-ignored) |
| tools | `tools/` |

---

## Phase 1: Setup

**Purpose**: repository hygiene and build switches that every later task needs.

- [X] T001 Restore the repository's `.gitignore` from `main` (a tool replaced it with a PlatformIO default), then add `src/generated/` to it; confirm `git status` shows no `managed_components/`, `sdkconfig.*`, `out/` or `__pycache__/` entries (CLAUDE.md "no secrets", 05 §4)
- [X] T002 [P] Enable `LV_USE_QRCODE 1` in `include/lv_conf.h` and confirm `pio run -e sim` still builds (R11)
- [X] T003 [P] Set `CONFIG_LWIP_MAX_SOCKETS=16` in `sdkconfig.defaults`, with a comment citing R5 and Espressif's captive-portal example
- [X] T004 [P] Create `test/test_net/test_main.cpp` (Unity runner, same pattern as `test/test_screen/test_main.cpp`), with an empty run list to be filled by later tasks
- [X] T005 [P] Create `data/portal/` with an empty `index.html`, `style.css`, `ui.js`, `app.js`, plus `data/portal/test/` and a `package.json`-free `node --test` layout; update `data/README.md` with the page and place-list rules (R7)

---

## Phase 2: Foundational (blocks every user story)

**Purpose**: the pure logic, the place list and the fonts. All of it is host-only (Stage 1).

### Tests first (`lib/core`)

- [X] T006 [P] Write `test/test_net/test_mode.cpp`: the full (state × event) grid of data-model.md's mode table, covering every row, every unlisted pair (no change, no action), `Boot` with all four combinations of `has_wifi`/`has_location`, `!silence_ok`, no `Run*` row ever starting the AP (MODE-05), and `Setup` having no timer (MODE-03) (FR-001–FR-003, R9)
- [X] T007 [P] Write `test/test_net/test_failure.cpp`: `classify_failure` for recorded reason sequences: [201]→NotFound; [210], [211], [212]→NotFound; [15,15] seen→WrongPassword; [15] once→Other; [204,204] not seen in scan→Other; [202,15]→WrongPassword; []→Other; timeout→Other (FR-016, NET-05, R4)
- [X] T008 [P] Write `test/test_net/test_dns_reply.cpp`: an A query for `connectivitycheck.gstatic.com` gives one A record 192.168.4.1 with TTL 0 and the ID and question echoed; AAAA gives NOERROR with 0 answers; a response packet (QR=1), a truncated header, two questions and a label loop are ignored; the answer length is ≤ query + 16 (contracts/portal-http.md DNS, R1, R14)
- [X] T009 [P] Write `test/test_net/test_scan.cpp`: `auth_class` for every `wifi_auth_mode_t` value 0–17 (open / secured / unsupported per R3); `merge_scan` keeps the strongest duplicate, drops empty SSIDs, sorts by RSSI and caps at 20; `signal_class` boundaries at −60 and −75 (FR-014, 02 §7.4)
- [X] T010 [P] Write `test/test_net/test_wifi_qr.cpp`: `wifi_qr_payload("NamazVakti-7F3A","48271935")` equals `WIFI:T:WPA;S:NamazVakti-7F3A;P:48271935;;`; escaping of `\ ; , : "`; refusal when the buffer is too small (FR-004, R11)
- [X] T011 [P] Write `test/test_net/test_places.cpp`: `place_lookup` on a small canned list accepts (9549, İSTANBUL, SULTANBEYLİ) and an alias (9541, İSTANBUL, KADIKÖY); refuses a wrong id, a wrong province, a name from another province and an empty name; handles `Â` (FR-012, FR-017, OD4-3, R6, R10)
- [X] T012 [P] Write `test/test_net/test_submission.cpp`: form decoding (`+`, `%XX`, UTF-8 SSIDs with Turkish letters, spaces, emoji); bounds (SSID 1–32 bytes, password empty or 8–63, body ≤ 512 B); a missing location field gives `location` (FR-012, contracts/portal-http.md)
- [X] T013 [P] Write `test/test_screen/test_touch.cpp`: `TouchClassifier` gives `Tap` for one finger ≤ 600 ms and ≤ 24 px; none for 601 ms or 25 px; `Hold5s` once at 5 000 ms; a second finger cancels a tap; `TwoFingerHold5s` stays as in Slice 3 and never yields `Tap` (FR-008, R12)
- [X] T014 [P] Write `test/test_screen/test_title_glyph.cpp`: `title_case_tr` gives SULTANBEYLİ→Sultanbeyli, AĞRI→Ağrı, KÂĞITHANE→Kâğıthane, IĞDIR→Iğdır, 19 MAYIS→19 Mayıs, AFYONKARAHİSAR→Afyonkarahisar; `glyph_filter` replaces code points outside a given set with `?`, one per code point, including a 4-byte emoji (02 §7.3, R8, R13)
- [X] T015 [P] Write `test/test_net/test_views.cpp`: `make_setup_view` groups the password `4827 1935` and sets the footer per FR-005 for waiting, phone joined and each failure class; `make_overlay_view` gives dashes for source, last update, last day and PIN, `bağlı değil` when down, `show_qr=false` on the device and `true` for fixture 17 (FR-005, FR-007, OD4-2)

### Implementation (`lib/core`)

- [X] T016 Implement `lib/core/src/nvp/net/mode.h/.cpp`: states, events, context, action flags and the table from data-model.md; delete `lib/core/src/nvp/diag/wifi_states.h/.cpp` and migrate `test/test_diag/test_wifi_states.cpp` into T006's file (R9); T006 passes
- [X] T017 [P] Implement `lib/core/src/nvp/net/failure.h/.cpp` (`FailureClass`, `classify_failure`, `failure_name` → `wrong_password|not_found|other|none`) so that T007 passes (R4)
- [X] T018 [P] Implement `lib/core/src/nvp/net/dns_reply.h/.cpp` (`size_t dns_reply(const uint8_t* q, size_t n, uint32_t ip, uint8_t* out, size_t cap)`) so that T008 passes (R1)
- [X] T019 [P] Implement `lib/core/src/nvp/net/scan.h/.cpp` (`auth_class`, `merge_scan`, `signal_class`, `scan_json` replacing `nvp::diag::portal_json`'s scan writer with the `auth`/`signal` fields of contracts/portal-http.md) so that T009 passes; update `test/test_diag/test_net_helpers.cpp`
- [X] T020 [P] Implement `lib/core/src/nvp/net/wifi_qr.h/.cpp` so that T010 passes (R11)
- [X] T021 [P] Implement `lib/core/src/nvp/net/places.h/.cpp` (streaming `place_lookup` over the compact JSON, no heap) so that T011 passes (R10)
- [X] T022 [P] Implement `lib/core/src/nvp/net/submission.h/.cpp` (form decoding, `Submission`, `validate_submission` → ok | ssid | password | location) so that T012 passes
- [X] T023 [P] Add `TouchClassifier` to `lib/core/src/nvp/screen/gesture.h/.cpp`, keeping `TwoFingerHold`'s behaviour, so that T013 and the Slice 3 gesture tests pass (R12)
- [X] T024 [P] Implement `lib/core/src/nvp/text/title_case.h/.cpp` and `lib/core/src/nvp/text/glyph_filter.h/.cpp` so that T014 passes (R8, R13)
- [X] T025 Add the setup, connecting, overlay and confirmation strings of 02 §7.2–§7.4, `desteklenmiyor` (OD4-4), and the signal words to `lib/core/src/nvp/text/tr.h`, byte for byte with U+2019 in `Ayar PIN’i`; extend `test/test_screen/test_state_text.cpp` with a constant check for each new string
- [X] T026 Implement `lib/core/src/nvp/net/views.h/.cpp` (`SetupView`, `ConnectingView`, `OverlayView`, `make_*_view` builders) so that T015 passes (data-model.md)
- [X] T027 Run `pio test -e native` and confirm everything is green, including the Slice 1–3 suites

### Place list (Stage 1)

- [X] T028 [P] Write `tools/test_gen_places.py` with canned responses (a 3-province states answer, two district pages including aliases, `times` answers with `copied_from_center`): checks Turkish ordering, centre first, alias→centre id, the failure on 80 provinces, the failure on an empty province, the failure on an alias whose centre lies in another province, and a stable checksum (FR-021, R6, R8)
- [X] T029 Implement `tools/gen_places.py` per R6: version 1 states, version 2 district pages (`limit=500`), one version 2 `times` call per alias for `center_district_id`; one request per 3.1 s; User-Agent `NamazVaktiPanel/<version>`; TLS verified through certifi as in `assets/fonts/gen_fonts.py`; spot check of 5 ids against emushaf `/ilceler/{provinceId}`; writes `data/places_tr.json` with `generated`, `source` and `sha256`; fails on any check of R6. T028 passes
- [X] T030 Run `python tools/gen_places.py`. Review the output: 81 provinces, 1003 entries (869 + 134), no province empty, Turkish characters intact (`İ Ş Ğ Ü Ö Ç Â`), İstanbul's centre first, Kadıköy → 9541, and the spot check 5/5. Record the counts, the date and the checksum in `specs/004-wifi-setup/research.md` R6 (LOC-02, SC-009)

### Fonts (Stage 1)

- [X] T031 Measure the address in step 3 and the PIN on references 15 and 17 with `tools/measure_snapshot.py`, and decide whether `nvp_num_25` serves (02 §2.2 consolidation) or a `0-9 .` font at the measured size is needed; record the result in research R13
- [X] T032 Extend `assets/fonts/gen_fonts.py` with `nvp_cond_600_38` (printable ASCII + Turkish letters + `Â â Î î Û û`), `nvp_txt_600_19` (`1-3`), `nvp_txt_400_18` (printable ASCII + Turkish set + `Â â Î î Û û`), plus any font from T031; add `Â â Î î Û û` to `nvp_txt_400_15`; regenerate; declare the new fonts in `lib/ui/src/fonts/fonts.h`; report the font total against 300 kB via `tools/size_by_module.py` (R13)

**Checkpoint**: all pure logic is tested, the place list is reviewed and committed, and the fonts are built. Story work can start.

---

## Phase 3: User Story 1 — First setup (Priority: P1) 🎯 MVP

**Goal**: from power-on, the device shows the setup screen, a phone joins and opens the page, and a correct submission ends on the main screen.

**Independent Test**: a device with no location goes through the flow from an iPhone and an Android phone without anyone typing an address (SC-001, SC-002).

### Screens (Stage 1)

- [X] T033 [P] [US1] Add fixtures 15 and 16 to `lib/core/src/nvp/screen/fixtures.h/.cpp` (network `NamazVakti-7F3A`, password `48271935`, SSID `EvAgi_2.4G`, footer waiting) and extend `test/test_screen/test_fixtures.cpp` with numbering and reference names up to 17
- [X] T034 [US1] Implement `lib/ui/src/setup/qr_box.h/.cpp`: an `ink` rounded square (radius 14, padding 14) holding an `lv_qrcode` with light `ink` and dark `bg`, integer module scale, centred (R11)
- [X] T035 [US1] Implement `lib/ui/src/setup/setup_screen.h/.cpp` per 02 §7.2: title, subtitle, three steps with 36 px circles (2 px `line`), the network and password rows, the step 3 address, the QR box at (508, 86) 268×268, the caption at top 364, and the footer at bottom 18 in the colour per state; change-driven `setup_screen_update` (FR-004, FR-005, contracts/screen-api.md)
- [X] T036 [US1] Implement `lib/ui/src/setup/connecting_screen.h/.cpp` per 02 §7.3: three centred lines from y 150, 14 px apart; the SSID through `glyph_filter` against `nvp_cond_600_38`; the `Connected` phase texts `Bağlandı` / `Saat ve vakitler alınıyor` (FR-006)
- [X] T037 [US1] Render fixtures 15 and 16 in `sim/ui_render/main.cpp`; add the geometry checks (QR square, step circles) to `tools/check_snapshots.py` with a test in `tools/test_check_snapshots.py`; run `python tools/compare_reference.py 15 16`, review the side-by-side images and fix until within 02 §12 (FR-009, SC-007)

### Device network, DNS, portal and page (Stage 2)

- [X] T038 [US1] Extend `src/platform/wifi_link.h/.cpp`: a scan returning the raw `authmode`; grouped scanning (4 channels per group, 120 ms dwell, 120 ms pause, R3); counting AP client events (`WIFI_EVENT_AP_STACONNECTED`/`STADISCONNECTED`); DHCP option 114 = `http://192.168.4.1/` (R1, R2); AP config WPA2, `max_connection` 2, PMF optional. No SDK type leaks into the header (constitution VI)
- [X] T039 [P] [US1] Implement `src/platform/dns_responder.h/.cpp`: one UDP socket on port 53 bound to the AP address, non-blocking, polled by the network worker, calling `nvp::net::dns_reply`; `start()`/`stop()`/`poll()`; logs only counts (R1, R5)
- [X] T040 [US1] Extend `src/platform/setup_portal.h/.cpp` per contracts/portal-http.md: `max_open_sockets` 7, LRU purge, the `/places.json` and `/api/scan?refresh=1` paths, gzip bodies with `Cache-Control: no-store`, a 404 handler answering `302` to `http://192.168.4.1/` with a body, `409` while busy, `httpd_*` logs at error level; the callbacks still run only inside `handle()` (FR-011, FR-012, R5)
- [X] T041 [P] [US1] Write the page in `data/portal/`. `ui.js` holds the shared components (section, network list, select pair, password field with `Göster`, primary button, inline message). `app.js` holds the flow of 02 §10: load status, places and scan; preselect the stored location by names; title case; `Ağım listede yok`; `desteklenmiyor` greyed (OD4-4); the validation texts; the disabled button until valid; after submit the fixed text and polling for 60 s at 3 s. `style.css` uses the 02 §2.1 tokens at 320 px. `index.html` holds the strings of 02 §10 byte for byte (FR-013–FR-015)
- [X] T042 [P] [US1] Write `data/portal/test/*.test.mjs` for `node --test`: title case (same cases as T014), validation rules and messages, form body encoding, preselection by names with alias entries, unsupported entries not selectable (FR-014, FR-015, R7, R8)
- [X] T043 [US1] Implement `tools/gen_web.py` (inline, conservative minify, gzip `mtime=0` level 9, emit `src/generated/web_assets.h/.cpp` with `portal_html_gz`, `places_json_gz` and the uncompressed `places_json` for `place_lookup`, plus their lengths and SHA-256; fail if the page is over 25 600 B) with `tools/test_gen_web.py`, and `tools/pio_gen_web.py` as a pre-script in `[env:crowpanel7_advance]` of `platformio.ini` (R7, SC-006)
- [X] T044 [US1] Add `src/platform/settings_store.h/.cpp`: read and write `loc/district_id`, `loc/province`, `loc/district`; `sec/pin` generated on first start by rejection sampling over `esp_random()`; ordered writes with `nvs_commit` (location → `wifi/pass` → `wifi/ssid`, R10); nothing logged but names and the id (FR-017, FR-022, FR-023)
- [X] T045 [US1] Rework `src/app/net_worker.h/.cpp` to drive `nvp::net::mode_step`: `Boot` from the stored details; scan once before starting the AP; DNS and portal started and stopped with the AP; phone count → `PhoneJoined`/`PhoneLeft`; remove Slice 1's 15 s periodic portal scan; publish a snapshot with mode, footer, views and last result; portal callbacks build answers from the snapshot and post `Submit` (05 §6, MODE-02, MODE-03, R3, R9)
- [X] T046 [US1] Rework `src/app/bringup_app.cpp` modes into Setup / Connecting / Main (+ overlay) / Diagnostic: the setup screen and connecting screen are created on demand and loaded on the worker's mode changes; the Slice 1 diagnostic Wi-Fi buttons map to `DevSetup`/`StartOver`; the main screen stays exactly as in Slice 3 (FR-001, FR-004, FR-006)
- [X] T047 [US1] Build the firmware, report image size, fonts and page size (constitution IX); flash; on the device check with the serial log that the board enters setup (OD4-1), the screen matches reference 15, and a laptop or phone joining sees the page, the network list and the province/district lists; then give the owner the first half of the checklist (quickstart §2 steps 1–2, iPhone and Android, plus the typed-address question) and record the answers in `specs/004-wifi-setup/owner-checklist.md`

### Attempt and success (Stage 3)

- [X] T048 [US1] Implement the attempt in `src/platform/wifi_link.cpp` and `src/app/net_worker.cpp`: `TryStart` 1 s after `202` (R2); station config per R4 (threshold OPEN or WPA2_PSK, `WIFI_ALL_CHANNEL_SCAN`); retries within 20 s; collect disconnect reasons into the attempt; `TryOk` on an IP address; `TryTimeout` at 20 s (MODE-04, FR-016)
- [X] T049 [US1] Success path in `src/app/net_worker.cpp`: wait for or stop any running scan, then write via `settings_store` in R10 order (FR-017, FR-018); `Grace` 5 s with the connecting screen in its `Connected` phase; then stop the portal, DNS and AP; main screen (FR-006, MODE-04); log lines per contracts/serial-console.md

**Checkpoint**: a full first setup works on the device (US1 independent test).

---

## Phase 4: User Story 2 — When it goes wrong (Priority: P1)

**Goal**: each failure class returns to setup with its own footer, and the owner can try again without a restart.

**Independent Test**: provoke wrong password, network not found and other; then a correct attempt (SC-003).

- [X] T050 [US2] Failure path in `src/app/net_worker.cpp`: on `TryFail`/`TryTimeout`, `classify_failure` with "seen in the cached scan"; discard the submission (zero the buffers); disconnect the station; keep the AP up with the same name and password; footer = class until the next attempt; `/api/status.result` = class (FR-016, FR-017, NET-05)
- [X] T051 [US2] Ensure the page re-enables the form when a status poll sees `mode == setup` again, adding no new text (contracts/portal-http.md); add the case to `data/portal/test/`
- [X] T052 [US2] On the device, provoke each class (wrong password; `YokBoyleAg` typed by hand; other, for example by switching the router off during the attempt) and confirm from the log the reasons and classes and from the screen the footers; record the results in `specs/004-wifi-setup/owner-checklist.md` for the owner to confirm (SC-003)

**Checkpoint**: US1 and US2 work together.

---

## Phase 5: User Story 3 — It remembers (Priority: P1)

**Goal**: after a power cut the device reconnects on its own; with the router off it keeps retrying and never opens its own network.

**Independent Test**: power cycle; router off for 30 minutes (SC-004, SC-005).

- [X] T053 [US3] In `src/app/net_worker.cpp`, run `RunJoining`/`RunUp`/`RunBackoff` from `mode_step` with `nvp::diag::Backoff` (5 s doubling to 5 min, NET-01); join in the background at start without delaying the main screen (FR-019, SYS-02); confirm by code review and the T006 grid that no `Run*` path starts the AP (MODE-05)
- [X] T054 [US3] Extend the `report` wifi line and add the `location:` line in `lib/core/src/nvp/diag/report.h/.cpp` per contracts/serial-console.md (mode, phones, last result, backoff seconds; never a password); extend `test/test_diag/test_selftest_report.cpp`
- [X] T055 [US3] On the device: power cycle and confirm `connected` within one minute from the log; router off, then confirm from the log that the retries grow to 5 min and no `mode: setup` line appears; record the results for the owner (SC-004, SC-005)

---

## Phase 6: User Story 4 — Status at a tap (Priority: P2)

**Goal**: a tap opens the overlay with the current values and dashes; another tap or 20 s closes it.

**Independent Test**: fixture 17 against reference 17; tap, read, tap, and wait 20 s on the device.

- [X] T056 [P] [US4] Add fixture 17 to `lib/core/src/nvp/screen/fixtures.h/.cpp` (location `İstanbul, Sultanbeyli`, Wi-Fi `EvAgi_2.4G, iyi`, address `192.168.1.37`, version `1.0.0`, dashes, `show_qr=true` with `http://192.168.1.37/`); extend `test/test_screen/test_fixtures.cpp`
- [X] T057 [US4] Implement `lib/ui/src/setup/status_overlay.h/.cpp` on `lv_layer_top()` per 02 §7.4: a `bg` scrim at 72 %; the panel 600×376 at (100, 52), radius 16, `panel` fill, 1 px `line`; the title; the two-column rows (150 px labels, `nvp_txt_400_15`/`nvp_txt_400_18`, 9 px apart); the PIN row in `now` Barlow Cond.; the QR box 132×132 (padding 8, radius 10) with caption `Ayar sayfası`, shown only when `show_qr`; the hint line; the confirmation panel with buttons 200×56, radius 12 (FR-007, contracts/screen-api.md)
- [X] T058 [US4] Render fixture 17 in `sim/ui_render/main.cpp` over fixture 01's main screen; add the panel and button geometry checks to `tools/check_snapshots.py`; run `python tools/compare_reference.py 17`; review side by side and fix until within 02 §12 (FR-009, SC-007)
- [X] T059 [US4] Wire the touch in `src/app/bringup_app.cpp`: feed `TouchClassifier` from `platform::touch` points; `Tap` on the main screen opens the overlay with `make_overlay_view` from the worker snapshot (RSSI class, station IP, location title-cased, version; `show_qr=false`, OD4-2); `Tap` closes it; 20 s without touch closes it; `TwoFingerHold5s` still opens diagnostics and never the overlay (FR-007, FR-008)
- [X] T060 [US4] Extend `fixture N` in `lib/core/src/nvp/diag/command.h/.cpp` and `src/app/serial_console.cpp` to 15–17 per contracts/serial-console.md; extend `test/test_diag/test_command_parser.cpp`

---

## Phase 7: User Story 5 — Start over (Priority: P2)

**Goal**: hold 5 s on the overlay and confirm; the device forgets only the Wi-Fi and returns to setup with the location preselected.

**Independent Test**: hold → `Vazgeç`; hold → `Baştan kur`; the page preselects the province and district (MODE-06).

- [X] T061 [US5] In `src/app/bringup_app.cpp`: `Hold5s` on the overlay shows the confirmation; `Vazgeç`, or 15 s without a response, closes it and the overlay; `Baştan kur` posts `StartOver` (FR-008, FR-020)
- [X] T062 [US5] In `src/app/net_worker.cpp` and `src/platform/wifi_link.cpp`: `StartOver` erases `wifi/ssid` and `wifi/pass` only (location, `ap/pass`, `sec/pin` kept), disconnects, scans, starts the AP, DNS and portal; `wifi forget` does the same, and `wifi setup`/`wifi cancel` map to `DevSetup`/`DevCancel` per contracts/serial-console.md (MODE-06, FR-024)
- [X] T063 [US5] On the device: run both confirmation paths and the page preselection; confirm from the log that the location was kept; record the results for the owner

---

## Phase 8: User Story 6 — Page without the pop-up (Priority: P3)

**Goal**: typing `192.168.4.1` or any address opens the page.

**Independent Test**: dismiss the sign-in sheet and type the address.

- [X] T064 [US6] On the device (Stage 2 build or later): from a joined phone or laptop, open `http://192.168.4.1/`, `http://example.com/` and `http://captive.apple.com/hotspot-detect.html` and confirm that the page or the 302 appears; confirm with `curl -v` from a laptop joined to the device's network that unknown paths get `302` with `Location: http://192.168.4.1/` and a body, and that port 443 is refused (NET-03, contracts/portal-http.md)

---

## Phase 9: Polish & cross-cutting concerns

- [X] T065 [P] Implement `tools/scan_log_secrets.py` with `tools/test_scan_log_secrets.py`: hidden prompts for the Wi-Fi and access-point passwords, kept in memory only; reports the count of occurrences and lists runs of six or more digits that are not timestamps or addresses (SC-008, quickstart §2 step 10)
- [X] T066 [P] Extend `tools/check.py`: `node --test data/portal/test`, the page-size report, the place-list checksum check (recomputes `sha256` of `data/places_tr.json`), and the new fonts in the size report; update `tools/README.md`
- [X] T067 Review the secrets: grep `src/` and `lib/` for every print or log of `pass`, `pin` or `password`; confirm `/api/status` and every report line omit them; confirm the submission buffers are zeroed after use (FR-023, NET-08, SEC-04)
- [X] T068 Measure the internal heap (free and largest block) in the serial report with the AP, DNS and portal up and a phone joined; record it in research R5 and 05 §5 (constitution IX)
- [X] T069 [P] Update the documents: 02-ui-spec §10 with `desteklenmiyor` (OD4-4); 04-data-sources §9 with the alias finding and the centre-id mapping (R6, OD4-3); 05-architecture §3/§4/§5 (QR via LVGL, DNS responder, the `nvp/net` module, the new NVS keys); 06-roadmap Slice 4 outcome; `README.md` commands and status
- [X] T070 Give the owner the rest of the checklist (quickstart §3: the brief's 13 questions + iOS camera-join, Android stay-joined, hidden network) and record the yes/no answers in `specs/004-wifi-setup/owner-checklist.md` (constitution VII)
- [X] T071 Run `python tools/check.py` and `pio run -e crowpanel7_advance`; report the image size against 2.9 MiB, the fonts against 300 kB and the page against 25 600 B; run `/speckit-converge` (05 §11)

---

## Dependencies & execution order

- **Phase 1** → **Phase 2** → stories.
- **Inside Phase 2:** T006–T015 (tests) come before T016–T026; T027 closes the group. T028 → T029 → T030. T031 → T032.
- **US1 screens** (T033–T037) need T020, T024–T026 and T032.
- **US1 device** (T038–T049) needs T016–T022, T030 and T043. T047 is the Stage 2 stop for the owner.
- **US2** needs T048. **US3** needs T045. **US4** needs T023 and T032, and its device part (T059) needs T046. **US5** needs T059 and T045. **US6** needs T040.
- **Polish** comes after all stories; T070 last but one, T071 last.

## Parallel examples

- **Phase 2 tests:** T006–T015 are all separate files and can run in parallel.
- **Phase 2 implementations:** T017–T024 can run in parallel once their tests exist.
- **US1:** T039 (DNS), T041/T042 (page) and T044 (store) are separate files and can run in parallel; T034–T036 are separate screen files.
- **Polish:** T065, T066 and T069 can run in parallel.

## Implementation strategy

1. **Stage 1:** Phases 1 and 2, then T033–T037 and T056–T058 (all screens on the host). Stop: review the renders against references 15 to 17.
2. **Stage 2:** T038–T047 and T064. Stop: the first half of the owner checklist.
3. **Stage 3:** T048–T055, T059–T063, then Polish. Stop: the rest of the checklist, `check`, converge.

The MVP is US1 (first setup).

## Phase 10: Convergence

- [X] T072 CRITICAL: get the owner's approval for the visible label `Ağ adı` on the hidden-network field of the setup page (`data/portal/app.js`), and record it as OD4-5 in `specs/004-wifi-setup/spec.md` and `docs/specification/02-ui-spec.md` §10; without approval, remove the label per Constitution III, FR-013 (unrequested)
- [X] T073 (skipped by the owner, 2026-11-24; SC-008 rests on the T067 code review) Save a serial log of a full session (setup, a failed attempt, a successful attempt, start over) outside the repository, have the owner run `python tools/scan_log_secrets.py <log>`, and record the result in `specs/004-wifi-setup/owner-checklist.md` per SC-008, FR-023 (partial)
- [X] T074 Update `specs/004-wifi-setup/contracts/serial-console.md` to the implemented replies and report lines (the `mode:` and `location:` lines; the `wifi:` line unchanged from Slice 1; the worker's log lines instead of `wifi: forgotten …`, `wifi setup refused (silence)`) per FR-024 (partial)
- [X] T075 Bring `specs/004-wifi-setup/contracts/screen-api.md` (`overlay_open(view)`, `overlay_hit()`, `overlay_confirm_shown()`), `specs/004-wifi-setup/data-model.md` (mode `RunNoRadio` and its rows) and `specs/004-wifi-setup/quickstart.md` (`node --test data/portal/test/logic.test.js`, `--upload-port`) in line with the code per plan: contracts and data model (partial)
- [X] T076 Add to `specs/004-wifi-setup/spec.md` Edge Cases: the silence step failed → no radio and no setup (analysis U1, `Mode::RunNoRadio`), and values that later slices fill show a `dim` dash in the overlay (analysis U3) per spec Edge Cases (partial)
- [X] T077 (skipped by the owner, 2026-11-24; hidden-network join untested) Ask the owner to try a hidden network typed by hand (or record that none is available) in `specs/004-wifi-setup/owner-checklist.md` per NET-06, US1 edge case "hidden home network" (partial)
