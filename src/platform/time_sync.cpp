#include "time_sync.h"

#include <esp_netif_sntp.h>
#include <esp_sntp.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include <sys/time.h>

#include "sysinfo.h"

namespace platform::time_sync {
namespace {

SemaphoreHandle_t g_lock = nullptr;
bool g_have = false;
nvp::time::TimeSample g_sample;
bool g_started = false;
bool g_running = false;

const char* kServers[3] = {"pool.ntp.org", "time.google.com", "time.cloudflare.com"};  // TIME-01

}  // namespace

void start() {
    if (g_started) {
        restart();
        return;
    }
    if (g_lock == nullptr) g_lock = xSemaphoreCreateMutex();
    esp_sntp_config_t cfg = ESP_NETIF_SNTP_DEFAULT_CONFIG_MULTIPLE(
        3, ESP_SNTP_SERVER_LIST(kServers[0], kServers[1], kServers[2]));
    cfg.start = true;
    cfg.smooth_sync = false;  // TIME-05: corrections apply at once
    if (esp_netif_sntp_init(&cfg) == ESP_OK) {
        esp_sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED);
        g_started = true;
        g_running = true;
    }
}

void restart() {
    if (!g_started) return;
    if (!g_running) {
        esp_netif_sntp_start();
        g_running = true;
    }
    esp_sntp_restart();
}

void stop() {
    if (!g_started || !g_running) return;
    esp_sntp_stop();
    g_running = false;
}

bool running() { return g_running; }

bool take_sample(nvp::time::TimeSample& out) {
    if (g_lock == nullptr) return false;
    bool have = false;
    if (xSemaphoreTake(g_lock, pdMS_TO_TICKS(5)) == pdTRUE) {
        have = g_have;
        if (have) out = g_sample;
        g_have = false;
        xSemaphoreGive(g_lock);
    }
    return have;
}

}  // namespace platform::time_sync

// lwIP calls this for every SNTP answer (components/lwip/apps/sntp/sntp.c, a weak default that
// would call settimeofday). Here it only records the sample; the app decides (spec 005 R1).
extern "C" void sntp_sync_time(struct timeval* tv) {
    using namespace platform::time_sync;
    if (tv == nullptr || g_lock == nullptr) return;
    nvp::time::TimeSample s;
    s.source = nvp::time::TimeSource::Ntp;
    s.utc_us = static_cast<int64_t>(tv->tv_sec) * 1000000 + tv->tv_usec;
    s.at_uptime_us = platform::sysinfo::uptime_us();
    s.server = "";  // lwIP does not say which of the three servers answered
    if (xSemaphoreTake(g_lock, pdMS_TO_TICKS(50)) == pdTRUE) {
        g_sample = s;
        g_have = true;
        xSemaphoreGive(g_lock);
    }
    sntp_set_sync_status(SNTP_SYNC_STATUS_COMPLETED);
}
