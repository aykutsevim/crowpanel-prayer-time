// Screen views of setup, connecting and the status overlay (spec 004 FR-004..FR-007, OD4-2;
// 02 §7.2-§7.4; data-model.md). Written before the code.
#include <unity.h>

#include <cstring>

#include "nvp/net/views.h"
#include "nvp/text/tr.h"

using namespace nvp::net;
namespace tr = nvp::text::tr;

static void test_setup_view() {
    SetupView v = make_setup_view("NamazVakti-7F3A", "48271935", 0, FailureClass::None);
    TEST_ASSERT_EQUAL_STRING("NamazVakti-7F3A", v.ap_ssid);
    TEST_ASSERT_EQUAL_STRING("4827 1935", v.ap_pass_grouped);
    TEST_ASSERT_EQUAL_STRING("WIFI:T:WPA;S:NamazVakti-7F3A;P:48271935;;", v.qr_payload);
    TEST_ASSERT_EQUAL_STRING(tr::kFooterWaiting, footer_text(v));
    TEST_ASSERT_TRUE(footer_colour(v) == FooterColour::Muted);

    v = make_setup_view("NamazVakti-7F3A", "48271935", 1, FailureClass::None);
    TEST_ASSERT_EQUAL_STRING(tr::kFooterPhoneJoined, footer_text(v));
    TEST_ASSERT_TRUE(footer_colour(v) == FooterColour::Ink);

    const FailureClass classes[3] = {FailureClass::WrongPassword, FailureClass::NotFound, FailureClass::Other};
    const char* texts[3] = {tr::kFooterWrongPassword, tr::kFooterNotFound, tr::kFooterOther};
    for (int i = 0; i < 3; ++i) {  // a failure wins over a joined phone until the next attempt (FR-005)
        v = make_setup_view("NamazVakti-7F3A", "48271935", 1, classes[i]);
        TEST_ASSERT_EQUAL_STRING(texts[i], footer_text(v));
        TEST_ASSERT_TRUE(footer_colour(v) == FooterColour::Alert);
    }
}

static void test_connecting_view() {
    ConnectingView v = make_connecting_view("Ev Ağı 🏠", false);
    TEST_ASSERT_EQUAL_STRING("Ev Ağı ?", v.ssid);
    TEST_ASSERT_EQUAL_STRING(tr::kConnectingTitle, connecting_title(v));
    TEST_ASSERT_EQUAL_STRING(tr::kConnectingNote, connecting_note(v));
    v = make_connecting_view("EvAgi_2.4G", true);
    TEST_ASSERT_EQUAL_STRING("EvAgi_2.4G", v.ssid);
    TEST_ASSERT_EQUAL_STRING(tr::kConnectedTitle, connecting_title(v));
    TEST_ASSERT_EQUAL_STRING(tr::kConnectedNote, connecting_note(v));
}

static void test_overlay_view() {
    OverlayInput in;
    in.province = "İSTANBUL";
    in.district = "SULTANBEYLİ";
    in.wifi_up = true;
    in.ssid = "EvAgi_2.4G";
    in.rssi = -54;
    in.ip = "192.168.1.37";
    in.version = "1.0.0";
    in.show_qr = false;  // device, OD4-2
    OverlayView v = make_overlay_view(in);
    TEST_ASSERT_EQUAL_STRING("İstanbul, Sultanbeyli", v.location);
    TEST_ASSERT_EQUAL_STRING(tr::kDash, v.source);
    TEST_ASSERT_EQUAL_STRING(tr::kDash, v.last_update);
    TEST_ASSERT_EQUAL_STRING(tr::kDash, v.last_day);
    TEST_ASSERT_EQUAL_STRING(tr::kDash, v.pin);
    TEST_ASSERT_EQUAL_STRING("EvAgi_2.4G, iyi", v.wifi);
    TEST_ASSERT_EQUAL_STRING("192.168.1.37", v.address);
    TEST_ASSERT_EQUAL_STRING("1.0.0", v.version);
    TEST_ASSERT_FALSE(v.show_qr);
    TEST_ASSERT_FALSE(v.confirm);

    in.show_qr = true;  // fixture 17
    v = make_overlay_view(in);
    TEST_ASSERT_TRUE(v.show_qr);
    TEST_ASSERT_EQUAL_STRING("http://192.168.1.37/", v.qr_url);

    in.wifi_up = false;
    v = make_overlay_view(in);
    TEST_ASSERT_EQUAL_STRING(tr::kWifiNotConnected, v.wifi);
    TEST_ASSERT_EQUAL_STRING(tr::kDash, v.address);
    TEST_ASSERT_FALSE(v.show_qr);  // hidden when Wi-Fi is down (02 §7.4)

    in.province = nullptr;
    v = make_overlay_view(in);
    TEST_ASSERT_EQUAL_STRING(tr::kDash, v.location);
}

static void test_overlay_store_rows() {  // spec 006 FR-014, 02 §7.4 formats, 02 §9.5 names
    OverlayInput in;
    in.has_source = true;
    in.source = nvp::engine::SourceId::Imsakiyem;
    in.has_last_update = true;
    in.last_update_local =
        static_cast<int64_t>(nvp::engine::to_day_number(nvp::engine::Date{2026, 11, 17})) * 86400 + 3 * 3600;
    in.has_last_day = true;
    in.last_day = nvp::engine::Date{2026, 12, 31};
    OverlayView v = make_overlay_view(in);
    TEST_ASSERT_EQUAL_STRING("Diyanet (imsakiyem)", v.source);
    TEST_ASSERT_EQUAL_STRING("17.11.2026 03:00", v.last_update);
    TEST_ASSERT_EQUAL_STRING("31.12.2026", v.last_day);
    TEST_ASSERT_EQUAL_STRING("Diyanet (resmî servis)", source_display_name(nvp::engine::SourceId::Diyanet));
    TEST_ASSERT_EQUAL_STRING("Diyanet (emushaf)", source_display_name(nvp::engine::SourceId::Emushaf));
    TEST_ASSERT_EQUAL_STRING("Hesaplama (Aladhan)", source_display_name(nvp::engine::SourceId::Aladhan));
    TEST_ASSERT_EQUAL_STRING("Hesaplama (cihaz)", source_display_name(nvp::engine::SourceId::Local));
    in.has_source = in.has_last_update = in.has_last_day = false;
    v = make_overlay_view(in);
    TEST_ASSERT_EQUAL_STRING(tr::kDash, v.source);
    TEST_ASSERT_EQUAL_STRING(tr::kDash, v.last_update);
    TEST_ASSERT_EQUAL_STRING(tr::kDash, v.last_day);
}

static void test_city_name() {  // M2 (02 §4.1)
    char c[48];
    city_name("İSTANBUL", "SULTANBEYLİ", c, sizeof c);
    TEST_ASSERT_EQUAL_STRING("Sultanbeyli", c);
    city_name("ANKARA", "ANKARA", c, sizeof c);  // the province centre
    TEST_ASSERT_EQUAL_STRING("Ankara", c);
    city_name(nullptr, nullptr, c, sizeof c);
    TEST_ASSERT_EQUAL_STRING("", c);
}

static void test_fixture_views() {  // reference renders 15-17 (02 §11)
    const SetupView s = fixture_setup_view();
    TEST_ASSERT_EQUAL_STRING("NamazVakti-7F3A", s.ap_ssid);
    TEST_ASSERT_EQUAL_STRING("4827 1935", s.ap_pass_grouped);
    TEST_ASSERT_EQUAL_STRING(tr::kFooterWaiting, footer_text(s));
    const ConnectingView c = fixture_connecting_view();
    TEST_ASSERT_EQUAL_STRING("EvAgi_2.4G", c.ssid);
    TEST_ASSERT_FALSE(c.connected);
    const OverlayView o = fixture_overlay_view();
    TEST_ASSERT_EQUAL_STRING("İstanbul, Sultanbeyli", o.location);
    TEST_ASSERT_EQUAL_STRING("Diyanet (imsakiyem)", o.source);
    TEST_ASSERT_EQUAL_STRING("17.11.2026 03:00", o.last_update);
    TEST_ASSERT_EQUAL_STRING("31.12.2026", o.last_day);
    TEST_ASSERT_EQUAL_STRING("EvAgi_2.4G, iyi", o.wifi);
    TEST_ASSERT_EQUAL_STRING("192.168.1.37", o.address);
    TEST_ASSERT_EQUAL_STRING("482 913", o.pin);
    TEST_ASSERT_EQUAL_STRING("1.0.0", o.version);
    TEST_ASSERT_TRUE(o.show_qr);
    TEST_ASSERT_EQUAL_STRING("http://192.168.1.37/", o.qr_url);
}

void run_views_tests() {
    RUN_TEST(test_fixture_views);
    RUN_TEST(test_setup_view);
    RUN_TEST(test_connecting_view);
    RUN_TEST(test_overlay_view);
    RUN_TEST(test_overlay_store_rows);
    RUN_TEST(test_city_name);
}
