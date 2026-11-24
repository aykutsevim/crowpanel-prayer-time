# Owner checklist: real time (spec 005)

Constitution VII. The board runs the spec 005 build (flashed 2026-11-24 through COM4). Commands are typed
in the serial monitor (`pio device monitor -p COM4 -b 115200`).

| # | Question | Answer |
|---|---|---|
| D1 | With Wi-Fi: after a restart the time appears, then is corrected within a few seconds; the clock agrees with a phone showing seconds, and the date line shows today's date and weekday in Turkish. || yes |
| D2 | `ntp off`, then `clock invalid`: the screen matches `docs/design/screens/13-state-time-not-set.png` (clock `--:--`, `Saat eşitleniyor`, `Wi-Fi bağlantısı bekleniyor`, no marker). `ntp on`: the real time returns within a few seconds. || yes |
| D3 | Fallback: `ntp off`, wait a minute: the clock stays right (compare with the phone, within 2 s). Then `ntp on`. (Time servers are not blocked at the router; this is the substitute, spec assumption.) || yes |
| D4 | `clock sim 2026-11-24 23:59:50`: at midnight the date line changes to 25 Kasım 2026 Çarşamba without flicker; `clock speed 60` runs on smoothly; `clock rtc` returns to real time. || yes |
| D5 | `tz UTC0`: the clock moves back 3 hours at once; after unplugging and replugging it is still on UTC; `tz <+03>-3` restores it. || yes |
| D6 | Left on overnight: the date changed at midnight and the clock is still right in the morning (send the serial log of the night if you can: the hourly corrections show the drift). || yes |
| D7 | No sound at any point. || yes |

Owner, 2026-11-24: "continue and mark tests as passed" (D1-D7 recorded as yes on the owner's
instruction). D6 (overnight) was marked on the same day as the build, so the overnight drift log was
not provided; research R7 keeps the drift as not yet measured.
