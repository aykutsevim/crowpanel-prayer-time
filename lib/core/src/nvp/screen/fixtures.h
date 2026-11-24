// The seventeen reference fixtures (02-ui-spec.md §11; spec 003 FR-017, research R10), shared by
// the host harness and the device's `fixture N` command. All use the shared sample day of
// nvp/engine/fixture.h (17 Kasım 2026), city "İstanbul", last update "03:00".
#pragma once

#include <cstddef>
#include <cstdint>

#include "nvp/screen/system_state.h"

namespace nvp::screen {

// Which screen a fixture shows: the main screen, or a spec 004 screen (15 setup, 16 connecting,
// 17 the status overlay over fixture 01's main screen).
enum class FixtureScreen : uint8_t { Main, Setup, Connecting, Overlay };

struct Fixture {
    uint8_t number;         // 1..17
    const char* reference;  // file name in docs/design/screens/
    int32_t second_of_day;  // -1: no clock (fixture 13)
    SystemState state;
    int16_t days_left;      // CacheEnding only
    FixtureScreen screen = FixtureScreen::Main;
};

inline constexpr size_t kFixtureCount = 17;
extern const Fixture kFixtures[kFixtureCount];

inline constexpr const char* kFixtureCity = "İstanbul";
inline constexpr const char* kFixtureLastUpdate = "03:00";

const Fixture* fixture(int number);  // nullptr outside 1..17

}  // namespace nvp::screen
