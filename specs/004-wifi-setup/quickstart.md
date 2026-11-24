# Quickstart: validating Wi-Fi setup

## Prerequisites

- Development machine with PlatformIO, Python 3 and Node 24, as for Slice 3.
- The board on USB; an iPhone and an Android phone; the home router, which must be on 2.4 GHz.
- **Before committing:** restore `.gitignore` if a tool has rewritten it (`git checkout -- .gitignore`).

## 1. On the development machine (no hardware)

```text
python tools/gen_places.py           # only when regenerating the place list (≈7 min, 1 request / 3.1 s)
pio test -e native                   # mode machine, failure classes, DNS reply, scan merge, QR payload,
                                     # place lookup, touch classifier, title case
node --test data/portal/test/logic.test.js   # page logic (Node 24 needs the file, not the folder)
pio run -e sim && <exe>              # renders fixtures 01–17
python tools/compare_reference.py 15 16 17
python tools/check.py                # all of the above + silence, TLS, size budgets, page ≤ 25 600 B gz
```

**Expected:**
- All tests pass.
- `gen_places.py` prints `81 provinces, 1003 entries (869 Diyanet + 134 centre-mapped), spot check 5/5`.
- The renders of 15 to 17 are within the 02 §12 tolerances.
- The firmware image is ≤ 2.9 MiB and the fonts ≤ 300 kB.

## 2. On the device

Flash it with `pio run -e crowpanel7_advance -t upload --upload-port COM4` (always name the panel's port), then open the monitor with `pio device monitor`.

1. **First start after the upgrade** (OD4-1): the log shows `mode: setup (no location)`, and the screen matches reference 15. The network name on the screen is the one the phone lists.
2. **Join:**
   - Scan the QR code with the iPhone camera. Does the page open by itself? If not, open a browser and type `192.168.4.1`.
   - Repeat with the Android phone.
   - The footer changes to `Telefon bağlandı. Açılan sayfayı doldur.`
3. **Wrong password:** submit the home network with a wrong password. The device shows the connecting screen, then returns to setup with `Wi-Fi şifresi kabul edilmedi. Tekrar dene.`
4. **Network not found:** choose `Ağım listede yok` and type `YokBoyleAg`. The footer reads `Ağ bulunamadı. Yalnızca 2,4 GHz ağlar desteklenir.`
5. **Correct details:** the device shows `Bağlandı`, the device's network disappears from the phone within about 5 s, and the main screen appears. The log shows `stored wifi + location`.
6. **Power cut:** unplug and replug. The main screen appears, the log shows `connected`, and nothing asks for setup.
7. **Router off for 30 min:** the main screen stays up, and the device's network never appears on the phone.
8. **Overlay:**
   - A tap opens it with the location, Wi-Fi and signal class, address, version and dashes, and without the QR code.
   - Another tap closes it. It also closes on its own after 20 s.
9. **Start over:**
   - Hold the overlay 5 s, then press `Vazgeç`: nothing changes.
   - Repeat and press `Baştan kur`: the setup screen appears, and on the page the province and district are preselected.
10. **Log scan (SC-008):** save the monitor output of steps 1 to 9 to a file outside the repository, then run `python tools/scan_log_secrets.py <log>`.
    - The tool asks for the home Wi-Fi password and the access-point password with a hidden prompt. It keeps them in memory only and searches the log for them.
    - It also lists every run of six or more digits that is not a timestamp or an address, for the owner to judge. The PIN is never shown, so it cannot be typed in.
    - It prints `0 occurrences` and the list. No command ever prints a secret to help the check.

## 3. Owner checklist

`owner-checklist.md` in this directory is filled from the answers. It contains the brief's 13 questions plus three:
- "Did the page open by itself after a camera QR join (iPhone)?"
- "Did the Android phone stay on the device's network long enough for the page to load?"
- "Was the hidden network typed by hand accepted?"

The last one is optional; answer it if there is a hidden network to test with.
