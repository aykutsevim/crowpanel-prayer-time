// Turkish strings and formatting (FR-015 to FR-018, 02-ui-spec.md §9, research R4, R5).
// Every expected string is copied from 02-ui-spec.md. Written before the code.
#include <unity.h>

#include <cstring>

#include "nvp/text/format.h"
#include "nvp/text/tr.h"

using namespace nvp::text;
using nvp::engine::Date;
using nvp::engine::HijriDate;

static void test_vakit_names() {
    const char* expected[6] = {"İmsak", "Güneş", "Öğle", "İkindi", "Akşam", "Yatsı"};
    for (int k = 0; k < 6; ++k) TEST_ASSERT_EQUAL_STRING(expected[k], tr::kVakitNames[k]);
}

static void test_period_names_and_labels() {  // 02 §9.2
    const char* names[6] = {"İmsak vakti", "Kuşluk vakti", "Öğle vakti", "İkindi vakti", "Akşam vakti", "Yatsı vakti"};
    const char* labels[6] = {"Güneşe kalan süre",  "Öğleye kalan süre",  "İkindiye kalan süre",
                             "Akşama kalan süre",  "Yatsıya kalan süre", "İmsaka kalan süre"};
    for (int k = 0; k < 6; ++k) {
        TEST_ASSERT_EQUAL_STRING(names[k], tr::kPeriodNames[k]);
        TEST_ASSERT_EQUAL_STRING(labels[k], tr::kCountdownLabels[k]);
    }
}

static void test_notes() {  // 02 §9.3
    TEST_ASSERT_EQUAL_STRING("vakit girdi", tr::kVakitGirdi);
    TEST_ASSERT_EQUAL_STRING("şu an", tr::kSuAn);
    TEST_ASSERT_EQUAL_STRING("yarın", tr::kYarin);
}

static void test_months_weekdays_hijri() {  // 02 §9.1
    const char* months[13] = {"",       "Ocak",   "Şubat",  "Mart",  "Nisan", "Mayıs", "Haziran",
                              "Temmuz", "Ağustos", "Eylül", "Ekim",  "Kasım", "Aralık"};
    const char* days[7] = {"Pazartesi", "Salı", "Çarşamba", "Perşembe", "Cuma", "Cumartesi", "Pazar"};
    const char* hijri[13] = {"",          "Muharrem",      "Safer",  "Rebiülevvel", "Rebiülahir", "Cemaziyelevvel",
                             "Cemaziyelahir", "Recep",     "Şaban",  "Ramazan",     "Şevval",     "Zilkade",
                             "Zilhicce"};
    for (int m = 1; m <= 12; ++m) {
        TEST_ASSERT_EQUAL_STRING(months[m], tr::kMonths[m]);
        TEST_ASSERT_EQUAL_STRING(hijri[m], tr::kHijriMonths[m]);
    }
    for (int d = 0; d < 7; ++d) TEST_ASSERT_EQUAL_STRING(days[d], tr::kWeekdays[d]);
}

static void test_apostrophe_is_u2019() {  // constitution III
    const char* p = std::strstr(tr::kLessThanAMinute, "dk");
    TEST_ASSERT_NOT_NULL(p);
    const unsigned char* b = reinterpret_cast<const unsigned char*>(p + 2);
    TEST_ASSERT_EQUAL_HEX8(0xE2, b[0]);
    TEST_ASSERT_EQUAL_HEX8(0x80, b[1]);
    TEST_ASSERT_EQUAL_HEX8(0x99, b[2]);
}

static void test_hhmm() {
    char out[8];
    format_hhmm(0, out, sizeof out);
    TEST_ASSERT_EQUAL_STRING("00:00", out);
    format_hhmm(774, out, sizeof out);
    TEST_ASSERT_EQUAL_STRING("12:54", out);
    TEST_ASSERT_EQUAL_UINT32(5, format_hhmm(1439, out, sizeof out));
    TEST_ASSERT_EQUAL_STRING("23:59", out);
}

static void test_hhmmss() {
    char out[12];
    format_hhmmss(0, out, sizeof out);
    TEST_ASSERT_EQUAL_STRING("00:00:00", out);
    format_hhmmss(3293, out, sizeof out);
    TEST_ASSERT_EQUAL_STRING("00:54:53", out);
    format_hhmmss(39780, out, sizeof out);
    TEST_ASSERT_EQUAL_STRING("11:03:00", out);
    TEST_ASSERT_EQUAL_UINT32(8, format_hhmmss(86399, out, sizeof out));
    TEST_ASSERT_EQUAL_STRING("23:59:59", out);
}

static void test_duration_notes() {  // 02 §9.1, floor to whole minutes
    char out[32];
    format_duration_note(0, out, sizeof out);
    TEST_ASSERT_EQUAL_STRING("1 dk’dan az sonra", out);
    format_duration_note(59, out, sizeof out);
    TEST_ASSERT_EQUAL_STRING("1 dk’dan az sonra", out);
    format_duration_note(60, out, sizeof out);
    TEST_ASSERT_EQUAL_STRING("1 dk sonra", out);
    format_duration_note(3599, out, sizeof out);
    TEST_ASSERT_EQUAL_STRING("59 dk sonra", out);
    format_duration_note(3600, out, sizeof out);
    TEST_ASSERT_EQUAL_STRING("1 sa sonra", out);
    format_duration_note(3660, out, sizeof out);
    TEST_ASSERT_EQUAL_STRING("1 sa 1 dk sonra", out);
    format_duration_note(36300, out, sizeof out);
    TEST_ASSERT_EQUAL_STRING("10 sa 5 dk sonra", out);
    format_duration_note(86340, out, sizeof out);
    TEST_ASSERT_EQUAL_STRING("23 sa 59 dk sonra", out);
}

static void test_dates() {
    char out[48];
    format_date(Date{2026, 11, 17}, out, sizeof out);
    TEST_ASSERT_EQUAL_STRING("17 Kasım 2026 Salı", out);
    format_date(Date{2026, 12, 31}, out, sizeof out);
    TEST_ASSERT_EQUAL_STRING("31 Aralık 2026 Perşembe", out);
    format_date(Date{2027, 1, 1}, out, sizeof out);
    TEST_ASSERT_EQUAL_STRING("1 Ocak 2027 Cuma", out);
    format_date(Date{2024, 2, 29}, out, sizeof out);
    TEST_ASSERT_EQUAL_STRING("29 Şubat 2024 Perşembe", out);
}

static void test_hijri() {
    char out[40];
    format_hijri(HijriDate{7, 6, 1448, true}, out, sizeof out);
    TEST_ASSERT_EQUAL_STRING("7 Cemaziyelahir 1448", out);
    TEST_ASSERT_EQUAL_UINT32(0, format_hijri(HijriDate{7, 6, 1448, false}, out, sizeof out));
    TEST_ASSERT_EQUAL_STRING("", out);
    TEST_ASSERT_EQUAL_UINT32(0, format_hijri(HijriDate{7, 0, 1448, true}, out, sizeof out));
    TEST_ASSERT_EQUAL_UINT32(0, format_hijri(HijriDate{7, 13, 1448, true}, out, sizeof out));
}

static void test_small_buffers_give_empty_text() {
    char out[4] = "xyz";
    TEST_ASSERT_EQUAL_UINT32(0, format_hhmm(754, out, sizeof out));
    TEST_ASSERT_EQUAL_STRING("", out);
    char six[6];
    TEST_ASSERT_EQUAL_UINT32(5, format_hhmm(754, six, sizeof six));  // exactly fits
    char eight[8] = "x";
    TEST_ASSERT_EQUAL_UINT32(0, format_hhmmss(3293, eight, sizeof eight));
    TEST_ASSERT_EQUAL_STRING("", eight);
    char ten[10] = "x";
    TEST_ASSERT_EQUAL_UINT32(0, format_duration_note(3660, ten, sizeof ten));
    TEST_ASSERT_EQUAL_STRING("", ten);
    char twelve[12] = "x";
    TEST_ASSERT_EQUAL_UINT32(0, format_date(Date{2026, 11, 17}, twelve, sizeof twelve));
    TEST_ASSERT_EQUAL_STRING("", twelve);
    TEST_ASSERT_EQUAL_UINT32(0, format_hijri(HijriDate{7, 6, 1448, true}, twelve, sizeof twelve));
    TEST_ASSERT_EQUAL_STRING("", twelve);
    TEST_ASSERT_EQUAL_UINT32(0, format_hhmm(754, nullptr, 0));
}

void run_text_tests() {
    RUN_TEST(test_vakit_names);
    RUN_TEST(test_period_names_and_labels);
    RUN_TEST(test_notes);
    RUN_TEST(test_months_weekdays_hijri);
    RUN_TEST(test_apostrophe_is_u2019);
    RUN_TEST(test_hhmm);
    RUN_TEST(test_hhmmss);
    RUN_TEST(test_duration_notes);
    RUN_TEST(test_dates);
    RUN_TEST(test_hijri);
    RUN_TEST(test_small_buffers_give_empty_text);
}
