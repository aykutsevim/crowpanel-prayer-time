// The prayer-time engine (spec 002; docs/specification/03-functional-spec.md §3 ENG-01..ENG-08).
//
// evaluate() is a pure function: given the local moment and the day records for yesterday, today
// and tomorrow, it fills a MainView with everything the main screen shows (02-ui-spec.md §4, §5):
// the running period, its bounds and countdown, the six columns, the kerahat intervals, and the
// ready Turkish strings. It never reads a clock, a time zone, storage or the network, keeps no
// state between calls and never allocates (FR-019, FR-023). Types: specs/002-prayer-engine/
// data-model.md. Rules: contracts/engine-api.md.
#pragma once

#include <cstdint>

#include "nvp/engine/calendar.h"
#include "nvp/engine/day_record.h"

namespace nvp::engine {

struct Settings {
    uint8_t soon_threshold_min = 15;  // 0 never sets "soon" (03 §8)
    bool kerahat_enabled = true;
    uint8_t kerahat_sunrise_min = 45;  // each limited to 0..90; 0 omits the interval
    uint8_t kerahat_noon_min = 45;
    uint8_t kerahat_sunset_min = 45;
};

struct Input {
    Moment now;
    const DayRecord* yesterday = nullptr;  // any may be null; invalid or wrongly dated = missing
    const DayRecord* today = nullptr;
    const DayRecord* tomorrow = nullptr;
    Settings settings;
};

enum class ColumnState : uint8_t { Past, Current, Next, Later };

struct Column {
    const char* name = "";  // never null
    uint16_t minutes = 0;   // the displayed time (tomorrow's İmsak after Yatsı)
    char time_text[6] = {};
    ColumnState state = ColumnState::Later;
    int32_t seconds_until = 0;  // 0 for Past and Current
    char note[32] = {};
};

struct KerahatInterval {
    bool shown = false;
    int32_t start_s = 0;  // seconds of today, start included
    int32_t end_s = 0;    // end excluded, at most 86400
};

struct MainView {
    // Status
    bool has_data = false;       // false: "no data" (ENG-08); only the clock and date are filled
    bool approx_bounds = false;  // a missing neighbour day was substituted (ENG-04)
    bool has_stored_times = false;  // today's (or the no-time caller's) record was usable (spec 003)

    // Hero (02 §4.1 M5..M15)
    char clock_hhmm[6] = {};
    char clock_ss[4] = {};
    char date_text[48] = {};
    char hijri_text[40] = {};
    uint8_t period = 0;
    const char* period_name = "";      // never null
    const char* countdown_label = "";  // never null
    LocalSeconds start = 0;
    LocalSeconds end = 0;
    char start_text[6] = {};
    char end_text[6] = {};
    uint8_t next = 0;
    bool next_is_tomorrow = false;
    int32_t remaining_s = 0;
    int32_t elapsed_s = 0;
    float fraction = 0.0f;  // 0 .. just below 1
    char countdown_text[10] = {};
    bool soon = false;
    bool entered = false;

    // Columns (02 §4.5)
    Column columns[kVakitCount];

    // Day strip (02 §3, §4.2)
    uint16_t strip_minutes[kVakitCount] = {};  // today's six times for strip, separators and leaders
    int32_t now_second_of_day = 0;
    KerahatInterval kerahat[3];  // after Güneş, before Öğle, before Akşam
};

inline constexpr int32_t kEnteredSeconds = 5 * 60;  // "vakit girdi" for the first 5 minutes (02 §5)
inline constexpr uint8_t kMaxKerahatMinutes = 90;

void evaluate(const Input& in, MainView& out);

// Without a valid clock (02-ui-spec.md §6 row 1, reference 13; spec 003 research R7): fills only
// what stored times allow: the six columns as Later with empty notes, and the kerahat intervals
// for the strip. No clock, date, period or countdown text. `stored` may be null or invalid, in
// which case nothing but the column names is filled.
void evaluate_no_time(const DayRecord* stored, const Settings& settings, MainView& out);

}  // namespace nvp::engine
