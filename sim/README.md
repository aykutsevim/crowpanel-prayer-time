# sim

Host UI harness (`handover/docs/05-architecture.md` §7, built in Slice 3): compiles `lib/ui`,
`lib/core` and LVGL for the development machine with a headless 800x480 display that writes PNG
snapshots to `out/snapshots/`, for comparison with `handover/design/screens/`.

**Layering rule:** no device code. Uses the same fixture table as the device serial commands.
