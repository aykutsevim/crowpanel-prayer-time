# Contract: engine output table (spec 002, FR-022, SC-005)

`pio run -e engine_table` builds, and `python tools/check_engine_table.py` runs and checks, the program that prints this table for the shared fixture (`03-functional-spec.md` §3: yesterday Yatsı 19:15; today 06:16, 07:46, 12:54, 15:27, 17:51, 19:14; tomorrow İmsak 06:17; date 17 Kasım 2026 Salı; soon threshold 15 min). The program's output must equal the table below byte for byte (one header line, one separator, fifteen rows, UTF-8, U+2019 in `dk’dan`, U+2013 between the bounds).

The expected values were computed independently of the engine (a separate Python model of the rules in `engine-api.md`, 2026-11-24) and checked against the reference renders `02-main-gece-0330.png` (03:30:00) and `09-main-yatsi-2130.png` (21:30:00): bounds, countdown, column times, states and notes agree. Renders `01`, `03` to `08` show the 14:32:07, 06:50, 10:00, 12:55, 16:20, 17:42 and 18:20 rows.

Column cells read `<state>` or `<state> <note>`. The İmsak column after Yatsı shows tomorrow's time (06:17).

| Time | Period | Bounds | Countdown label | Remaining | Soon | Entered | İmsak | Güneş | Öğle | İkindi | Akşam | Yatsı |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 03:30:00 | Yatsı vakti | 19:15–06:16 | İmsaka kalan süre | 02:46:00 | no | no | next 2 sa 46 dk sonra | later 4 sa 16 dk sonra | later 9 sa 24 dk sonra | later 11 sa 57 dk sonra | later 14 sa 21 dk sonra | later 15 sa 44 dk sonra |
| 06:15:59 | Yatsı vakti | 19:15–06:16 | İmsaka kalan süre | 00:00:01 | yes | no | next 1 dk’dan az sonra | later 1 sa 30 dk sonra | later 6 sa 38 dk sonra | later 9 sa 11 dk sonra | later 11 sa 35 dk sonra | later 12 sa 58 dk sonra |
| 06:16:00 | İmsak vakti | 06:16–07:46 | Güneşe kalan süre | 01:30:00 | no | yes | current vakit girdi | next 1 sa 30 dk sonra | later 6 sa 38 dk sonra | later 9 sa 11 dk sonra | later 11 sa 35 dk sonra | later 12 sa 58 dk sonra |
| 06:50:00 | İmsak vakti | 06:16–07:46 | Güneşe kalan süre | 00:56:00 | no | no | current şu an | next 56 dk sonra | later 6 sa 4 dk sonra | later 8 sa 37 dk sonra | later 11 sa 1 dk sonra | later 12 sa 24 dk sonra |
| 10:00:00 | Kuşluk vakti | 07:46–12:54 | Öğleye kalan süre | 02:54:00 | no | no | past | current şu an | next 2 sa 54 dk sonra | later 5 sa 27 dk sonra | later 7 sa 51 dk sonra | later 9 sa 14 dk sonra |
| 12:54:00 | Öğle vakti | 12:54–15:27 | İkindiye kalan süre | 02:33:00 | no | yes | past | past | current vakit girdi | next 2 sa 33 dk sonra | later 4 sa 57 dk sonra | later 6 sa 20 dk sonra |
| 12:55:00 | Öğle vakti | 12:54–15:27 | İkindiye kalan süre | 02:32:00 | no | yes | past | past | current vakit girdi | next 2 sa 32 dk sonra | later 4 sa 56 dk sonra | later 6 sa 19 dk sonra |
| 14:32:07 | Öğle vakti | 12:54–15:27 | İkindiye kalan süre | 00:54:53 | no | no | past | past | current şu an | next 54 dk sonra | later 3 sa 18 dk sonra | later 4 sa 41 dk sonra |
| 16:20:00 | İkindi vakti | 15:27–17:51 | Akşama kalan süre | 01:31:00 | no | no | past | past | past | current şu an | next 1 sa 31 dk sonra | later 2 sa 54 dk sonra |
| 17:42:00 | İkindi vakti | 15:27–17:51 | Akşama kalan süre | 00:09:00 | yes | no | past | past | past | current şu an | next 9 dk sonra | later 1 sa 32 dk sonra |
| 17:51:00 | Akşam vakti | 17:51–19:14 | Yatsıya kalan süre | 01:23:00 | no | yes | past | past | past | past | current vakit girdi | next 1 sa 23 dk sonra |
| 18:20:00 | Akşam vakti | 17:51–19:14 | Yatsıya kalan süre | 00:54:00 | no | no | past | past | past | past | current şu an | next 54 dk sonra |
| 19:14:00 | Yatsı vakti | 19:14–06:17 | İmsaka kalan süre | 11:03:00 | no | yes | next yarın | past | past | past | past | current vakit girdi |
| 21:30:00 | Yatsı vakti | 19:14–06:17 | İmsaka kalan süre | 08:47:00 | no | no | next yarın | past | past | past | past | current şu an |
| 23:59:59 | Yatsı vakti | 19:14–06:17 | İmsaka kalan süre | 06:17:01 | no | no | next yarın | past | past | past | past | current şu an |
