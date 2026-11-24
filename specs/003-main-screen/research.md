# Research: Main screen with sample data (spec 003)

Date: 2026-11-24. Sources: `docs/specification/02-ui-spec.md`, `docs/design/` (renders 01–14, `reference.html`, `tokens.json`), `05-architecture.md` §4, §5, §7, `03-functional-spec.md` §2 and §4, spec 001 research R3/R5 and decision 0002, spec 002 (engine), the V1.4 schematic, the NXP PCF8563 data sheet, the LVGL 9.6.0 sources, the npm registry and GitHub.

## R1. Real-time clock: hardware (OD3-1)

**Facts**:
- **Part**: U4 on the V1.4 board is a **BM8563** (BM8563EMA), with **PCF8563MDTR** named as its alternative in the parts list (`Eagle_SCH&PCB/version1.4`, parts list and netlist).
- **Wiring**: SDA on net `IO15_SDA_P2_4`, SCL on `IO16_SCL_P2_5` (the shared bus of the auxiliary controller 0x30 and the GT911 0x5D). Crystal Y1 on OSCI/OSCO. VDD through Q5 from 3V3 or battery BT1 (CR1220). The INT pin is not connected.
- **Vendor usage**: the factory firmware (`HMI-bigInch7.ino`) uses the `I2C_BM8563` library at `I2C_BM8563_DEFAULT_ADDRESS` = 0x51 on SDA 15 / SCL 16, and sets the clock from NTP.
- **Register facts** (NXP PCF8563 product data sheet, Rev. 11.1, 19 January 2026; the BM8563 is register-compatible):
  - **Address**: write A2h / read A3h, so the 7-bit address is 0x51 (p. 1).
  - **Registers 02h–08h**: VL_seconds, Minutes, Hours, Days, Weekdays, Century_months, Years, all BCD (Table 4, §8.4). Masks: seconds 0x7F with VL in bit 7; minutes 0x7F; hours 0x3F; days 0x3F; weekday 0x07; months 0x1F with C in bit 7; years 0xFF.
  - **VL bit** (02h bit 7), Table 8: "0 – Clock integrity is guaranteed. 1 – The integrity of the clock information is not guaranteed." Its start-up value is 1. §8.4.1.1: it is set when VDD drops below Vlow or the oscillator stops, "It indicates that the time can be corrupted", and "can only be cleared by using the interface".
  - **Access** (§8.5): "During read/write operations, the time counting circuits (memory locations 02h through 08h) are blocked … all accesses must be completed within 1 second … setting or reading seconds through to years must be made in one single access." Register addresses auto-increment (p. 1).
  - **STOP bit** (00h bit 5): 0 = the clock runs (default) (Table 5).
  - **Century bit C** (07h bit 7): toggles when the years register overflows from 99 to 00, and the user may re-assign it (Table 15).
  - **Reset**: the chip resets while the oscillator is stopped; all registers take the Table 27 values (§8.11).
  - **Bus speed**: 400 kHz maximum; the project bus runs at 100 kHz.

**Decision**: one I²C write-then-read (register 02h, 7 bytes) per read and one write (02h + 7 bytes) per set, on the existing `platform::i2c` bus.
- **Encoding**: the clock holds **UTC** (FR-027), with C = 0 for 2000–2099. A set writes C = 0 and VL = 0; this follows the vendor library's convention (C = 0 means 20xx).
- **Validity** (FR-026): the read is accepted only if the chip acknowledges, VL = 0, C = 0, every BCD digit is ≤ 9, and every field is in range. The weekday register is ignored; it is derived from the date. The time must also be ≥ the firmware build time (TIME-03).
- **Running time**: read at start, then run on the system timer (TIME-05). STOP and the control registers are not written.

## R2. Time zone and build time

**Decision**:
- **Zone**: local = UTC + 180 minutes, a fixed offset for now. This is the default zone `<+03>-3` of TIME-04; Türkiye has no daylight saving. Parsing a general POSIX TZ string belongs to Slice 5. The offset lives in one constant, `nvp::time::kDefaultUtcOffsetMinutes`.
- **Build time**: a PlatformIO pre-script (`tools/pio_build_time.py`) adds `-DNVP_BUILD_UTC=<seconds>` from the host clock in UTC, for the device build only. The host test builds use fixed values.

## R3. Fonts (FR-003)

**Tool**: `lv_font_conv` **1.5.3**, the latest on npm (registry `dist-tags.latest`), run as `npx --yes lv_font_conv@1.5.3`. Node 24.20 and npm 11.19 are installed on the development machine.
- **Evidence it works with 9.6**: LVGL 9.6.0 generates its own built-in fonts with this tool. The header of `src/font/lv_font_montserrat_14.c` records the options: `--no-compress --no-prefilter --bpp 4 … --format lvgl --force-fast-kern-format`.
- **Options**: we use the same ones, so no LVGL configuration changes. `LV_USE_FONT_COMPRESSED` stays 0 (LVGL 9.6 `lv_conf_template.h`). The LVGL documentation pages could not be fetched (301 to a 404, 2026-11-24); the shipped sources are the evidence.

**Sources**: `google/fonts` `ofl/barlow` (`Barlow-Regular.ttf`, `Barlow-Medium.ttf`, `Barlow-SemiBold.ttf`) and `ofl/barlowcondensed` (`BarlowCondensed-SemiBold.ttf`), static TTFs with `OFL.txt`. Their `METADATA.pb` points at upstream `jpt/barlow` commit `b4726dd` (upstream tag v1.408). The latest upstream tag is 1.422 (2019-07-10).
- **Pin**: the google/fonts files at the commit current on the day of download. The download script records that commit hash in `assets/fonts/SOURCES.md`. The TTFs and `OFL.txt` are committed under `assets/fonts/src/`.

**Font set** (02 §2.2, with the allowed consolidations: hour labels use the 14 px font; the current-column note uses Regular 15):

| Name | Family / weight | px | Glyphs |
|---|---|---|---|
| `nvp_num_114` | Barlow Condensed 600 | 114 | `0-9 : -` |
| `nvp_num_96` | Barlow Condensed 600 | 96 | `0-9 : -` |
| `nvp_num_48` | Barlow Condensed 600 | 48 | `0-9 : -` |
| `nvp_num_46` | Barlow Condensed 600 | 46 | `0-9 :` |
| `nvp_num_44` | Barlow Condensed 600 | 44 | `0-9 :` |
| `nvp_num_25` | Barlow Condensed 600 | 25 | `0-9 : . space` |
| `nvp_txt_600_26` | Barlow 600 | 26 | Turkish set |
| `nvp_txt_600_18` | Barlow 600 | 18 | Turkish set |
| `nvp_txt_500_21` | Barlow 500 | 21 | Turkish set + `≈` (U+2248) |
| `nvp_txt_500_19` | Barlow 500 | 19 | Turkish set |
| `nvp_txt_400_20` | Barlow 400 | 20 | Turkish set |
| `nvp_txt_400_17` | Barlow 400 | 17 | Turkish set |
| `nvp_txt_400_15` | Barlow 400 | 15 | Turkish set |
| `nvp_txt_400_14` | Barlow 400 | 14 | Turkish set |

The Turkish set is space, 0–9, A–Z, a–z, `ÇĞİÖŞÜçğıöşü`, `. , : ; - ' ( ) / % ! ? +`, U+2019 and U+2026 (02 §2.2). The 38 px setup font and the overlay fonts belong to later slices.

**Budget**: 300 kB for all fonts (05 §5), 4 bpp, no compression. Estimate before conversion: number fonts about 75 kB, text fonts about 110 kB. `tools/size_by_module.py` gains a `fonts` bucket so the real figure is reported by every build (FR-025).

## R4. Fixed-cell numbers (FR-004)

**Decision**: a small widget, `ui::mono_text`, draws each character centred in its own cell in its draw event. Digit cells are `round(0.475 × px)` and colon cells `round(0.252 × px)`. Other characters (`-`, `.`, space) take the digit cell. The widget's width is the sum of the cells, so the text never moves when digits change. It is used for the clock (96), seconds (46), countdown (114), period bounds (25) and column times (48 / 44). One object per value, not one label per character.

**Alternatives considered**:
- One `lv_label` per character: about 50 objects for the clock, countdown and columns. More memory, the same result.
- A monospaced font: Barlow has none, and the fixed advance would not centre the glyphs.

## R5. Timeline drawing (FR-005 to FR-008, FR-015)

**Decision**: custom draw events on plain LVGL objects. There is no canvas.

| Object | Area | Draws | Redrawn |
|---|---|---|---|
| `scale` | y 236–262 | hour labels, ticks | never (static) |
| `strip` | x 24–776, y 264–294 | 7 segments, 6 separators, kerahat hatch | at a vakit, a day change or a settings change |
| `leaders` | y 294–336 | 6 polylines | at a vakit change (current leader) |
| `marker` | 14 × 48 px, moved | gap, line, triangle | when the minute changes (moves) |

- **Segments**: rectangles; the first and last are rounded on their outer side (radius 7). Separators are 2 px `bg` rectangles centred on X(t).
- **Hatch**: 45° `bg` lines 3 px wide, 6 px apart, drawn with the clip area limited to the interval and the strip.
- **Marker**: drawn above everything. Moving it invalidates only its old and new 14 × 48 px boxes.

**Rationale**: a canvas for the band would cost 800 × 100 × 2 = 160 kB of RAM and still need redrawing. Draw events use LVGL's partial rendering and the existing 800 × 40 internal draw buffer, so only invalidated areas are redrawn (UI-03).

## R6. Icons (M1, M4)

**Decision**: the four icons are drawn with LVGL primitives in a 16 × 16 box. The paths come from the `<defs>` block of `reference.html`.
- **Pin**: the teardrop as an arc (r = 4.4) and two lines to the tip, plus a circle (r = 1.3).
- **Wi-Fi**: three arcs (r ≈ 9.2/2, 5.9/2, 2.7/2 chords) and a dot.
- **Wi-Fi off**: Wi-Fi plus the diagonal line.
- **Approx**: two wave lines, each approximated by short line segments.
- **Style**: stroke 1.6 px, round caps, in the colour of the status text.

**Rationale**: 02 §1 allows no bitmaps except the QR code. LVGL's vector API needs ThorVG, which is too large for four 16-px icons. The result is checked against the renders at 2× zoom in the review step.

## R7. View model and states (FR-012 to FR-014)

**Decision**: the main screen takes `const engine::MainView&` (spec 002) plus `const ui::SystemView&`. `SystemView` holds:
- the chosen state,
- the city,
- the last-update text,
- the stored-days count,
- whether the clock is valid,
- whether today's times are calculated.

Choosing the state is a pure function in `lib/core`, `nvp::screen::select_state()`, with the order of 02 §6, tested on the host.

The status strings of 02 §6 go into `nvp/text/tr.h` with the engine's strings. "No time" with stored times uses a new engine entry point, `nvp::engine::evaluate_no_time(record, settings, MainView&)`. It fills the strip, leaders and columns (`later`, empty notes) without a clock, and is tested like the engine.

## R8. Redraw bounding (FR-015, FR-016)

**Decision**: the screen keeps the last `MainView`/`SystemView` it drew and compares them field by field. Each tick it updates only changed objects:
- seconds: clock and countdown digits, progress fill;
- minute: also the marker and notes;
- vakit: also column states, leaders, hero texts;
- day: everything.

**Measurement**: the host harness counts the pixels LVGL flushes per tick. Tests assert that a plain second redraws less than 8 % of the screen and that a period change redraws everything that changed. On the device, a development build logs the longest `lv_timer_handler` time per minute. The owner checks for tearing or flicker during a fast-forward day (SC-004).

## R9. Host harness and review (FR-020, FR-021)

**Decision**: extend `sim/diag_render` (Slice 1) into `sim/ui_render`. It renders the diagnostic screens as before, plus fixtures 01–14 to `out/snapshots/<reference name>.png`, through the same LodePNG path.

New tools:
- `tools/measure_snapshot.py` (standard library only, like `check_snapshots.py`) reports the bounding boxes of the strip, the current column box and the marker from the pixel colours.
- `tools/compare_reference.py` writes a side-by-side image and a difference image per fixture to `out/compare/` for review, and prints the share of differing pixels outside text areas.

Geometry assertions run in `check.py`; the visual review is done by the implementer before the owner is asked.

## R10. Fixtures (FR-017)

**Decision**: one table in `lib/core/src/nvp/screen/fixtures.h`, used by the harness and by the device's `fixture N` command. Each row holds number, reference file name, moment, system state and its values. All rows use the spec 002 fixture records, location `İstanbul` and last update `03:00`:

| # | Time | State |
|---|---|---|
| 01–09 | 14:32:07, 03:30, 06:50, 10:00, 12:55, 16:20, 17:42, 18:20, 21:30 | `ok` |
| 10 | 14:32:07 | `offline` |
| 11 | 14:32:07 | `cache_ending`, 5 days |
| 12 | 14:32:07 | `calculated` |
| 13 | — | `no_time`, stored times shown |
| 14 | 14:32:07 | `no_data` |

## R11. Clock sources (FR-018, FR-019, FR-028 to FR-030)

**Decision**: `nvp::time::SimClock` in `lib/core` is pure and tested: it holds a base time, a speed and a pause flag, and advances from a monotonic millisecond input. `src/app/clock` chooses between two sources:
- **RTC**: base = the board clock read at start, plus elapsed time.
- **Simulated**: a fixture, or a `clock sim` time and speed.

`clock rtc`, or leaving a fixture, returns to the RTC source (FR-030). With an invalid RTC reading the RTC source has no time, and the screen shows `no_time`.

## R12. Serial commands (FR-018, FR-022, FR-029)

**Decision**: the Slice 1 parser (`nvp::diag::parse_command`) gains commands with arguments. Parsing stays pure and tested; argument validation is done in `lib/core`. The commands are listed in [contracts/serial-console.md](contracts/serial-console.md). They exist only when `NVP_DEV_TOOLS` is defined: set for `env:crowpanel7_advance` in this slice, removed for the release build of Slice 9 (SYS-06).

## R13. Hidden gesture (FR-022, FR-023)

**Facts** (GT911 data sheet in the vendor repository, register table):
- 0x814E: bit 7 = buffer status, bits 3–0 = number of touch points.
- Point 1 at 0x8150 and point 2 at 0x8158, each x low/high, y low/high.
- 0x8146–0x8149 give the coordinate resolution.

**Decision**:
- **Touch driver**: `platform::touch` reports the number of points as well as point 1.
- **Detector**: a pure state machine `nvp::screen::TwoFingerHold` in `lib/core`. It fires once after 5000 ms of continuous reports with ≥ 2 points, and resets when fewer than 2 are seen.
- **Main screen**: it has no clickable objects, so a tap there does nothing (02 §7.4 and §8 are out of scope).
- **Leaving**: the diagnostic screen gains a `Main screen` button, and the serial command `main` does the same; both stop stress mode (FR-023). The `diag` command switches to the diagnostic screen.

## R14. Size

Expected additions are about 190 kB of fonts and about 40 kB of screen code; the image is 1.47 MB now, against a 2.9 MiB budget. `check.py` reports the total, the fonts and the diagnostic mode.

## R15. Review log (2026-11-24)

**Method**: `pio run -e sim -t exec` renders fixtures 01–14; `tools/compare_reference.py` writes reference | snapshot | difference images to `out/compare/`; each was opened and reviewed; `tools/check_snapshots.py` asserts the geometry; element boxes were measured in both images with a scratch script (`boxes.py`).

**Result**: all fourteen fixtures match their references within 02 §12.
- **Geometry**: strip 24..775 × 264..293, current column box and marker within 1 px of the formulas and of the reference (identical boxes in every fixture; marker 0.5 px from X(now)). Checked automatically on every `check.py`.
- **Text**: positions within ±2 px of the reference, with one exception explained below.
- **Remaining pixel differences**: 2.8–5.3 % of pixels per fixture differ by more than 24 per channel. They are glyph anti-aliasing (LVGL 4-bit fonts vs. the browser) and the hatch stripe phase, both allowed by 02 §12.

**Findings**:
1. **Reference size**: every reference render is **800 × 481**, not 800 × 480 as 02 §1 states. Row 480 is the light grey browser page (236, 238, 242), an export artefact. The comparison uses the top 480 rows; the spec text is right, the files carry one extra row.
2. **Tabular glyphs**: the reference renders use `font-variant-numeric: tabular-nums` (reference.html), so the browser substitutes Barlow Condensed's tabular glyphs. `lv_font_conv` maps by code point and cannot apply OpenType features, so:
   - **digit 1**: the leading "1" of the clock is 27 px wide in the reference and 21 px in ours (right edge identical, x 56). This is accepted as a glyph-shape difference (02 §12): the glyph is narrower, not misplaced. Centring each digit's ink in its cell (instead of its advance) matched the reference better for the other digits and is kept.
   - **placeholder dash**: the tabular hyphen is much wider than the plain one, and the difference was obvious (gaps between the dashes of `--:--`). The fixed-cell widget therefore draws `-` as a bar with the reference's measured proportions (advance 0.328 em, bar 0.274 em × 0.104 em, centred 0.307 em above the baseline; measured at 48, 96 and 114 px). After the change, references 13 and 14 match.
3. **Font conversion**: `lv_font_conv@1.5.3` with LVGL 9.6's own options; 14 fonts, 110.1 kB linked (300 kB budget). Regenerating gives identical files. google/fonts commit `06bfe0d`.
4. **Sub-pixel coordinates**: `LV_USE_FLOAT 1` in `include/lv_conf.h`, so leader lines and the marker sit at the fractional X(t) of 02 §3.

**Redraw accounting** (harness, FR-015, FR-016): a one-second step invalidates 5 132 px (1.3 % of the screen; bound 8 %); a minute step 2.4 %; a vakit change 69 %; midnight 61 %; a one-hour backward jump 5.6 %; a 17-hour forward jump 59 %. After every step the picture equals a full redraw of the same moment (no leftovers). The fixed-cell widget invalidates only the cells whose character changed.

**Device evidence** (COM4, 2026-11-24, firmware 1 615 008 B):
- **First boot**: the board clock answered with the integrity bit set: `rtc: not valid (integrity lost)`. The main screen showed `no_time` at 264 ms (`screen: main (state no_time)`).
- **Setting the clock**: `time set 2026-11-24 12:07:27` → `rtc set: 2026-11-24 09:07:27 UTC`; `time get` → `rtc: 2026-11-24 09:07:29 UTC (12:07:29 local), valid`; the screen changed to `no_data` (reference 14 with the real time).
- **After a reset**: `rtc: 2026-11-24 12:07:41 local, valid`, host clock 12:07:42 just before the reset; `screen: main (state no_data)`.
- **Commands**: every command of contracts/serial-console.md answered as specified, including invalid arguments (`fixture 15`, `clock speed 0`) and `clock speed` refused on the board clock. `diag`/`main` switch modes.
- **Convergence checks** (T045–T047): the harness renders a long city (`Kahramanmaraş Onikişubat Büyükşehir Merkezi`) as `Kahramanmaraş Onikişubat Büyükş…`, 295 px wide (≤ 300 px, U+2026). The marker at 00:00:00 spans x 14..34 and at 23:59:59 x 766..786, both drawn. On the device, `diag`, `stress on`, `main` logged `stress: off, last run 4 s …` before `main screen` (FR-023).
