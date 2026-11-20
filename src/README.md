# src

Firmware application code for the ESP32-S3. `main.cpp` is the entry point.

- `platform/`: board bring-up (audio off first, panel, touch, backlight), storage, Wi-Fi,
  access point + DNS + HTTP server, HTTPS client, time, tasks, watchdog.
- `app/`: mode state machine (BOOT / SETUP / CONNECTING / RUN), wiring of platform, core and
  ui, development serial commands.

**Layering rule:** only `src/platform` may include board, driver or SDK headers. The audio-off
function (`src/platform/audio_off.*`, SIL-02) is the only code allowed to touch audio pins and
is the first call after reset.
