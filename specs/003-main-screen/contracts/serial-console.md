# Contract: serial console additions (spec 003)

Extends [spec 001's serial console](../../001-silent-board-bringup/contracts/serial-console.md): same port, line format and case/space rules. The commands below marked **dev** exist only in development builds (`NVP_DEV_TOOLS`, SYS-06); a release build answers `unknown command, type help`. All replies are one line unless stated.

## Mode commands (FR-022, FR-023)

| Command | Effect | Reply |
|---|---|---|
| `diag` | main screen → diagnostic screen. On the diagnostic screen it changes nothing. | `diagnostic mode` / `already in diagnostic mode` |
| `main` | diagnostic or settings screen → main screen; stops stress mode if running | `main screen` / `already on the main screen` |

## Clock (dev; FR-018, FR-026 to FR-030)

| Command | Effect | Reply |
|---|---|---|
| `time get` | reads the board clock now | `rtc: 2026-11-24 09:30:05 UTC (12:30:05 local), valid` or `rtc: not valid (integrity lost)` / `(no answer)` / `(century)` / `(bad field)` / `(before build)` |
| `time set YYYY-MM-DD HH:MM:SS` | writes the board clock from this **local** time (UTC+3), clears the integrity mark, switches the screen to the board clock | `rtc set: 2026-11-24 09:30:05 UTC` or `rtc set failed (<reason>)`; refused for dates outside 2000–2099 |
| `clock rtc` | the screen follows the board clock again | `clock: rtc` |
| `clock sim YYYY-MM-DD HH:MM:SS` | simulated local time, starts at speed 1 | `clock: sim 2026-11-17 14:32:07 x1` |
| `clock speed N` | speed 1–3600 for the simulated clock; refused on the board clock | `clock: speed xN` |
| `clock pause` / `clock run` | freezes / resumes the simulated clock | `clock: paused` / `clock: running` |

## Screen state and fixtures (dev; FR-017, FR-018)

| Command | Effect | Reply |
|---|---|---|
| `state ok\|offline\|calculated\|no_time\|no_data` | forces that state | `state: forced <name>` |
| `state cache_ending N` | forces `cache_ending` with N stored days (0–7) | `state: forced cache_ending N` |
| `state auto` | clears a forced state | `state: auto` |
| `fixture N` | N = 1–14: loads that fixture's moment (simulated clock, paused) and state | `fixture N: <reference file name>` |
| `fixture off` | leaves the fixture: board clock, state auto | `fixture: off` |

Invalid arguments reply `invalid argument, type help` and change nothing.

## Report additions (FR-026)

Two lines after `reset:` in the report, before `=== END ===`:

```
clock: rtc 2026-11-24 12:30:05 local, valid | rtc not valid (integrity lost) | sim 2026-11-17 14:32:07 x60
screen: main (state ok) | diagnostic | settings
```

No password or secret is ever printed (FR-024 of spec 001).
