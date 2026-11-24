# Contract: main screen and harness (spec 003)

## `lib/ui` main screen

| Function | Meaning |
|---|---|
| `lv_obj_t* ui::main_screen_create()` | builds the screen objects once (fonts from `assets/fonts`, tokens from 02 §2) |
| `void ui::main_screen_update(const engine::MainView&, const ui::SystemView&)` | draws the given moment and state; touches only what changed since the last call (research R8) |

`lib/ui` depends only on LVGL and `lib/core` (05 §4). It reads no clock, storage or network (UI-02, UI-04).

## Fonts (`assets/fonts/`)

- `src/`: the four TTFs and `OFL.txt`.
- `SOURCES.md`: the google/fonts commit and download date.
- `gen_fonts.py`: runs `npx --yes lv_font_conv@1.5.3` with `--no-compress --no-prefilter --bpp 4 --format lvgl --force-fast-kern-format` for each font of research R3.
- `lib/ui/src/fonts/*.c`: the generated fonts, committed and compiled with `lib/ui`; `lib/ui/src/fonts/fonts.h` declares them.

Regenerating gives byte-identical files.

## Host harness

| Command | Output |
|---|---|
| `pio run -e sim -t exec` | `out/snapshots/*.png`: Slice 1 diagnostic screens plus fixtures `01-main-ogle-1432.png` … `14-state-no-data.png` (800 × 480, RGB565 colours) |
| `python tools/measure_snapshot.py out/snapshots/<file>.png` | JSON: `strip` box, `current_column` box (or null), `marker` x and box (or null) |
| `python tools/compare_reference.py` | `out/compare/<file>.png`: reference, snapshot and difference side by side; a summary line per fixture |
| `python tools/check_snapshots.py` | fails if any expected snapshot is missing or the measured geometry is off by more than 1 px from the formulas (02 §3, §4.2–§4.5) |

## Geometry formulas used by the checks

- X(t) = 24 + t / 86 400 × 752.
- The strip spans 24…776 × 264…294.
- The current column box is at x = 24 + k × 125.33, y 336…464, 125.33 × 128.
- The marker line is 3 px wide, centred on X(now), y 259…299.
- Checks round to whole pixels and allow ±1 px (02 §12).
