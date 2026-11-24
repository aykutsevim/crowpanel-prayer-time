// Design tokens of the main screen (02-ui-spec.md §2.1, §3, §4; docs/design/tokens.json).
// Spec 003 FR-001, FR-002, FR-005. Every number below is copied from those two sources.
#pragma once

#include <cmath>
#include <cstdint>

#include "lvgl.h"

namespace nvp::ui::tok {

// --- Colours (02 §2.1) ---
inline lv_color_t bg() { return lv_color_hex(0x0F1B3D); }
inline lv_color_t ink() { return lv_color_hex(0xF5F1E6); }
inline lv_color_t muted() { return lv_color_hex(0xA3AFD6); }
inline lv_color_t dim() { return lv_color_hex(0x6474A6); }
inline lv_color_t line() { return lv_color_hex(0x3A4C8A); }
inline lv_color_t now() { return lv_color_hex(0xFFCF5C); }
inline lv_color_t alert() { return lv_color_hex(0xFF8F66); }
inline lv_color_t track() { return lv_color_hex(0x2A3B78); }
inline lv_color_t tick() { return lv_color_hex(0x6474A6); }
// Strip segments, index = the vakit that starts the segment (0 İmsak .. 5 Yatsı); night = yatsi.
inline lv_color_t strip(int k) {
    static const uint32_t kStrip[6] = {0x7C6DC6, 0x7FC4E8, 0xD6EEFA, 0xF0994A, 0xC2507A, 0x2A3B78};
    return lv_color_hex(kStrip[k]);
}
inline lv_color_t strip_night() { return lv_color_hex(0x2A3B78); }

// --- Canvas and timeline (02 §1, §3, §4.2..§4.4) ---
inline constexpr int32_t kWidth = 800;
inline constexpr int32_t kHeight = 480;
inline constexpr int32_t kMarginX = 24;
inline constexpr float kTimelineX0 = 24.0f;
inline constexpr float kTimelineWidth = 752.0f;
inline float x_of_seconds(int32_t s) { return kTimelineX0 + static_cast<float>(s) / 86400.0f * kTimelineWidth; }
inline int32_t round_px(float v) { return static_cast<int32_t>(std::lround(v)); }

inline constexpr float kHourLabelBaseline = 248.0f;
inline constexpr int32_t kTickMajorY0 = 254, kTickMinorY0 = 257, kTickY1 = 260;
inline constexpr int32_t kStripY = 264, kStripH = 30, kStripRadius = 7, kSeparatorW = 2;
inline constexpr int32_t kHatchStripe = 3, kHatchPeriod = 6;
inline constexpr int32_t kMarkerY0 = 259, kMarkerY1 = 299, kMarkerW = 3, kMarkerGapW = 9;
inline constexpr int32_t kLeaderY0 = 294, kLeaderY1 = 304, kLeaderY2 = 325, kLeaderY3 = 334;

// --- Columns (02 §4.5) ---
inline constexpr int32_t kColumnTop = 336, kColumnH = 128, kColumnRadius = 12;
inline constexpr float kColumnW = 752.0f / 6.0f;  // 125.333
inline float column_x(int k) { return kTimelineX0 + static_cast<float>(k) * kColumnW; }
inline float column_cx(int k) { return kTimelineX0 + (static_cast<float>(k) + 0.5f) * kColumnW; }
inline constexpr float kColumnNameBaseline = 363.0f, kColumnTimeBaseline = 413.0f, kColumnNoteBaseline = 436.5f;

// --- Hero and status row (02 §4.1, tokens.json "main") ---
inline constexpr int32_t kIconSize = 16;
inline constexpr int32_t kPinX = 24, kPinY = 18;
inline constexpr int32_t kCityX = 46, kCityMaxW = 300;
inline constexpr float kCityBaseline = 32.5f;
inline constexpr int32_t kStatusRight = 776, kStatusIconGap = 8;
inline constexpr float kStatusBaseline = 31.5f;
inline constexpr int32_t kClockX = 24, kSecondsGap = 3;
inline constexpr float kClockBaseline = 130.0f;
inline constexpr int32_t kDateX = 24;
inline constexpr float kDateBaseline = 172.0f, kHijriBaseline = 196.0f;
inline constexpr int32_t kLegendSwatchX = 24, kLegendSwatchY = 218, kLegendSwatchW = 22, kLegendSwatchH = 10;
inline constexpr int32_t kLegendTextX = 53;
inline constexpr float kLegendBaseline = 228.0f;
inline constexpr int32_t kPeriodX = 340;
inline constexpr float kPeriodBaseline = 76.5f;
inline constexpr int32_t kBoundEndRight = 776, kBoundStartRight = 570;
inline constexpr float kBoundsBaseline = 75.3f;
inline constexpr int32_t kTrackX = 580, kTrackY = 62, kTrackW = 132, kTrackH = 8, kTrackRadius = 4;
inline constexpr int32_t kLabelX = 340;
inline constexpr float kLabelBaseline = 116.0f;
inline constexpr int32_t kCountdownX = 337;
inline constexpr float kCountdownBaseline = 221.0f;

// Top of a text line whose baseline is at `baseline` (02 §4.1 gives baselines; LVGL places tops).
inline int32_t top_for_baseline(const lv_font_t* f, float baseline) {
    return round_px(baseline - static_cast<float>(f->line_height - f->base_line));
}

}  // namespace nvp::ui::tok
