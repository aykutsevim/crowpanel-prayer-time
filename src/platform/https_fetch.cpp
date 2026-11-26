#include "https_fetch.h"

#include <esp_crt_bundle.h>
#include <esp_heap_caps.h>
#include <esp_http_client.h>

#include "nvp/version.h"

namespace platform::https_fetch {
namespace {

constexpr int kTimeoutMs = 15000;
constexpr int kChunk = 1024;

uint32_t internal_free_min() {
    return static_cast<uint32_t>(heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL));
}

}  // namespace

nvp::providers::HttpResult Client::get(const char* url, nvp::providers::HttpBody& body) {
    using nvp::providers::HttpOutcome;
    nvp::providers::HttpResult r;
    error_ = "";
    if (!time_valid_) {
        r.outcome = HttpOutcome::Refused;  // certificate dates need a valid clock (OD5-1)
        error_ = "time not valid";
        return r;
    }
    esp_http_client_config_t cfg = {};
    cfg.url = url;
    cfg.crt_bundle_attach = esp_crt_bundle_attach;  // verification on: built-in CA bundle (SEC-01)
    cfg.timeout_ms = kTimeoutMs;
    cfg.user_agent = nvp::kUserAgent;  // SEC-03
    cfg.disable_auto_redirect = true;
    cfg.buffer_size = kChunk;
    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (client == nullptr) {
        error_ = "client init failed";
        r.outcome = HttpOutcome::Error;
        return r;
    }
    esp_http_client_set_header(client, "Accept", "application/json");  // 04 §10
    esp_http_client_set_header(client, "Accept-Encoding", "identity");  // no inflater (R3)

    const esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        error_ = esp_err_to_name(err);
        r.outcome = err == ESP_ERR_HTTP_CONNECTING ? HttpOutcome::Timeout : HttpOutcome::Connect;
    } else {
        esp_http_client_fetch_headers(client);
        r.status = esp_http_client_get_status_code(client);
        body.on_status(r.status);
        static char buf[kChunk];  // the worker is the only caller (one request at a time)
        r.outcome = HttpOutcome::Done;
        for (;;) {
            const int n = esp_http_client_read(client, buf, sizeof buf);
            if (n == 0) break;
            if (n == -ESP_ERR_HTTP_EAGAIN) {
                r.outcome = HttpOutcome::Timeout;
                error_ = "read timeout";
                break;
            }
            if (n < 0) {
                r.outcome = HttpOutcome::Error;
                error_ = "read failed";
                break;
            }
            r.bytes += static_cast<uint32_t>(n);
            if (!body.on_chunk(buf, static_cast<size_t>(n))) {
                r.outcome = HttpOutcome::Aborted;
                break;
            }
        }
        if (r.outcome == HttpOutcome::Done && !esp_http_client_is_complete_data_received(client)) {
            r.outcome = HttpOutcome::Error;  // the connection ended early; the parser also sees it
            error_ = "body incomplete";
        }
    }
    const uint32_t low = internal_free_min();
    if (low < heap_min_) heap_min_ = low;
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    return r;
}

}  // namespace platform::https_fetch
