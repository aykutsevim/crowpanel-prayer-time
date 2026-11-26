#include "nvp/providers/emushaf.h"

#include <cstring>

namespace nvp::providers {
namespace {

constexpr const char* kTimeKeys[engine::kVakitCount] = {"Imsak", "Gunes", "Ogle", "Ikindi", "Aksam", "Yatsi"};

// Reads up to `max` digits; returns -1 when none.
int32_t digits(const char*& p, const char* end, int max) {
    int32_t v = 0;
    int n = 0;
    while (p < end && n < max && *p >= '0' && *p <= '9') {
        v = v * 10 + (*p - '0');
        ++p;
        ++n;
    }
    return n == 0 ? -1 : v;
}

// "D.M.YYYY" -> Hijri date; false when malformed.
bool parse_hijri(const char* text, size_t length, engine::HijriDate& out) {
    const char* p = text;
    const char* end = text + length;
    const int32_t d = digits(p, end, 2);
    if (p >= end || *p++ != '.') return false;
    const int32_t m = digits(p, end, 2);
    if (p >= end || *p++ != '.') return false;
    const int32_t y = digits(p, end, 4);
    if (p != end || d < 1 || d > 30 || m < 1 || m > 12 || y < 1) return false;
    out.day = static_cast<uint8_t>(d);
    out.month = static_cast<uint8_t>(m);
    out.year = static_cast<uint16_t>(y);
    out.present = true;
    return true;
}

}  // namespace

bool parse_dmy(const char* text, size_t length, char sep, engine::Date& out) {
    if (length != 10 || text[2] != sep || text[5] != sep) return false;
    const char* p = text;
    const char* end = text + length;
    const int32_t d = digits(p, end, 2);
    ++p;
    const int32_t m = digits(p, end, 2);
    ++p;
    const int32_t y = digits(p, end, 4);
    if (p != end || d < 0 || m < 0 || y < 0) return false;
    engine::Date r{static_cast<int16_t>(y), static_cast<uint8_t>(m), static_cast<uint8_t>(d)};
    if (!engine::valid(r)) return false;
    out = r;
    return true;
}

EmushafParser::EmushafParser(RecordSink& sink, engine::Date keep_from)
    : sink_(sink), keep_from_(keep_from), json_(*this) {
    info_.place_unnamed = true;
    info_.source_diyanet = true;  // the service republishes Diyanet's monthly table (04 §3.2)
}

bool EmushafParser::feed(const char* data, size_t n) { return json_.feed(data, n); }

void EmushafParser::finish() {
    json_.finish();
    info_.json_error = json_.error();
    info_.complete = json_.complete();
    // A whole, well-formed array is the service's only success signal.
    info_.success_seen = info_.complete && info_.records_array_seen;
    info_.success = info_.success_seen;
}

void EmushafParser::on_json(const JsonEvent& e) {
    if (e.path[0] == '\0') {
        if (e.type == JsonType::BeginArray) info_.records_array_seen = true;
        return;
    }
    if (std::strcmp(e.path, "[]") == 0) {
        if (e.type == JsonType::BeginObject) {
            in_record_ = true;
            rec_ = engine::DayRecord{};
            rec_.source = engine::SourceId::Emushaf;
            rec_.date = engine::Date{0, 0, 0};
            for (uint16_t& m : rec_.minutes) m = kBadMinutes;
        } else if (e.type == JsonType::EndObject) {
            end_record();
        } else {
            ++info_.records_seen;  // an element that is not an object: Malformed in validation
        }
        return;
    }
    if (!in_record_ || std::strncmp(e.path, "[].", 3) != 0) return;
    const char* f = e.path + 3;
    if (std::strcmp(f, "MiladiTarihKisa") == 0) {
        if (e.overflow) info_.field_overflow = true;
        if (e.type != JsonType::String || !parse_dmy(e.text, e.length, '.', rec_.date)) rec_.date = engine::Date{0, 0, 0};
        return;
    }
    if (std::strcmp(f, "HicriTarihKisa") == 0) {
        if (e.type == JsonType::String && !e.overflow) parse_hijri(e.text, e.length, rec_.hijri);
        return;
    }
    for (uint8_t i = 0; i < engine::kVakitCount; ++i) {
        if (std::strcmp(f, kTimeKeys[i]) == 0) {
            if (e.overflow) info_.field_overflow = true;
            rec_.minutes[i] = (e.type == JsonType::String && !e.overflow) ? parse_hhmm(e.text, e.length) : kBadMinutes;
            return;
        }
    }
}

void EmushafParser::end_record() {
    in_record_ = false;
    ++info_.records_seen;
    if (engine::valid(rec_.date) && engine::valid(keep_from_) &&
        engine::to_day_number(rec_.date) < engine::to_day_number(keep_from_)) {
        ++info_.records_dropped;
        return;
    }
    sink_.on_record(rec_);
}

}  // namespace nvp::providers
