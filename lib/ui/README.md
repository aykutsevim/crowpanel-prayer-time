# lib/ui

Screens and widgets from `handover/docs/02-ui-spec.md`, implemented with LVGL.

**Layering rule:** depends on LVGL and `lib/core` only. Takes a plain view-model struct; knows
nothing about Wi-Fi, flash, time sources or the panel driver. The same sources build for the host
UI harness (`sim/`) and the device. No dynamic allocation in the steady-state update path.
On-screen text is Turkish and copied exactly from the spec.

Exception: `src/diag/` holds the engineering screens (boot, diagnostic, settings). They are in
English and outside `02-ui-spec.md` (spec 001, Assumptions).
