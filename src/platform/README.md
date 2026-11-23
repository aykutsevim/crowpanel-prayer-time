# src/platform

Board and SDK code: audio off (first call after reset, SIL-02), panel, touch, backlight (always
fully on through the auxiliary controller; no PWM, owner decision OD1), serial console, storage (NVS, LittleFS), Wi-Fi, AP/DNS/HTTP server, HTTPS client
(certificate verification always on), time, tasks, watchdog.

**Layering rule:** the only place allowed to include board, driver or SDK headers. Pins and timings
are cited from the vendor documentation for the exact board revision (CLAUDE.md rule 4).
