// The main screen's input besides the engine result (spec 003 data-model "Screen view", FR-012,
// FR-014). Plain values; the screen reads no clock, storage or network (UI-02, UI-04).
#pragma once

#include <cstdint>

#include "nvp/screen/system_state.h"

namespace nvp::ui {

struct SystemView {
    screen::SystemState state = screen::SystemState::Ok;
    char city[48] = {};             // M2; cut with "…" above 300 px
    char last_update[8] = {};       // "HH:MM" or "DD.MM" for the ok state
    int16_t days_left = 0;          // cache_ending
    bool kerahat_enabled = true;    // legend M9 and the hatch
    char hijri_fallback[40] = {};   // no_data: Hijri date of the newest stored record, else empty
};

}  // namespace nvp::ui
