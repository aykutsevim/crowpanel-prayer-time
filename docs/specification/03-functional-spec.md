# Functional specification

Requirements are numbered so that specs, plans, tasks and tests can cite them. “Must” is mandatory for version 1. Where a requirement belongs to a later slice, the slice is named in `06-roadmap.md`.

Related documents: screens and strings in `02-ui-spec.md`; providers, cache format and refresh policy in `04-data-sources.md`; hardware and code structure in `05-architecture.md`.

## 1. Device modes

```
            ┌────────┐  no Wi-Fi credentials or no location
 power on → │  BOOT  │ ───────────────────────────────────────┐
            └───┬────┘                                        ▼
                │ credentials + location present         ┌─────────┐
                ▼                                        │  SETUP  │◄───────────┐
            ┌────────┐   owner confirms “Baştan kur”     │ (AP on) │            │
            │  RUN   │ ─────────────────────────────────►└────┬────┘            │
            └────────┘                                        │ form submitted  │
                ▲                                             ▼                 │
                │  connected, settings saved            ┌────────────┐  failed  │
                └────────────────────────────────────── │ CONNECTING │ ─────────┘
                                                        └────────────┘
```

- **MODE-01** BOOT disables audio hardware first (§9), then initialises display, touch, storage and settings, and shows the boot screen. It must not wait for the network.
- **MODE-02** If Wi-Fi credentials or a location are missing, enter SETUP. Otherwise enter RUN and show the main screen from cache at once; networking starts in the background.
- **MODE-03** SETUP runs the access point and captive portal and shows the setup screen. It has no timeout.
- **MODE-04** CONNECTING tries the submitted credentials for up to 20 s while keeping the access point up. On success: persist credentials and location, stop the access point after a 5 s grace period, enter RUN. On failure: discard the submitted credentials, return to SETUP and show the matching error on the device screen.
- **MODE-05** RUN never starts the access point by itself. Only the owner can leave RUN, through the confirmation in the status overlay (`02-ui-spec.md` §7.4) or a factory reset.
- **MODE-06** “Baştan kur” erases Wi-Fi credentials only. Location, settings and cached times are kept, and the portal preselects the stored location.

## 2. Time

- **TIME-01** Wall-clock time comes from SNTP over Wi-Fi, using at least three servers from different operators (for example `pool.ntp.org`, `time.google.com`, `time.cloudflare.com`). Resynchronise at least every 6 hours.
- **TIME-02** If SNTP has not succeeded within 60 s of Wi-Fi coming up, set the clock from the `Date` header of an HTTP response from a well-known host, then keep trying SNTP. Accuracy of ±2 s is acceptable for this fallback.
- **TIME-03** Time is **valid** only after a successful sync since power-on (or from a detected RTC, see TIME-08) and only if it is later than the firmware build time. Until then the UI is in the `no_time` state.
- **TIME-04** The time zone is a POSIX TZ string stored in settings. Default `<+03>-3` (Türkiye, no daylight saving). All prayer-time logic uses local civil time.
- **TIME-05** Between syncs the clock runs from the system tick. A sync that moves the clock is applied immediately and all derived state is recomputed; the countdown may jump.
- **TIME-06** At local midnight the engine switches to the new day’s record without a visible glitch: the night period continues, the columns show the new day’s six times.
- **TIME-07** The clock on screen updates within 100 ms of each second boundary.
- **TIME-08** (optional, Slice 10) If a battery-backed real-time clock is available (the chip built into the Advance board, or a DS3231 module detected on the basic board’s I²C connector), read it at boot so the device works after a power cut without Wi-Fi, and write it after each successful network sync. Absence of a clock is the normal case on the basic board and must not log errors.

## 3. Prayer-time engine

The engine is a pure function with no hardware or network dependencies, so that it can be tested on a development machine.

**Input:** current local date-time; day records for yesterday, today and tomorrow (any may be missing); settings (`soon_threshold_min`, kerahat durations).
**Day record:** date, six times as minutes after local midnight in the order İmsak, Güneş, Öğle, İkindi, Akşam, Yatsı, optional Hijri date string, source id.

- **ENG-01** A record is valid only if its six times are strictly increasing and lie between 00:00 and 23:59. Invalid records are treated as missing.
- **ENG-02** A vakit begins at second 00 of its minute. The period that begins at vakit *k* ends when vakit *k+1* begins; the Yatsı period ends at the next day’s İmsak.
- **ENG-03** Current period:
  - now < today’s İmsak → Yatsı period that began yesterday; start = yesterday’s Yatsı, end = today’s İmsak, next = İmsak.
  - today’s vakit *k* ≤ now < vakit *k+1* → period *k*.
  - now ≥ today’s Yatsı → Yatsı period; start = today’s Yatsı, end = tomorrow’s İmsak, next = İmsak (tomorrow).
- **ENG-04** If yesterday’s record is missing, use today’s Yatsı time as yesterday’s. If tomorrow’s record is missing, use today’s İmsak time as tomorrow’s. The error is at most a couple of minutes and affects only the progress bar and, after Yatsı, the countdown; set an `approx_bounds` flag in the output for logging.
- **ENG-05** Outputs: period index, period start and end as absolute local times, next vakit index, remaining seconds, elapsed seconds, elapsed fraction 0…1, `soon` (remaining ≤ threshold), `entered` (elapsed < 5 min), and for each of the six columns: displayed time, state (`past`, `current`, `next`, `later`) and seconds until it. Rules for column states are in `02-ui-spec.md` §4.5, including the after-Yatsı and before-İmsak cases.
- **ENG-06** Kerahat intervals for the strip: [Güneş, Güneş + a], [Öğle − b, Öğle], [Akşam − c, Akşam], with a, b, c from settings, each 0…90 minutes, 0 = hidden. Intervals are clipped to the day.
- **ENG-07** The engine must be correct at these boundaries, each covered by a unit test: one second before and at each vakit; 23:59:59 → 00:00:00; missing yesterday; missing tomorrow; all three records identical; a record that changes at midnight by ±2 minutes; the last day of a month and of a year.
- **ENG-08** With today’s record missing the engine returns “no data” and the UI enters `no_data`.

Test fixture used throughout (also in the reference renders): yesterday Yatsı 19:15; today 06:16, 07:46, 12:54, 15:27, 17:51, 19:14; tomorrow İmsak 06:17. Expected values at 14:32:07: period Öğle, bounds 12:54–15:27, remaining 00:54:53, fraction 0.641, İkindi `next` with note `54 dk sonra`, Akşam `3 sa 18 dk sonra`, Yatsı `4 sa 41 dk sonra`.

## 4. Display behaviour

- **UI-01** The main screen follows `02-ui-spec.md` exactly. System states are evaluated in the order given in its §6.
- **UI-02** Screen content never depends on the network being reachable at that moment; it is rendered from the engine output and cached records.
- **UI-03** Rendering work is bounded: per second only the clock, countdown and progress fill change; per minute the now marker and column notes; on vakit change, column states, leaders, hero texts; on day change, everything.
- **UI-04** The UI thread never performs network or flash I/O.
- **UI-05** Backlight level comes from settings (§8). Changes take effect within one second. *Amended by owner decision OD1 (2026-11-20):* there is no backlight setting; the backlight is always fully on (see SET-04). The requirement still applies to the other display settings in §8.
- **UI-06** Touch behaviour is limited to `02-ui-spec.md` §8.

## 5. Wi-Fi and provisioning

- **NET-01** Station mode uses stored credentials. On loss of connection, retry with exponential backoff from 5 s up to 5 min, indefinitely. Only 2.4 GHz networks are supported.
- **NET-02** Access point: SSID `NamazVakti-` followed by the last four hexadecimal digits of the station MAC address, upper case. WPA2-PSK. Password: eight decimal digits generated from the hardware random number generator on first boot and kept until factory reset. Address 192.168.4.1/24 with DHCP. At most two clients.
- **NET-03** While the access point is up, a DNS responder answers every query with 192.168.4.1, and the web server answers the usual captive-portal probe paths (`/generate_204`, `/gen_204`, `/hotspot-detect.html`, `/library/test/success.html`, `/connecttest.txt`, `/ncsi.txt`, `/canonical.html`, `/success.txt`, `/redirect`) and any unknown host or path with a redirect to `http://192.168.4.1/`, so that phones open the portal page automatically.
- **NET-04** Portal HTTP interface (setup mode, no authentication, reachable only on the access point):

| Method and path | Purpose |
|---|---|
| `GET /` | setup page (`02-ui-spec.md` §10) |
| `GET /api/scan` | JSON list of visible networks: ssid, signal class, secured flag; at most 20, strongest first, duplicates merged |
| `GET /places.json` | bundled provinces and districts with Diyanet district ids |
| `GET /api/status` | mode, last connect result, stored location |
| `POST /api/setup` | body: ssid, password, district id; starts CONNECTING; responds at once with `202` |

- **NET-05** A failed connection attempt is classified as wrong password, network not found, or other, and shown with the texts in `02-ui-spec.md` §7.2.
- **NET-06** Networks with a hidden SSID can be entered by hand. Open networks (no password) are accepted. Enterprise (802.1X) networks are not supported and are shown as such in the list.
- **NET-07** In station mode the device announces itself as `namazvakti.local` (mDNS) and serves the settings page (§8).
- **NET-08** No API returns a stored password. Password fields are write-only.

## 6. Data, cache and fallback

Full detail is in `04-data-sources.md`. The behavioural contract:

- **DATA-01** Times shown as official are Diyanet-published values obtained from one of the Diyanet-derived providers.
- **DATA-02** The device stores day records on flash and serves the screen from that store. A fresh install shows real times as soon as the first fetch completes; after that the screen is independent of network availability for as long as the store covers today.
- **DATA-03** The store is refreshed automatically: after setup or a location change, at boot if needed, on a weekly schedule at night, and whenever fewer than 30 days ahead are stored.
- **DATA-04** Each refresh walks an ordered list of providers and stops at the first one that returns valid data. Failure of one provider never surfaces as an error on screen if another succeeds.
- **DATA-05** If no Diyanet-derived provider can supply today’s record, calculated times are used and the UI enters the `calculated` state. A calculated record never replaces a stored Diyanet-derived record for the same date, and is replaced as soon as a Diyanet-derived one arrives.
- **DATA-06** Every fetched batch is validated (ENG-01 plus plausibility checks) before it is stored. A batch that fails validation is discarded as a whole.
- **DATA-07** Writes to the store survive power loss at any moment: after a restart the store is either the old or the new version, never a corrupt mix.
- **DATA-08** A location change discards the store for the old district only after the first record for the new district has been stored.

## 7. Location

- **LOC-01** The location is one Diyanet district, identified by Diyanet’s numeric district id together with its province and district names.
- **LOC-02** Provinces and districts of Türkiye are bundled with the firmware so that the location can be chosen in the setup portal without internet access.
- **LOC-03** Coordinates for the district are obtained once after setup (see `04-data-sources.md` §6) and stored for the calculated fallbacks. The owner can override latitude and longitude in the settings page.
- **LOC-04** Locations outside Türkiye are out of scope for version 1.

## 8. Settings

Stored in non-volatile storage; applied without restart unless noted.

| Key | Meaning | Default | Range | Editable in |
|---|---|---|---|---|
| `district_id`, `province_name`, `district_name` | location | none | bundled list | portal, settings page |
| `lat`, `lon` | coordinates for fallbacks | resolved automatically | Türkiye bounding box | settings page (advanced) |
| `tz` | POSIX TZ string | `<+03>-3` | any valid string | settings page (advanced) |
| `kerahat_enabled` | show kerahat layer | true | bool | settings page |
| `kerahat_sunrise_min`, `kerahat_noon_min`, `kerahat_sunset_min` | durations | 45, 45, 45 | 0…90 | settings page |
| `soon_threshold_min` | countdown turns to alert colour | 15 | 0, 5, 10, 15, 30 | settings page |
| `diyanet_user`, `diyanet_pass` | official API credentials (optional) | empty | text | settings page (advanced) |
| `ap_password` | access-point password | generated | 8 digits | not editable |
| `settings_pin` | PIN for the settings page | generated | 6 digits | not editable |

- **SET-01** Settings page (station mode, Slice 8) at `http://<device-ip>/` and `http://namazvakti.local/`, Turkish, same visual language as the portal. Sections: `Konum`, `Görünüm`, `Veri` (source, last update, stored range, button `Şimdi güncelle`), `Gelişmiş`, `Sıfırla`.
- **SET-02** Every state-changing request needs the settings PIN shown in the status overlay. Five wrong attempts lock the page for five minutes. Read-only status needs no PIN.
- **SET-03** `Fabrika ayarlarına dön` erases credentials, settings, PIN, access-point password and cached times, then restarts into SETUP. It asks for confirmation.
- **SET-04** *Dropped (owner decision OD1, 2026-11-20).* There are no brightness or night-dimming settings, and the keys `brightness_day`, `night_dim_enabled` and `brightness_night` were removed from the table above. On the target board the backlight is dimmed only through the auxiliary controller, whose dimming frequency is undocumented, so SIL-04 cannot be confirmed for any dimmed level; the backlight is therefore always fully on (`specs/001-silent-board-bringup/research.md` R6, `05-architecture.md` §2). The id is kept so that it is not reused.

## 9. Silence

- **SIL-01** The device must not emit sound. There is no audio feature, setting, icon or wording that suggests one.
- **SIL-02** The first action after reset, before any other peripheral, is to put the audio amplifier into shutdown, drive the I²S lines to a fixed level and, on boards that have one, hold the buzzer inactive. The I²S peripheral is never initialised.
- **SIL-03** No code path generates tones on any pin (no buzzer, no PWM in the audible band on any output).
- **SIL-04** Backlight PWM, if used, runs at 20 kHz or above. If the backlight driver produces audible noise at any duty cycle, PWM dimming is disabled and the backlight is held fully on. On the target board (DIS02170A V1.4) the ESP32 has no backlight pin and configures no PWM; the backlight is always fully on (OD1, see SET-04).
- **SIL-05** The build fails if source code or dependencies reference audio APIs (see `05-architecture.md` §9 for the check).
- **SIL-06** Touch gives no audible or haptic feedback.

## 10. Reliability

- **SYS-01** A hardware watchdog restarts the device if the UI loop stalls for 10 s or the network task for 120 s.
- **SYS-02** After any restart the device returns to RUN with the main screen from cache without user action.
- **SYS-03** Flash writes are infrequent: settings on change, the day store on refresh. No periodic writes more often than once per day in steady state.
- **SYS-04** Memory use is steady: no growth of heap use over a 24 h soak test beyond 4 kB.
- **SYS-05** Serial logging at 115200 baud: one line per mode change, sync, refresh attempt per provider (provider, result, records, duration), and validation failure. No passwords, PINs or tokens in logs.
- **SYS-06** A serial test command set, compiled only in development builds, lets a developer set a simulated time, time speed, system state and data fixture, so that every screen state can be shown on the device on demand.

## 11. Security and privacy

- **SEC-01** HTTPS requests verify server certificates against a bundled root store. Certificate checks are never disabled, including in development builds.
- **SEC-02** The device contacts only the hosts listed in `04-data-sources.md` plus the time servers.
- **SEC-03** Requests carry no identifier of the owner or the device beyond what the protocol requires. The `User-Agent` names the project and version.
- **SEC-04** Wi-Fi and API credentials are stored on the device only and are never logged or served back.
- **SEC-05** No analytics, no remote logging, no update checks to third parties.

## 12. Targets

| Area | Target |
|---|---|
| Boot to main screen from cache | ≤ 5 s |
| First real times after setup, on a normal home connection | ≤ 20 s after Wi-Fi association |
| Offline autonomy after a full refresh | until the end of the stored range, at least 30 days, normally to the end of the calendar year |
| Outbound requests in steady state | ≤ 10 per day on average |
| Firmware image (kept as a discipline on the 16 MB target board) | ≤ 2.9 MB |
| Clock error | ≤ 1 s after SNTP sync, drift corrected at each resync |
