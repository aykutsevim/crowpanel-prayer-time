#include "nvp/screen/fixtures.h"

namespace nvp::screen {
namespace {

constexpr int32_t hms(int h, int m, int s) { return h * 3600 + m * 60 + s; }
constexpr int32_t kCheck = hms(14, 32, 7);

}  // namespace

const Fixture kFixtures[kFixtureCount] = {
    {1, "01-main-ogle-1432.png", kCheck, SystemState::Ok, 0},
    {2, "02-main-gece-0330.png", hms(3, 30, 0), SystemState::Ok, 0},
    {3, "03-main-imsak-0650.png", hms(6, 50, 0), SystemState::Ok, 0},
    {4, "04-main-kusluk-1000.png", hms(10, 0, 0), SystemState::Ok, 0},
    {5, "05-main-vakit-girdi-1255.png", hms(12, 55, 0), SystemState::Ok, 0},
    {6, "06-main-ikindi-1620.png", hms(16, 20, 0), SystemState::Ok, 0},
    {7, "07-main-az-kaldi-1742.png", hms(17, 42, 0), SystemState::Ok, 0},
    {8, "08-main-aksam-1820.png", hms(18, 20, 0), SystemState::Ok, 0},
    {9, "09-main-yatsi-2130.png", hms(21, 30, 0), SystemState::Ok, 0},
    {10, "10-state-offline.png", kCheck, SystemState::Offline, 0},
    {11, "11-state-cache-ending.png", kCheck, SystemState::CacheEnding, 5},
    {12, "12-state-calculated.png", kCheck, SystemState::Calculated, 0},
    {13, "13-state-time-not-set.png", -1, SystemState::NoTime, 0},
    {14, "14-state-no-data.png", kCheck, SystemState::NoData, 0},
};

const Fixture* fixture(int number) {
    if (number < 1 || number > static_cast<int>(kFixtureCount)) return nullptr;
    return &kFixtures[number - 1];
}

}  // namespace nvp::screen
