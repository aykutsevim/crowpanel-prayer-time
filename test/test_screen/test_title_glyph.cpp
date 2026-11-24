// Turkish title case and the glyph filter (spec 004 02 §7.3, research R8, R13). Written before the code.
#include <unity.h>

#include "nvp/text/glyph_filter.h"
#include "nvp/text/title_case.h"

using namespace nvp::text;

static void expect_title(const char* in, const char* want) {
    char out[64];
    title_case_tr(in, out, sizeof out);
    TEST_ASSERT_EQUAL_STRING(want, out);
}

static void test_title_case() {
    expect_title("SULTANBEYLİ", "Sultanbeyli");
    expect_title("AĞRI", "Ağrı");
    expect_title("KÂĞITHANE", "Kâğıthane");
    expect_title("IĞDIR", "Iğdır");
    expect_title("19 MAYIS", "19 Mayıs");
    expect_title("AFYONKARAHİSAR", "Afyonkarahisar");
    expect_title("ÇANKAYA", "Çankaya");
    expect_title("ŞEBİNKARAHİSAR", "Şebinkarahisar");
    expect_title("ÜSKÜDAR", "Üsküdar");
    expect_title("ÖDEMİŞ", "Ödemiş");
    expect_title("ARNAVUTKOY", "Arnavutkoy");
    expect_title("MERKEZ-KUZEY (YENİ).A", "Merkez-Kuzey (Yeni).A");
    expect_title("İSTANBUL", "İstanbul");
    char small[4];
    TEST_ASSERT_EQUAL_UINT32(0, title_case_tr("İSTANBUL", small, sizeof small));  // too small: empty
    TEST_ASSERT_EQUAL_STRING("", small);
}

static void test_glyph_filter() {
    char out[64];
    filter_display_glyphs("Ev Ağı 🏠 ÂÎÛ_#@", out, sizeof out);
    TEST_ASSERT_EQUAL_STRING("Ev Ağı ? ÂÎÛ_#@", out);
    filter_display_glyphs("日本", out, sizeof out);
    TEST_ASSERT_EQUAL_STRING("??", out);
    filter_display_glyphs("a\x80z", out, sizeof out);  // stray continuation byte
    TEST_ASSERT_EQUAL_STRING("a?z", out);
    filter_display_glyphs("tab\there", out, sizeof out);  // control characters
    TEST_ASSERT_EQUAL_STRING("tab?here", out);
    TEST_ASSERT_TRUE(display_has_glyph('~'));
    TEST_ASSERT_TRUE(display_has_glyph(0x0131));   // ı
    TEST_ASSERT_FALSE(display_has_glyph(0x00E9));  // é
}

void run_title_glyph_tests() {
    RUN_TEST(test_title_case);
    RUN_TEST(test_glyph_filter);
}
