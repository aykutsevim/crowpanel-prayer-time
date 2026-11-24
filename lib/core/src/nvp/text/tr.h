// Every Turkish string the prayer-time engine emits, copied byte for byte from
// docs/specification/02-ui-spec.md §9 (spec 002 FR-015, constitution III, research R4).
// This file is UTF-8 without BOM. The apostrophe in "dk’dan" is U+2019 (bytes E2 80 99); a test
// checks those bytes, so do not let an editor replace it.
#pragma once

namespace nvp::text::tr {

// Column names, index = vakit (0 İmsak .. 5 Yatsı).
inline constexpr const char* kVakitNames[6] = {"İmsak", "Güneş", "Öğle", "İkindi", "Akşam", "Yatsı"};

// 02 §9.2: the period that begins at vakit k, and the countdown label towards the next vakit.
inline constexpr const char* kPeriodNames[6] = {"İmsak vakti", "Kuşluk vakti", "Öğle vakti",
                                                "İkindi vakti", "Akşam vakti", "Yatsı vakti"};
inline constexpr const char* kCountdownLabels[6] = {"Güneşe kalan süre",  "Öğleye kalan süre",
                                                    "İkindiye kalan süre", "Akşama kalan süre",
                                                    "Yatsıya kalan süre", "İmsaka kalan süre"};

// 02 §9.3 column notes.
inline constexpr const char* kVakitGirdi = "vakit girdi";
inline constexpr const char* kSuAn = "şu an";
inline constexpr const char* kYarin = "yarın";
inline constexpr const char* kSonra = " sonra";

// 02 §9.1 duration parts.
inline constexpr const char* kLessThanAMinute = "1 dk’dan az";
inline constexpr const char* kMinutes = " dk";
inline constexpr const char* kHours = " sa";

// 02 §9.1 calendars. Index 0 of the month tables is unused.
inline constexpr const char* kMonths[13] = {"",       "Ocak",    "Şubat", "Mart", "Nisan", "Mayıs", "Haziran",
                                            "Temmuz", "Ağustos", "Eylül", "Ekim", "Kasım", "Aralık"};
inline constexpr const char* kWeekdays[7] = {"Pazartesi", "Salı", "Çarşamba", "Perşembe",
                                             "Cuma",      "Cumartesi", "Pazar"};  // 0 = Monday
inline constexpr const char* kHijriMonths[13] = {
    "",       "Muharrem", "Safer",   "Rebiülevvel", "Rebiülahir", "Cemaziyelevvel", "Cemaziyelahir",
    "Recep",  "Şaban",    "Ramazan", "Şevval",      "Zilkade",    "Zilhicce"};

// 02 §6 and §4.1: status row and hero texts of the system states (spec 003 FR-011, FR-012).
inline constexpr const char* kStatusNoTime = "Saat alınamadı, Wi-Fi bekleniyor";
inline constexpr const char* kStatusNoData = "Vakitler alınamadı";
inline constexpr const char* kStatusCalculated = "Yaklaşık vakitler, hesaplama ile";
inline constexpr const char* kStatusCacheEndingPrefix = "Kayıtlı vakitler ";
inline constexpr const char* kStatusCacheEndingDays = " gün sonra bitiyor";
inline constexpr const char* kStatusCacheEndingToday = "Kayıtlı vakitler bugün bitiyor";
inline constexpr const char* kStatusOffline = "Çevrimdışı, kayıtlı vakitler kullanılıyor";
inline constexpr const char* kStatusLastUpdate = "Son güncelleme ";
inline constexpr const char* kHeroNoTimeTitle = "Saat eşitleniyor";
inline constexpr const char* kHeroNoTimeDate = "Saat bekleniyor";
inline constexpr const char* kHeroNoTimeLabel = "Wi-Fi bağlantısı bekleniyor";
inline constexpr const char* kHeroNoDataTitle = "Vakitler alınamadı";
inline constexpr const char* kHeroNoDataLabel = "Wi-Fi ve konum ayarını kontrol edin";
inline constexpr const char* kKerahatLegend = "Kerahat vakti";
inline constexpr const char* kApproxPrefix = "≈";  // calculated state, before each column time
inline constexpr const char* kEllipsis = "…";      // U+2026, city truncation

// Setup screen, 02 §7.2 (spec 004 FR-004, FR-005).
inline constexpr const char* kSetupTitle = "Kurulum";
inline constexpr const char* kSetupSubtitle = "Wi-Fi ayarını telefonundan yap";
inline constexpr const char* kSetupStep1 =
    "Telefonun kamerasıyla QR kodu okut ya da Wi-Fi ayarlarından bu ağa bağlan:";
inline constexpr const char* kSetupNetworkLabel = "Ağ adı";
inline constexpr const char* kSetupPasswordLabel = "Şifre";
inline constexpr const char* kSetupStep2 = "Açılan sayfada evdeki Wi-Fi ağını seç, şifresini yaz, ilini ve ilçeni seç.";
inline constexpr const char* kSetupStep3Before = "Sayfa kendiliğinden açılmazsa tarayıcıya ";
inline constexpr const char* kSetupAddress = "192.168.4.1";
inline constexpr const char* kSetupStep3After = " yaz.";
inline constexpr const char* kSetupQrCaption = "Bu ağa bağlanmak için okut";
inline constexpr const char* kFooterWaiting = "Telefonun bağlanması bekleniyor";
inline constexpr const char* kFooterPhoneJoined = "Telefon bağlandı. Açılan sayfayı doldur.";
inline constexpr const char* kFooterWrongPassword = "Wi-Fi şifresi kabul edilmedi. Tekrar dene.";
inline constexpr const char* kFooterNotFound = "Ağ bulunamadı. Yalnızca 2,4 GHz ağlar desteklenir.";
inline constexpr const char* kFooterOther = "Bağlanılamadı. Tekrar dene.";

// Connecting screen, 02 §7.3 (FR-006).
inline constexpr const char* kConnectingTitle = "Wi-Fi ağına bağlanılıyor";
inline constexpr const char* kConnectingNote = "Bu işlem yarım dakika kadar sürebilir.";
inline constexpr const char* kConnectedTitle = "Bağlandı";
inline constexpr const char* kConnectedNote = "Saat ve vakitler alınıyor";

// Status overlay and confirmation, 02 §7.4 (FR-007, FR-008).
inline constexpr const char* kOverlayTitle = "Cihaz durumu";
inline constexpr const char* kOverlayLabels[8] = {"Konum",       "Vakit kaynağı", "Son güncelleme", "Kayıtlı son gün",
                                                  "Wi-Fi",       "Ayar adresi",   "Ayar PIN’i",     "Sürüm"};
inline constexpr const char* kOverlayQrCaption = "Ayar sayfası";
inline constexpr const char* kOverlayHint = "Kapatmak için dokun. Kurulumu baştan yapmak için 5 saniye basılı tut.";
inline constexpr const char* kConfirmTitle = "Kurulum baştan yapılsın mı?";
inline constexpr const char* kConfirmText = "Kayıtlı Wi-Fi ayarı silinir. Vakitler ve konum korunur.";
inline constexpr const char* kConfirmCancel = "Vazgeç";
inline constexpr const char* kConfirmStartOver = "Baştan kur";
inline constexpr const char* kWifiNotConnected = "bağlı değil";
inline constexpr const char* kDash = "—";  // U+2014, a value that a later slice fills (analysis U3)

// Setup page (02 §10, OD4-4): the mark of an unsupported network. The page has its own copy.
inline constexpr const char* kUnsupported = "desteklenmiyor";

}  // namespace nvp::text::tr
