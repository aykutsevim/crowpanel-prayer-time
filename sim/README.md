# sim

Host UI harness (`docs/specification/05-architecture.md` §7): compiles `lib/ui`,
`lib/core` and LVGL for the development machine with a headless 800x480 display that writes PNG
snapshots to `out/snapshots/`, for comparison with `docs/design/screens/`.

Slice 1 adds the engineering screens (boot, diagnostic, settings) under `sim/diag_render/`; the
product screens follow from Slice 3. Run it with `pio run -e sim -t exec`.

**Layering rule:** no device code. Uses the same fixture table as the device serial commands.
