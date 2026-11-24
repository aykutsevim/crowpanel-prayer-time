#include "http_date.h"

#include <esp_http_client.h>

#include <cstring>

#include "nvp/time/http_date.h"
#include "nvp/version.h"
#include "sysinfo.h"

namespace platform::http_date {
namespace {

// Hosts of 04-data-sources.md §11 (SEC-02); both answer port 80 with a redirect and a Date header
// (checked 2026-11-24, research R2). The redirect is not followed.
const char* kUrls[2] = {"http://ezanvakti.imsakiyem.com/", "http://ezanvakti.emushaf.net/"};
const char* kHosts[2] = {"ezanvakti.imsakiyem.com", "ezanvakti.emushaf.net"};
constexpr uint32_t kMaxRttMs = 2000;  // beyond this the ±2 s of TIME-02 cannot be promised

char g_date[40];
bool g_have_date = false;

esp_err_t on_event(esp_http_client_event_t* e) {
    if (e->event_id == HTTP_EVENT_ON_HEADER && e->header_key != nullptr && e->header_value != nullptr &&
        strcasecmp(e->header_key, "Date") == 0) {
        std::strncpy(g_date, e->header_value, sizeof g_date - 1);
        g_date[sizeof g_date - 1] = '\0';
        g_have_date = true;
    }
    return ESP_OK;
}

}  // namespace

const char* host(int index) { return kHosts[index == 1 ? 1 : 0]; }

Result fetch(int index) {
    Result r;
    g_have_date = false;
    esp_http_client_config_t cfg = {};
    cfg.url = kUrls[index == 1 ? 1 : 0];
    cfg.method = HTTP_METHOD_HEAD;
    cfg.timeout_ms = 5000;
    cfg.disable_auto_redirect = true;
    cfg.user_agent = nvp::kUserAgent;
    cfg.event_handler = on_event;
    esp_http_client_handle_t c = esp_http_client_init(&cfg);
    if (c == nullptr) {
        r.why = "client init failed";
        return r;
    }
    const int64_t t0 = sysinfo::uptime_us();
    const esp_err_t err = esp_http_client_perform(c);
    const int64_t t1 = sysinfo::uptime_us();
    esp_http_client_cleanup(c);
    r.rtt_ms = static_cast<uint32_t>((t1 - t0) / 1000);
    int64_t date_s = 0;
    if (err != ESP_OK) {
        r.why = "no answer";
    } else if (!g_have_date) {
        r.why = "no Date header";
    } else if (!nvp::time::parse_http_date(g_date, &date_s)) {
        r.why = "malformed Date header";
    } else if (r.rtt_ms > kMaxRttMs) {
        r.why = "round trip over 2 s";
    } else {
        // The server truncates to the second: add half a second, and half the round trip (R2).
        r.utc_us = date_s * 1000000 + 500000 + static_cast<int64_t>(r.rtt_ms) * 500;
        r.at_uptime_us = t1;
        r.ok = true;
    }
    return r;
}

}  // namespace platform::http_date
