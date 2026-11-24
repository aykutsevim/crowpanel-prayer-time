// The six prayer columns (02-ui-spec.md §4.5, §6; spec 003 FR-009, FR-013).
#pragma once

#include <cstdint>

#include "lvgl.h"
#include "nvp/engine/engine.h"

namespace nvp::ui {

enum class ColumnsMode : uint8_t {
    Normal,       // states and notes from the engine
    Calculated,   // as Normal, times with the ≈ prefix at 44 px
    StoredOnly,   // no_time: stored times as `later`, no notes (reference 13)
    Placeholder,  // no_data, or no_time without stored times: `--:--` in dim (reference 14)
};

void columns_create(lv_obj_t* parent);
void columns_set(const engine::MainView& v, ColumnsMode mode);

}  // namespace nvp::ui
