// Formatting of times, durations and dates in Turkish (spec 002 FR-016 to FR-018,
// 02-ui-spec.md §9.1). No C library formatting and no heap (research R5).
//
// Every function writes into `out` with capacity `cap` (bytes, including the terminator), always
// terminates when cap > 0, never writes past cap, and returns the number of bytes written without
// the terminator. When the text does not fit it writes an empty string and returns 0.
#pragma once

#include <cstddef>
#include <cstdint>

#include "nvp/engine/calendar.h"
#include "nvp/engine/day_record.h"

namespace nvp::text {

size_t format_hhmm(uint16_t minutes, char* out, size_t cap);           // "HH:MM", minutes 0..1439
size_t format_hhmmss(int32_t seconds, char* out, size_t cap);          // "HH:MM:SS", hours may pass 23
size_t format_duration_note(int32_t seconds, char* out, size_t cap);   // "54 dk sonra", "1 dk’dan az sonra"
size_t format_date(const engine::Date& d, char* out, size_t cap);      // "17 Kasım 2026 Salı"
size_t format_hijri(const engine::HijriDate& h, char* out, size_t cap); // "7 Cemaziyelahir 1448"

}  // namespace nvp::text
