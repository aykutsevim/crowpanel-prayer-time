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

}  // namespace nvp::text::tr
