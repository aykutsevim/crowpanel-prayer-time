# Quickstart: validating spec 007

## Host (no hardware)

```text
pio test -e native -f test_providers -f test_store -f test_planner -f test_calc
python tools/check.py
```

Expected:
- **test_planner**: every scenario of the 04 §12 table and of the brief passes (all sources up; each source down in turn; both mirrors down with a full, nearly empty and empty store; 429/503; malformed data; year end; reconnect storm; location change mid-refresh). Each asserts the request sequence and the resulting store; the budget test fails above 400 requests a week (research R6).
- **test_providers**: emushaf and Aladhan parsers on `test/fixtures/providers/emushaf/` and `aladhan/`, plausibility rejections (shifted hour, shifted day, another district).
- **test_calc**: the port against adhan-js `Shared/Times/*.json` within their variance, and against the 128 published Ankara days within ±2 minutes; calibration and the ±10 limit; tabular Hijri with offset.
- **test_store**: merge never lets rank 10/20 replace rank 40 (SC-006); `refresh.bin` survives a cut at every file-system call.

## Device (COM4, Ankara 9206)

Flash: `pio run -e crowpanel7_advance -t upload --upload-port COM4`; monitor: `pio device monitor -p COM4 -b 115200`.

| Check | Commands | Expected |
|---|---|---|
| Normal | `data plan`, `data show`, tap the screen | status `Son güncelleme …`; panel `Diyanet (imsakiyem)`, last day ≥ today + 30; next weekly refresh at the night minute between 03:00 and 04:59 |
| First mirror down | `data block imsakiyem`, `data refresh` | screen unchanged; log `emushaf … ok`; panel names emushaf for its days |
| Both mirrors down | `data block imsakiyem`, `data block emushaf`, `data erase`, `data refresh` | reference 12; times within 5 min of Diyanet's website |
| Return | `data unblock all` | within 70 min the `≈` goes away by itself (hourly retry) |
| No internet | router off, `data erase` | calculated times from the device, `≈`, `Hesaplama (cihaz)` in the panel |
| Cache ending | `data block all…`, `clock sim <last day − 5> 12:00:00` | reference 11 with `5 gün sonra bitiyor` |
| Silence | all of the above | no sound |
