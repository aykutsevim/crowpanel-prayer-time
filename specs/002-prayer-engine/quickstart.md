# Quickstart: validate the prayer-time engine (spec 002)

No board is needed. Everything runs on the development machine; the device build only proves the code compiles for the ESP32-S3.

## Prerequisites

As in the project README: Python 3.11+, PlatformIO Core 6.2+ with `native@1.2.1`, a host C++ compiler. Branch `feat/002-prayer-engine`.

## 1. Engine tests

```text
pio test -e native -f test_engine
```

Expected: every test passes. The suite covers each acceptance scenario of [spec.md](spec.md), every ENG-07 boundary, the full-day sweep with its invariants (SC-003), the no-allocation check (SC-007), and every string of `02-ui-spec.md` §9 byte for byte (SC-004).

## 2. Output table

```text
pio run -e engine_table
python tools/check_engine_table.py
```

The second command runs the built program, prints the table and compares it with the contract. (`pio run -e engine_table -t exec` also prints it, but PlatformIO's console echo fails on Turkish letters on a cp1252 Windows console unless `PYTHONIOENCODING=utf-8` is set.)

Expected: the table in [contracts/engine-table.md](contracts/engine-table.md), byte for byte. A test also compares the two, so a mismatch fails step 1 or step 3.

Then open the reference renders in `docs/design/screens/` (`01` to `09`) next to the matching rows and confirm period name, bounds, countdown label and value, column states and notes (SC-005).

## 3. Full check

```text
python tools/check.py
```

Expected: `CHECK PASSED`. This runs all host tests, builds the device firmware (so `lib/core` with the engine compiles for the ESP32-S3 under the ESP-IDF warning flags, SC-006), reports the image size, prints the engine table, and renders the UI snapshots.

## 4. Layering

```text
grep -rn "#include <\(esp_\|freertos/\|driver/\|lvgl\)" lib/core
```

Expected: no output (constitution VI).
