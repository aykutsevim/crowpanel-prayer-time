// Colours of the engineering screens (contracts/diag-screens.md). Kept in one place.
#pragma once

#include "lvgl.h"

namespace nvp::ui::diag::palette {

inline lv_color_t black() { return lv_color_hex(0x000000); }
inline lv_color_t white() { return lv_color_hex(0xFFFFFF); }
inline lv_color_t alert() { return lv_color_hex(0xFF3B30); }  // failed results
inline lv_color_t ok() { return lv_color_hex(0x34C759); }
inline lv_color_t muted() { return lv_color_hex(0x8E8E93); }

}  // namespace nvp::ui::diag::palette
