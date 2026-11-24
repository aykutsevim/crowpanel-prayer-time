# Contract: device network, DNS and portal HTTP (NET-02 to NET-06, NET-08)

Extends `specs/001-silent-board-bringup/contracts/setup-portal.md`. Paths, form encoding and the `202` answer are unchanged. What is new is marked **new**.

## Device network

| Item | Value |
|---|---|
| SSID | `NamazVakti-XXXX` (last 4 hex digits of the station MAC, upper case) |
| Security | WPA2-PSK; password = NVS `ap/pass` (8 digits); PMF optional |
| Address | 192.168.4.1/24; DHCP with option 6 = 192.168.4.1 and **new** option 114 = `http://192.168.4.1/` |
| Clients | at most 2 |
| Up | only in modes `Setup`, `Connecting` and `Grace` (data-model.md) |

## DNS (UDP 53, access-point interface only) — new

- **A query:** answered with one A record, 192.168.4.1, TTL 0.
- **AAAA or any other type:** NOERROR, no answers.
- **Not answered:** malformed packets, responses (QR=1), and queries with more than one question.
- The answer is never longer than the query plus 16 bytes.

## HTTP (TCP 80)

| Method and path | Answer |
|---|---|
| `GET /` | `200`, `text/html; charset=utf-8`, `Content-Encoding: gzip`, `Cache-Control: no-store`: the setup page (02 §10), under 25 600 B gzipped |
| `GET /places.json` **new** | `200`, `application/json`, gzip: `data/places_tr.json` (data-model.md) |
| `GET /api/scan` | `200 {"scanning":false,"networks":[{"ssid":"…","rssi":-54,"signal":"iyi","auth":"secured"}]}`. `auth` is `open`, `secured` or `unsupported` (**new**; replaces `secured:true`). At most 20 entries, strongest first, duplicates merged. |
| `GET /api/scan?refresh=1` **new** | starts a grouped rescan (research R3) unless one is running; answers at once with the cached list and `"scanning":true` |
| `GET /api/status` | `200 {"mode":"setup|connecting|connected","result":"none|wrong_password|not_found|other","ssid":"…","location":{"id":9549,"province":"İSTANBUL","district":"SULTANBEYLİ"}}`. `location` is `null` when none is stored. `ssid` is the last submitted network name. **Never a password** (NET-08). |
| `POST /api/setup` | `application/x-www-form-urlencoded`, body ≤ 512 B, fields `ssid`, `password`, `district_id`, `province`, `district` (**new**: the last three). Answers: `202 {"accepted":true}`; `400 {"error":"ssid|password|location"}`; `409 {"error":"busy"}` while an attempt runs. |
| anything else, any `Host` **new** | `302 Found`, `Location: http://192.168.4.1/`, `Content-Type: text/html`, `Cache-Control: no-store`, body `<a href="http://192.168.4.1/">Kurulum</a>` |

- **Server settings:** `max_open_sockets` 7, LRU purge on, `httpd_*` logs at error level (research R5).
- **No listener on 443.**
- **No request handler blocks.** Handlers read the published snapshot or post a command to the network worker (05 §6).
- **Validation:**
  - `ssid`: 1–32 bytes after URL decoding.
  - `password`: empty, or 8–63 bytes.
  - `district_id`, `province`, `district` must pass `place_lookup`.
  - An empty password is allowed only when the chosen network is open in the cached scan, or it was typed by hand. The device cannot know a hidden network's security, so it tries.

## The page (02 §10)

1. **Loading:** the page fetches `/api/status`, `/places.json` and `/api/scan`.
   - It preselects the stored location (MODE-06), matched by (province, district) names. Two entries can share an id (aliases), so the id alone is not enough.
   - It applies Turkish title case.
2. **Before submitting:**
   - Networks with `auth == "unsupported"` are shown greyed with `desteklenmiyor` and cannot be chosen.
   - The validation texts are `Bir ağ seç`, `Şifre en az 8 karakter olmalı` and `İl ve ilçe seç`.
3. **After submitting:**
   - It shows `Cihaz Wi-Fi ağına bağlanıyor. Sonucu cihazın ekranından takip et.`
   - It polls `/api/status` every 3 s for at most 60 s. Errors and timeouts are ignored (the phone may have dropped off).
   - The page shows nothing beyond the fixed message: the device screen is the source of truth (02 §10 item 6). If a poll sees `mode == setup` again, it re-enables the form without any new text, so the owner can try again after reading the device screen.

**Wording added to 02 §10 by the owner (OD4-4):** enterprise and other unsupported networks are shown greyed, with `desteklenmiyor` after the name.
