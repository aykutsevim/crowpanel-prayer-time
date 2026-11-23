# 0001: PlatformIO platform and Arduino core

Date: 2026-11-20 (Slice 0). Status: accepted; revisit in Slice 1 if the stress test or the
display stack requires it.

## Decision

`platform = espressif32@8.0.0` (official PlatformIO platform), which ships
**Arduino-ESP32 3.3.12** based on ESP-IDF 5.5.5. Host tests use `platform = native@1.2.1`.

## Evidence (read 2026-11-20)

| Source | Finding |
|---|---|
| Vendor repo `Elecrow-RD/CrowPanel-7.0-HMI-ESP32-Display-800x480`, `example/V3.0/PlatformIO70/platformio.ini` | pioarduino platform `55.03.38` with Arduino core **3.3.8** (`platform_packages`), `lvgl/lvgl@9.1.0`, `lovyan03/LovyanGFX@1.2.25`, custom board JSON (4 MB flash, `qio_opi` memory, `-DBOARD_HAS_PSRAM`), `huge_app.csv`. |
| Vendor wiki, Arduino tutorial for the 7.0" HMI | install Arduino-ESP32 **3.3.8**; LVGL 9.1.0; OPI PSRAM; "Huge APP (3MB No OTA/1MB SPIFFS)". |
| `platformio/platform-espressif32` releases | `v8.0.0` (2026-10-19): Arduino v3.3.12 (IDF 5.5.5). The 7.x line ships Arduino 2.0.17. |
| `pioarduino/platform-espressif32` releases | `55.03.313` (2026-11-17, Arduino 3.3.13); `55.03.312-1` (2026-09-22, Arduino 3.3.12); `61.04.00` (2026-11-19, Arduino 4.0.0, new major). |
| `espressif/arduino-esp32` | latest 2.0.x = 2.0.17 (2024-05-22); latest 3.x = 3.3.13; 4.0.0 released 2026-11-19. |

## Consequences

- **Correction to `docs/specification/05-architecture.md` §3**: the vendor's current examples use the
  3.x core, not 2.x, and the official PlatformIO platform now ships 3.x as well (from 8.0.0).
- pioarduino `55.03.312-1` (the vendor's platform family) was tried first; on this machine with
  PlatformIO Core 6.2.0 it failed while installing its Python dependencies and then reported
  an incompatible Core version. The official 8.0.0 gives the same Arduino core version
  without that problem.
- The vendor example targets 3.3.8; we are on 3.3.12. Slice 1 must confirm that the vendor's
  LovyanGFX `Bus_RGB`/`Panel_RGB` configuration (and its `patch_lovyangfx.py` patch) works on
  3.3.12, or pin `platform_packages` to 3.3.8, and record the result.
- Board definition `boards/crowpanel7_basic.json` follows the vendor's board JSON. Partition
  table `partitions/crowpanel7_basic.csv` follows `05-architecture.md` §5 instead of the
  vendor's `huge_app.csv` (no OTA data, no core dump, larger LittleFS).
