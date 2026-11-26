# Owner checklist: real prayer times (spec 006)

Constitution VII. The board runs the spec 006 build (flashed 2026-11-24 through COM4, firmware
1 745 184 B). Commands are typed in the serial monitor (`pio device monitor -p COM4 -b 115200`).

## Already shown by the serial log (implementer, 2026-11-24)

These are log lines, not the owner's eyes; the screen itself still needs the questions below.

- `data: imsakiyem v2 ok, 366 records stored (2026-11-24..2027-11-24), 2.1 s` came 2.2 s after
  `wifi: connected` (SC-001 target 20 s); the report said `screen: main (state ok)` and
  `data: 366 records …, today yes, source imsakiyem` (T021).
- After a restart the store loaded from flash at 0.44 s (`store_9206.bin loaded, 366 records`).
- `data corrupt`, restart: `store_9206.bin bad checksum, ignored`, report `state no_data`, then
  `imsakiyem v2 ok` 2.1 s after the connection (SC-006, T026).
- `data erase` while connected: `store erased`, then a refresh at once (today missing); `data refresh`
  answered `refresh requested` and refreshed; `data show` said `crc ok`.

## Three dates to compare with Diyanet (SC-002)

Open `https://namazvakitleri.diyanet.gov.tr/`, choose Ankara / Ankara (merkez), and compare. To see a
future date on the device, type `clock sim YYYY-MM-DD 12:00:00`; the screen then shows that day's
stored times. `clock rtc` returns to real time.

| Date | İmsak | Güneş | Öğle | İkindi | Akşam | Yatsı | Hijri date |
|---|---|---|---|---|---|---|---|
| 24.11.2026 (today) | 06:08 | 07:36 | 12:40 | 15:12 | 17:34 | 18:57 | 15 Cemaziyelahir 1448 |
| 04.12.2026 (+10 days) | 06:17 | 07:47 | 12:44 | 15:09 | 17:31 | 18:55 | 25 Cemaziyelahir 1448 |
| 03.01.2027 (+40 days) | 06:33 | 08:03 | 12:58 | 15:21 | 17:42 | 19:07 | 25 Recep 1448 |

These are the values of the İmsakiyem v2 answer the device stored (the same answer recorded in
`test/fixtures/providers/`, which agreed with v1 and emushaf, research R1). The screen is the
device's own copy: if the screen and this table differ, say so.

## Questions

| # | Question | Answer |
|---|---|---|
| E1 | After a restart with Wi-Fi, real times appear on the main screen within about 20 seconds, and the city line says `Ankara`. | yes |
| E2 | Today's six times on the screen equal Diyanet's website for Ankara. | yes |
| E3 | With `clock sim 2026-12-04 12:00:00` and `clock sim 2027-01-03 12:00:00`, the screen's times equal the website's for those days (then `clock rtc`). | yes |
| E4 | The Hijri date under the date line equals the one on Diyanet's site for each of the three days. | yes |
| E5 | A tap opens the status panel: `Vakit kaynağı Diyanet (imsakiyem)`, `Son güncelleme` today's date and time, `Kayıtlı son gün 24.11.2027`. | yes |
| E6 | Router off, device unplugged and plugged in again: the same times appear at once; after 10 minutes the status line turns to the offline text (compare with `docs/design/screens/10-state-offline.png`). Send the `report` output. | yes |
| E7 | `data corrupt`, then a restart with Wi-Fi on: the device shows the times again by itself within a minute. | yes |
| E8 | Router off: `data erase`; the screen matches `docs/design/screens/14-state-no-data.png`. Router on, `data refresh` (after the Wi-Fi reconnects): the times come back without a restart. | yes |
| E9 | While a refresh runs (`data refresh`), the picture stays steady (TLS now works in PSRAM, research R3). | yes |
| E10 | No sound at any point. | yes |

Answers given by the owner on 2026-11-26: "all test ok, E6-E10 all yes". E1 to E5 were confirmed
by "all test ok", including the comparison of the three dates with Diyanet's website (SC-002).
The `report` output asked for in E6 was not sent; T023 rests on the owner's eyes, not on a log line.
