// The timeline band of the main screen: hour scale, day strip with separators and kerahat hatch,
// leader lines, and the now marker (02-ui-spec.md §3, §4.2..§4.4; spec 003 FR-005 to FR-008,
// research R5). Drawn in draw events; redrawn only when its inputs change (UI-03).
#pragma once

#include <cstdint>

#include "lvgl.h"

namespace nvp::ui {

struct TimelineState {
    bool segments = false;          // false: one night-colour bar, no separators, leaders or hatch
    uint16_t minutes[6] = {};       // today's six times
    bool hatch[3] = {};             // kerahat intervals shown
    int32_t hatch_start[3] = {};    // seconds of the day
    int32_t hatch_end[3] = {};
    int8_t current = -1;            // column whose leader is drawn in `now`; -1 none
};

bool operator==(const TimelineState& a, const TimelineState& b);

void timeline_create(lv_obj_t* parent);
void timeline_set(const TimelineState& s);
// Marker at second `second_of_day`; hidden when `show` is false. Moves only when its pixel changes.
void marker_set(bool show, int32_t second_of_day);
lv_obj_t* marker_obj();  // for the host harness

}  // namespace nvp::ui
