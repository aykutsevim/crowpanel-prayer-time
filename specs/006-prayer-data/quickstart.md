# Quickstart: validating real prayer times

## 1. Development machine (Stage 1)

```text
pio test -e native     # JsonStream (chunk sizes 1/7/64/1024), parsers on the recorded fixtures and their
                       # truncated/reordered/corrupted/wrong-district variants, validate_batch per reason,
                       # store format, merge, save with a failure injected at every step
python tools/check.py
```

**Expected:**
- All pass.
- The power-loss test reports that every step was interrupted and each outcome was exactly the old or exactly the new store.

## 2. Device (Stage 2, always `--upload-port COM4`)

1. **After flashing**, with the board already set up: the log shows `data: imsakiyem v2 ok, N records stored …` within 20 s of `wifi: connected`, and the screen leaves `no_data`.
2. **`data show`** lists 9206, the date range and `crc ok`. The report shows `data: … today yes`.
3. **Compare with Diyanet:** compare three dates on `https://namazvakitleri.diyanet.gov.tr/` (Ankara). The implementer lists the device's values for the three dates in the owner checklist.
4. **Offline:**
   - Router off, then restart. The stored times appear at once.
   - After 10 minutes the state is `offline` (reference 10).
5. **Corrupt store:**
   - `data corrupt`, then restart with Wi-Fi on.
   - The log shows `bad checksum, ignored`; the screen shows `no_data`, then the real times after the refresh.
6. **Erase:** `data erase` with Wi-Fi off, then restart. The screen matches reference 14.
7. **Overlay:** a tap shows `Diyanet (imsakiyem)`, the update time and the last stored date.

## 3. Owner checklist

The brief's nine questions, plus the three dates with the device's six times and Hijri date for each.
