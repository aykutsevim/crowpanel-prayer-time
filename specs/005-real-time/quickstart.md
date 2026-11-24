# Quickstart: validating real time

## 1. Development machine (no hardware)

```text
pio test -e native          # WallClock (validity, sources, refusals, RTC writes), Zone (POSIX TZ incl.
                            # DST transitions), HTTP date parser, state selection from real inputs
pio run -e sim -t exec      # snapshots + redraw checks, including the midnight run
python tools/check.py       # all of the above, TLS rules (date checks on), silence, size
```

**Expected:**
- All tests pass.
- `check_tls.py` reports `CONFIG_MBEDTLS_HAVE_TIME_DATE=y`.
- The midnight check changes the date exactly once.

## 2. Device (always `--upload-port COM4`)

1. **Start with Wi-Fi.** The log shows the board clock line, `time: ntp … ok, correction …, rtc written`, and the report shows `time: valid, source ntp`. Compare the screen with a phone showing seconds.
2. **Second alignment:** run `time jitter 600`. The p99 must be ≤ 100 ms (SC-003).
3. **Fallback:** `ntp off`. After 60 s the log shows `time: http … ok`; compare with the phone (±2 s). Then `ntp on`: the next SNTP answer replaces it.
4. **No time:** `time set` cannot make the board clock invalid. To see reference 13 the owner either:
   - pulls the CR1220 cell (optional); or
   - with the router off, runs `clock invalid` (marks the time not valid for this boot).

   In both cases the router must be off (or `ntp off` with the router on, within the 60 s before the fallback). The screen must match reference 13.
5. **Midnight:**
   - `clock sim 2026-11-24 23:59:50` at speed 1, then at speed 60.
   - Leave the device on overnight.
6. **Zone:** `tz UTC0`. The clock moves 3 h at once. Restart: it is kept. Then `tz <+03>-3`.
7. **Drift:** after 24 h, read the hourly `correction` values in the log.

## 3. Owner checklist (draft)

1. With Wi-Fi: the real time and Turkish date appear, and the clock agrees with a phone showing seconds.
2. With the router off and the board clock made invalid (step 4): the screen matches reference 13 and stays so.
3. The fallback (step 3) gives the right time within 2 s. Instead of blocking time servers at the router, `ntp off` is used (spec assumption).
4. Overnight: the date changed at midnight, and the clock is still right in the morning.
5. `tz UTC0` changes the clock at once and survives a restart.
6. No sound.
