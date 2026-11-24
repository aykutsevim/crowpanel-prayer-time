#include "https_probe.h"

#include <esp_crt_bundle.h>
#include <esp_http_client.h>
#include <esp_timer.h>

#include <cstdio>

#include "nvp/version.h"

namespace platform::https {
namespace {

constexpr const char* kUrl = "https://ezanvakti.imsakiyem.com/api/v2/locations/countries/2";
constexpr int kTimeoutMs = 10000;
constexpr uint32_t kMaxBody = 4096;

uint32_t now_ms() { return static_cast<uint32_t>(esp_timer_get_time() / 1000); }

}  // namespace

Result probe() {
    Result r;
    const uint32_t start = now_ms();
    esp_http_client_config_t cfg = {};
    cfg.url = kUrl;
    cfg.crt_bundle_attach = esp_crt_bundle_attach;  // verification on: built-in CA bundle (SEC-01)
    cfg.timeout_ms = kTimeoutMs;
    cfg.user_agent = nvp::kUserAgent;  // SEC-03
    cfg.disable_auto_redirect = true;
    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (client == nullptr) {
        std::snprintf(r.reason, sizeof r.reason, "https: client init failed");
        r.duration_ms = now_ms() - start;
        return r;
    }
    const esp_err_t err = esp_http_client_open(client, 0);
    if (err == ESP_OK) {
        esp_http_client_fetch_headers(client);
        r.status = esp_http_client_get_status_code(client);
        char buf[512];
        int n = 0;
        while ((n = esp_http_client_read(client, buf, sizeof buf)) > 0) {
            r.bytes += static_cast<uint32_t>(n);
            if (r.bytes > kMaxBody) break;  // no need to read further
        }
    } else {
        r.status = -1;
    }
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    r.duration_ms = now_ms() - start;
    if (r.status == 200 && r.bytes <= kMaxBody) {
        r.ok = true;
    } else if (r.status == 200) {
        std::snprintf(r.reason, sizeof r.reason, "https: body too large (%lu B)", static_cast<unsigned long>(r.bytes));
    } else if (r.status > 0) {
        std::snprintf(r.reason, sizeof r.reason, "https: HTTP %d", r.status);
    } else {
        std::snprintf(r.reason, sizeof r.reason, "https: %s", esp_err_to_name(err));
    }
    return r;
}

}  // namespace platform::https
