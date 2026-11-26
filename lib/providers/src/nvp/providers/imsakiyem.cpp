#include "nvp/providers/imsakiyem.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "nvp/engine/calendar.h"

namespace nvp::providers {
namespace {

// The six time keys, in vakit order (the identifiers of CLAUDE.md are also the API's keys).
constexpr const char* kTimeKeys[engine::kVakitCount] = {"imsak", "gunes", "ogle", "ikindi", "aksam", "yatsi"};

bool digits(const char* s, size_t n) {
    for (size_t i = 0; i < n; ++i)
        if (s[i] < '0' || s[i] > '9') return false;
    return true;
}

int value(const char* s, size_t n) {
    int v = 0;
    for (size_t i = 0; i < n; ++i) v = v * 10 + (s[i] - '0');
    return v;
}

// A small non-negative integer written as a JSON number or string. -1 when not one.
int32_t small_int(const JsonEvent& e) {
    if (e.type != JsonType::Number && e.type != JsonType::String) return -1;
    if (e.length == 0 || e.length > 9 || !digits(e.text, e.length)) return -1;
    return value(e.text, e.length);
}

bool starts_with(const char* s, const char* prefix) { return std::strncmp(s, prefix, std::strlen(prefix)) == 0; }

}  // namespace

uint16_t parse_hhmm(const char* text, size_t length) {
    if (length != 5 || text[2] != ':' || !digits(text, 2) || !digits(text + 3, 2)) return kBadMinutes;
    int h = value(text, 2);
    int m = value(text + 3, 2);
    if (h > 23 || m > 59) return kBadMinutes;
    return static_cast<uint16_t>(h * 60 + m);
}

bool parse_iso_date(const char* text, size_t length, bool allow_time, engine::Date& out) {
    if (length < 10) return false;
    if (length > 10 && !(allow_time && text[10] == 'T')) return false;
    if (text[4] != '-' || text[7] != '-' || !digits(text, 4) || !digits(text + 5, 2) || !digits(text + 8, 2))
        return false;
    engine::Date d;
    d.year = static_cast<int16_t>(value(text, 4));
    d.month = static_cast<uint8_t>(value(text + 5, 2));
    d.day = static_cast<uint8_t>(value(text + 8, 2));
    if (!engine::valid(d)) return false;
    out = d;
    return true;
}

ImsakiyemParser::ImsakiyemParser(ImsakiyemApi api, RecordSink& sink) : api_(api), sink_(sink), json_(*this) {}

bool ImsakiyemParser::feed(const char* data, size_t n) {
    bool ok = json_.feed(data, n);
    if (!ok) info_.json_error = json_.error();
    return ok;
}

void ImsakiyemParser::finish() {
    info_.complete = json_.finish();
    info_.json_error = json_.error();
    if (api_ == ImsakiyemApi::V1) {
        info_.source_diyanet = info_.records_seen > 0 && diyanet_records_ == info_.records_seen;
    }
}

void ImsakiyemParser::begin_record() {
    in_record_ = true;
    rec_ = engine::DayRecord{};
    rec_.date = engine::Date{0, 0, 0};  // invalid until the date is read
    for (uint16_t& m : rec_.minutes) m = kBadMinutes;
    rec_.source = engine::SourceId::Imsakiyem;
    hijri_seen_ = 0;
}

void ImsakiyemParser::end_record() {
    in_record_ = false;
    if (hijri_seen_ == 0x7 && hijri_[0] >= 1 && hijri_[0] <= 30 && hijri_[1] >= 1 && hijri_[1] <= 12 &&
        hijri_[2] >= 1 && hijri_[2] <= 9999) {
        rec_.hijri.day = static_cast<uint8_t>(hijri_[0]);
        rec_.hijri.month = static_cast<uint8_t>(hijri_[1]);
        rec_.hijri.year = static_cast<uint16_t>(hijri_[2]);
        rec_.hijri.present = true;
    }
    ++info_.records_seen;
    sink_.on_record(rec_);
}

void ImsakiyemParser::set_district(const JsonEvent& e, bool per_record) {
    int32_t id = small_int(e);
    uint32_t d = id < 0 ? 0 : static_cast<uint32_t>(id);
    if (!info_.district_seen) {
        info_.district_seen = true;
        info_.district = d;
    } else if (per_record && d != info_.district) {
        info_.district_mixed = true;
    }
}

// `field` is the path below the record element, for example "hijri.day" or "times.imsak".
void ImsakiyemParser::on_record_field(const char* field, const JsonEvent& e) {
    const bool v1 = api_ == ImsakiyemApi::V1;
    if (std::strcmp(field, "date") == 0) {
        if (e.type != JsonType::String || e.overflow ||
            !parse_iso_date(e.text, e.length, /*allow_time=*/v1, rec_.date)) {
            rec_.date = engine::Date{0, 0, 0};
        }
        if (e.overflow) info_.field_overflow = true;
        return;
    }
    const char* hijri_prefix = v1 ? "hijri_date." : "hijri.";
    if (starts_with(field, hijri_prefix)) {
        const char* f = field + std::strlen(hijri_prefix);
        int slot = std::strcmp(f, "day") == 0 ? 0 : std::strcmp(f, "month") == 0 ? 1 : std::strcmp(f, "year") == 0 ? 2 : -1;
        if (slot >= 0) {
            hijri_[slot] = small_int(e);
            if (hijri_[slot] >= 0) hijri_seen_ |= static_cast<uint8_t>(1u << slot);
        }
        return;
    }
    const char* time_key = field;
    if (v1) {
        if (!starts_with(field, "times.")) {
            if (std::strcmp(field, "district_id._id") == 0) set_district(e, true);
            if (std::strcmp(field, "meta.source") == 0 && e.type == JsonType::String && starts_with(e.text, "Diyanet"))
                ++diyanet_records_;
            return;
        }
        time_key = field + 6;
    }
    for (uint8_t i = 0; i < engine::kVakitCount; ++i) {
        if (std::strcmp(time_key, kTimeKeys[i]) == 0) {
            if (e.overflow) info_.field_overflow = true;
            rec_.minutes[i] = (e.type == JsonType::String && !e.overflow) ? parse_hhmm(e.text, e.length) : kBadMinutes;
            return;
        }
    }
}

void ImsakiyemParser::on_json(const JsonEvent& e) {
    const bool v1 = api_ == ImsakiyemApi::V1;
    const char* records = v1 ? "data" : "data.times";
    const char* element = v1 ? "data[]" : "data.times[]";
    const size_t element_len = std::strlen(element);

    if (std::strcmp(e.path, "success") == 0) {
        info_.success_seen = e.type == JsonType::True || e.type == JsonType::False;
        info_.success = e.type == JsonType::True;
        return;
    }
    if (std::strcmp(e.path, records) == 0 && e.type == JsonType::BeginArray) {
        info_.records_array_seen = true;
        return;
    }
    if (std::strcmp(e.path, element) == 0) {
        if (e.type == JsonType::BeginObject) begin_record();
        if (e.type == JsonType::EndObject && in_record_) end_record();
        if (e.type != JsonType::BeginObject && e.type != JsonType::EndObject) ++info_.records_seen;  // not an object
        return;
    }
    if (in_record_ && std::strncmp(e.path, element, element_len) == 0 && e.path[element_len] == '.') {
        on_record_field(e.path + element_len + 1, e);
        return;
    }
    if (v1) return;
    if (std::strcmp(e.path, "error.code") == 0 && e.type == JsonType::String) {
        std::strncpy(info_.error_code, e.text, sizeof info_.error_code - 1);
        return;
    }
    if (std::strcmp(e.path, "data.location.district_id") == 0) {
        set_district(e, false);
        return;
    }
    if (std::strcmp(e.path, "data.source.type") == 0 && e.type == JsonType::String) {
        info_.source_diyanet = !e.overflow && std::strcmp(e.text, "diyanet") == 0;
    }
}

// --- District detail (spec 007 T033) ---

namespace {

// "39.92063" -> 3992063 (degrees x 100 000, rounded); false when not a number.
bool degrees_e5(const char* text, int32_t& out) {
    char* end = nullptr;
    const double v = std::strtod(text, &end);
    if (end == text || *end != '\0' || v < -180.0 || v > 180.0) return false;
    out = static_cast<int32_t>(v * 100000.0 + (v < 0 ? -0.5 : 0.5));
    return true;
}

}  // namespace

bool DistrictDetail::accepted() const {
    if (!success || !has_coords) return false;
    if (std::strcmp(confidence, "high") != 0 && std::strcmp(confidence, "medium") != 0) return false;
    return lat_e5 >= 3580000 && lat_e5 <= 4220000 && lon_e5 >= 2560000 && lon_e5 <= 4490000;
}

DistrictDetailParser::DistrictDetailParser() : json_(*this) {}

bool DistrictDetailParser::feed(const char* data, size_t n) { return json_.feed(data, n); }

void DistrictDetailParser::finish() {
    if (!json_.finish()) d_.success = false;
    d_.has_coords = d_.has_coords && lat_seen_ && lon_seen_;
}

void DistrictDetailParser::on_json(const JsonEvent& e) {
    if (std::strcmp(e.path, "success") == 0) {
        d_.success = e.type == JsonType::True;
    } else if (std::strcmp(e.path, "error.code") == 0) {
        d_.not_found = e.type == JsonType::String && std::strcmp(e.text, "DISTRICT_NOT_FOUND") == 0;
    } else if (std::strcmp(e.path, "data.coordinates.lat") == 0) {
        lat_seen_ = e.type == JsonType::Number && degrees_e5(e.text, d_.lat_e5);
        d_.has_coords = true;
    } else if (std::strcmp(e.path, "data.coordinates.lon") == 0) {
        lon_seen_ = e.type == JsonType::Number && degrees_e5(e.text, d_.lon_e5);
        d_.has_coords = true;
    } else if (std::strcmp(e.path, "data.coordinates.confidence") == 0 && e.type == JsonType::String && !e.overflow) {
        std::snprintf(d_.confidence, sizeof d_.confidence, "%s", e.text);
    }
}

bool district_detail_url(uint32_t district, char* out, size_t cap) {
    const int n = std::snprintf(out, cap, "https://ezanvakti.imsakiyem.com/api/v2/locations/districts/%lu",
                                static_cast<unsigned long>(district));
    return n > 0 && static_cast<size_t>(n) < cap;
}

}  // namespace nvp::providers
