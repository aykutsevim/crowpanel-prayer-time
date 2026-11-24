# 0002: Display stack and framework: ESP-IDF 5.5.5 with esp_lcd and LVGL 9.6.0

Date: 2026-11-24 (Slice 1, Stage 3). Status: accepted. Supersedes the framework part of
[0001](0001-platform.md); the platform pin `espressif32@8.0.0` stays.

## Decision

- **Framework:** plain **ESP-IDF 5.5.5** (`framework = espidf`, `platformio/framework-espidf@4.50505.0`
  pinned exactly). Arduino-ESP32 3.3.12 is no longer used.
- **Display:** `esp_lcd` RGB panel, PCLK **14 MHz**, timing 8/4/8, one frame buffer in PSRAM,
  **bounce buffer 10 × 800 px** in internal RAM, `dma_burst_size` 64. LVGL **9.6.0**, PARTIAL mode,
  800 × 40 draw buffer in internal RAM, on core 1. Network work on core 0.
- **Build options** (`sdkconfig.defaults`) the display depends on:
  `CONFIG_SPIRAM_XIP_FROM_PSRAM=y`, `CONFIG_LCD_RGB_ISR_IRAM_SAFE=y`,
  `CONFIG_LCD_RGB_RESTART_IN_VSYNC=y`, `CONFIG_ESP32S3_DATA_CACHE_LINE_64B=y`.
- **Managed components:** `joltwallet/littlefs ==1.22.3` only (the version Arduino 3.3.12 used, so
  the existing LittleFS partition stays readable).

## Why

The SC7277 panel drifted, garbled or flickered under Wi-Fi load with every configuration the
precompiled Arduino libraries allow. The full ladder is in
`specs/001-silent-board-bringup/research.md`, "Stack decision log". In short:

| Step | Change | Under stress |
|---|---|---|
| L0 | vendor 16 MHz, no bounce buffer | drift even at idle |
| L1 | 14 MHz (12 MHz rejected by the panel: datasheet DCLK 23–27 MHz) | drift with Wi-Fi |
| L3, L3a, L3b | bounce buffer 10 and 20 lines, cache invalidate | flicker or garbling around flash writes and scans |
| L3c | no flash write while a scan runs | still garbled for 20–45 s at a time |
| **L4 spike** | ESP-IDF with the four options above | **steady** for 10 minutes of scan plus write at the same moment, and HTTPS (owner, 2026-11-24) |

The cause is a flash operation suspending the cache while the bounce-buffer refill interrupt is
not IRAM-safe and code runs from flash. Only build options fix that, and the Arduino libraries are
precompiled with them off. Building Arduino as an ESP-IDF component would have pulled in
`espressif/esp-sr` (audio) and cloud telemetry components through its manifest (research R8, L4
findings), against Principle I and SEC-05; plain ESP-IDF avoids that.

## Consequences

- Only `src/platform` and `src/main.cpp` changed. `lib/core`, `lib/ui`, `src/app` and the host
  tests are unchanged apart from a new form decoder (`nvp::diag::form_value`) that the Arduino
  web server used to provide.
- Replacements: Wire → `i2c_master`, Serial → UART driver, Preferences → `nvs` (same namespaces and
  keys), LittleFS → `esp_littlefs`, WiFi → `esp_wifi`/`esp_netif`, WebServer → `esp_http_server`,
  HTTPClient → `esp_http_client` with `esp_crt_bundle_attach`.
- Silence: every device build now also checks the linked image for I2S, DAC, LEDC and codec
  symbols (`tools/check_elf_silence.py`), because the framework is no longer filtered through
  `lib_ignore`.
- TLS: `tools/check_tls.py` also rejects `skip_cert_common_name_check`, an `esp_http_client`
  configuration without a CA, and insecure ESP-TLS options in `sdkconfig*`.
- Rule for later slices: no flash write (NVS, cache files) while a Wi-Fi scan is in progress.
  Not required by the L4 result, but it costs nothing and keeps a margin.
- The first clean ESP-IDF build of the framework takes about 10 minutes; later builds are quick.
- `spikes/l4-idf/` holds the throwaway spike firmware as evidence.
