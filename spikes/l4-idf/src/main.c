// L4 spike: does the RGB panel stay steady under Wi-Fi scans, flash writes and HTTPS with plain
// ESP-IDF 5.5.5 and the options in sdkconfig.defaults? (spec 001, research R8, 2026-11-23)
//
// Throwaway firmware. It keeps the product's hard rules:
// - Silence first (FR-001, SIL-02): GPIO4/5/6 low, then amplifier mute (249) and the second silence command (247)
//   to the auxiliary controller at 0x30, through the same byte gate. No audio peripheral is used.
//   Without an acknowledgement the radio is never started (FR-001a).
// - TLS certificate checks on (SEC-01): built-in CA bundle.
// - No secrets in the log: the Wi-Fi password is read from NVS and never printed.
//
// Timeline: 60 s idle (Wi-Fi connected), then 10 minutes of stress, then idle again. Every 10 s a
// Wi-Fi scan starts and a 4 kB flash write runs AT THE SAME TIME (the worst case, `test mix`);
// every 30 s an HTTPS GET. A block at the bottom right shows the phase: grey idle, green stress,
// blue done, red if silence failed. A small square moves along the bottom edge every 100 ms, so
// the CPU keeps writing to the frame buffer as the product's screen updates would.
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_crt_bundle.h"
#include "esp_event.h"
#include "esp_heap_caps.h"
#include "esp_http_client.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_partition.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"

// --- Board values: copied from src/platform/board.h (cited there) ---
#define LCD_W 800
#define LCD_H 480
static const int kLcdData[16] = {21, 47, 48, 45, 38, 9, 10, 11, 12, 13, 14, 7, 17, 18, 3, 46};
#define LCD_DE 42
#define LCD_VSYNC 41
#define LCD_HSYNC 40
#define LCD_PCLK 39
#define LCD_PCLK_HZ (14 * 1000 * 1000)  // L1 result: the panel accepts 14 MHz, not 12
#define BOUNCE_LINES 10                 // vendor IDF Lesson03 value
#define I2C_SDA 15
#define I2C_SCL 16
#define AUX_ADDR 0x30
static const int kAmpPins[3] = {4, 5, 6};

// --- Auxiliary controller byte gate: same rule as src/platform/audio_off.h ---
#define AUX_BACKLIGHT_FULL 0
#define AUX_SILENCE_247 247
#define AUX_AMP_MUTE 249
static bool aux_byte_allowed(uint8_t b) { return b == 0 || b == 247 || b == 249 || b == 250; }

#define STRESS_START_S 60
#define STRESS_LENGTH_S 600
#define URL "https://ezanvakti.imsakiyem.com/api/v2/locations/countries/2"

static uint16_t* g_fb = NULL;
static i2c_master_dev_handle_t g_aux = NULL;
static volatile bool g_connected = false;
static volatile bool g_scan_running = false;
static volatile uint32_t g_scan_count = 0, g_scan_fail = 0, g_write_count = 0, g_write_fail = 0;
static volatile uint32_t g_https_ok = 0, g_https_fail = 0, g_cycles = 0;
static volatile uint16_t g_phase_colour = 0x8410;  // grey

static uint32_t ms(void) { return (uint32_t)(esp_timer_get_time() / 1000); }
#define LOG(fmt, ...) printf("[%8" PRIu32 "] " fmt "\n", ms(), ##__VA_ARGS__)

// ---------------------------------------------------------------- silence
static bool aux_send(uint8_t b) {
    if (!aux_byte_allowed(b) || g_aux == NULL) return false;
    return i2c_master_transmit(g_aux, &b, 1, 50) == ESP_OK;
}

static bool silence_first(void) {
    for (int i = 0; i < 3; ++i) {
        gpio_reset_pin(kAmpPins[i]);
        gpio_set_direction(kAmpPins[i], GPIO_MODE_OUTPUT);
        gpio_set_level(kAmpPins[i], 0);
    }
    i2c_master_bus_config_t bus = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = I2C_SDA,
        .scl_io_num = I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t h = NULL;
    if (i2c_new_master_bus(&bus, &h) != ESP_OK) return false;
    i2c_device_config_t dev = {.dev_addr_length = I2C_ADDR_BIT_LEN_7, .device_address = AUX_ADDR, .scl_speed_hz = 100000};
    if (i2c_master_bus_add_device(h, &dev, &g_aux) != ESP_OK) return false;
    for (int attempt = 1; attempt <= 20; ++attempt) {
        const bool mute = aux_send(AUX_AMP_MUTE);
        const bool s247 = aux_send(AUX_SILENCE_247);
        if (mute && s247) {
            LOG("silence: acknowledged (attempts %d)", attempt);
            return true;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    LOG("silence: NOT acknowledged, radio stays off");
    return false;
}

// ---------------------------------------------------------------- picture
static inline uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
    return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

static void fill(int x0, int y0, int w, int h, uint16_t c) {
    for (int y = y0; y < y0 + h; ++y) {
        uint16_t* row = g_fb + y * LCD_W;
        for (int x = x0; x < x0 + w; ++x) row[x] = c;
    }
}

// Colour bars on top, fine vertical lines in the middle (the B4 field), 1 px white border.
static void draw_pattern(void) {
    const uint16_t bars[8] = {rgb(255, 255, 255), rgb(255, 255, 0), rgb(0, 255, 255), rgb(0, 255, 0),
                              rgb(255, 0, 255), rgb(255, 0, 0),     rgb(0, 0, 255),   rgb(0, 0, 0)};
    fill(0, 0, LCD_W, LCD_H, 0);
    for (int i = 0; i < 8; ++i) fill(i * 100, 0, 100, 180, bars[i]);
    for (int y = 180; y < 400; ++y) {
        uint16_t* row = g_fb + y * LCD_W;
        for (int x = 0; x < LCD_W; ++x) row[x] = (x & 1) ? 0xFFFF : 0x0000;
    }
    fill(0, 0, LCD_W, 1, 0xFFFF);
    fill(0, LCD_H - 1, LCD_W, 1, 0xFFFF);
    fill(0, 0, 1, LCD_H, 0xFFFF);
    fill(LCD_W - 1, 0, 1, LCD_H, 0xFFFF);
}

static void display_task(void* arg) {
    (void)arg;
    esp_lcd_rgb_panel_config_t cfg = {
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .timings = {
            .pclk_hz = LCD_PCLK_HZ, .h_res = LCD_W, .v_res = LCD_H,
            .hsync_pulse_width = 4, .hsync_back_porch = 8, .hsync_front_porch = 8,
            .vsync_pulse_width = 4, .vsync_back_porch = 8, .vsync_front_porch = 8,
            .flags = {.pclk_active_neg = 1},
        },
        .data_width = 16,
        .bits_per_pixel = 16,
        .num_fbs = 1,
        .bounce_buffer_size_px = LCD_W * BOUNCE_LINES,
        .dma_burst_size = 64,
        .hsync_gpio_num = LCD_HSYNC,
        .vsync_gpio_num = LCD_VSYNC,
        .de_gpio_num = LCD_DE,
        .pclk_gpio_num = LCD_PCLK,
        .disp_gpio_num = -1,
        .flags = {.fb_in_psram = 1},
    };
    for (int i = 0; i < 16; ++i) cfg.data_gpio_nums[i] = kLcdData[i];
    esp_lcd_panel_handle_t panel = NULL;
    ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&cfg, &panel));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel));
    void* fb = NULL;
    ESP_ERROR_CHECK(esp_lcd_rgb_panel_get_frame_buffer(panel, 1, &fb));
    g_fb = (uint16_t*)fb;
    draw_pattern();
    aux_send(AUX_BACKLIGHT_FULL);
    LOG("display: on, %d MHz, bounce %d lines, fb in PSRAM", LCD_PCLK_HZ / 1000000, BOUNCE_LINES);

    int x = 1;
    for (;;) {
        fill(1, 420, LCD_W - 2, 40, 0);       // heartbeat strip
        fill(x, 430, 30, 30, rgb(255, 255, 0));
        x = (x + 8) % (LCD_W - 140);
        fill(LCD_W - 101, 410, 100, 69, g_phase_colour);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ---------------------------------------------------------------- network
static void on_wifi(void* arg, esp_event_base_t base, int32_t id, void* data) {
    (void)arg;
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        g_connected = false;
        LOG("wifi: disconnected, retrying");
        esp_wifi_connect();
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        g_connected = true;
        LOG("wifi: connected");
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_SCAN_DONE) {
        const wifi_event_sta_scan_done_t* d = (const wifi_event_sta_scan_done_t*)data;
        if (d->status == 0) {
            ++g_scan_count;
            LOG("stress: scan done %u networks", (unsigned)d->number);
        } else {
            ++g_scan_fail;
            LOG("stress: scan failed");
        }
        esp_wifi_clear_ap_list();
        g_scan_running = false;
    }
}

static bool wifi_start(void) {
    esp_err_t e = nvs_flash_init();
    if (e != ESP_OK) {  // never erase: the product's stored details live here
        LOG("nvs: init failed (%s), radio stays off", esp_err_to_name(e));
        return false;
    }
    nvs_handle_t h;
    char ssid[33] = {0}, pass[65] = {0};
    size_t ls = sizeof ssid, lp = sizeof pass;
    if (nvs_open("wifi", NVS_READONLY, &h) != ESP_OK || nvs_get_str(h, "ssid", ssid, &ls) != ESP_OK) {
        LOG("wifi: no stored details (run the product's setup first)");
        return false;
    }
    if (nvs_get_str(h, "pass", pass, &lp) != ESP_OK) pass[0] = '\0';
    nvs_close(h);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, on_wifi, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, on_wifi, NULL));
    wifi_config_t wc = {0};
    memcpy(wc.sta.ssid, ssid, strlen(ssid));
    memcpy(wc.sta.password, pass, strlen(pass));
    memset(pass, 0, sizeof pass);
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));  // do not copy details into Wi-Fi NVS
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wc));
    memset(&wc, 0, sizeof wc);
    ESP_ERROR_CHECK(esp_wifi_start());
    LOG("wifi: joining %s", ssid);
    return true;
}

static void flash_write(const esp_partition_t* part, uint32_t counter) {
    static uint8_t block[4096];
    memset(block, (int)(counter & 0xFF), sizeof block);
    const int64_t t0 = esp_timer_get_time();
    esp_err_t e = esp_partition_erase_range(part, 0, 4096);
    if (e == ESP_OK) e = esp_partition_write(part, 0, block, sizeof block);
    const uint32_t dt = (uint32_t)((esp_timer_get_time() - t0) / 1000);
    if (e == ESP_OK) {
        ++g_write_count;
        LOG("stress: write ok 4096 B in %" PRIu32 " ms", dt);
    } else {
        ++g_write_fail;
        LOG("stress: write failed (%s)", esp_err_to_name(e));
    }
}

static void https_get(void) {
    esp_http_client_config_t c = {.url = URL, .crt_bundle_attach = esp_crt_bundle_attach, .timeout_ms = 15000};
    const int64_t t0 = esp_timer_get_time();
    esp_http_client_handle_t cl = esp_http_client_init(&c);
    int status = -1, total = 0;
    if (cl != NULL && esp_http_client_open(cl, 0) == ESP_OK) {
        esp_http_client_fetch_headers(cl);
        status = esp_http_client_get_status_code(cl);
        char buf[256];
        int n;
        while ((n = esp_http_client_read(cl, buf, sizeof buf)) > 0) total += n;
    }
    if (cl != NULL) esp_http_client_cleanup(cl);
    const uint32_t dt = (uint32_t)((esp_timer_get_time() - t0) / 1000);
    if (status == 200) {
        ++g_https_ok;
        LOG("stress: https ok 200 %d B in %" PRIu32 " ms", total, dt);
    } else {
        ++g_https_fail;
        LOG("stress: https failed (status %d)", status);
    }
}

static void report(const char* tag) {
    LOG("%s: cycles %" PRIu32 " scan %" PRIu32 "/%" PRIu32 " write %" PRIu32 "/%" PRIu32 " https %" PRIu32 "/%" PRIu32
        " heap internal %u free %u largest, psram %u free",
        tag, g_cycles, g_scan_count, g_scan_fail, g_write_count, g_write_fail, g_https_ok, g_https_fail,
        (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
        (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL),
        (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
}

static void stress_task(void* arg) {
    (void)arg;
    const esp_partition_t* part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, 0x40, "scratch");
    if (part == NULL) LOG("stress: no scratch partition, writes skipped");
    vTaskDelay(pdMS_TO_TICKS(STRESS_START_S * 1000));
    LOG("stress: on (10 minutes; scan and write together every 10 s, https every 30 s)");
    g_phase_colour = rgb(0, 200, 0);
    const uint32_t start = ms();
    uint32_t tick = 0;
    while (ms() - start < STRESS_LENGTH_S * 1000u) {
        if (g_connected && !g_scan_running) {
            wifi_scan_config_t sc = {0};
            if (esp_wifi_scan_start(&sc, false) == ESP_OK) {
                g_scan_running = true;
                LOG("stress: scan start");
            } else {
                ++g_scan_fail;
                LOG("stress: scan could not start");
            }
        }
        if (part != NULL) flash_write(part, tick);  // during the scan on purpose
        if (tick % 3 == 0) {
            if (g_connected) https_get();
            else LOG("stress: https skipped (not connected)");
        }
        ++g_cycles;
        ++tick;
        const uint32_t next = start + tick * 10000u;
        while (ms() < next) vTaskDelay(pdMS_TO_TICKS(50));
    }
    g_phase_colour = rgb(0, 0, 255);
    report("stress: off");
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(60000));
        report("idle");
    }
}

void app_main(void) {
    const bool silent = silence_first();  // before any other peripheral
    if (!silent) g_phase_colour = rgb(255, 0, 0);
    xTaskCreatePinnedToCore(display_task, "display", 6144, NULL, 5, NULL, 1);
    if (!silent) return;
    if (!wifi_start()) return;
    xTaskCreatePinnedToCore(stress_task, "stress", 8192, NULL, 4, NULL, 0);
}
