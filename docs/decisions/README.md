# Decisions

Technical decisions that shape more than one slice, with the evidence they rest on. One file per decision, numbered in order. A decision is changed by adding a new file that supersedes it, not by rewriting history.

| # | Decision | Status |
|---|---|---|
| [0001](0001-platform.md) | PlatformIO platform `espressif32@8.0.0` with Arduino-ESP32 3.3.12 | accepted (Slice 0); framework superseded by 0002 |
| [0002](0002-display-stack.md) | ESP-IDF 5.5.5 with `esp_lcd` RGB panel (bounce buffer) and LVGL 9.6.0 | accepted (Slice 1) |

Owner decisions about the product (for example no backlight dimming, OD1) are recorded in the spec of the slice that made them, under "Owner decisions".
