# Owner checklist: automatic refresh and fallback (spec 007)

Constitution VII. The board runs the spec 007 build (flashed 2026-11-26 through COM4, firmware
1 809 472 B). Commands are typed in the serial monitor (`pio device monitor -p COM4 -b 115200`).
The `data block` commands stand in for blocking a host on the router; they are kept in RAM, so a
restart clears them. `data refresh` is accepted once per minute.

## Already shown by the serial log (implementer, 2026-11-26)

These are log lines, not the owner's eyes; the screen itself still needs the questions below.

- First start: `refresh started (boot: never refreshed)`, `imsakiyem ok, 366 records, 2.1 s`,
  `calibration 0 0 0 0 0 0 from 30 days`, next refresh `04.12 03:35` (night minute 03:35).
- `data block imsakiyem`, `data refresh`: `emushaf ok, 26 records`, `refresh complete`, `state ok`.
- `data block emushaf`, `data erase`: `screen: main (state calculated)`, source `local` (8 days).
- `data refresh` with both blocked: `aladhan ok, 37 records`, retry in one hour.
- `data unblock all`, `data refresh`: `imsakiyem ok`, `state ok`.

## Steps and questions

Start each step from a normal device (`data unblock all`, Wi-Fi on), unless it says otherwise.

| # | Steps | Question | Answer |
|---|---|---|---|
| F1 | Look at the main screen; tap it; type `data plan`. | The status line says `Son güncelleme …`; the panel shows `Vakit kaynağı Diyanet (imsakiyem)` and a `Kayıtlı son gün` at least 30 days ahead. | yes |
| F2 | `data block imsakiyem`, then `data refresh`; tap the screen. | Nothing changes on the main screen; the panel names `Diyanet (emushaf)` as source. | yes |
| F3 | `data block emushaf`, `data erase`, wait for the screen, then `data refresh` (a minute after the last one). Compare with `docs/design/screens/12-state-calculated.png` and with Diyanet's website for today. | The layout matches reference 12 (`≈` before every time, `Yaklaşık vakitler, hesaplama ile`); every time is within 5 minutes of the website. | yes |
| F4 | After F3: `data unblock all`. Do nothing else. | Within about 70 minutes the `≈` marks disappear and the times equal Diyanet's website again, by themselves. | yes |
| F5 | Router off (or Wi-Fi out of reach), then `data erase`. | Times calculated on the device appear at once, marked `≈`; the panel names `Hesaplama (cihaz)`. Router back on afterwards. | yes |
| F6 | Read the last stored day from `data show`. Router off (Wi-Fi down), then `clock sim <last stored day minus 5> 12:00:00`. Compare with `docs/design/screens/11-state-cache-ending.png`. Afterwards `clock rtc` and router on. | The status line says `Kayıtlı vakitler 5 gün sonra bitiyor` and matches reference 11. | yes |
| F7 | `data plan`. | The `next weekly` time is between 03:00 and 04:59. | yes |
| F8 | All of the above. | No sound at any point. | yes |

Notes for F6: the `cache_ending` state needs the last refresh to have failed or Wi-Fi to be down
(02 §6 row 4). A refresh with both mirrors blocked still counts as complete while the store reaches
30 days ahead, so Wi-Fi down is the reliable way to show it.

Answers given by the owner on 2026-11-26: "mark all as OK" (F1 to F8 yes). No log output was sent with
the answers; the screen checks rest on the owner's eyes, the log lines above on the implementer's capture.
