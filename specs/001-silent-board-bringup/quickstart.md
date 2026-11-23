# Quickstart: validating the Silent Board Bring-up

Commands run from the repository root. Environment names come from the [plan](plan.md).

## Prerequisites

- PlatformIO Core, plus a host C/C++ compiler for `native` and `sim` (as in Slice 0)
- The board, a USB-C data cable, a serial monitor, a phone, and a 2.4 GHz home network
- The owner has answered I1–I8 in [owner-checklist.md](owner-checklist.md) and set the DIP switches

## 1. Checks on the development machine (every stage)

```
python tools/check.py
```

It runs, in order:

- the tool self-tests
- the silence check (SIL-05)
- the TLS source check (SEC-01)
- `pio test -e native`, which covers the command gate, parser, counters, touch judge, report and JSON
- `pio run -e crowpanel7_advance` with the size report

**Expected**: all steps pass, and the size line reads `Firmware size: … of the … (2.9 MiB) budget`, followed by the size of the diagnostic mode.

Snapshots of the engineering screens (from Stage 2):

```
pio run -e sim -t exec
```

This writes `out/snapshots/{boot,diag_idle,diag_failures,diag_stress,diag_touch,settings_none,settings_confirm,settings_hotspot}.png`. The implementer reviews them against [contracts/diag-screens.md](contracts/diag-screens.md) before asking the owner to look at the panel.

## 2. Stage 1: silence and report (no display)

**Step 0, before the first flash** (nothing is written to the board):

```
pio pkg exec -p tool-esptool -- esptool --chip esp32s3 flash-id
pio pkg exec -p tool-esptool -- esptool --chip esp32s3 read-flash 0 0x1000000 backup/factory-DIS02170A-V1.4.bin
```

- `flash-id` answers I2: look for `Detected flash size: 16MB`, and for `Embedded PSRAM 8MB` in the `Features:` line printed on connect. esptool finds the port by itself; add `--port COMx` if more than one is connected.
- `read_flash` saves the factory firmware (16 MB, a few minutes) so it can be restored later with `write-flash 0 <file>`. `backup/` is git-ignored: the image may contain Wi-Fi details stored by the factory firmware, and it must never be committed (SEC-04).
- With the factory firmware still running, answer I7a and I7b: power on, press reset, listen, and note when the sound starts.
- Command names checked on 2026-11-23 against the installed esptool v5.4.0 (PlatformIO package `tool-esptool` 2.50400.0), which uses `flash-id`, `read-flash` and `write-flash`.

**Then flash Stage 1:**

```
pio run -e crowpanel7_advance -t upload
pio device monitor -b 115200
```

Expected serial output after power-on. Numbers vary; keys and order do not.

```
audio-off: acknowledged first (attempts 1)
=== NVP REPORT ===
firmware: 0.1.0
board: DIS02170A V1.4 (configured; not detected)
memory: flash 16 MB, psram 8 MB (detected)
audio-off: acknowledged first (attempts 1)
selftest silence: passed
selftest display: skipped (not in this stage)
selftest touch: skipped (not in this stage)
selftest backlight: passed
selftest storage: passed
selftest wifi: skipped (not in this stage)
...
=== END ===
```

The screen may light up white or stay dark in this stage; both are expected. The owner answers A1, A2 and F1.

If `audio-off` reads `failed`:

- The rest of the report still prints.
- `stress on` and `wifi setup` answer `refused: silence step not acknowledged`.
- Paste the output.

## 3. Stage 2: display and touch

After flashing:

1. Boot screen, then the diagnostic screen within 5 s, with the backlight fully on. The serial log shows `diag screen shown`, and the report line `boot-to-diag` must be at most 5000 ms (SC-006). The owner answers B1–B4, D1 and A3.
2. Touch the four corner targets. The owner answers C1, C2 and A6. The serial `report` shows the corner results.

## 4. Stage 3: Wi-Fi setup and stress

1. **Wi-Fi setup**:
   1. On the device, open `Settings`, choose `Start Wi-Fi setup` and confirm. The screen shows the hotspot name, password and address.
   2. On the phone, join the hotspot and open `http://192.168.4.1/`. Pick the network and enter its password.
   3. The device screen shows `Connected to <ssid>`.
   4. Press reset and wait for `Connected` again. The owner answers E1, E2 and A5.
   5. Try `Forget Wi-Fi`, then set it up again. The owner answers E3.
2. **Stress**:
   1. Tap `Stress on`, or type `stress on`. Watch for ten minutes.
   2. Type `stress off` and paste the report.
   3. Expected: `https` successes about every 30 s, and a cycle count around 60 after 10 minutes (one cycle about every 10 s). The pass bar of SC-003 is lower: at least 10 cycles in 10 minutes.
   4. The owner answers B5, A4 and F2.
3. If B5 is "no", apply the mitigation ladder in [research.md](research.md) R8 one step at a time. After each step, repeat B4 and B5 and add a row to the stack decision log.

## 5. Done when

- Every checklist row is answered with yes (F2: no).
- The stack decision log has its final row.
- `python tools/check.py` passes.
- The FR-033 document updates are committed.
