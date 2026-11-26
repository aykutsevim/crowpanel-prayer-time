// Plausibility check on altered copies of the recorded answers (spec 007 T028; FR-016, SC-008,
// 04-data-sources §7).
#include <unity.h>

#include <string>
#include <vector>

#include "../support/fixture_file.h"
#include "nvp/providers/emushaf.h"
#include "nvp/providers/imsakiyem.h"
#include "nvp/providers/plausible.h"

using nvp::calc::Coordinates;
using nvp::engine::DayRecord;
using nvp::providers::Plausibility;

namespace {

const Coordinates kAnkara{39.92063, 32.85403};
const Coordinates kEdirne{41.67719, 26.55597};
const Coordinates kHakkari{37.57444, 43.74083};
constexpr int32_t kTr = 10800;

struct Collect : nvp::providers::RecordSink {
    std::vector<DayRecord> records;
    void on_record(const DayRecord& r) override { records.push_back(r); }
};

std::vector<DayRecord> imsakiyem() {
    Collect c;
    nvp::providers::ImsakiyemParser p(nvp::providers::ImsakiyemApi::V2, c);
    const std::string body = nvp_test::read_fixture("imsakiyem/v2_times_9206_2026-11-24_2027-03-31.json");
    p.feed(body.data(), body.size());
    p.finish();
    return c.records;
}

std::vector<DayRecord> emushaf() {
    Collect c;
    nvp::providers::EmushafParser p(c, nvp::engine::Date{2026, 11, 23});
    const std::string body = nvp_test::read_fixture("emushaf/vakitler_9206_2026-11-24.json");
    p.feed(body.data(), body.size());
    p.finish();
    return c.records;
}

void test_published_is_plausible() {
    const std::vector<DayRecord> a = imsakiyem();
    Plausibility p = nvp::providers::check_plausible(a.data(), a.size(), kAnkara, kTr);
    TEST_ASSERT_TRUE(p.checked);
    TEST_ASSERT_TRUE(p.ok);
    const std::vector<DayRecord> b = emushaf();
    p = nvp::providers::check_plausible(b.data(), b.size(), kAnkara, kTr);
    TEST_ASSERT_TRUE(p.ok);
}

void test_shifted_hour_rejected() {
    std::vector<DayRecord> a = imsakiyem();
    for (DayRecord& r : a)
        for (uint16_t& m : r.minutes) m = static_cast<uint16_t>(m + 60);
    const Plausibility p = nvp::providers::check_plausible(a.data(), a.size(), kAnkara, kTr);
    TEST_ASSERT_FALSE(p.ok);
    TEST_ASSERT_TRUE(p.date == a[0].date);
    TEST_ASSERT_EQUAL_INT8(0, p.prayer);
    TEST_ASSERT_TRUE(p.diff >= 59 && p.diff <= 61);
}

// Each record carries the times of the record 60 days later: the dates no longer match the times.
void test_shifted_days_rejected() {
    const std::vector<DayRecord> a = imsakiyem();
    std::vector<DayRecord> b(a.begin(), a.begin() + 60);
    for (size_t i = 0; i < b.size(); ++i)
        for (int k = 0; k < 6; ++k) b[i].minutes[k] = a[i + 60].minutes[k];
    const Plausibility p = nvp::providers::check_plausible(b.data(), b.size(), kAnkara, kTr);
    TEST_ASSERT_FALSE(p.ok);
}

// Ankara's times checked against another district's coordinates.
void test_another_district_rejected() {
    const std::vector<DayRecord> a = imsakiyem();
    TEST_ASSERT_FALSE(nvp::providers::check_plausible(a.data(), a.size(), kEdirne, kTr).ok);
    TEST_ASSERT_FALSE(nvp::providers::check_plausible(a.data(), a.size(), kHakkari, kTr).ok);
}

void test_empty_not_checked() {
    const Plausibility p = nvp::providers::check_plausible(nullptr, 0, kAnkara, kTr);
    TEST_ASSERT_FALSE(p.checked);
    TEST_ASSERT_TRUE(p.ok);
}

void test_batches_agree() {
    const std::vector<DayRecord> a = imsakiyem();
    std::vector<DayRecord> b = emushaf();
    TEST_ASSERT_TRUE(nvp::providers::batches_agree(a.data(), a.size(), b.data(), b.size()));
    b[3].minutes[2] = static_cast<uint16_t>(b[3].minutes[2] + 1);
    TEST_ASSERT_FALSE(nvp::providers::batches_agree(a.data(), a.size(), b.data(), b.size()));
    TEST_ASSERT_FALSE(nvp::providers::batches_agree(a.data(), 0, b.data(), b.size()));
}

}  // namespace

void run_plausible_tests() {
    RUN_TEST(test_published_is_plausible);
    RUN_TEST(test_shifted_hour_rejected);
    RUN_TEST(test_shifted_days_rejected);
    RUN_TEST(test_another_district_rejected);
    RUN_TEST(test_empty_not_checked);
    RUN_TEST(test_batches_agree);
}
