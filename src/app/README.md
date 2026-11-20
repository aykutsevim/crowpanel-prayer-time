# src/app

Mode state machine (BOOT / SETUP / CONNECTING / RUN, `03-functional-spec.md` §1), wiring of
platform <-> core <-> ui, and development serial commands (SYS-06, development builds only).

**Layering rule:** uses `src/platform` through its interfaces; keeps logic that can be tested on
the host in `lib/core` or `lib/providers`. The UI task never does network or flash I/O (UI-04).
