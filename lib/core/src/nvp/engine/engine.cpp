#include "nvp/engine/engine.h"

#include <cstddef>

#include "nvp/text/format.h"
#include "nvp/text/tr.h"

namespace nvp::engine {
namespace {

namespace tr = nvp::text::tr;
using text::format_duration_note;
using text::format_hhmm;
using text::format_hhmmss;

// A neighbour record counts only when it is valid and dated exactly one day away (FR-003).
bool usable(const DayRecord* r, DayNumber expected_day) {
    return r != nullptr && valid(*r) && to_day_number(r->date) == expected_day;
}

uint16_t minute_of_day(LocalSeconds t) {
    LocalSeconds s = t % kSecondsPerDay;
    if (s < 0) s += kSecondsPerDay;
    return static_cast<uint16_t>(s / 60);
}

// Bounded copy of a short constant string (the notes fit; see data-model.md buffer sizes).
void copy(const char* src, char* dst, size_t cap) {
    size_t i = 0;
    for (; src[i] != '\0' && i + 1 < cap; ++i) dst[i] = src[i];
    dst[i] = '\0';
}

int32_t clamp_minutes(uint8_t m) { return (m > kMaxKerahatMinutes ? kMaxKerahatMinutes : m) * 60; }

void fill_kerahat(const DayRecord& today, const Settings& s, MainView& out) {
    if (!s.kerahat_enabled) return;
    const int32_t gunes = today.minutes[kGunes] * 60;
    const int32_t ogle = today.minutes[kOgle] * 60;
    const int32_t aksam = today.minutes[kAksam] * 60;
    const int32_t a = clamp_minutes(s.kerahat_sunrise_min);
    const int32_t b = clamp_minutes(s.kerahat_noon_min);
    const int32_t c = clamp_minutes(s.kerahat_sunset_min);
    auto set = [](KerahatInterval& i, int32_t len, int32_t start, int32_t end) {
        if (len == 0) return;
        i.shown = true;
        i.start_s = start < 0 ? 0 : start;
        i.end_s = end > kSecondsPerDay ? kSecondsPerDay : end;
    };
    set(out.kerahat[0], a, gunes, gunes + a);  // after sunrise (ENG-06)
    set(out.kerahat[1], b, ogle - b, ogle);    // before Öğle
    set(out.kerahat[2], c, aksam - c, aksam);  // before Akşam
}

}  // namespace

void evaluate(const Input& in, MainView& out) {
    out = MainView{};  // every field is rewritten on every call; no heap is involved

    const Moment& now = in.now;
    if (!valid(now.date) || now.second_of_day < 0 || now.second_of_day >= kSecondsPerDay) return;

    // The clock and date are shown even without data (02 §6 no_data keeps "clock and date normal").
    format_hhmm(static_cast<uint16_t>(now.second_of_day / 60), out.clock_hhmm, sizeof out.clock_hhmm);
    out.clock_ss[0] = ':';
    out.clock_ss[1] = static_cast<char>('0' + (now.second_of_day % 60) / 10);
    out.clock_ss[2] = static_cast<char>('0' + now.second_of_day % 10);
    out.clock_ss[3] = '\0';
    text::format_date(now.date, out.date_text, sizeof out.date_text);
    out.now_second_of_day = now.second_of_day;

    const DayNumber day = to_day_number(now.date);
    if (!usable(in.today, day)) return;  // ENG-08: no data
    const DayRecord& today = *in.today;
    out.has_data = true;

    // Today's six vakits, and the two neighbour times the night period needs (ENG-03, ENG-04).
    LocalSeconds t[kVakitCount];
    for (uint8_t k = 0; k < kVakitCount; ++k) t[k] = at(day, today.minutes[k]);
    const bool have_yesterday = usable(in.yesterday, day - 1);
    const bool have_tomorrow = usable(in.tomorrow, day + 1);
    const LocalSeconds yatsi_before = at(day - 1, have_yesterday ? in.yesterday->minutes[kYatsi] : today.minutes[kYatsi]);
    const uint16_t imsak_after_min = have_tomorrow ? in.tomorrow->minutes[kImsak] : today.minutes[kImsak];
    const LocalSeconds imsak_after = at(day + 1, imsak_after_min);

    const LocalSeconds now_s = static_cast<LocalSeconds>(day) * kSecondsPerDay + now.second_of_day;
    const bool before_imsak = now_s < t[kImsak];
    const bool after_yatsi = now_s >= t[kYatsi];

    // Running period: half-open intervals, each vakit begins at second 00 (ENG-02, ENG-03).
    if (before_imsak) {
        out.period = kYatsi;
        out.start = yatsi_before;
        out.end = t[kImsak];
        out.next = kImsak;
        out.approx_bounds = !have_yesterday;
    } else if (after_yatsi) {
        out.period = kYatsi;
        out.start = t[kYatsi];
        out.end = imsak_after;
        out.next = kImsak;
        out.next_is_tomorrow = true;
        out.approx_bounds = !have_tomorrow;
    } else {
        uint8_t p = 0;
        while (p + 1 < kYatsi && now_s >= t[p + 1]) ++p;
        out.period = p;
        out.start = t[p];
        out.end = t[p + 1];
        out.next = static_cast<uint8_t>(p + 1);
    }

    // Countdown and progress (ENG-05).
    out.remaining_s = static_cast<int32_t>(out.end - now_s);
    out.elapsed_s = static_cast<int32_t>(now_s - out.start);
    const int32_t length = static_cast<int32_t>(out.end - out.start);
    out.fraction = length > 0 ? static_cast<float>(out.elapsed_s) / static_cast<float>(length) : 0.0f;
    if (out.fraction < 0.0f) out.fraction = 0.0f;
    if (out.fraction >= 1.0f) out.fraction = 0.99999f;
    const int32_t threshold = in.settings.soon_threshold_min * 60;
    out.soon = threshold > 0 && out.remaining_s <= threshold;
    out.entered = out.elapsed_s < kEnteredSeconds;

    out.period_name = tr::kPeriodNames[out.period];
    out.countdown_label = tr::kCountdownLabels[out.period];
    format_hhmm(minute_of_day(out.start), out.start_text, sizeof out.start_text);
    format_hhmm(minute_of_day(out.end), out.end_text, sizeof out.end_text);
    format_hhmmss(out.remaining_s, out.countdown_text, sizeof out.countdown_text);

    // Columns (02 §4.5, FR-010 to FR-013).
    const char* current_note = out.entered ? tr::kVakitGirdi : tr::kSuAn;
    for (uint8_t k = 0; k < kVakitCount; ++k) {
        Column& c = out.columns[k];
        c.name = tr::kVakitNames[k];
        c.minutes = today.minutes[k];
        LocalSeconds when = t[k];
        if (after_yatsi) {
            // Yatsı is current; İmsak shows tomorrow's time, "yarın"; the rest have passed.
            if (k == kYatsi) {
                c.state = ColumnState::Current;
            } else if (k == kImsak) {
                c.state = ColumnState::Next;
                c.minutes = imsak_after_min;
                when = imsak_after;
            } else {
                c.state = ColumnState::Past;
            }
        } else if (before_imsak) {
            // The night period started yesterday: no column is current, all six are ahead.
            c.state = k == kImsak ? ColumnState::Next : ColumnState::Later;
        } else if (k == out.period) {
            c.state = ColumnState::Current;
        } else if (t[k] <= now_s) {
            c.state = ColumnState::Past;
        } else {
            c.state = k == out.next ? ColumnState::Next : ColumnState::Later;
        }
        format_hhmm(c.minutes, c.time_text, sizeof c.time_text);
        switch (c.state) {
            case ColumnState::Current:
                copy(current_note, c.note, sizeof c.note);
                break;
            case ColumnState::Past:
                break;
            case ColumnState::Next:
            case ColumnState::Later:
                c.seconds_until = static_cast<int32_t>(when - now_s);
                if (after_yatsi && k == kImsak) {
                    copy(tr::kYarin, c.note, sizeof c.note);
                } else {
                    format_duration_note(c.seconds_until, c.note, sizeof c.note);
                }
                break;
        }
    }

    text::format_hijri(today.hijri, out.hijri_text, sizeof out.hijri_text);
    fill_kerahat(today, in.settings, out);
}

}  // namespace nvp::engine
