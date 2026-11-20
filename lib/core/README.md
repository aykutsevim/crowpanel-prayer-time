# lib/core

Pure C++17. Day records, validation, the prayer-time engine, Turkish text formatting (durations,
dates, period names), settings model, source ranks and merge rules, calibration maths.

**Layering rule:** no Arduino, ESP-IDF, LVGL or other platform headers. Must build and pass its
tests on the development machine (`pio test -e native`). Tests are written before the code
(constitution, Principle VI). No heap allocation in per-second paths, no global mutable state.
UI strings live in one table here, UTF-8, with U+2019 where the spec uses it.
