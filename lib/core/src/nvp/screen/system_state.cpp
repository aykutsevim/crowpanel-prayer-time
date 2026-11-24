#include "nvp/screen/system_state.h"

#include <cstring>

namespace nvp::screen {
namespace {

struct Name {
    SystemState state;
    const char* text;
};
constexpr Name kNames[] = {{SystemState::NoTime, "no_time"},           {SystemState::NoData, "no_data"},
                           {SystemState::Calculated, "calculated"},     {SystemState::CacheEnding, "cache_ending"},
                           {SystemState::Offline, "offline"},           {SystemState::Ok, "ok"}};

}  // namespace

SystemState select_state(const StateInputs& in) {
    if (in.forced) return in.forced_state;
    if (!in.clock_valid) return SystemState::NoTime;
    if (!in.today_present) return SystemState::NoData;
    if (in.today_rank < kDiyanetRank) return SystemState::Calculated;
    if (in.refresh_failed_or_wifi_down && in.stored_days_after_today <= kCacheEndingDays) {
        return SystemState::CacheEnding;
    }
    if (in.wifi_down_minutes > kOfflineAfterMinutes) return SystemState::Offline;
    return SystemState::Ok;
}

const char* state_name(SystemState s) {
    for (const Name& n : kNames) {
        if (n.state == s) return n.text;
    }
    return "?";
}

bool parse_state_name(const char* name, SystemState* out) {
    if (name == nullptr) return false;
    for (const Name& n : kNames) {
        if (std::strcmp(n.text, name) == 0) {
            *out = n.state;
            return true;
        }
    }
    return false;
}

}  // namespace nvp::screen
