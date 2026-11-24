# Quickstart: validate the main screen (spec 003)

## Prerequisites

Project README requirements, plus Node.js (tested 24.20) for regenerating fonts. Board: CrowPanel Advance 7.0" V1.4 with the CR1220 cell fitted, USB-C on COM port, branch `feat/003-main-screen`.

## 1. Host checks (no board)

```text
python tools/check.py --skip-firmware
pio run -e sim -t exec
python tools/compare_reference.py
```

Expected:
- host tests pass, including the RTC codec, clock, state choice, gesture and fixtures;
- `out/snapshots/` holds fourteen fixture images;
- the geometry check passes;
- `out/compare/` shows each fixture next to its reference.

Review every comparison image before going to the board (SC-001, constitution III).

## 2. Fonts (only when changing them)

```text
python assets/fonts/gen_fonts.py
git diff --stat assets/fonts/out
```

Expected: no diff when nothing changed, and the reported font total is within 300 kB.

## 3. Device

```text
pio run -e crowpanel7_advance -t upload
```

Serial at 115200. These commands are in [contracts/serial-console.md](contracts/serial-console.md):

1. `time get`. A new or battery-less clock shows `not valid (integrity lost)`, and the screen shows `no_time`.
2. `time set 2026-11-24 12:30:00`, using the current local time. The screen shows the real time in the `no_data` state (reference 14), since no prayer times are loaded.
3. Unplug for a few minutes, plug in, and compare the clock with a phone (SC-009).
4. `fixture 1` … `fixture 14`. Each matches its reference (SC-003, SC-005).
5. `fixture 1`, `clock sim 2026-11-17 00:00:00`, `clock speed 3600`. The sample day passes in 24 s (a fixture keeps its sample data while the simulated clock runs): no flicker, no leftover pixels, no moving digits (SC-004). `fixture off` returns to the board clock.
6. Restart. The main screen shows. A two-finger hold of 5 s opens the diagnostic screen; one tap does nothing; `main` returns (SC-006).

The owner checklist with these questions is created in the implementation stage.
