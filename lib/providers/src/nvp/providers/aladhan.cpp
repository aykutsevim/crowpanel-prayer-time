#include "nvp/providers/aladhan.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "nvp/providers/emushaf.h"

namespace nvp::providers {
namespace {

constexpr const char* kTimeKeys[engine::kVakitCount] = {"Fajr", "Sunrise", "Dhuhr", "Asr", "Maghrib", "Isha"};

// "-3.85403" style degrees from degrees x 100 000.
void degrees(int32_t e5, char* out, size_t cap) {
    const char* sign = e5 < 0 ? "-" : "";
    const uint32_t a = static_cast<uint32_t>(e5 < 0 ? -static_cast<int64_t>(e5) : e5);
    std::snprintf(out, cap, "%s%lu.%05lu", sign, static_cast<unsigned long>(a / 100000),
                  static_cast<unsigned long>(a % 100000));
}

}  // namespace

bool aladhan_url(int year, int month, int32_t lat_e5, int32_t lon_e5, char* out, size_t cap) {
    char lat[16], lon[16];
    degrees(lat_e5, lat, sizeof lat);
    degrees(lon_e5, lon, sizeof lon);
    const int n = std::snprintf(out, cap,
                                "https://api.aladhan.com/v1/calendar/%04d/%d?latitude=%s&longitude=%s&method=13"
                                "&school=0&timezonestring=Europe/Istanbul",
                                year, month, lat, lon);
    return n > 0 && static_cast<size_t>(n) < cap;
}

AladhanParser::AladhanParser(RecordSink& sink, engine::Date keep_from, engine::Date keep_to)
    : sink_(sink), keep_from_(keep_from), keep_to_(keep_to), json_(*this) {
    info_.place_unnamed = true;
    info_.calculated_source = true;
}

bool AladhanParser::feed(const char* data, size_t n) { return json_.feed(data, n); }

void AladhanParser::finish() {
    json_.finish();
    info_.json_error = json_.error();
    info_.complete = json_.complete();
    info_.success_seen = code_200_ || status_ok_;
    info_.success = code_200_ && status_ok_;
}

void AladhanParser::on_json(const JsonEvent& e) {
    if (std::strcmp(e.path, "code") == 0) {
        code_200_ = e.type == JsonType::Number && std::strcmp(e.text, "200") == 0;
        return;
    }
    if (std::strcmp(e.path, "status") == 0) {
        status_ok_ = e.type == JsonType::String && std::strcmp(e.text, "OK") == 0;
        return;
    }
    if (std::strcmp(e.path, "data") == 0) {
        if (e.type == JsonType::BeginArray) info_.records_array_seen = true;
        return;
    }
    if (std::strcmp(e.path, "data[]") == 0) {
        if (e.type == JsonType::BeginObject) {
            in_record_ = true;
            rec_ = engine::DayRecord{};
            rec_.source = engine::SourceId::Aladhan;
            rec_.date = engine::Date{0, 0, 0};
            for (uint16_t& m : rec_.minutes) m = kBadMinutes;
        } else if (e.type == JsonType::EndObject) {
            end_record();
        } else {
            ++info_.records_seen;
        }
        return;
    }
    if (!in_record_ || std::strncmp(e.path, "data[].", 7) != 0) return;
    const char* f = e.path + 7;
    if (std::strcmp(f, "date.gregorian.date") == 0) {
        if (e.type != JsonType::String || !parse_dmy(e.text, e.length, '-', rec_.date)) rec_.date = engine::Date{0, 0, 0};
        return;
    }
    if (std::strncmp(f, "timings.", 8) != 0) return;
    const char* key = f + 8;
    for (uint8_t i = 0; i < engine::kVakitCount; ++i) {
        if (std::strcmp(key, kTimeKeys[i]) == 0) {
            // "06:08 (+03)": the first five characters.
            const bool ok = e.type == JsonType::String && !e.overflow && e.length >= 5 &&
                            (e.length == 5 || e.text[5] == ' ');
            rec_.minutes[i] = ok ? parse_hhmm(e.text, 5) : kBadMinutes;
            return;
        }
    }
}

void AladhanParser::end_record() {
    in_record_ = false;
    ++info_.records_seen;
    if (engine::valid(rec_.date)) {
        const engine::DayNumber d = engine::to_day_number(rec_.date);
        const bool before = engine::valid(keep_from_) && d < engine::to_day_number(keep_from_);
        const bool after = engine::valid(keep_to_) && d > engine::to_day_number(keep_to_);
        if (before || after) {
            ++info_.records_dropped;
            return;
        }
    }
    sink_.on_record(rec_);
}

}  // namespace nvp::providers
