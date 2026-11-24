// Builds the screen input for reference fixture N (1..14), for the host harness and the device's
// `fixture N` command alike (spec 003 FR-017, research R10).
#pragma once

#include <cstdint>

#include "main/main_view.h"
#include "nvp/engine/engine.h"

namespace nvp::ui {

// Fixture at its own moment. Returns false for an unknown number.
bool fixture_view(int number, engine::MainView& v, SystemView& s);

// The same fixture data and state at another second of the sample day (used for fast-forward
// tests and the redraw accounting). `second_of_day` < 0 gives the no-clock view.
bool fixture_view_at(int number, int32_t second_of_day, engine::MainView& v, SystemView& s);

}  // namespace nvp::ui
