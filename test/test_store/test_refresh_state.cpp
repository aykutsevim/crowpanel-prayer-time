// The persisted refresh state (spec 007 T004, research R7, FR-014, DATA-07).
#include <unity.h>

#include <cstring>

#include "../support/fake_fs.h"
#include "nvp/store/refresh_state.h"

using nvp::store::LoadResult;
using nvp::store::RefreshOutcome;
using nvp::store::RefreshState;
using nvp::store::SaveResult;

namespace {

RefreshState sample(int64_t t) {
    RefreshState s;
    s.last_success_utc = t;
    s.last_attempt_utc = t + 60;
    s.last_outcome = RefreshOutcome::Partial;
    s.retry_step = 3;
    s.next_retry_utc = t + 3600;
    for (size_t i = 0; i < nvp::store::kProviderSlots; ++i) {
        s.providers[i].failures = static_cast<uint8_t>(i + 1);
        s.providers[i].cool_down_until = t + 1800 * static_cast<int64_t>(i);
        s.providers[i].skip_until = t + 86400 * static_cast<int64_t>(i);
    }
    s.calibration.valid = true;
    const int8_t off[6] = {0, -1, 2, -3, 4, -10};
    std::memcpy(s.calibration.offsets, off, sizeof off);
    s.calibration.days = 30;
    s.calibration.district = 9206;
    s.calibration.hijri_offset = 1;
    s.coords_suspect = true;
    return s;
}

void test_round_trip() {
    nvp_test::FakeFs fs;
    const RefreshState a = sample(1795600000);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SaveResult::Ok), static_cast<int>(nvp::store::save_refresh_state(fs, a)));
    RefreshState b;
    TEST_ASSERT_EQUAL_INT(static_cast<int>(LoadResult::Ok), static_cast<int>(nvp::store::load_refresh_state(fs, b)));
    TEST_ASSERT_TRUE(a == b);
}

void test_missing_gives_defaults() {
    nvp_test::FakeFs fs;
    RefreshState b = sample(5);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(LoadResult::Missing), static_cast<int>(nvp::store::load_refresh_state(fs, b)));
    TEST_ASSERT_TRUE(b == RefreshState{});
}

// Damage one byte at `offset` of the saved file and expect `want` and the defaults.
void damaged(size_t offset, uint8_t value, LoadResult want) {
    nvp_test::FakeFs fs;
    nvp::store::save_refresh_state(fs, sample(1795600000));
    std::string& bytes = fs.files[nvp::store::kRefreshStateFile];
    TEST_ASSERT_TRUE(offset < bytes.size());
    bytes[offset] = static_cast<char>(value);
    RefreshState b = sample(1);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(want), static_cast<int>(nvp::store::load_refresh_state(fs, b)));
    TEST_ASSERT_TRUE(b == RefreshState{});
}

void test_bad_magic_version_crc() {
    damaged(0, 'X', LoadResult::BadMagic);
    damaged(4, 9, LoadResult::BadVersion);
    damaged(20, 0xA5, LoadResult::BadCrc);
}

void test_truncated() {
    nvp_test::FakeFs fs;
    nvp::store::save_refresh_state(fs, sample(1795600000));
    fs.files[nvp::store::kRefreshStateFile].resize(10);
    RefreshState b;
    TEST_ASSERT_EQUAL_INT(static_cast<int>(LoadResult::Truncated), static_cast<int>(nvp::store::load_refresh_state(fs, b)));
}

// A cut at every call of a save over an existing state: exactly the old or the new state.
void test_cut_at_every_call() {
    const RefreshState old_state = sample(1000), new_state = sample(2000);
    int total = 0;
    {
        nvp_test::FakeFs fs;
        nvp::store::save_refresh_state(fs, old_state);
        fs.reset_counter();
        nvp::store::save_refresh_state(fs, new_state);
        total = fs.calls;
    }
    TEST_ASSERT_TRUE(total >= 6);
    int saw_old = 0, saw_new = 0;
    for (int n = 1; n <= total; ++n) {
        nvp_test::FakeFs fs;
        nvp::store::save_refresh_state(fs, old_state);
        fs.power_cut_at(n);
        nvp::store::save_refresh_state(fs, new_state);
        fs.remount();
        RefreshState b;
        TEST_ASSERT_EQUAL_INT(static_cast<int>(LoadResult::Ok), static_cast<int>(nvp::store::load_refresh_state(fs, b)));
        if (b == old_state) ++saw_old;
        else if (b == new_state) ++saw_new;
        else TEST_FAIL_MESSAGE("a mix of old and new");
        TEST_ASSERT_FALSE(fs.exists(nvp::store::kRefreshStateTemp));
    }
    TEST_ASSERT_EQUAL_INT(total, saw_old + saw_new);
    TEST_ASSERT_TRUE(saw_old > 0);
}

}  // namespace

void run_refresh_state_tests() {
    RUN_TEST(test_round_trip);
    RUN_TEST(test_missing_gives_defaults);
    RUN_TEST(test_bad_magic_version_crc);
    RUN_TEST(test_truncated);
    RUN_TEST(test_cut_at_every_call);
}
