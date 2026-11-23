# Research: Silent Board Bring-up (Slice 1)

Read 2026-11-20. Every hardware fact below comes from vendor sources for the CrowPanel Advance 7.0"
(DIS02170A), cited by path. Nothing is taken from the basic board (DIS08070H) or from memory.

**Source shorthands**

- **REPO**: `https://github.com/Elecrow-RD/CrowPanel-Advance-7-HMI-ESP32-S3-AI-Powered-IPS-Touch-Screen-800x480`, branch `master`, commit `70db9eda4f0d11672508a9461de10ef9d79bd12e` (2026-08-21). All repo paths below are relative to this tree.
- **EX**: `REPO/example/V1.3_and_V1.4_and_V1.5/`
- **SCH14**: `REPO/Eagle_SCH&PCB/version1.4/` (`ESP32 Display 7.0 inch V1.4(1).pdf`, `.sch`, `.brd`). The Eagle `.sch` XML was parsed to extract nets.
- **WIKI**: `https://www.elecrow.com/wiki/ESP32_Display-7.0_inch(Advance_Series)wiki.html`
- **COURSE**: `https://www.elecrow.com/wiki/CrowPanel_Advance_7.0inch_Arduino_Course_V15.html`
- **PRODUCT**: Elecrow product page for DIS02170A (SKU DIS02170A-1)
- **IDFRGB**: `https://docs.espressif.com/projects/esp-idf/en/v5.5/esp32s3/api-reference/peripherals/lcd/rgb_lcd.html`
- **FAQ**: `https://docs.espressif.com/projects/esp-faq/en/latest/software-framework/peripherals/lcd.html`

---

## R1. Board revision V1.4

- **Decision**: treat V1.4 as electrically identical to V1.3, and use the examples in EX and the schematic in SCH14.
- **Evidence**:
  - REPO `readme.md`: "Version 1.4: Only the button component has been updated. All other hardware and I/O pins remain the same as in the 1.3 version."
  - Diffing the V1.3, V1.4 and V1.5 `.sch` netlists shows identical nets. Only the BOOT and RESET key part changed (V1.4), and the touch connector footprint (V1.5).
  - PRODUCT says "The version currently on sale is V1.4" and "Some of the code of version V1.4 is not universal and is still being updated."
- **Not used**: REPO `readme.md` §"Pin definition" lists I²S on 11/12/13 and SPI on 39/41/42. Those are LCD pins on this board, so that section belongs to another board.
- `REPO/Eagle_SCH&PCB/version1.5/readme.md` declares itself AI-generated. It was used only as a pointer, and every fact was re-checked against the schematic or code.

## R2. Module and memory

| Fact | Value | Source |
|---|---|---|
| Module | ESP32-S3-WROOM-1-N16R8: 16 MB flash, 8 MB PSRAM | SCH14 (U5), REPO `readme.md`, PRODUCT. Owner (I2, 2026-11-23): shield reads `ESP32-S3-WROOM-1`, suffix not visible; confirmed by a chip read before the first flash and by the report's `memory` line. |
| PSRAM | octal (OPI), 80 MHz | EX `IDF_Code/Lesson03-LVGL_Control_LED/sdkconfig.defaults` (`CONFIG_SPIRAM_MODE_OCT=y`, `CONFIG_SPIRAM_SPEED_80M=y`), COURSE ("OPI PSRAM") |
| Flash | QIO, 80 MHz, 16 MB | same `sdkconfig.defaults` (`CONFIG_ESPTOOLPY_FLASHMODE_QIO=y`, `FLASHSIZE_16MB`) |
| Serial | UART0 RX GPIO44 / TX GPIO43 through a CH340K USB bridge | WIKI, SCH14 |

**Decision**: a new board definition `boards/crowpanel7_advance.json` with memory type `qio_opi` and 16 MB flash, plus a new partition table (see plan).

**Note on GPIO19/20**: these pins are the S3's native USB pins. On this board they carry the PDM microphone or UART1, depending on the DIP switches. The firmware therefore never enables USB-CDC (`ARDUINO_USB_CDC_ON_BOOT=0`). Serial runs over UART0 through the CH340K.

## R3. The auxiliary controller (STC8H1K28, I²C address 0x30): the centre of silence on this board

**Finding**: on V1.4 the ESP32 has **no direct connection** to the audio amplifier's control pin, the buzzer or the backlight driver. All three hang off U13, an STC8H1K28 microcontroller on the shared I²C bus (SDA GPIO15, SCL GPIO16). Its firmware is not published. It accepts single-byte writes to address 0x30, with no register address.

Nets of U13 in SCH14:

| STC pin | Net | Goes to |
|---|---|---|
| P1.1 | `LED_BK_EN` | MT9201 backlight driver EN (10 kΩ pull-down R25) |
| P3.5 | `LCD_BK_PWR` | Q7 → Q6, switches the backlight boost supply |
| P3.6 | `NS_CTRL` | gate of P-MOSFET Q9 (10 kΩ pull-up to 3V3), which feeds the NS4168 amplifier CTRL pin (100 kΩ pull-down R36) |
| P2.7 | `BEEP` | base of Q8 (SS8050) driving the passive buzzer B2 (47 kΩ pull-down R67) |
| P1.7 | `TP_RST` | GT911 reset (5.1 kΩ pull-down R62) |
| P5.4 | `ESP32_EN` | can hold the ESP32 in reset |

Documented command bytes for V1.3 and later:

| Byte | Meaning | Source |
|---|---|---|
| 0…245 | backlight: 0 brightest, 244 dimmest, 245 off | REPO `readme.md`, WIKI, code comments in EX |
| 246 / 247 | buzzer on / off | REPO `readme.md`, WIKI (not used in any vendor code) |
| 248 | unmute the amplifier ("turn on the speaker") | EX `Arduino/lesson-02`, `lesson-10` |
| 249 | mute the amplifier; the factory firmware sends it at the start of `setup()` ("enable mute") | EX `lesson-10`, `REPO/factory_sourcecode/V1.3_V1.4_V1.5/HMI-bigInch7/HMI-bigInch7.ino` |
| 250 | "Activate touch screen" (touch reset sequence) | EX `lesson-02` to `lesson-05`, IDF example |
| 2, 3 | lesson-10 uses them for the microphone path. They overlap the backlight range. | EX `lesson-10` |

### Silence decisions

1. **Silence function** (`src/platform/audio_off.cpp`, the only file allowed to name audio terms). Its order:
   1. Drive the ESP32 ends of the I²S lines (GPIO4 SDATA, GPIO5 BCLK, GPIO6 LRCLK, routed to the amplifier by U11 channel 0; EX `#define I2S_DOUT 4 / I2S_BCLK 5 / I2S_LRC 6`) as outputs, low.
   2. Start I²C on 15/16.
   3. Send **249** (amplifier mute), then **247** (buzzer off), to 0x30.
   4. Retry while the STC does not acknowledge. The STC may boot later than the ESP32; the vendor code polls for 0x30. Use up to 20 attempts, 50 ms apart.
   5. Result: `confirmed` when both bytes were acknowledged, otherwise `failed` (FR-001a).
2. **What "confirmed" means**: an I²C acknowledge proves the STC received the byte, not that it acted. The STC has no read-back. The effect is proven only by the owner's listening checks. The report must say "acknowledged", not "verified silent".
3. **Command gate**: every byte to 0x30 goes through one function in `audio_off.cpp` that allows only:
   - backlight value 0 (full on) only, because the owner decided against dimming (OD1, 2026-11-20);
   - 247 and 249 (silence);
   - 250 (touch reset).

   It rejects every other byte: **246** (buzzer on), **248** (amplifier on), 1…245 (dimming, backlight off, and the overlapping microphone and audio-path commands 2 and 3) and 251…255. The gate is a pure function, unit-tested on the host (`test/test_audio_off/`).
4. **Hardware default, from the schematic**: Q8's base has a pull-down, so the buzzer is off unless the STC drives P2.7. Q9's gate has a pull-up, so the amplifier CTRL stays low unless the STC pulls P3.6 low.
   - "CTRL low = shutdown" is **confirmed** by the NS4168 datasheet V1.2 (0 to 0.4 V = shutdown; see O3, 2026-11-23).
   - The amplifier's marking is unreadable on the owner's board (I5, 2026-11-23). Its identity as NS4168 rests on the V1.4 schematic alone, which is a vendor source.
   - What the STC does before the ESP32 starts is undocumented. Owner question I7 covers this. Owner (2026-11-23): a buzzer is fitted (I8), and the owner heard no sound at power-on or reset with the factory firmware (I7 = no, corrected the same day from a first "yes"). No sign that the auxiliary controller sounds the buzzer on its own before the ESP32 runs; Stage 1 checks A1 and A2 confirm this with our firmware.
5. **DIP switches K1 (S1/S0)**: they route GPIO4/5/6 to the amplifier (channel 0), the wireless module, or the SD card.
   - Per WIKI: S1/S0 = 0/0 is "MIC&SPK", 0/1 is the wireless module, 1/1 is "MIC&TF Card".
   - Any position other than 0/0 disconnects the ESP32 from the amplifier entirely. The DIP switches do not affect the buzzer.
   - **Recommendation**: the owner sets 1/1 and records it. The firmware stays silent in every position.
   - **Owner's board (2026-11-23)**: DIP switches 1/1 (I3) and no speaker connected (I4). The amplifier therefore has neither an input from the ESP32 nor a load. This is a second layer of protection should the mute command ever fail; it does not replace the silence step.
6. **Banned dependencies**: the Arduino core ships `ESP_I2S` and `ESP_SR` libraries. The device environment lists both under `lib_ignore`, so a stray include fails the build. The silence check already bans the tokens.
7. **Conflict noted**: WIKI names "Amp MUTE P3.7, SHUT P3.6". In the V1.4 netlist P3.7 is unconnected. The schematic wins.

## R4. RGB panel

Pins, identical in EX `Arduino/lesson-03/BigInch_LVGL/LovyanGFX_Driver.h`, the factory source, EX `IDF_Code/Lesson03…/waveshare_rgb_lcd_port.h`, and the SCH14 net names:

| Signal | GPIO |
|---|---|
| B0…B4 | 21, 47, 48, 45, 38 |
| G0…G5 | 9, 10, 11, 12, 13, 14 |
| R0…R4 | 7, 17, 18, 3, 46 |
| DE / VSYNC / HSYNC / PCLK | 42 / 41 / 40 / 39 |

Timing (vendor, 800×480, panel driver SC7277):

- HSYNC front porch 8, pulse 4, back porch 8
- VSYNC front porch 8, pulse 4, back porch 8
- polarities 0
- PCLK sampled on the falling edge (`pclk_active_neg = 1` in the IDF example; `pclk_idle_high = 1` in LovyanGFX)

Vendor pixel clocks differ between examples:

| Example | Pixel clock |
|---|---|
| IDF example, lesson-03, lesson-05 | 16 MHz |
| lesson-04 | 18 MHz |
| ESPHome | 20 MHz |
| factory firmware | 21 MHz |

No vendor text recommends a value.

Refresh rate = PCLK / ((800+8+4+8) × (480+8+4+8)) = PCLK / 410 000:

| PCLK | Refresh |
|---|---|
| 16 MHz | 39.0 Hz |
| 14 MHz | 34.1 Hz |
| 12 MHz | 29.3 Hz |

GPIO3, 45 and 46 are strapping pins used as LCD data. They are only driven by the LCD peripheral after boot, so no action is needed. Keep them in mind when diagnosing boot problems.

## R5. Touch

- **Controller**: GT911, I²C address 0x5D, on SDA 15 / SCL 16 at 400 kHz (EX lesson-03 `LovyanGFX_Driver.h`: `cfg.i2c_addr = 0x5D`).
- **Interrupt**: on **GPIO1** (SCH14 `IO1_TP_INT`).
- **Reset**: via STC P1.7, so not on an ESP32 pin.
- **Vendor start-up sequence** (EX lesson-02…05 `.ino`):

  ```
  Wire.begin(15, 16); delay(50);
  while (1) { if (i2cScanForAddress(0x30) && i2cScanForAddress(0x5D)) break;
    else { sendI2CCommand(250); pinMode(1, OUTPUT); digitalWrite(1, LOW); delay(120); pinMode(1, INPUT); delay(100); } }
  ```

  Holding INT low while reset is released selects address 0x5D.
- **Decision**: implement this sequence with a **bounded** loop (at most 10 attempts, about 2.5 s). On exhaustion, the touch self-test fails with the reason "GT911 not found at 0x5D" and boot continues (FR-005, spec edge case). The vendor's unbounded `while (1)` is not acceptable.
- **Driver decision**: a small GT911 reader written against the GT911 datasheet in `REPO/Datasheet/`. It reads the status register 0x814E and the first point at 0x8150, then clears the status. No third-party touch library is used, for these reasons:
  - LovyanGFX's `Touch_GT911` comes with the display stack rejected in R7.
  - `TAMC_GT911 1.0.2` (2021, used in COURSE) is unmaintained and drives reset and INT itself, which conflicts with the STC-controlled reset.
  - `esp_lcd_touch_gt911 1.2.1` needs the ESP-IDF component manager.

## R6. Backlight

- **Driver**: MT9201 boost constant-current LED driver (U6, datasheet `REPO/Datasheet/C182966_LED_driver_MT9201_WJ220906.PDF`).
  - EN comes from STC P1.1 and the supply is gated by STC P3.5.
  - The datasheet says: "Drive EN with a 20kHz to 1MHz unfiltered PWM dimming signal", and soft start "can eliminate … audio noise when dimming".
- **The ESP32 has no backlight pin.** The vendor IDF example sets `EXAMPLE_PIN_NUM_BK_LIGHT (-1)`.
- **Brightness** is set with command bytes 0…244 to the STC, and 245 turns it off. **The PWM frequency the STC uses is undocumented.**
- **Consequences**:
  - The firmware configures **no PWM at all** (no LEDC). SIL-04's "≥ 20 kHz on the backlight pin" can therefore neither be met nor violated by ESP32 code.
  - Whether the STC's dimming is at or above 20 kHz is unknown.
  - Whether the STC still switches EN at byte 0 (a 100 % duty PWM or a constant level) is also undocumented. The firmware therefore does not claim "no PWM anywhere": the evidence that the backlight is inaudible is owner checks A3 and D1.
- **Decision (owner, OD1 = C, 2026-11-20)**: no dimming, ever. The firmware sends byte 0 (full on) right after the silence step (FR-017) and never any other backlight byte; the command gate rejects 1…245 (FR-018). There is no backlight control on screen or on the serial console.
- **Consequences for later slices**: SET-04 (brightness, night dimming) is dropped; the owner's open question 4 in `01-product-brief.md` is answered "no".

## R7. Display stack

**Decision**: Espressif's own `esp_lcd` RGB panel driver, called directly from the Arduino build, with LVGL 9.6.0 on top.

**Rationale**:

- The vendor's own ESP-IDF example for this board (EX `IDF_Code/Lesson03…`, IDF 5.5.4) uses `esp_lcd` with exactly the pins and timing above. Our build is Arduino 3.3.12 on IDF **5.5.5**, so this is the closest vendor-verified configuration to what we run.
- The `esp_lcd` RGB API is present in the precompiled Arduino libraries: header `esp_lcd_panel_rgb.h`, library `libesp_lcd.a`. That includes `bounce_buffer_size_px`, `num_fbs`, `esp_lcd_rgb_panel_set_pclk` and `esp_lcd_rgb_panel_restart`, so every mitigation step in R8 is available without changing the stack.
- No third-party display library to pin.

**Alternatives considered**:

| Option | Status | Why not first |
|---|---|---|
| LovyanGFX `Bus_RGB` (the vendor's Arduino examples, LovyanGFX 1.2.25 on Arduino 3.3.8) | Latest LovyanGFX 1.3.0 (2026-11-06) | It drives LCD_CAM and GDMA through its own register code and has **no bounce-buffer support**. Issue lovyan03/LovyanGFX#841 (2026-05, Elecrow 7" board, "No free interrupt inputs for LCD_CAM" and a blank screen on cores newer than 3.2.1) is closed without a clear fix. Matching the vendor exactly would mean pinning Arduino 3.3.8 and dropping the platform decision of Slice 0. Kept as the last fallback. |
| `ESP32_Display_Panel` 1.0.4 | | Has no preset for the Advance board. Not in the PlatformIO registry. Its dependency range excludes the current esp-lib-utils 0.3.0. |
| `esp_lvgl_port` 2.9.0, `esp_lcd_touch_gt911` 1.2.1 | | Need `framework = arduino, espidf`. Considered only if mitigation step 5 is reached. |

## R8. Picture stability under Wi-Fi, TLS and flash writes

**Finding** (from the precompiled `framework-arduinoespressif32-esp32s3-libs` 3.3.12, `qio_opi/include/sdkconfig.h`):

| Setting | Value |
|---|---|
| `CONFIG_SPIRAM_XIP_FROM_PSRAM` | off |
| `CONFIG_ESP32S3_DATA_CACHE_LINE_32B` | `y` |
| `CONFIG_SPIRAM_SPEED_80M` | `y` |
| `CONFIG_LCD_RGB_RESTART_IN_VSYNC` | `y` |
| `CONFIG_LCD_RGB_ISR_IRAM_SAFE` | off |
| `CONFIG_SPIRAM_TRY_ALLOCATE_WIFI_LWIP` | `y` |

IDFRGB says the following about the precompiled settings above:

- In bounce-buffer mode "the LCD CANNOT function if the external memory cache is disabled, such as during OTA or NVS writes".
- It recommends `CONFIG_SPIRAM_XIP_FROM_PSRAM`, which **cannot be changed in an Arduino-only build** on `espressif32@8.0.0`. Changing it needs `framework = arduino, espidf`, or pioarduino's `custom_sdkconfig`.

FAQ adds two points:

- A 32-byte data cache line in bounce mode "may cause the screen to drift".
- The PCLK ceiling with octal PSRAM at 80 MHz is about 22 MHz.

**Root causes and symptoms**:

- **PSRAM and SPI0 bandwidth contention**: horizontal drift.
- **Cache disabled during a flash erase or write**: a shift or stall.
- **Late GDMA or bounce refill**: a lasting shift until DMA restarts. `RESTART_IN_VSYNC` limits it to one frame.
- **Single-buffer writes during scan-out**: tearing. That is cosmetic and does not matter for a static screen.

**Decision: starting configuration (L0)**:

- `esp_lcd` RGB, vendor timing, **PCLK 16 MHz**, `pclk_active_neg = 1`
- **one** frame buffer in PSRAM (`fb_in_psram`, `num_fbs = 1`), **no bounce buffer**: EDMA reads PSRAM directly, so a cache-off window does not stop the scan-out
- `CONFIG_LCD_RGB_RESTART_IN_VSYNC` is already on
- LVGL in PARTIAL mode with one draw buffer of 800 × 40 lines × 2 B = 64 000 B in **internal** RAM. The flush callback copies the area into the PSRAM frame buffer. Little PSRAM traffic, because the diagnostic screen is mostly static.
- LVGL and the panel initialised on the same core (core 1, the Arduino loop). FAQ: "can significantly raise the PCLK limit".
- Wi-Fi, TLS and flash work run in a separate task on core 0.

**Mitigation ladder**, one change at a time. After each step the owner answers checklist questions B4 and B5 again, and the outcome is logged in the "Stack decision log" below.

| Step | Change | Fixes | Cost |
|---|---|---|---|
| L1 | PCLK 14 MHz, then 12 MHz | bandwidth starvation (FAQ, IDFRGB) | refresh 34 Hz / 29 Hz |
| L2 | Around each storage write, lower PCLK with `esp_lcd_rgb_panel_set_pclk` (for example to 8 MHz) and restore it afterwards | drift during writes (FAQ) | a possible single blank frame during writes |
| L3 | Bounce buffer of 10 × 800 px (2 × 16 000 B internal DMA RAM), as in the vendor IDF example | contention during Wi-Fi traffic | internal RAM, ISR CPU. Without XIP it makes flash writes worse, so it is used only together with L2. |
| L4 | Switch the build to `framework = arduino, espidf` with `CONFIG_SPIRAM_XIP_FROM_PSRAM=y`, `CONFIG_ESP32S3_DATA_CACHE_LINE_64B=y` and bounce buffers. This is Espressif's documented set for "Wi-Fi, BLE and continuous flash writes" (FAQ). | flash-write stalls | longer builds, a second framework package to pin, PSRAM used for code |
| L5 | LovyanGFX 1.2.25 on a pinned Arduino 3.3.8, the vendor's exact Arduino combination | last resort | undoes the Slice 0 platform decision |

**Rejected**:

- PSRAM at 120 MHz: experimental, temperature-sensitive, and unsuitable for an always-on device (IDF `flash_psram_config`, FAQ).
- `num_fbs = 2`: fixes tearing only, not drift, and costs another 768 kB of PSRAM.

**Test pattern rationale**:

- One-pixel alternating columns: any horizontal shift by an odd number of pixels swaps the phase of the whole field. A byte slip shows as a colour tint instead of neutral grey.
- One-pixel border on all edges: a shift pushes one edge off the screen and wraps it onto the opposite side.
- Corner colours and a heartbeat: distinguish a frozen frame from a shifted one.
- Log lines with timestamps for each stress activity: let the owner's observations be matched to activity.

## R9. Libraries and versions (all exact)

| Item | Pin | Source and date |
|---|---|---|
| Platform | `espressif32@8.0.0` (2026-10-19) | Slice 0 decision 0001 |
| Arduino core package | `platformio/framework-arduinoespressif32@4.30312.261116` = Arduino-ESP32 3.3.12. The platform only requires `~4.30312.0`, and several builds exist, so it is pinned via `platform_packages`. | installed package, PlatformIO registry |
| S3 precompiled libraries | `platformio/framework-arduinoespressif32-esp32s3-libs@3.3.12` | installed package |
| LVGL | `lvgl/lvgl@9.6.0` (2026-09-16), the latest release and the final v9 | github.com/lvgl/lvgl/releases/tag/v9.6.0 |
| Host test platform | `native@1.2.1` | Slice 0 |

No other third-party libraries are used.

Built-ins from the core (3.3.12):

- `Wire`
- `WiFi`: `scanNetworks(async=true)` with `scanComplete` / `scanDelete`; `softAP(ssid, pass, channel, hidden, max_connection=2)`
- `WebServer`
- `NetworkClientSecure` with `useBuiltinCACertBundle()`
- `HTTPClient`
- `Preferences` (NVS)
- `LittleFS`
- `esp_random()`

LVGL 9.6 notes that apply:

- configure with `LV_CONF_INCLUDE_SIMPLE` and `include/lv_conf.h`
- `LV_COLOR_FORMAT_DEFAULT = LV_COLOR_FORMAT_RGB565` instead of `LV_COLOR_DEPTH`
- `LV_MEM_SIZE` is given in bytes
- include only `lvgl.h`
- always pass the display explicitly to `lv_display_*`
- tick via `lv_tick_set_cb(millis-wrapper)`

## R10. Encrypted request (stress mode)

- **Decision**: `GET https://ezanvakti.imsakiyem.com/api/v2/locations/countries/2`.
  - The host is in the allowed list (`04-data-sources.md` §11, SEC-02).
  - It is a single small record (Türkiye), so the request is polite. It is at most one every 30 s (clarification 2026-11-20).
  - `User-Agent: NamazVaktiPanel/<version>` (SEC-03).
- **TLS**:
  - Use `NetworkClientSecure` + `useBuiltinCACertBundle()` and `HTTPClient::begin(client, url)`. The bundle is compiled into the 3.3.12 libraries: `CONFIG_MBEDTLS_CERTIFICATE_BUNDLE`, `DEFAULT_FULL`, 200 certificates.
  - Verification is `MBEDTLS_SSL_VERIFY_REQUIRED`.
  - **Never** call `HTTPClient::begin(String url)` for https: in 3.3.12 it calls `setInsecure()` when no CA is set (SEC-01). Tasks add a source check that forbids both `setInsecure` and the one-argument `begin("https…`.
- A success is any HTTP status 200 with a body of 4 kB or less. Every other outcome is an error with a short reason (TLS failure, timeout, HTTP status).

## R11. Wi-Fi setup for the test (owner decision 2026-11-20)

- **Access point**: name `NamazVakti-XXXX` (last four hex digits of the station MAC, upper case), WPA2-PSK, at most two clients, address 192.168.4.1 (NET-02).
  - **Password**: eight decimal digits from `esp_random()`, generated on first use and stored in NVS (namespace `ap`, key `pass`). It is kept until factory reset and shown only on the device screen (FR-020, FR-024).
  - **Mode**: AP+STA, so scanning and connecting work while the AP is up.
- **Setup page**:
  - Served by the core `WebServer` on port 80.
  - Paths are a subset of NET-04, so that Slice 4 extends them instead of replacing them: `GET /`, `GET /api/scan`, `POST /api/setup`, `GET /api/status`.
  - No captive-portal DNS. The screen shows the address `http://192.168.4.1/` (Slice 4 adds the DNS responder).
- **Stored credentials**: NVS namespace `wifi`, keys `ssid` and `pass`. Slice 4 reads the same keys (FR-025).
  - A connection attempt lasts up to 20 s (MODE-04).
  - The password is write-only (NET-08). `/api/status` and the report show only the SSID.
- **Reconnect**: exponential backoff from 5 s to 5 min (NET-01).

## R12. Host rendering of the diagnostic screens (FR-008)

- **Decision**: a native PlatformIO environment `sim` that builds `lib/ui` and LVGL 9.6.0 with LVGL's test module (`LV_USE_TEST=1`, `LV_USE_TEST_SCREENSHOT_COMPARE=1`, `LV_USE_LODEPNG=1`). It creates an 800×480 in-memory display with `lv_test_display_create`, renders each diagnostic view model, and writes PNG files to `out/snapshots/`.
  - Source: `lvgl/docs/src/debugging/test.mdx` at v9.6.0.
  - It uses the LodePNG copy bundled in LVGL, so nothing else is needed.
- **Alternative**: a custom flush callback plus `stb_image_write.h`. Not needed.

## Open items and owner questions

**Owner decision (answered)**

- **OD1. Backlight dimming.** The dimming frequency of the auxiliary controller is undocumented, so SIL-04 cannot be confirmed. Answer 2026-11-20: **C, no dimming at all**; the backlight is always fully on (R6).

**Owner inspection before flashing (Stage 1)**

- **I1.** Confirm the PCB silkscreen says V1.4.
- **I2.** What is the full marking on the ESP32 module (expected ESP32-S3-WROOM-1-N16R8)?
- **I3.** What are the current positions of DIP switches S1 and S0? Recommended: 1/1 ("MIC&TF Card"), which disconnects the amplifier from the ESP32.
- **I4.** Is a speaker plugged into the 2-pin speaker connector? It is recommended to unplug it for good. It is still needed for the silence check: the owner listens with it connected once, then it may stay unplugged.
- **I5.** What is printed on the 8-pin chip next to the speaker connector (NS4168 expected)?
- **I6.** What is printed on the 32-pin chip (STC8H1K28 expected)?
- **I7.** With the factory firmware still on the board: any click, beep or tone at power-on or reset? (yes/no) This tells us whether the auxiliary controller makes sound before our firmware runs.
- **I8.** Is a buzzer (round black part, about 5 × 5 mm) fitted? (yes/no)

**For the implementer**

- **O3. Resolved 2026-11-23.** NS4168 datasheet, Shenzhen Nsiway Technology, V1.2, Apr. 2023 (LCSC C910588, `https://atta.szlcsc.com/upload/public/pdf/source/20240722/6A2F478F91F6E02E759B805C93276334.pdf`), pin table p. 2, electrical characteristics p. 4, §10.2.4 table 3 p. 8. Pin 1 CTRL is "左右通道切换控制和芯片关断控制脚" (channel select and chip shutdown). CTRL voltage: **0 to 0.4 V = shutdown (芯片关断)**, 0.9 to 1.15 V = left channel, 1.5 V to VDD = right channel. Shutdown leakage ISD = 1 µA at VCTRL = 0 V; shutdown time TOFF = 100 µs. With the V1.4 schematic (R3.4: CTRL has the 100 kΩ pull-down R36 and is fed from 3V3 only while Q9 conducts), CTRL sits at 0 V, i.e. **shutdown**, unless the STC pulls P3.6 low. "CTRL low = shutdown" is therefore confirmed. The part's marking is unreadable on the owner's board (I5), so its identity comes from the V1.4 schematic.
- **O4.** Record in the stack decision log below whether the `esp_lcd` RGB panel starts cleanly on 3.3.12 with the vendor timing.

## Stack decision log

| Date | Configuration | Idle stable? | Stress stable? | Evidence |
|---|---|---|---|---|
| | L0: esp_lcd, 16 MHz, no bounce buffer, PARTIAL 800×40 internal | | | owner checklist B4/B5 |
