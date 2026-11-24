# Data model: Wi-Fi setup from the phone

All types live in `lib/core` (pure C++17, constitution VI), unless a type is named in `src/`.

## Stored settings (NVS, research R10)

| Entity | Fields | Rules |
|---|---|---|
| `WifiDetails` | `ssid` (1–32 bytes, any bytes but NUL), `pass` (empty, or 8–63 characters) | Stored only after `TryOk`. Present means `wifi/ssid` exists. The password is write-only: no API, log or report returns it (NET-08). |
| `Location` | `district_id` (u32 > 0), `province` (1–40 bytes UTF-8, upper case as in the list), `district` (same) | Stored only after `TryOk`. The (id, province, district) triple must exist in the bundled list (`place_lookup`). An alias district stores its centre's id (OD4-3). `Baştan kur` keeps it. |
| `ApPassword` | 8 decimal digits | Generated once (Slice 1). Kept until factory reset. Shown only on the device screen and in the QR code. |
| `SettingsPin` | 6 decimal digits | Generated at the first start of this firmware. Kept until factory reset. Not shown in this slice (dash). |

**Write order on success:** `loc/*` → `wifi/pass` → `wifi/ssid`. Each write is committed. No write happens while a scan runs.

## Submission and attempt

```text
Submission { ssid[33], pass[64], district_id, province[41], district[41] }
  valid when: 1 ≤ len(ssid) ≤ 32; len(pass) == 0 or 8 ≤ len(pass) ≤ 63;
              place_lookup(district_id, province, district) is true
FailureClass { None, WrongPassword, NotFound, Other }
Attempt { submission, started_ms, reasons[8] (ring), class }
```

`classify_failure(reasons, n, seen_in_scan)` returns the class (research R4):
- the last reason is 201 or 210–212 → `NotFound`;
- the last reason is in {2, 14, 15, 202, 204}, it occurred at least twice, and the network was seen in the scan → `WrongPassword`;
- otherwise → `Other`.

## Mode state machine (`nvp::net::mode_step`, research R9)

The context is `{ silence_ok, has_wifi, has_location }`. Timers are delivered as events by `src/app`.

| State | Event | Next | Actions |
|---|---|---|---|
| (start) | `Boot`, `has_wifi && has_location` | `RunJoining` | start join |
| (start) | `Boot`, otherwise | `Setup` | scan once, then start AP + DNS + portal; show setup screen |
| (start) | `Boot`, `!silence_ok` | `RunNoRadio` | show main screen; no radio, no setup (spec 001 FR-001a, analysis U1) |
| `Setup` | `PhoneJoined` / `PhoneLeft` | `Setup` | update footer (count of joined phones) |
| `Setup` | `Submit` (valid) | `Connecting` | answer `202`; schedule `TryStart` in 1 s; show connecting screen |
| `Setup` | `Submit` (invalid) | `Setup` | answer `400` |
| `Connecting` | `Submit` | `Connecting` | answer `409` (attempt already running) |
| `Connecting` | `TryStart` | `Connecting` | start the station with the submission; 20 s timer |
| `Connecting` | `TryOk` (got an IP) | `Grace` | store submission; show `Bağlandı`; 5 s timer |
| `Connecting` | `TryFail(c)` or `TryTimeout` | `Setup` | discard submission; disconnect the station; footer = class (timeout → `Other`) |
| `Grace` | `GraceElapsed` | `RunUp` | stop portal, DNS and AP; show main screen |
| `RunJoining` | `JoinOk` | `RunUp` | reset backoff |
| `RunJoining` | `JoinFail` | `RunBackoff` | wait `Backoff::next_delay_ms()` (5 s … 5 min) |
| `RunUp` | `LinkLost` | `RunBackoff` | as above |
| `RunBackoff` | `BackoffElapsed` | `RunJoining` | start join |
| any `Run*` | `StartOver` (`Baştan kur`, `wifi forget`) | `Setup` | erase `wifi/*`; disconnect; scan; start AP + DNS + portal |
| any `Run*` | `DevSetup` (`wifi setup`, diagnostic button) | `Setup` | disconnect; scan; start AP + DNS + portal (stored details kept) |
| `Setup` | `DevCancel` (`wifi cancel`), `has_wifi && has_location` | `RunJoining` | stop portal, DNS and AP; start join |
| `Setup` | `DevCancel`, otherwise | `Setup` | refused (nothing to return to) |

| `RunNoRadio` | `StartOver` | `RunNoRadio` | erase `wifi/*` only |
| `RunNoRadio` or any `Run*` without silence | `DevSetup` | unchanged | refused |

- **Pairs not listed** leave the state unchanged with no action.
- **No `Run*` row starts the AP by itself** (MODE-05).
- **`Setup` has no timer** (MODE-03).
- The host test checks the full (state × event) grid against this table.

## Scan entries (research R3)

```text
ScanEntry { ssid[33], rssi (int8), auth: Open | Secured | Unsupported }
merge: same ssid → keep strongest; drop empty ssid (hidden); sort by rssi desc; keep 20
signal_class(rssi): ≥ -60 → "iyi", ≥ -75 → "orta", else "zayıf"   (02 §7.4)
```

`auth_class(wifi_auth_mode_t value)` is pure and maps raw values per research R3. `src/platform` passes the raw number.

## Screen views (`lib/core`, consumed by `lib/ui`)

```text
SetupView      { ap_ssid[33], ap_pass_grouped[10] ("4827 1935"), qr_payload[96],
                 footer: Waiting | PhoneJoined | Failed(class) }
ConnectingView { ssid_display[33×4] (unsupported glyphs → '?'), phase: Trying | Connected }
OverlayView    { location[96] ("İstanbul, Sultanbeyli" title-cased) or "—",
                 source "—", last_update "—", last_day "—",
                 wifi[64] ("EvAgi_2.4G, iyi" | "bağlı değil"), address[16] or "—",
                 pin "—", version[16], show_qr (false on device, OD4-2), qr_url[40],
                 confirm: Hidden | Shown }
```

- The device stores names in upper case. `title_case_tr()` in `nvp::text` converts them for the overlay, using the same rules as the page (research R8).
- The footer colours come from 02 §7.2: `muted` while waiting, `ink` once a phone is joined, `alert` after a failure.

## Touch classifier (`nvp::screen::TouchClassifier`, research R12)

The input is a sample `{points (0..5), x, y, ms}`. The output is `None | Tap | Hold5s | TwoFingerHold5s`, reported once per touch.

**Thresholds:**
- tap: at most 600 ms and at most 24 px of movement;
- hold: 5 000 ms;
- overlay auto-close: 20 000 ms;
- confirmation default: 15 000 ms, counted as `Vazgeç`.

## Place list (`data/places_tr.json`, research R6)

```text
{ "generated": "YYYY-MM-DD", "source": "<urls>", "sha256": "<hex of canonical provinces>",
  "provinces": [ [province_id, "İSTANBUL", [ [9541, "İSTANBUL"], [9541, "ADALAR"], [9535, "ARNAVUTKOY"], … ] ], … ] }
```

- 81 provinces in Turkish order.
- Within a province, the centre comes first, then the districts in Turkish order. The ids may repeat (aliases).
- `place_lookup(json, id, province, district) → bool` streams the uncompressed copy in flash and is tested on the host.
