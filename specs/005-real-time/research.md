# Research: Real time

Sources are the local ESP-IDF 5.5.5 framework package (`~/.platformio/packages/framework-espidf`, version file `5.5.5`), read on 2026-11-24, and live requests made that day with the User-Agent `NamazVaktiPanel/0.4-dev`. Items marked ⚠ need a measurement on the device.

## R1. Network time (TIME-01, FR-001)

**Decision.**
- **Client:** ESP-IDF's `esp_netif_sntp` with three servers from different operators: `pool.ntp.org`, `time.google.com`, `time.cloudflare.com`. That needs `CONFIG_LWIP_SNTP_MAX_SERVERS=3`; the default is 1.
- **Resync:** every hour (`CONFIG_LWIP_SNTP_UPDATE_DELAY` = 3 600 000 ms, the default). TIME-01 requires at most 6 h.
- **Start:** SNTP starts once, when the station first gets an address with the owner's network: at the end of a successful setup attempt (`TryOk`, so the `Saat ve vakitler alınıyor` step is true) or at the first join in running mode (analysis I2). When the link returns after a loss, `esp_sntp_restart()` asks for a fresh answer at once. In setup mode there is no internet, so SNTP never runs there.
- **Start-up delay:** the RFC 4330 random delay stays on (`CONFIG_LWIP_SNTP_STARTUP_DELAY=y`, at most 5 000 ms by default). Even with the worst delay and a slow answer, SC-001's 10 s holds.
- **Every answer goes through our validity rule.** lwIP calls the weak function `sntp_sync_time(struct timeval*)` (`components/lwip/apps/sntp/sntp.c` line 40), whose default calls `settimeofday()` directly in immediate mode. The firmware overrides it in `src/platform/time_sync`. The override only records the sample (UTC microseconds and the uptime when it arrived) under a lock. The app applies it through `nvp::time::WallClock`, which refuses a time before the build (TIME-03) and logs the correction (FR-014).
- **Immediate, not smooth:** smooth mode (`adjtime`) would hide a correction over minutes, but TIME-05 requires immediate application.

**Rationale.** The API, its configuration struct (`esp_sntp_config_t`: `num_of_servers`, `servers[]`, `sync_cb`, `start`) and the weak sync hook are in the local headers `esp_netif_sntp.h` and `apps/esp_sntp.h`. Overriding the hook is the one place every answer passes through, with no second path that could set the clock unchecked.

**Alternatives considered.**
- Using the `sync_cb` notification after lwIP has already set the time: an invalid time would be on the system clock for a moment, and TLS (R3) reads that clock.
- A hand-written NTP client: more code, and no gain.

## R2. Fallback through an HTTP `Date` header (TIME-02, FR-002, OD5-1)

**Decision.**
- **When:** 60 s after the first address without an accepted SNTP answer, the worker requests `http://ezanvakti.imsakiyem.com/`. If that fails, it tries `http://ezanvakti.emushaf.net/`. The attempts repeat every 60 s until an SNTP or fallback sample is accepted.
- **How:** plain HTTP, port 80, redirects not followed, only the response headers read.
- **Live check, 2026-11-24:** both hosts answer `301 Moved Permanently` with a `Date: Tue, 24 Nov 2026 11:41:17 GMT` header (nginx, and Cloudflare respectively). Both are on the host list of 04 §11 (SEC-02), so no new host is contacted.
- **Value:** `Date` has one-second resolution and is truncated by the server. The sample is `date + 0.5 s + round_trip / 2`, which gives ±1 s plus half the round trip. That is within TIME-02's ±2 s for round trips under 2 s; a longer round trip makes the sample be ignored.
- **Parsing:** a pure parser in `lib/core` (`nvp::time::parse_http_date`) accepts only the IMF-fixdate form of RFC 9110 §5.6.7 (`Sun, 06 Nov 1994 08:49:37 GMT`). It has host tests.
- **Plausibility:** a sample is accepted only if it is not earlier than the build. A sample from SNTP always replaces a fallback time. (A rule tying the fallback to the board's clock was considered and dropped: a board clock that is wrong by more than a day would then block the fallback for good, and a forged NTP answer is no harder.)
- **Why not HTTPS:** owner decision OD5-1 turns on certificate date checks (R3). A TLS handshake therefore needs a valid clock, which is exactly what the fallback exists to provide. Plain HTTP is used only to read this one header. Nothing else from the answer is used, and nothing is sent but the request line, `Host` and `User-Agent`.
- **Security:** someone on the local network could forge the header, but SNTP is unauthenticated too, so this adds no new risk. Forging is bounded by the plausibility rule and replaced by the next SNTP answer.

**Alternatives considered.**
- HTTPS with date checks off: rejected by the owner (OD5-1).
- `time.google.com` over HTTP: that is an NTP host and has no web server.

## R3. Certificate dates (OD5-1, SEC-01, FR-016)

**Decision.**
- `CONFIG_MBEDTLS_HAVE_TIME_DATE=y` in `sdkconfig.defaults`. The local `components/mbedtls/Kconfig` line 679 says it is off by default, and that then "valid from" and "valid to" are ignored. When on, mbedTLS checks them against `time()`.
- `platform::https` refuses to start a request while the app's clock is not valid, with the reason "time not valid". In this slice that is the diagnostic HTTPS probe; from Slice 6, the data fetches.
- `tools/check_tls.py` gains two rules:
  - fail when `sdkconfig.defaults` does not set `CONFIG_MBEDTLS_HAVE_TIME_DATE=y`;
  - allow an `esp_http_client_config_t` without a CA only in `src/platform/http_date.cpp`, whose URLs must start with `http://`.
- **System time while the clock is not valid:** it is set to the build time. Code that reads `time()` then never sees 1970, but nothing is shown as valid, and HTTPS stays refused by the rule above.

## R4. One clock for everything (FR-003, FR-005, FR-006, FR-011)

**Decision.**
- **The base is the system clock** (`gettimeofday`/`settimeofday`, in `src/platform/systime`). Accepted samples set it, so TLS (R3) and the app agree.
- **The pure model `nvp::time::WallClock`** in `lib/core` decides validity and source, and holds the last sync:
  - `accept(sample, now_system, build_utc)` returns accepted (with the signed correction), or refused as before the build, or ignored (an Http sample while an Ntp time is younger than 6 h).
  - Validity: valid from a board-clock reading that passed Slice 3's checks, or from an accepted SNTP or fallback sample. Once valid, the clock stays valid until restart (FR-006); time keeps running between syncs (TIME-05).
  - Sources: `Rtc`, `Ntp`, `Http`, with priority Ntp > Http > Rtc. An Http sample never replaces an Ntp time younger than 6 h.
- **The app** (`src/app/clock`): `now()` returns the simulated time while the simulated clock is on (unchanged from Slice 3). Otherwise it returns the system time converted with the stored zone (R5), valid as `WallClock` says.
- **Board clock:** at start, a valid reading sets the system clock (source `Rtc`). After every accepted Ntp or Http sample, the board's clock is written again (TIME-08, FR-004). The clock is an I²C register write, not flash, so there is nothing to wear out and no reason to limit it (analysis I1).
- **Where the write runs (analysis A1):** in `clock::apply_pending()` on the UI loop. The board clock shares its I²C bus with the touch controller, which is read on the UI thread; writing from the network core would need a cross-core bus lock. The write is one short transaction (about 1 ms) after each sync, hourly in steady state. That is within constitution IX's intent: no network or flash I/O on the UI thread. A failing write is logged once per boot (analysis C1).
- **Tests:** host tests use a test double: `WallClock` driven with explicit uptimes and samples.

## R5. Time zone (TIME-04, FR-012, FR-013)

**Decision.**
- **A pure POSIX TZ parser and converter in `lib/core`** (`nvp::time::Zone`):
  - std name (letters, or `<…>` quoted), offset `[+-]hh[:mm[:ss]]`;
  - optional dst name and offset;
  - optional rules `,start[/time],end[/time]` in the forms `Mm.w.d`, `Jn` and `n`.
  - `utc_to_local` and `local_to_utc` follow those rules.
  - Host tests: `<+03>-3` (Türkiye), `UTC0`, `CET-1CEST,M3.5.0,M10.5.0/3`, `EST5EDT,M3.2.0,M11.1.0` around both transitions, and invalid strings.
- **Replaces** Slice 3's fixed `nvp::time::zone` (UTC+3) everywhere it is used.
- **Storage:** NVS namespace `set`, key `tz`, beside Slice 4's keys. Default `<+03>-3`. An invalid string is never stored.
- **Not newlib:** `setenv("TZ")`/`tzset` are not used. The engine and the tests need the same conversion on the development machine, and newlib's parser cannot be tested there.

## R6. The second on screen within 100 ms (TIME-07, FR-009, SC-003)

**Decision.**
- **Mechanism:**
  - `loop()` (UI task, core 1) reads the clock on every pass, not every 100 ms.
  - When the second differs from the one last drawn, it updates the main screen at once and calls `platform::display::refresh_now()`. That flushes the changed area in the same pass.
  - The 100 ms view refresh stays for everything else.
- **Measurement:**
  - A development command `time jitter 600` records, for 600 s, the millisecond within the real second (from the system clock) at which the new second's flush finished.
  - It prints the count, the median, the 99th percentile and the maximum. SC-003 needs the 99th percentile ≤ 100 ms.
  - ⚠ To be measured on the device (T-task).

## R7. Drift between syncs (FR-006, SC-002)

**Finding.**
- The ESP32-S3 system time counts from the high-resolution timer, clocked by the 40 MHz crystal of the ESP32-S3-WROOM-1 module.
- The module datasheet gives no crystal tolerance figure that we could cite. ⚠ The drift is measured on the device instead: each SNTP resync logs its correction, which is the drift over the hour.
- With hourly resyncs, a drift below 0.1 s per hour keeps SC-002's 1 s easily. The log of the owner's 24-hour test gives the numbers.

## R8. Which state the screen shows (FR-007)

**Decision.** `nvp::screen::select_state` (Slice 3) is fed from real inputs:
- `clock_valid` comes from `WallClock`;
- `today_present` is false until Slice 6.

So the device shows `no_time` or `no_data` (references 13 and 14). The other rows stay forced-only (`state …`, development builds).

## R9. Midnight (TIME-06, FR-010, SC-006)

**Decision.**
- The engine's day change is Slice 2's (tested by its full-day sweep).
- The date line comes from `MainView`.
- **New host check:** `sim/ui_render/redraw_check.cpp` gains a midnight run with the real date path. 23:59:59 → 00:00:00 must change the date exactly once, and no frame may mix the old and new day.
- **Device check:**
  - `clock sim 2026-11-24 23:59:50` at speed 1, then at speed 60, watched by the owner.
  - The real midnight is seen in the overnight owner test.

## R10. Logging (FR-014, FR-015, SYS-05)

**Decision.** One line per sync attempt result:

```text
[  5123] time: ntp time.google.com ok, correction -0.184 s, rtc written
[ 65200] time: http ezanvakti.imsakiyem.com ok, correction +2.0 s (rtt 140 ms)
[ 65200] time: ntp refused (before build)
```

The report gains a line:

```text
time: valid, source ntp, last sync 12 min ago, zone <+03>-3
```

The line reads `time: not valid (waiting for network)` until the clock is valid.

## R11. Development commands (FR-013, SYS-06)

- **`tz`:** prints the zone. **`tz <string>`:** sets and stores a new zone, or refuses an invalid one.
- **`time sync`:** asks SNTP for a fresh answer now.
- **`ntp off` / `ntp on`:** stops or starts SNTP. `ntp off` also forgets the freshness of the last SNTP time and starts the 60 s fallback timer, so the fallback can be shown without blocking time servers at the router (spec assumption).
- **`clock invalid`:** marks the time not valid for this boot (system clock back to the build time), so the `no_time` screen can be shown without removing the board's battery.
- **`time jitter N`:** R6.
- `time get`, `time set` and `clock …` from Slice 3 are kept. `time set` writes the board clock and the system clock and sets the source `Rtc`.

## Measured on the device (2026-11-24, serial log, COM4)

- **SNTP (T020, SC-001):** `time: ntp ok` 2.9 s after `wifi: connected` on the first boot, 0.8 s on later boots. The correction of the board-clock time was +0.902 s on the first sync (the board clock had been set by hand in Slice 3) and +0.42 to +0.51 s after restarts a few minutes after a write: the BM8563 keeps whole seconds, so a reading lags by up to one second. The board clock only bridges a start without Wi-Fi.
- **Linking (R1):** the linked `sntp_sync_time` is the firmware's (`addr2line` on `firmware.elf` points to `src/platform/time_sync.cpp`), so lwIP never sets the clock by itself.
- **Second alignment (T020, R6, SC-003):** `time jitter 60`: 61 samples, median 5 ms, p99 8 ms, maximum 8 ms after the real second boundary.
- **Fallback (T029, R2, SC-004):** after `ntp off`, the fallback ran 60 s later: `ezanvakti.imsakiyem.com` answered with a 47 ms round trip, correction +0.424 s. After `ntp on` the next SNTP answer corrected -0.500 s, so the fallback was within 0.5 s of network time.
- **No time (T024):** `ntp off` then `clock invalid` gave `screen: main (state no_time)` and `time: not valid (waiting for network)`; `ntp on` made the time valid again 2.2 s later.
- **Zone (T034, SC-007):** `tz UTC0` moved the clock from 15:20 to 12:20 at once; `tz bad zone` was refused. The first test showed the zone lost across a restart: the clock read it before NVS was opened by the network worker. `platform::settings` now opens NVS itself; after the fix the zone was kept across a reset.
- **Midnight (T031):** `clock sim 2026-11-24 23:59:55` reached `2026-11-25 00:00:03` with the state unchanged; the owner watches the screen (checklist).
- **Drift (R7):** pending the owner's overnight run; each hourly `time: ntp ok, correction …` line is the drift over that hour.
- **Fallback repeat (T038, convergence):** while SNTP stays silent, an accepted fallback sample re-arms the fallback for 6 h later (the age at which `WallClock` lets an Http sample replace an older time); an accepted SNTP sample disarms it. Re-checked on the device: `ntp off` → `http … ok, correction +0.107 s` after 60 s → `ntp on` → `ntp ok, correction -0.219 s`. The 6 h repeat itself was not observed in a short session.
