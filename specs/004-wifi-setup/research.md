# Research: Wi-Fi setup from the phone

All sources accessed on 2026-11-24 unless noted. "Live" means a request made from the development machine on that day with the User-Agent `NamazVaktiPanel/0.4-dev`. Items marked ⚠ could not be verified from documentation and are owner-checklist items.

## R1. How phones detect a captive portal (NET-03)

**Decision.**
- **DNS:** while the access point is up, a DNS responder answers every A query with 192.168.4.1. AAAA and other query types get an empty answer (NOERROR, no records).
- **HTTP:** the server on port 80 serves the portal paths (§ contracts/portal-http.md). Every other request, any host or path, gets the same answer:
  - status `302 Found`;
  - `Location: http://192.168.4.1/`;
  - `Content-Type: text/html`;
  - `Cache-Control: no-store`;
  - a short non-empty body of more than 4 bytes.
- **Never answered:** a `204` and Apple's `Success` page.
- **Nothing on port 443.** A refused connection is correct.
- **DHCP:** the server advertises the device as the DNS server (option 6, `CONFIG_LWIP_DHCPS_ADD_DNS=y`, the v5.5.5 default). DHCP option 114 is set to `http://192.168.4.1/`. It does no harm, but neither platform uses an HTTP URL from it.

**Rationale.**
- **iOS.** Apple does not document its probe. Live, `http://captive.apple.com/hotspot-detect.html` answers `200` with the 69-byte body `<HTML><HEAD><TITLE>Success</TITLE></HEAD><BODY>Success</BODY></HTML>`. Any other answer opens the Captive Network Assistant (CNA).
  - Espressif's example notes that "iOS requires content in the response to detect a captive portal, simply redirecting is not sufficient". Hence the non-empty body.
  - Sources: https://developer.apple.com/news/?id=q78sq5rv (Captive Portal API, HTTPS only), https://support.apple.com/en-us/102554 (CNA buttons).
- **Android (AOSP NetworkStack, main).**
  - Probes `http://connectivitycheck.gstatic.com/generate_204`, `https://www.google.com/generate_204`, and the fallbacks `http://www.google.com/gen_204` and `http://play.googleapis.com/generate_204`.
  - Only `204` means internet. `isPortalCode` is any status from 200 to 399 except 204.
  - A `200` with a body of 4 bytes or less counts as *no internet*, not as a portal. Hence the 302 with a body.
  - `sendCapportApiProbe` rejects non-HTTPS URLs, so option 114 with `http://` is ignored.
  - The probes bypass Private DNS, so the wildcard DNS works even with Private DNS set.
  - Sources: https://android.googlesource.com/platform/packages/modules/NetworkStack/+/refs/heads/main/res/values/config.xml and `.../src/com/android/networkstack/util/NetworkStackUtils.java`, `CaptivePortalProbeResult.java`, `NetworkMonitor.java`.
- **Windows** fetches `www.msftconnecttest.com/connecttest.txt` and expects `Microsoft Connect Test` (https://learn.microsoft.com/en-us/troubleshoot/windows-client/networking/internet-explorer-edge-open-connect-corporate-public-network). It is covered by the catch-all.
- **Espressif's example** in ESP-IDF v5.5.5 (`examples/protocols/http_server/captive_portal`, also in the local framework package `framework-espidf@4.50505.0`) uses the same pattern:
  - wildcard DNS;
  - a 404 error handler that redirects (303 at the v5.5.5 tag, 302 on the branch) with a body;
  - option 114 via `esp_netif_dhcps_option(..., ESP_NETIF_CAPTIVEPORTAL_URI, ...)`, available since PR #12971 (ESP-IDF 5.4) and present in the local header `esp_netif_types.h:94`;
  - `max_open_sockets = 13`, `lru_purge_enable = true`, `LWIP_MAX_SOCKETS = 16`.

**Risks (owner checklist).**
- ⚠ **iOS after a camera QR join.** An Apple forum thread (https://developer.apple.com/forums/thread/852321, iOS 18.7) reports that the CNA is **not shown when the phone joins through a camera QR code** until the user opens Settings → Wi-Fi. Apple gave no workaround. This is why User Story 6 exists: step 3 of the setup screen shows `192.168.4.1`. The checklist asks separately whether the sheet opened by itself and whether the typed address works.
- ⚠ **iOS `Cancel`** may drop the Wi-Fi. `Other Network` documentedly does. The page therefore lets the owner finish everything inside the sheet.
- ⚠ **Android private-IP flag.** The flag `dns_probe_private_ip_no_internet` (default off in AOSP) would make a probe host resolving to a private address report "no internet". It needs a live check on the owner's Android phone.
- ⚠ Samsung's own probe hosts are not documented. The catch-all covers any host.

**Alternatives considered.**
- Registering each probe path as its own handler: the default `max_uri_handlers` is 8, so the catch-all is simpler.
- Answering 200 with the page on every path: the page's relative URLs and the address bar become wrong, and a body of 4 bytes or less misleads Android.
- HTTPS for the Captive Portal API: needs a trusted certificate for a local address, which is impossible.

## R2. Access point and station at once (MODE-04, NET-02)

**Decision.**
- **Mode:** AP+STA, as in Slice 1.
- **The device's network:**
  - WPA2-PSK;
  - `max_connection = 2`;
  - the channel chosen by the driver (0);
  - PMF not required, so older phones can join.
- **Joined phones are counted** from `WIFI_EVENT_AP_STACONNECTED` and `WIFI_EVENT_AP_STADISCONNECTED`, for the footer (FR-005).
- **Starting an attempt:** after answering `POST /api/setup` with `202`, the device waits **1 s** before it starts the station connection. The channel may change when the station joins the router, and this gives the answer time to reach the phone.
- The page never depends on reaching the device after submitting (02 §10 item 6).

**Rationale.**
- In AP+STA mode "the soft-AP will adjust its channel automatically to be the same as the channel of the station" (`esp_wifi.h`, `esp_wifi_set_config`). The router's channel wins. The access point announces the change with a Channel Switch Announcement, and phones that support it move without disconnecting (Wi-Fi driver guide v5.5.4, https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-guides/wifi.html).
- Espressif's provisioning manager waits one second before connecting, "so that the response can be sent to the client successfully, before a channel change happens" (https://raw.githubusercontent.com/espressif/esp-idf/release/v5.5/components/wifi_provisioning/src/manager.c).
- The default AP netif has `ESP_NETIF_DHCP_SERVER | ESP_NETIF_FLAG_AUTOUP`, so DHCP starts with it (`esp_netif_defaults.h`). The address 192.168.4.1 is already working since Slice 1.
- The soft-AP's `max_connection` default is 10 and its maximum 15. Two is allowed.

⚠ Whether the owner's phones follow the channel switch or drop off is not documented. The design does not depend on it.

## R3. Scanning while a phone is joined (FR-014, FR-018)

**Decision.**
1. **On entering setup:** the device scans once *before* starting its own network. No phone can be joined yet. It keeps the result (at most 20 entries after merging).
2. **`Yenile` on the page** (`GET /api/scan?refresh=1`): the device scans again in **groups of four channels** with active dwell 120 ms per channel and a **120 ms pause** on the home channel between groups. It answers with the cached list at once and marks it `"scanning": true`. The page polls until `"scanning": false`.
3. **Slice 1 kept rescanning every 15 s** while the hotspot was up (`kPortalScanMs`). That periodic rescan is removed.
4. **No storage write while a scan runs** (Slice 1 rule, FR-018). The success path waits for any running scan to finish, or stops it, before writing the details.

**Rationale.**
- Espressif warns that scanning all channels in one pass "may not give the Wi-Fi driver enough time to send out beacons" and can drop joined stations. Their fix is grouped scanning, about 4 channels per group with 120 ms between groups (https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/provisioning/wifi_provisioning.html). `manager.c` implements this with one channel per scan call (`scan_cfg.channel`) and a 120 ms `vTaskDelay` at each group boundary.
- Background scans already return to the home channel for `home_chan_dwell_time` (default 30 ms) between scanned channels (`esp_wifi.h`).
- Hidden networks are left out unless `show_hidden = true`. They are not needed in the list, because `Ağım listede yok` covers them (NET-06).

**Classification of scan entries** (`wifi_auth_mode_t`, local header `esp_wifi_types_generic.h` lines 87–104, framework 5.5.5):
- **Open:** `WIFI_AUTH_OPEN`.
- **Enterprise, not supported:** `WIFI_AUTH_ENTERPRISE` (= WPA2_ENTERPRISE), `WIFI_AUTH_WPA3_ENT_192`, `WIFI_AUTH_WPA3_ENTERPRISE`, `WIFI_AUTH_WPA2_WPA3_ENTERPRISE`, `WIFI_AUTH_WPA_ENTERPRISE`.
- **Also not supported:** `WIFI_AUTH_WEP`, `WIFI_AUTH_WAPI_PSK`, `WIFI_AUTH_OWE`, `WIFI_AUTH_DPP` (needs DPP provisioning, not a password; added during implementation) and unknown values. They are listed the same way, with the same "not supported" mark.
- **Secured:** everything else.

The classification is a pure function in `lib/core` with a host test.

## R4. Classifying a failed attempt (NET-05, FR-016)

**Decision.** Map the station's last disconnect reason within the 20 s window. Reason codes are from the local header `esp_wifi_types_generic.h` lines 114–177 of framework 5.5.5.

| Class | Reasons | Screen text (02 §7.2) |
|---|---|---|
| wrong password | 15 `4WAY_HANDSHAKE_TIMEOUT`, 202 `AUTH_FAIL`, 204 `HANDSHAKE_TIMEOUT`, 14 `MIC_FAILURE`, 2 `AUTH_EXPIRE`, *only if the network was seen* | `Wi-Fi şifresi kabul edilmedi. Tekrar dene.` |
| network not found | 201 `NO_AP_FOUND`, 210 `NO_AP_FOUND_W_COMPATIBLE_SECURITY`, 211 `NO_AP_FOUND_IN_AUTHMODE_THRESHOLD`, 212 `NO_AP_FOUND_IN_RSSI_THRESHOLD` | `Ağ bulunamadı. Yalnızca 2,4 GHz ağlar desteklenir.` |
| other | anything else, no IP address within 20 s, or no event at all | `Bağlanılamadı. Tekrar dene.` |

- **Retries.** The station retries within the window: after a disconnect it calls connect again, up to the 20 s limit. The class comes from the **last** reason. A reason in the wrong-password group counts only if it occurred **at least twice**, because 15 and 204 can also come from a weak signal. Otherwise the attempt is classed as other.
- **Open network.** A password submitted for an open network, or none for a secured one, shows up as 201 (driver guide). The page prevents both: it hides the field for open networks and requires 8 characters for secured ones.
- **Security threshold.** `threshold.authmode` is set to `WIFI_AUTH_OPEN` when the password is empty and `WIFI_AUTH_WPA2_PSK` otherwise. That accepts WPA2, WPA2/WPA3 and WPA3. `sae_pwe_h2e` stays at its default, `WPA3_SAE_PWE_BOTH`.
- **Hidden networks.** The station connects with `scan_method = WIFI_ALL_CHANNEL_SCAN`, so a hidden network is probed on every channel. ⚠ Connecting to a hidden network is not documented; owner check.

**Rationale.**
- The driver guide lists 15, 204 and 201 as possibly password-related.
- Espressif's provisioning manager maps 15, 202, 204 and 14 to `WIFI_PROV_STA_AUTH_ERROR`, and 201 to `WIFI_PROV_STA_AP_NOT_FOUND` (`manager.c`, release/v5.5).
- Requiring the reason twice keeps a weak signal from showing up as "wrong password".

**Testing.** The mapping (`classify_failure(reasons…)`) is pure and tested on the host with recorded reason sequences.

## R5. Portal implementation: extend Slice 1 (FR-012)

**Decision.** Keep Slice 1's `esp_http_server` portal and its mutex gate, under which the callbacks run only inside `handle()` on the network worker (`src/platform/setup_portal`). Extend it with:
- the full NET-04 path set and the catch-all redirect (R1);
- `config.max_open_sockets = 7`, `lru_purge_enable = true`, `max_uri_handlers = 8`;
- `CONFIG_LWIP_MAX_SOCKETS = 16` in `sdkconfig.defaults`. Keep at least 3 sockets above httpd's count for its internal use, plus the DNS socket and the HTTPS client of later slices;
- gzip-encoded bodies for `/` and `/places.json` (`Content-Encoding: gzip`, `Cache-Control: no-store`);
- the `httpd_uri`, `httpd_txrx` and `httpd_parse` log levels lowered to error, as in Espressif's example, so the redirect flood does not fill the serial log.

**Measured on the device (T047, 2026-11-24):** with the device's network, DNS responder and portal up and no phone joined, the internal heap had 51 779 B free with a largest block of 31 744 B (55 351 / 31 744 with Slice 3 firmware connected as a station). The largest block is unchanged.

**T068 (2026-11-24):** after `wifi setup` from running mode (station disconnecting, device network, DNS and portal up, no phone yet) the report showed 37 879 B free, largest block 27 648 B; back in running mode 51 075 B / 31 744 B, so nothing leaks across a setup round. A report with a phone joined was not captured on the serial port; the owner's checks B6-B9 ran with phones joined (page load, scans, three attempts) without a restart or failure. Watch the largest block in Slice 6, when TLS runs alongside.

**DNS responder.**
- The wire-format work, parsing a query and building the answer, is a **pure function in `lib/core/src/nvp/net/dns_reply`**, tested on the host with captured queries.
- `src/platform/dns_responder` owns one UDP socket on port 53, bound to the access point's address, and is polled by the network worker.
- It is modelled on Espressif's example `components/dns_server/dns_server.c` (Unlicense OR CC0-1.0, 290 lines). The code is not copied, so there is no licence notice to carry.

**Wi-Fi manager libraries** (for example WiFiManager or `wifi_provisioning`) were not reconsidered. Slice 1's reason stands: the page must also choose the location and match 02 §10 exactly (05 §3). ESP-IDF's `wifi_provisioning` targets its own phone apps (protocomm), not a captive web page.

## R6. Place list: provider shape and generator (LOC-02, FR-021, OD4-3)

Live findings, 2026-11-24:

1. **`GET https://ezanvakti.imsakiyem.com/api/v2/locations/districts?countryId=2&page=N&limit=500`**
   - Envelope: `{success, data:[…], meta:{version:"2", pagination:{page, limit, total, pages}}}`.
   - Headers: `RateLimit-Policy: 100;w=300` and `X-API-Version: 2`.
   - **`total` is 1003 districts in 81 states.** All 1003 ids are distinct.
   - Each item has `district_id` (a string), `name` (title case, for example `Sultanbeyli`), `diyanet_name` (upper case, for example `SULTANBEYLİ`), `state{id, name, names{tr,en,ar}}`, `country{id:"2"}`, `coordinates{lat, lon, confidence}` and `timezone`.
   - `state.name` is inconsistent (`Samsun İli`, `Bursa İli`, `Kastamonu`), so province names come from version 1 (item 2).
2. **`GET /api/locations/states?countryId=2`** (version 1)
   - Returns **81 provinces** `{_id, name, name_en, country_id}`.
   - Names are upper case (`İSTANBUL`, `AĞRI`) and sorted by code point, not Turkish order. The generator sorts them itself (R8).
3. **134 of the 1003 districts have ids ≥ 100000.** The largest groups are İstanbul 21, İzmir 10, Ankara 9 and Sakarya 8.
   - Their `times` answer says `"source":{"type":"copied_from_center","center_district_id":"9541",…}`. Live example: Kadıköy 100059 → İstanbul centre 9541, with times identical to 9541 for 2026-11-24.
   - Diyanet's own districts answer `"source":{"type":"diyanet"}`.
   - **869 Diyanet districts remain.** That fits the reported 880 for Türkiye and Northern Cyprus together (04 §9).
4. **Second source.** `GET https://ezanvakti.emushaf.net/ilceler/539` returns 19 districts for İstanbul, among them `{"IlceAdi":"SULTANBEYLİ","IlceID":"9549"}`. That matches İmsakiyem's 9549, and emushaf has no 100000+ ids.
5. **Diacritics.** Upper-case names contain `Â` (for example `KÂĞITHANE`) besides the Turkish set. Some Diyanet names lack Turkish letters (`ARNAVUTKOY`). They are kept as Diyanet spells them.

**Decision (generator `tools/gen_places.py`).**
- **Requests:** 1 call for the version 1 states, 3 calls for the version 2 district pages, and 1 version 2 `times?district=<id>&date=<today>&period=daily` call per alias to read `center_district_id`. That is about 138 requests, paced at **one per 3.1 s** to stay under 100 per 5 min, about 7 minutes in all.
- **Alias entries** (OD4-3): stored as `[centre_id, "KADIKÖY"]`. The page shows the district's own name, and the device stores the centre's id. When the owner chooses `İSTANBUL` or `KADIKÖY`, the times are the same, as Diyanet publishes them.
- **Checks** (the generator fails otherwise):
  - exactly 81 provinces, each with at least one district;
  - every alias resolves to a centre id inside the same province;
  - no duplicate (province, name) pairs;
  - total entries between 900 and 1100.
- **Spot check:** 5 ids are compared against emushaf `/ilceler/{provinceId}`: İstanbul 9549 and 9541, Ankara 9206, plus two random ones. The result is printed, and a mismatch fails the run.
- **Output:** `data/places_tr.json`, committed, in the compact form of 04 §9 with Turkish ordering (R8):
  `{"generated":"YYYY-MM-DD","source":"…","sha256":"…","provinces":[[id,"İSTANBUL",[[9541,"İSTANBUL"],[9541,"ADALAR"],…]],…]}`
  - The checksum covers `provinces` serialised canonically.
- **Firmware:** the build embeds the list gzip-compressed (R7). The device also needs the names of a chosen id, for storage and the overlay. It does not parse the list: the page submits the names together with the id, and the device checks them against the list (R10).

**Result (T030, 2026-11-24).** `python tools/gen_places.py` made 143 requests in about 8 minutes (one name-lookup failure on the development machine's network was retried; the generator now retries a lookup up to three times, 10 s apart). Output: 81 provinces, 1003 entries (869 Diyanet + 134 centre-mapped), 19 970 bytes as served, 7 938 bytes gzipped; checksum `00971e28…2d8b43` in the file. Review: every province lists its centre first; the fewest districts are Bayburt 3, Bartın, Iğdır and Kilis 4; the most İstanbul 40, Konya 32, İzmir 31, Ankara 26; Turkish letters intact (İ 475, Ş 148, Ç 121, Ü 119, Ö 79, Ğ 77, Â 1); Kadıköy → 9541. Spot check against emushaf 5/5 (9549, 9541, 9206 and two random ids).

**Alternatives considered.**
- Listing only the 869 Diyanet districts: owners would not find `Kadıköy`. Rejected by the owner (OD4-3).
- Storing alias ids: they break the emushaf fallback.
- Version 1 `districts?stateId=` for 81 provinces: 81 requests, and the alias information is still not given.

## R7. The setup page and how it gets into the firmware (FR-013)

**Decision.**
- **Sources** live in `data/portal/`: `index.html`, `style.css`, `app.js` and `ui.js`.
  - `ui.js` holds the shared components: section, network list, select pair, field with toggle, primary button, inline message. Slice 8's settings page reuses them.
  - Plain ES2017, no framework. The palette comes from 02 §2.1 tokens as CSS variables.
- **Build:** `tools/gen_web.py`, run as a PlatformIO pre-script before every firmware build:
  - inlines the CSS and JS into one HTML document;
  - minifies conservatively, removing comments and leading whitespace only, so no JS rewriting;
  - gzips with `mtime=0`, level 9, so the output is deterministic;
  - writes `src/generated/web_assets.cpp/.h` with the two byte arrays `portal_html_gz` and `places_json_gz`, their lengths, and SHA-256 comments. It also gzips `data/places_tr.json`.
  - **It fails if the page is over 25 600 bytes gzipped** (SC-006).
  - The generated files are git-ignored and regenerated when their inputs change.
- **Page logic is tested with `node --test`** (Node 24 is already on the development machine for the fonts). Covered: Turkish title case, the validation rules, merging scan entries for display, and building the form body. `tools/check.py` runs these tests.

**Rationale.**
- A C++ array needs no PlatformIO embed machinery: the local builder registers embedded files only when it parses `build.ninja` (`platforms/espressif32/builder/frameworks/espidf.py` lines 2304–2313). It is also visible to a host test.
- Minifying conservatively avoids an npm minifier dependency. The 25 kB budget is generous for a page of this size.

## R8. Turkish order and title case

**Decision.**
- **Order:** the generator sorts with an explicit key over the Turkish alphabet `A B C Ç D E F G Ğ H I İ J K L M N O Ö P R S Ş T U Ü V Y Z`, with `Â→A`, `Î→İ` and `Û→U` for ordering. This is deterministic and needs no locale on the development machine.
  - **Districts:** the centre comes first (the entry whose name equals the province name, for example `İSTANBUL`, with a non-alias id); then Turkish order, aliases mixed in by name.
- **Title case in the page:** an explicit map for `I→ı` and `İ→i` when lowering, then the first letter of each word is raised with `i→İ` and `ı→I`.
  - Word boundaries: space, `-`, `(` and `.`. Digits are kept (`19 MAYIS` → `19 Mayıs`).
  - It does not rely on `toLocaleLowerCase('tr')`, which older WebViews get wrong.
  - Tested with `node --test`: `SULTANBEYLİ→Sultanbeyli`, `AĞRI→Ağrı`, `KÂĞITHANE→Kâğıthane`, `IĞDIR→Iğdır`, `19 MAYIS→19 Mayıs`.

## R9. Mode state machine (MODE-02 to MODE-06, NET-01, FR-003)

**Decision.**
- **One machine.** Slice 1's `nvp::diag::wifi_step` is evolved into the product machine `nvp::net::mode_step` in `lib/core/src/nvp/net/mode.h/.cpp`, a table-driven pure function. There are not two machines.
  - **States:** `Setup`, `Connecting`, `Grace` (connected, access point closing in 5 s), `RunJoining`, `RunUp`, `RunBackoff`.
  - **Events:** `Boot`, `PhoneJoined`, `PhoneLeft`, `Submit`, `TryStart` (1 s after `202`), `TryOk`, `TryFail(class)`, `TryTimeout`, `GraceElapsed`, `JoinOk`, `JoinFail`, `LinkLost`, `BackoffElapsed`, `StartOver`, and the dev commands `DevSetup`, `DevCancel` and `DevForget`.
- **Inputs:** the context `{silence_ok, has_wifi, has_location}`.
- **Output:** the next state plus action flags, the same style as Slice 1. The flags are start/stop AP, DNS, portal, try submitted, store submitted, discard submitted, erase Wi-Fi, start join, backoff, and screen to show.
- **Timers** are events produced by `src/app` from `uptime_ms`. The machine holds no clock.
- **The host test** enumerates every (state, event) pair against a table in the test, which is the contract in data-model.md. It covers every timeout and failure class, and `Boot` with each combination of stored details.
- **Slice 1's diagnostic Wi-Fi actions** (`wifi setup`, `wifi cancel`, `wifi forget`, and the diagnostic buttons) map to `DevSetup`, `DevCancel` and `StartOver`. The Slice 1 tests are migrated to the new names. The silence rule (radio refused when the silence step failed, FR-001a of spec 001) is kept.

**Rationale.** Constitution VI: the logic is host-testable in `lib/core`, and `src/app` only wires it.

## R10. Storage (FR-017, FR-020, FR-022)

**Decision.** NVS, written only on the network core and never during a scan.

| Namespace / key | Type | Written | Erased by |
|---|---|---|---|
| `wifi/ssid`, `wifi/pass` | string ≤32 B, string ≤63 B | after a successful attempt (Slice 1 keys) | `Baştan kur`, `wifi forget`, factory reset |
| `ap/pass` | 8 digits | first use (Slice 1) | factory reset (Slice 8) |
| `loc/district_id` | u32 | after a successful attempt | factory reset |
| `loc/province`, `loc/district` | string ≤40 B, upper case as in the list | after a successful attempt | factory reset |
| `sec/pin` | 6 digits | first start of this firmware, `esp_random()` | factory reset |

- **Write order on success:** location first, then `wifi/pass`, then `wifi/ssid`, each followed by `nvs_commit`. Wi-Fi is "present" only when the SSID exists. A power cut mid-way therefore leaves either the old state or "location but no Wi-Fi", and the device enters setup. Nothing half-written is used.
- **Names are checked:** the submitted district id must appear in the bundled list under the submitted province with the submitted district name. The device checks this by streaming the gzip-free list. The list is kept uncompressed in flash as well (about 30 kB), so the check needs no inflater. *Alternative:* `miniz` from ROM (`tinfl`). Rejected as more code to get wrong. The 30 kB is affordable within the 2.9 MiB budget.
- **PIN:** six digits, leading zeros allowed, uniform from `esp_random()` by rejection sampling. The same holds for the AP password (Slice 1 already does this).

## R11. QR codes (FR-004)

**Decision.**
- **Generator:** LVGL 9.6.0's own `lv_qrcode` widget (`LV_USE_QRCODE 1`). It embeds Project Nayuki's `qrcodegen` (MIT, `src/libs/qrcode/LICENSE.txt`) and always encodes with **error correction M** (`lv_qrcode.c` line 382: `qrcodegen_getMinFitVersion(qrcodegen_Ecc_MEDIUM, …)`). That is what 02 §7.2 asks for. **No new dependency** is added; LVGL is already pinned at 9.6.0.
- **Payload:** built by a pure function `nvp::net::wifi_qr_payload(ssid, password)` in `lib/core`: `WIFI:T:WPA;S:<ssid>;P:<password>;;`. The characters `\ ; , : "` are escaped with a backslash, per the de-facto ZXing format. The device's own name and password never contain them, but the function is general and tested.
- **Drawing:** the widget is drawn on an `ink` rounded square (radius 14) with 14 px padding. The modules are `bg` on `ink`.
  - **The light colour is `ink` and the dark colour is `bg`.** 02 §2.1: `ink` is the QR background.
  - The module scale is an integer: a payload of about 45 bytes is version 3 (29 modules), and 240 px / 29 gives 8 px modules.
  - Margins are centred. The placement is compared with reference 15.
- **The overlay's QR** is hidden on the device (OD4-2). Fixture 17 in the harness draws it with `http://192.168.1.37/`.

## R12. Touch: tap, hold, two-finger hold (FR-008)

**Decision.**
- **Gestures:** Slice 3's `nvp::screen` gesture module gets a pure `TouchClassifier` that reports `Tap`, `Hold5s` and `TwoFingerHold5s` from (points, x, y, ms) samples.
  - **Tap:** one finger, down for at most 600 ms, moved at most 24 px. A second finger at any time cancels the tap.
  - **Hold5s:** one finger, kept down for 5 000 ms within 24 px. Reported once.
  - **TwoFingerHold5s:** unchanged from Slice 3.
- **Overlay timers:** it closes after 20 000 ms without touch. The confirmation counts as `Vazgeç` after 15 000 ms.
- These thresholds are not in 02 §8. They are design parameters, chosen to tell a tap from a hold, and tested on the host.

## R13. Screens and fonts (FR-004 to FR-009)

**Decision.**
- **New screens** in `lib/ui/src/setup/`: `setup_screen`, `connecting_screen` and `status_overlay` (a layer on the top layer over the main screen, with a `bg` scrim at 72 % opacity). Each takes a plain view struct from `lib/core` (data-model.md). Strings come from `tr.h`.
- **New fonts,** generated by extending `assets/fonts/gen_fonts.py` (02 §2.2):

  | Font | Role | Glyphs |
  |---|---|---|
  | `nvp_cond_600_38` | network name, password, SSID on the connecting screen | printable ASCII + Turkish letters + `Â â Î î Û û` |
  | `nvp_txt_600_19` | step numbers in circles | `1-3` |
  | `nvp_txt_400_18` | overlay values (location, SSID, address, version) | printable ASCII + Turkish set + `Â â Î î Û û` |

  - **`Â/Î/Û` are added to the text fonts the overlay uses**, because Diyanet's names contain `Â` (R6).
  - **The step 3 address and the overlay PIN** were measured in Stage 1 (T031): their digits are 15 px tall on references 15 and 17 (address rows 325–339, PIN rows 316–330). `nvp_num_25` draws 18 px digits, a 21 px Barlow Condensed draws 15 px. The references win over the written table (02 §0 order of authority), so `nvp_num_21` (`0-9 : . space`, 0.7 kB) was added.
  - **Tabular digits:** the references set the setup password, the address and the PIN in tabular figures (the password `4827 1935` is 148 px wide, as eight 18 px cells plus a space). They use the Slice 3 fixed-cell widget, whose space now takes the font's own space advance (the main screen has no spaces in fixed cells, so its renders are unchanged).
  - **The SSID on the connecting screen** is filtered against the font's glyph set. Characters it lacks (emoji, other scripts) become `?` (02 §7.3). The filter is pure, in `lib/core`, and works on UTF-8 code points.
- **Fixtures** 15, 16 and 17 join the Slice 3 fixture table. The `sim` harness renders them, and `compare_reference.py` checks them against the references within 02 §12.
- **Budget:** the fonts stay under 300 kB. Result (T032): the four new fonts add 25.8 kB of glyph bitmaps (`nvp_cond_600_38` 19.8 kB, `nvp_txt_400_18` 5.1 kB, `nvp_num_21` 0.7 kB, `nvp_txt_600_19` 0.2 kB); all 18 fonts hold 112.5 kB of glyph bitmaps. The 14 existing font files regenerate byte for byte.
- **Comparison with the references (T037, T058):** outside the QR modules, 4.8 % of the pixels of fixture 15 and 1.3 % of fixture 16 differ by more than 24 levels, the same level as the main-screen fixtures (glyph rendering). The QR modules differ because the payloads differ (the reference's code is not a real join code). Fixture 17 differs by 6.1 % outside the QR modules, almost all of it behind the scrim: the reference shows a simplified main screen behind the panel, while the device shows fixture 01 at 28 % as 02 §7.4 says. The panel, its rows and its QR square match within 1 px; `check_snapshots.py` asserts the QR squares, circles, panel and confirmation button, and a unit test proves the references meet the same boxes.

## R14. Security review (FR-026)

**While the device's own network is up** (setup only, MODE-05):
- **Joining requires the 8-digit password.** It is shown only on the device screen and in its QR code, so an attacker needs line of sight to the screen. 10^8 possibilities over WPA2-PSK allow an offline dictionary attack on a captured handshake, but only for as long as setup lasts.
  - Accepted: the network exists only during setup, gives no access to the home network, and its password never leaves the device except on screen.
- **An attacker who joins**, as the second of at most two clients, could:
  - read the scan list and the stored location (not secret);
  - submit their own Wi-Fi details. That moves the device to an attacker's network, which the owner sees on the overlay. The owner can start over.
  - The attacker cannot read any password: NET-08, and no endpoint returns one.
- **Denial of service** by filling both client slots: the owner can still finish by waiting, and a restart closes all associations.
- **The portal has no authentication** (NET-04, by specification). Input is bounded and validated:
  - SSID ≤ 32 B, password ≤ 63 B, body ≤ 512 B;
  - district checked against the list;
  - a request while an attempt runs is refused.
- **The DNS responder** answers only on the access point's interface, at most one answer per query, and ignores responses and malformed packets. It never amplifies: the answer is at most query size + 16 B.

**After setup** (running mode):
- No access point and no DNS responder.
- No listening HTTP server until Slice 8, whose settings page will be PIN-protected (SET-02).
- The only network traffic is the Wi-Fi association. Later slices add outbound HTTPS with certificate checks (SEC-01).

**At rest:**
- The Wi-Fi password is in NVS, unencrypted. Flash encryption is not enabled in version 1, so someone with physical access and a serial cable can read it.
- Accepted for a home device and recorded here. The serial console never prints it.

**Logs:** no Wi-Fi password, AP password or PIN in any log line, report or response (FR-023). Checked by SC-008: the quickstart log scan greps the session log for the actual secrets.

**Code review (T067, 2026-11-24):** every `printf`/`log` of `src/app/net_worker.cpp`, `src/platform/wifi_link.cpp`, `setup_portal.cpp`, `settings_store.cpp` and `dns_responder.cpp` was read: none takes a password or PIN argument. The Wi-Fi password goes only to the driver's station configuration (zeroed after use) and NVS; the device-network password only to the driver, the setup screen and its QR payload; the PIN is generated and stored inside `settings_store` and never leaves it. `/api/status` and the report have no password parameter at all. The submission is wiped after a failure, after storing, and in the portal handler (`nvp::net::wipe`, volatile writes).

**SC-008 on the device:** the owner skipped the full-session log scan (owner checklist C1, 2026-11-24). The logs captured during development (boot, setup, the owner's first setup, `wifi setup`/`wifi cancel`, fixtures) carried only network names, addresses and classes.

## R15. Upgrade from Slice 1 (OD4-1)

**Decision.**
- The owner's board has `wifi/ssid`, `wifi/pass` and `ap/pass` and no `loc/*`. At the first start of this firmware, MODE-02 sends it to setup.
- **The stored Wi-Fi details are not erased** by entering setup. They are replaced only when an attempt succeeds. A failed attempt keeps the old ones, so a later `wifi cancel` (development) can still return to running with them.
- **The access-point password is kept**, so the network name and password on the screen are the ones Slice 1 showed.

## R16. LVGL memory on the device (found in Stage 3, 2026-11-24)

**Finding.** On the panel, `fixture 17` (and so a tap that opens the status overlay) crashed with
`StoreProhibited` in `lv_draw_add_task`: the draw-task allocation returned NULL. LVGL's built-in
pool is 64 kB of internal RAM (`include/lv_conf.h`); with the setup and connecting screens, their
QR images and the overlay, it held 48.9 kB used with a largest free block of 9 kB before the overlay
was built. The host harness did not show it because it uses the C library allocator.

**Decision.** At start-up `platform::display` adds four 60 kB pools from PSRAM to LVGL's allocator
(`lv_mem_add_pool`; one pool must stay below 64 kB because LVGL sizes its TLSF tables from
`LV_MEM_SIZE`). The 64 kB internal pool is unchanged, so internal RAM stays free for Wi-Fi and TLS.

**Measured (serial log, 2026-11-24):** LVGL 305 kB in total; 48.9 kB used after start-up, 49.2 kB
with the setup and connecting screens shown, 58.1 kB with the overlay open (20 %); largest free block
62 kB. Internal heap with the station connected: 51.1 kB free, largest block 31.7 kB (unchanged).
The board now logs one `lvgl memory:` line after start-up and after each `fixture` command.

**Alternatives considered.** A larger internal pool: costs internal RAM that Wi-Fi and TLS need.
LVGL on the C library allocator: small allocations would land in internal RAM too
(`SPIRAM_MALLOC_ALWAYSINTERNAL`).
