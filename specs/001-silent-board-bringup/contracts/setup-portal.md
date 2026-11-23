# Contract: test Wi-Fi setup (hotspot and HTTP)

A subset of NET-02 and NET-04, so that Slice 4 extends it rather than replacing it. The page and its wording are engineering text in English.

## Hotspot

| Item | Value |
|---|---|
| SSID | `NamazVakti-XXXX`: last 4 hex digits of the station MAC, upper case |
| Security | WPA2-PSK. Password: 8 decimal digits from NVS `ap/pass`, generated on first use |
| Address | 192.168.4.1/24, DHCP |
| Clients | at most 2 |
| Mode | AP+STA, so scan and connect work while it is up |

- **Started**: only by a confirmed on-screen action or by `wifi setup`.
- **Closed**: on success, on cancel, or on restart.
- **Refused**: while the silence step is not acknowledged.

## HTTP (port 80, reachable only on the hotspot)

| Method and path | Response |
|---|---|
| `GET /` | the single setup page (inline HTML, CSS and JS, under 4 kB) |
| `GET /api/scan` | `200` with `{"networks":[{"ssid":"…","rssi":-54,"secured":true}]}`. At most 20 entries, strongest first, duplicate SSIDs merged. Strings are JSON-escaped. |
| `POST /api/setup` | form or JSON `ssid`, `password`. Answers `202 {"accepted":true}` at once, then tries for up to 20 s. Rejects an empty or over-32-byte SSID, or a password longer than 63 bytes, with `400`. |
| `GET /api/status` | `200 {"state":"setup|trying|connected|failed","ssid":"…"}`. **Never contains a password** (NET-08). |
| anything else | `404` |

## Outcomes

- **Success**: the details are stored in NVS `wifi/ssid` and `wifi/pass`. The device screen shows `Connected to <ssid>`. The page shows "connected" on its next status poll. The hotspot closes 5 s later, as a grace period (MODE-04).
- **Failure**: the details are discarded. The hotspot stays up. Both the screen and the page show `Connection failed`. Classifying the reason is Slice 4 (NET-05).
