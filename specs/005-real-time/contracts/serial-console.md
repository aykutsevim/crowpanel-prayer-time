# Contract: serial console additions (FR-013..FR-015, SYS-05, SYS-06)

Extends the contracts of specs 001, 003 and 004. All commands below are development-only
(`NVP_DEV_TOOLS`), except that the report lines appear in every build.

## Commands

| Command | Effect | Reply |
|---|---|---|
| `tz` | prints the stored zone | `tz: <+03>-3` |
| `tz <POSIX string>` | parses, stores in NVS `set/tz` and applies at once | `tz: set <string>` or `tz: refused (invalid)` |
| `time sync` | asks SNTP for a fresh answer now (running mode only) | `time: sync requested` or `time: refused (not connected)` |
| `ntp off` / `ntp on` | stops / starts SNTP. `ntp off` also forgets the freshness of the last SNTP time and starts the 60 s fallback timer, so the HTTP fallback runs and is accepted (research R2, R11) | `ntp: off` / `ntp: on` |
| `clock invalid` | marks the time not valid for this boot (system clock back to the build time, source none), so `no_time` can be shown without removing the board's battery; the next accepted sample makes it valid again | `clock: invalid until the next sync` |
| `time jitter N` | for N seconds (10–3600), records when each new second reaches the panel; then prints the result | `jitter: 600 s, median 12 ms, p99 41 ms, max 77 ms` |
| `time get`, `time set …` | unchanged from spec 003. `time set` also sets the system clock and makes the source `rtc` | unchanged |
| `clock rtc` | (spec 003) back to the real clock after `clock sim` or a fixture; the real clock is now the board clock corrected by network time | `clock: real (board clock and network time, spec 005)` |

## Report line (after `clock:`)

```text
time: valid, source ntp, last sync 12 min ago, correction -0.184 s, zone <+03>-3
time: valid, source rtc (no network sync yet), zone <+03>-3
time: not valid (waiting for network), zone <+03>-3
```

## Log lines (one per sync result, FR-014)

```text
[   8087] time: ntp ok, correction +0.508 s, rtc written
[ 167211] time: http ezanvakti.imsakiyem.com answered (rtt 47 ms)
[ 167214] time: http ezanvakti.imsakiyem.com ok, correction +0.424 s, rtc written
[ 167300] time: http ezanvakti.emushaf.net failed (no Date header)
[   5124] time: ntp refused (before build)
[ 107160] ntp: off

(lwIP does not say which of the three servers answered, so the ntp line names none.)
```

No line contains a password, PIN or token (FR-023 of spec 004).
