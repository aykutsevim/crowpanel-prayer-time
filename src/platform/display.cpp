#include "display.h"

#include <esp_heap_caps.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_rgb.h>
#include <esp_timer.h>

#include <cstdio>

#include "board.h"
#include "lvgl.h"
#include "sysinfo.h"

namespace platform::display {
namespace {

// 800 x 40 x 2 B = 64 000 B internal RAM (research R8 L0). Halved in L3b on the Arduino libraries;
// restored with L4, where internal RAM is no longer short.
constexpr int32_t kDrawLines = 40;

esp_lcd_panel_handle_t g_panel = nullptr;
lv_display_t* g_display = nullptr;
uint8_t* g_draw_buf = nullptr;
TouchReadFn g_touch = nullptr;

uint32_t tick_ms() { return static_cast<uint32_t>(esp_timer_get_time() / 1000); }

// Copies the rendered area into the PSRAM frame buffer. esp_lcd_panel_draw_bitmap copies
// synchronously and writes the cache back, so the buffer can be reused at once.
void flush(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
    esp_lcd_panel_draw_bitmap(g_panel, area->x1, area->y1, area->x2 + 1, area->y2 + 1, px_map);
    lv_display_flush_ready(disp);
}

void read_touch(lv_indev_t*, lv_indev_data_t* data) {
    int16_t x = 0;
    int16_t y = 0;
    const bool pressed = g_touch != nullptr && g_touch(&x, &y);
    data->state = pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
    if (pressed) {
        data->point.x = x;
        data->point.y = y;
    }
}

}  // namespace

Result begin() {
    Result r;
    sysinfo::note_peripheral_init();

    esp_lcd_rgb_panel_config_t cfg = {};
    cfg.clk_src = LCD_CLK_SRC_DEFAULT;
    cfg.timings.pclk_hz = board::kPclkHz;
    cfg.timings.h_res = board::kLcdWidth;
    cfg.timings.v_res = board::kLcdHeight;
    cfg.timings.hsync_pulse_width = board::kHsyncPulseWidth;
    cfg.timings.hsync_back_porch = board::kHsyncBackPorch;
    cfg.timings.hsync_front_porch = board::kHsyncFrontPorch;
    cfg.timings.vsync_pulse_width = board::kVsyncPulseWidth;
    cfg.timings.vsync_back_porch = board::kVsyncBackPorch;
    cfg.timings.vsync_front_porch = board::kVsyncFrontPorch;
    cfg.timings.flags.hsync_idle_low = board::kHsyncIdleLow;
    cfg.timings.flags.vsync_idle_low = board::kVsyncIdleLow;
    cfg.timings.flags.pclk_active_neg = board::kPclkActiveNeg;
    cfg.data_width = 16;
    cfg.bits_per_pixel = 16;
    cfg.num_fbs = 1;
    cfg.bounce_buffer_size_px = board::kBounceBufferPx;  // research R8 (L3, L4)
    cfg.dma_burst_size = 64;
    cfg.hsync_gpio_num = board::kLcdHsync;
    cfg.vsync_gpio_num = board::kLcdVsync;
    cfg.de_gpio_num = board::kLcdDe;
    cfg.pclk_gpio_num = board::kLcdPclk;
    cfg.disp_gpio_num = -1;
    for (int i = 0; i < 16; ++i) cfg.data_gpio_nums[i] = board::kLcdData[i];
    cfg.flags.fb_in_psram = 1;
    cfg.flags.bb_invalidate_cache = board::kBounceInvalidateCache ? 1 : 0;  // research R8

    esp_err_t err = esp_lcd_new_rgb_panel(&cfg, &g_panel);
    if (err == ESP_OK) err = esp_lcd_panel_reset(g_panel);
    if (err == ESP_OK) err = esp_lcd_panel_init(g_panel);
    if (err != ESP_OK) {
        std::snprintf(r.reason, sizeof r.reason, "panel start failed (%s)", esp_err_to_name(err));
        g_panel = nullptr;
        return r;
    }

    const size_t buf_bytes = static_cast<size_t>(board::kLcdWidth) * kDrawLines * 2;
    g_draw_buf = static_cast<uint8_t*>(heap_caps_malloc(buf_bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    if (g_draw_buf == nullptr) {
        std::snprintf(r.reason, sizeof r.reason, "no internal RAM for draw buffer");
        return r;
    }

    lv_init();
    lv_tick_set_cb(tick_ms);
    g_display = lv_display_create(board::kLcdWidth, board::kLcdHeight);
    if (g_display == nullptr) {
        std::snprintf(r.reason, sizeof r.reason, "LVGL display create failed");
        return r;
    }
    lv_display_set_color_format(g_display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(g_display, g_draw_buf, nullptr, buf_bytes, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(g_display, flush);
    r.ok = true;
    return r;
}

bool ready() { return g_display != nullptr; }

bool set_pclk(uint32_t hz) { return g_panel != nullptr && esp_lcd_rgb_panel_set_pclk(g_panel, hz) == ESP_OK; }

void attach_touch(TouchReadFn read) {
    if (!ready()) return;
    g_touch = read;
    lv_indev_t* indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, read_touch);
}

void handle() {
    if (ready()) lv_timer_handler();
}

void refresh_now() {
    if (ready()) lv_refr_now(g_display);
}

}  // namespace platform::display
