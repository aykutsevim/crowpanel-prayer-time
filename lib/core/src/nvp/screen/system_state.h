// The main screen's system state (02-ui-spec.md §6; spec 003 FR-012, UI-01, research R7).
// Exactly one state is active, chosen top to bottom; a forced state (development command) wins.
#pragma once

#include <cstdint>

namespace nvp::screen {

// In the order of 02 §6: the first match wins.
enum class SystemState : uint8_t { NoTime, NoData, Calculated, CacheEnding, Offline, Ok };

struct StateInputs {
    bool clock_valid = false;                  // TIME-03
    bool today_present = false;                // a record for today from any source
    uint8_t today_rank = 0;                    // source rank of today's record; below 40 = calculated
    bool refresh_failed_or_wifi_down = false;  // last refresh failed or Wi-Fi is down
    int16_t stored_days_after_today = 0;
    int32_t wifi_down_minutes = 0;
    bool forced = false;  // development command `state ...`
    SystemState forced_state = SystemState::Ok;
};

inline constexpr uint8_t kDiyanetRank = 40;           // 04-data-sources.md §2
inline constexpr int16_t kCacheEndingDays = 7;        // 02 §6 row 4
inline constexpr int32_t kOfflineAfterMinutes = 10;   // 02 §6 row 5: "more than 10 min"

SystemState select_state(const StateInputs& in);

// "no_time", "no_data", "calculated", "cache_ending", "offline", "ok" (serial commands, report).
const char* state_name(SystemState s);
bool parse_state_name(const char* name, SystemState* out);

}  // namespace nvp::screen
