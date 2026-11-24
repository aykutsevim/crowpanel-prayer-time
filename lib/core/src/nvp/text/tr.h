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

}  // namespace nvp::text::tr
