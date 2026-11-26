// The adhan-js port against adhan's own shared test data and against the published Ankara table
// (spec 007 T024, research R3, FR-017).
#include <unity.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "../support/fixture_file.h"
#include "nvp/calc/prayer_times.h"
#include "nvp/engine/calendar.h"
#include "nvp/providers/imsakiyem.h"

using nvp::calc::Coordinates;
using nvp::calc::Params;
using nvp::engine::Date;

namespace {

std::string read_calc(const char* name) {
    std::string path = std::string("test/fixtures/calc/adhan/") + name;
    std::FILE* f = std::fopen(path.c_str(), "rb");
    if (f == nullptr) return std::string();
    std::string out;
    char buf[4096];
    size_t n;
    while ((n = std::fread(buf, 1, sizeof buf, f)) > 0) out.append(buf, n);
    std::fclose(f);
    return out;
}

// The string value after `"key": "` from `from`; empty when absent.
std::string value(const std::string& s, size_t from, const char* key, size_t* end = nullptr) {
    const std::string k = std::string("\"") + key + "\"";
    size_t at = s.find(k, from);
    if (at == std::string::npos) return std::string();
    at = s.find('"', at + k.size());
    const size_t stop = s.find('"', at + 1);
    if (end != nullptr) *end = stop;
    return s.substr(at + 1, stop - at - 1);
}

double number(const std::string& s, const char* key) {
    const std::string k = std::string("\"") + key + "\"";
    size_t at = s.find(k);
    at = s.find(':', at);
    return std::strtod(s.c_str() + at + 1, nullptr);
}

// "6:33 AM" -> minutes after midnight.
int clock_minutes(const std::string& t) {
    int h = 0, m = 0;
    char ap[3] = {};
    std::sscanf(t.c_str(), "%d:%d %2s", &h, &m, ap);
    if (h == 12) h = 0;
    if (ap[0] == 'P') h += 12;
    return h * 60 + m;
}

struct Case {
    const char* file;
    Params (*method)();
    int32_t offset_s;  // fixed, except Tehran (below)
};

int32_t tehran_offset(Date d) {
    // Iran 2018: +04:30 from 22 March to 21 September, else +03:30.
    const int n = nvp::engine::to_day_number(d);
    const bool dst = n >= nvp::engine::to_day_number(Date{2018, 3, 22}) && n <= nvp::engine::to_day_number(Date{2018, 9, 21});
    return dst ? 4 * 3600 + 1800 : 3 * 3600 + 1800;
}

void check_file(const Case& c) {
    const std::string s = read_calc(c.file);
    TEST_ASSERT_FALSE_MESSAGE(s.empty(), c.file);
    const Coordinates where{number(s, "latitude"), number(s, "longitude")};
    const bool has_variance = s.find("\"variance\"") != std::string::npos;
    const int variance = has_variance ? static_cast<int>(number(s, "variance")) : 0;
    const char* keys[6] = {"fajr", "sunrise", "dhuhr", "asr", "maghrib", "isha"};
    int days = 0, worst = 0;
    size_t at = s.find("\"times\"");
    for (;;) {
        size_t end = 0;
        const std::string date = value(s, at, "date", &end);
        if (date.empty()) break;
        at = end;
        Date d{static_cast<int16_t>(std::atoi(date.c_str())), static_cast<uint8_t>(std::atoi(date.c_str() + 5)),
               static_cast<uint8_t>(std::atoi(date.c_str() + 8))};
        const int32_t offset = c.offset_s == 0 ? tehran_offset(d) : c.offset_s;
        uint16_t got[6];
        TEST_ASSERT_TRUE_MESSAGE(nvp::calc::prayer_times_local(where, d, c.method(), offset, got), date.c_str());
        for (int i = 0; i < 6; ++i) {
            const int want = clock_minutes(value(s, at, keys[i]));
            const int diff = std::abs(want - got[i]);
            if (diff > worst) worst = diff;
            char msg[96];
            std::snprintf(msg, sizeof msg, "%s %s %s: want %d got %d", c.file, date.c_str(), keys[i], want, got[i]);
            TEST_ASSERT_TRUE_MESSAGE(diff <= variance, msg);
        }
        ++days;
    }
    std::printf("adhan %s: %d days, worst difference %d min (variance %d)\n", c.file, days, worst, variance);
    TEST_ASSERT_TRUE(days >= 12);
}

void test_adhan_shared_times() {
    const Case cases[] = {
        {"Ankara-Turkey.json", nvp::calc::turkey, 3 * 3600},
        {"Doha-Qatar.json", nvp::calc::qatar, 3 * 3600},
        {"Dubai-Gulf.json", nvp::calc::dubai, 4 * 3600},
        {"Kuwait City-Kuwait.json", nvp::calc::kuwait, 3 * 3600},
        {"Makkah-UmmAlQura.json", nvp::calc::umm_al_qura, 3 * 3600},
        {"Singapore-Singapore.json", nvp::calc::singapore, 8 * 3600},
        {"Tehran-Tehran.json", nvp::calc::tehran, 0},
    };
    for (const Case& c : cases) check_file(c);
}

// The published table for Ankara (128 days, İmsakiyem) against the calculation with İmsakiyem's
// coordinates for 9206: every time within ±2 minutes; per-prayer median and range printed (R3).
struct Collect : nvp::providers::RecordSink {
    std::vector<nvp::engine::DayRecord> records;
    void on_record(const nvp::engine::DayRecord& r) override { records.push_back(r); }
};

void test_ankara_published_within_two_minutes() {
    Collect c;
    nvp::providers::ImsakiyemParser p(nvp::providers::ImsakiyemApi::V2, c);
    const std::string body = nvp_test::read_fixture("imsakiyem/v2_times_9206_2026-11-24_2027-03-31.json");
    p.feed(body.data(), body.size());
    p.finish();
    TEST_ASSERT_EQUAL_UINT32(128, c.records.size());
    const Coordinates ankara{39.92063, 32.85403};
    std::vector<int> diffs[6];
    for (const nvp::engine::DayRecord& r : c.records) {
        uint16_t got[6];
        TEST_ASSERT_TRUE(nvp::calc::prayer_times_local(ankara, r.date, nvp::calc::turkey(), 10800, got));
        for (int i = 0; i < 6; ++i) {
            const int d = static_cast<int>(r.minutes[i]) - static_cast<int>(got[i]);
            diffs[i].push_back(d);
            TEST_ASSERT_TRUE(std::abs(d) <= 2);
        }
    }
    const char* names[6] = {"imsak", "gunes", "ogle", "ikindi", "aksam", "yatsi"};
    for (int i = 0; i < 6; ++i) {
        std::vector<int> v = diffs[i];
        std::sort(v.begin(), v.end());
        std::printf("ankara %s: median %+d, range %+d..%+d (published minus calculated)\n", names[i], v[v.size() / 2],
                    v.front(), v.back());
    }
}

}  // namespace

void run_prayer_times_tests() {
    RUN_TEST(test_adhan_shared_times);
    RUN_TEST(test_ankara_published_within_two_minutes);
}
