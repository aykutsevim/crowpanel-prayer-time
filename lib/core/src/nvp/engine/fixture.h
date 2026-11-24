// The shared test fixture of docs/specification/03-functional-spec.md §3, also used by every
// reference render (02-ui-spec.md §11). Used by the engine tests, the output table and, later, the
// UI harness (spec 002 FR-021, research R11). Illustrative values, not authoritative times.
//
// 03 §3 fixes today's six times, yesterday's Yatsı (19:15) and tomorrow's İmsak (06:17). The other
// neighbour times follow the November trend (İmsak later, Yatsı earlier each day).
#pragma once

#include "nvp/engine/day_record.h"

namespace nvp::engine::fixture {

inline constexpr uint16_t hm(int h, int m) { return static_cast<uint16_t>(h * 60 + m); }

inline constexpr Date kDate{2026, 11, 17};  // 17 Kasım 2026 Salı

inline constexpr DayRecord kYesterday{
    Date{2026, 11, 16},
    {hm(6, 15), hm(7, 45), hm(12, 54), hm(15, 28), hm(17, 52), hm(19, 15)},
    HijriDate{6, 6, 1448, true},
    SourceId::Imsakiyem};

inline constexpr DayRecord kToday{
    kDate,
    {hm(6, 16), hm(7, 46), hm(12, 54), hm(15, 27), hm(17, 51), hm(19, 14)},
    HijriDate{7, 6, 1448, true},  // 7 Cemaziyelahir 1448
    SourceId::Imsakiyem};

inline constexpr DayRecord kTomorrow{
    Date{2026, 11, 18},
    {hm(6, 17), hm(7, 47), hm(12, 54), hm(15, 26), hm(17, 50), hm(19, 13)},
    HijriDate{8, 6, 1448, true},
    SourceId::Imsakiyem};

// Expected at 14:32:07 (03 §3): period Öğle 12:54–15:27, remaining 00:54:53, fraction 0.641.
inline constexpr int32_t kCheckSecond = 14 * 3600 + 32 * 60 + 7;
inline constexpr int32_t kExpectedRemaining = 3293;
inline constexpr int32_t kExpectedElapsed = 5887;
inline constexpr int32_t kExpectedFractionTimes1000 = 641;

}  // namespace nvp::engine::fixture
