# Contract: engineering screens

800×480, landscape. The screens use LVGL's built-in Montserrat fonts (14 and 20 px) and English text. They are not product screens and fall outside `02-ui-spec.md`. All of them render on the host (env `sim`) from a `DiagViewModel` fixture.

**Colours**:

- black background `#000000`, white `#FFFFFF`
- alert `#FF3B30` (failed results)
- ok `#34C759`
- muted `#8E8E93`

## Boot screen

Black background with centred text:

- `Namaz Vakti Panel`
- the firmware version
- `DIS02170A V1.4`
- `starting...` (three dots: the built-in Montserrat font has no U+2026 glyph; checked 2026-11-23)

It shows until the diagnostic screen is ready, at most 5 s after power-on (SC-006).

## Diagnostic screen

| Region (x, y, w, h) | Content |
|---|---|
| whole screen | a one-pixel white border at x=0, x=799, y=0, y=479 |
| (1, 1, 798, 179) | eight colour bars, about 100 px wide each, labelled at the bottom of each bar, left to right: `WHITE`, `YELLOW`, `CYAN`, `GREEN`, `MAGENTA`, `RED`, `BLUE`, `BLACK` (pure RGB565 primaries and their mixes) |
| top edge | tick marks every 10 px, 3 px long, inside the border |
| (1, 180, 798, 100) | alternating one-pixel vertical lines, white on even x and black on odd x |
| centre of the line field | a label `800 x 480` on a black box |
| (1, 280, 520, 199) | info panel: the report lines (self-tests coloured ok or alert), Wi-Fi state, `Backlight: full (dimming disabled)`, heap, stress counters, touch coordinates or `no touch`, a heartbeat dot that toggles every second |
| (521, 280, 278, 199) | controls: `Stress on/off` and `Settings`. No brightness control (OD1). |
| corners | four crosshair targets (24 px) centred at (20,20), (779,20), (20,459), (779,459). Each turns ok-green or alert-red with `+dx,+dy` once judged. |
| next to the panel edge nearest the USB-C connector | an orientation arrow and `USB-C this side`. The side is taken from the vendor board drawing (`REPO/Datasheet/LI070DE42Y_0812.dwg` and the product photos) during Stage 2, with the source cited in `board.h`. Checklist B2 confirms it; a "no" means the panel is rotated. |

A touch outside the controls shows a 20 px ring marker at the touch point.

## Settings screen

| Element | Behaviour |
|---|---|
| Wi-Fi state line | `Not set up`, `Joining <ssid>`, `Connected to <ssid>` or `Hotspot open` |
| `Start Wi-Fi setup` | opens a confirmation (`Start hotspot?` with `Yes` / `No`). After `Yes` the screen shows the hotspot name, password and `Open http://192.168.4.1/`. |
| `Cancel setup` | visible while the hotspot is open; acts at once |
| `Forget Wi-Fi` | opens a confirmation (`Forget <ssid>?` with `Yes` / `No`) |
| `Back` | returns to the diagnostic screen |

An unanswered confirmation closes itself after 15 s and nothing changes (FR-019a). Nothing on these screens refers to sound.

## Snapshot fixtures (env `sim`)

| Fixture | State |
|---|---|
| `boot` | boot screen |
| `diag_idle` | all tests passed, stress off |
| `diag_failures` | touch failed, Wi-Fi skipped, storage failed |
| `diag_stress` | stress running with counters, one HTTPS error |
| `diag_touch` | marker shown, two corners passed, one failed |
| `settings_none` | settings with no Wi-Fi set up |
| `settings_confirm` | the forget-Wi-Fi confirmation |
| `settings_hotspot` | hotspot open; the password is the fixture value `12345678` |
