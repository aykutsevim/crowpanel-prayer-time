// One day's published prayer times (spec 002 FR-001 to FR-003, ENG-01, research R8).
// The layout mirrors the stored record of docs/specification/04-data-sources.md §9.
#pragma once

#include <cstdint>

#include "nvp/engine/calendar.h"

namespace nvp::engine {

// Vakit indices, in the order of the day (identifiers per CLAUDE.md).
inline constexpr uint8_t kImsak = 0;
inline constexpr uint8_t kGunes = 1;
inline constexpr uint8_t kOgle = 2;
inline constexpr uint8_t kIkindi = 3;
inline constexpr uint8_t kAksam = 4;
inline constexpr uint8_t kYatsi = 5;
inline constexpr uint8_t kVakitCount = 6;

// Providers of docs/specification/04-data-sources.md §2. The engine passes the source through.
enum class SourceId : uint8_t { Diyanet, Imsakiyem, Emushaf, Aladhan, Local };

// 50, 40, 40, 20, 10. Rank >= 40 is "Diyanet-derived" (04 §2).
uint8_t rank(SourceId s);

struct HijriDate {
    uint8_t day = 0;
    uint8_t month = 0;  // 1..12
    uint16_t year = 0;
    bool present = false;
};

struct DayRecord {
    Date date;
    uint16_t minutes[kVakitCount] = {};  // minutes after local midnight, İmsak..Yatsı
    HijriDate hijri;
    SourceId source = SourceId::Local;
};

// A record is valid when its date is valid and its six times are strictly increasing within
// 00:00..23:59 (ENG-01). An invalid record is treated as missing.
bool valid(const DayRecord& r);

}  // namespace nvp::engine
