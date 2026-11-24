// Status row of the main screen per system state: text, colour and icon (02-ui-spec.md §6;
// spec 003 FR-011, FR-012).
#pragma once

#include <cstddef>
#include <cstdint>

#include "nvp/screen/system_state.h"

namespace nvp::screen {

enum class StatusIcon : uint8_t { Wifi, WifiOff, Approx };

// Writes the M3 status text. `last_update` is "HH:MM" (today) or "DD.MM"; `days_left` is used by
// CacheEnding (0 = "bugün"). Returns the length; writes "" and returns 0 if `cap` is too small.
size_t status_text(SystemState s, const char* last_update, int16_t days_left, char* out, size_t cap);

bool status_is_alert(SystemState s);  // alert colour, else muted
StatusIcon status_icon(SystemState s);

}  // namespace nvp::screen
