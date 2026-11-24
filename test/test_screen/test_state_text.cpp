// Status-row and hero strings of the system states (02-ui-spec.md §6, §9.4; spec 003 FR-011).
// Every expected string is copied from 02-ui-spec.md. Written before the code.
#include <unity.h>

#include "nvp/screen/status_text.h"
#include "nvp/text/tr.h"

using namespace nvp::screen;
namespace tr = nvp::text::tr;

static void test_constants() {
    TEST_ASSERT_EQUAL_STRING("Saat alınamadı, Wi-Fi bekleniyor", tr::kStatusNoTime);
    TEST_ASSERT_EQUAL_STRING("Vakitler alınamadı", tr::kStatusNoData);
    TEST_ASSERT_EQUAL_STRING("Yaklaşık vakitler, hesaplama ile", tr::kStatusCalculated);
    TEST_ASSERT_EQUAL_STRING("Çevrimdışı, kayıtlı vakitler kullanılıyor", tr::kStatusOffline);
    TEST_ASSERT_EQUAL_STRING("Saat eşitleniyor", tr::kHeroNoTimeTitle);
    TEST_ASSERT_EQUAL_STRING("Saat bekleniyor", tr::kHeroNoTimeDate);
    TEST_ASSERT_EQUAL_STRING("Wi-Fi bağlantısı bekleniyor", tr::kHeroNoTimeLabel);
    TEST_ASSERT_EQUAL_STRING("Vakitler alınamadı", tr::kHeroNoDataTitle);
    TEST_ASSERT_EQUAL_STRING("Wi-Fi ve konum ayarını kontrol edin", tr::kHeroNoDataLabel);
    TEST_ASSERT_EQUAL_STRING("Kerahat vakti", tr::kKerahatLegend);
}

static void test_status_text_per_state() {
    char out[64];
    status_text(SystemState::NoTime, "03:00", 0, out, sizeof out);
    TEST_ASSERT_EQUAL_STRING("Saat alınamadı, Wi-Fi bekleniyor", out);
    status_text(SystemState::NoData, "03:00", 0, out, sizeof out);
    TEST_ASSERT_EQUAL_STRING("Vakitler alınamadı", out);
    status_text(SystemState::Calculated, "03:00", 0, out, sizeof out);
    TEST_ASSERT_EQUAL_STRING("Yaklaşık vakitler, hesaplama ile", out);
    status_text(SystemState::CacheEnding, "03:00", 5, out, sizeof out);
    TEST_ASSERT_EQUAL_STRING("Kayıtlı vakitler 5 gün sonra bitiyor", out);  // reference 11
    status_text(SystemState::CacheEnding, "03:00", 0, out, sizeof out);
    TEST_ASSERT_EQUAL_STRING("Kayıtlı vakitler bugün bitiyor", out);
    status_text(SystemState::Offline, "03:00", 0, out, sizeof out);
    TEST_ASSERT_EQUAL_STRING("Çevrimdışı, kayıtlı vakitler kullanılıyor", out);
    status_text(SystemState::Ok, "03:00", 0, out, sizeof out);
    TEST_ASSERT_EQUAL_STRING("Son güncelleme 03:00", out);
    status_text(SystemState::Ok, "16.11", 0, out, sizeof out);
    TEST_ASSERT_EQUAL_STRING("Son güncelleme 16.11", out);
}

static void test_alert_colour_states() {  // 02 §6 colour column
    TEST_ASSERT_TRUE(status_is_alert(SystemState::NoTime));
    TEST_ASSERT_TRUE(status_is_alert(SystemState::NoData));
    TEST_ASSERT_TRUE(status_is_alert(SystemState::Calculated));
    TEST_ASSERT_TRUE(status_is_alert(SystemState::CacheEnding));
    TEST_ASSERT_FALSE(status_is_alert(SystemState::Offline));
    TEST_ASSERT_FALSE(status_is_alert(SystemState::Ok));
}

static void test_icons() {  // 02 §6 icon column
    TEST_ASSERT_EQUAL_INT((int)StatusIcon::WifiOff, (int)status_icon(SystemState::NoTime));
    TEST_ASSERT_EQUAL_INT((int)StatusIcon::WifiOff, (int)status_icon(SystemState::NoData));
    TEST_ASSERT_EQUAL_INT((int)StatusIcon::Approx, (int)status_icon(SystemState::Calculated));
    TEST_ASSERT_EQUAL_INT((int)StatusIcon::WifiOff, (int)status_icon(SystemState::CacheEnding));
    TEST_ASSERT_EQUAL_INT((int)StatusIcon::WifiOff, (int)status_icon(SystemState::Offline));
    TEST_ASSERT_EQUAL_INT((int)StatusIcon::Wifi, (int)status_icon(SystemState::Ok));
}

static void test_small_buffer() {
    char out[8] = "x";
    TEST_ASSERT_EQUAL_UINT32(0, status_text(SystemState::Offline, "03:00", 0, out, sizeof out));
    TEST_ASSERT_EQUAL_STRING("", out);
}


static void test_setup_strings() {  // 02 §7.2-§7.4, OD4-4 (spec 004 FR-004..FR-007)
    TEST_ASSERT_EQUAL_STRING("Kurulum", tr::kSetupTitle);
    TEST_ASSERT_EQUAL_STRING("Wi-Fi ayarını telefonundan yap", tr::kSetupSubtitle);
    TEST_ASSERT_EQUAL_STRING("Telefonun kamerasıyla QR kodu okut ya da Wi-Fi ayarlarından bu ağa bağlan:",
                             tr::kSetupStep1);
    TEST_ASSERT_EQUAL_STRING("Ağ adı", tr::kSetupNetworkLabel);
    TEST_ASSERT_EQUAL_STRING("Şifre", tr::kSetupPasswordLabel);
    TEST_ASSERT_EQUAL_STRING("Açılan sayfada evdeki Wi-Fi ağını seç, şifresini yaz, ilini ve ilçeni seç.",
                             tr::kSetupStep2);
    TEST_ASSERT_EQUAL_STRING("Sayfa kendiliğinden açılmazsa tarayıcıya ", tr::kSetupStep3Before);
    TEST_ASSERT_EQUAL_STRING("192.168.4.1", tr::kSetupAddress);
    TEST_ASSERT_EQUAL_STRING(" yaz.", tr::kSetupStep3After);
    TEST_ASSERT_EQUAL_STRING("Bu ağa bağlanmak için okut", tr::kSetupQrCaption);
    TEST_ASSERT_EQUAL_STRING("Telefonun bağlanması bekleniyor", tr::kFooterWaiting);
    TEST_ASSERT_EQUAL_STRING("Telefon bağlandı. Açılan sayfayı doldur.", tr::kFooterPhoneJoined);
    TEST_ASSERT_EQUAL_STRING("Wi-Fi şifresi kabul edilmedi. Tekrar dene.", tr::kFooterWrongPassword);
    TEST_ASSERT_EQUAL_STRING("Ağ bulunamadı. Yalnızca 2,4 GHz ağlar desteklenir.", tr::kFooterNotFound);
    TEST_ASSERT_EQUAL_STRING("Bağlanılamadı. Tekrar dene.", tr::kFooterOther);
    TEST_ASSERT_EQUAL_STRING("Wi-Fi ağına bağlanılıyor", tr::kConnectingTitle);
    TEST_ASSERT_EQUAL_STRING("Bu işlem yarım dakika kadar sürebilir.", tr::kConnectingNote);
    TEST_ASSERT_EQUAL_STRING("Bağlandı", tr::kConnectedTitle);
    TEST_ASSERT_EQUAL_STRING("Saat ve vakitler alınıyor", tr::kConnectedNote);
    TEST_ASSERT_EQUAL_STRING("Cihaz durumu", tr::kOverlayTitle);
    TEST_ASSERT_EQUAL_STRING("Konum", tr::kOverlayLabels[0]);
    TEST_ASSERT_EQUAL_STRING("Vakit kaynağı", tr::kOverlayLabels[1]);
    TEST_ASSERT_EQUAL_STRING("Son güncelleme", tr::kOverlayLabels[2]);
    TEST_ASSERT_EQUAL_STRING("Kayıtlı son gün", tr::kOverlayLabels[3]);
    TEST_ASSERT_EQUAL_STRING("Wi-Fi", tr::kOverlayLabels[4]);
    TEST_ASSERT_EQUAL_STRING("Ayar adresi", tr::kOverlayLabels[5]);
    TEST_ASSERT_EQUAL_STRING("Ayar PIN’i", tr::kOverlayLabels[6]);
    TEST_ASSERT_EQUAL_STRING("Sürüm", tr::kOverlayLabels[7]);
    TEST_ASSERT_EQUAL_STRING("Ayar sayfası", tr::kOverlayQrCaption);
    TEST_ASSERT_EQUAL_STRING("Kapatmak için dokun. Kurulumu baştan yapmak için 5 saniye basılı tut.",
                             tr::kOverlayHint);
    TEST_ASSERT_EQUAL_STRING("Kurulum baştan yapılsın mı?", tr::kConfirmTitle);
    TEST_ASSERT_EQUAL_STRING("Kayıtlı Wi-Fi ayarı silinir. Vakitler ve konum korunur.", tr::kConfirmText);
    TEST_ASSERT_EQUAL_STRING("Vazgeç", tr::kConfirmCancel);
    TEST_ASSERT_EQUAL_STRING("Baştan kur", tr::kConfirmStartOver);
    TEST_ASSERT_EQUAL_STRING("bağlı değil", tr::kWifiNotConnected);
    TEST_ASSERT_EQUAL_STRING("desteklenmiyor", tr::kUnsupported);
    TEST_ASSERT_EQUAL_STRING("—", tr::kDash);
}

void run_state_text_tests() {
    RUN_TEST(test_constants);
    RUN_TEST(test_status_text_per_state);
    RUN_TEST(test_alert_colour_states);
    RUN_TEST(test_icons);
    RUN_TEST(test_small_buffer);
    RUN_TEST(test_setup_strings);
}
