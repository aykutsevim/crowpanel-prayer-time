#include "setup_portal.h"

#include <esp_http_server.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include <cstdio>
#include <cstring>

#include "generated/web_assets.h"
#include "nvp/net/submission.h"
#include "sysinfo.h"

namespace platform::portal {
namespace {

// The server runs its own task. Callbacks touch the network worker's state, so they run only
// while the worker sits in handle(): the worker holds g_gate the rest of the time.
httpd_handle_t g_server = nullptr;
SemaphoreHandle_t g_gate = nullptr;
Callbacks g_cb;

constexpr TickType_t kGateWait = pdMS_TO_TICKS(2000);
constexpr const char* kPortal = "http://192.168.4.1/";

bool enter() { return xSemaphoreTake(g_gate, kGateWait) == pdTRUE; }
void leave() { xSemaphoreGive(g_gate); }

esp_err_t send_json(httpd_req_t* req, const char* status, const char* body) {
    httpd_resp_set_status(req, status);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    return httpd_resp_sendstr(req, body);
}

esp_err_t busy(httpd_req_t* req) { return send_json(req, "503 Service Unavailable", "{\"error\":\"busy\"}"); }

esp_err_t send_gz(httpd_req_t* req, const char* type, const unsigned char* data, size_t len) {
    httpd_resp_set_type(req, type);
    httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    return httpd_resp_send(req, reinterpret_cast<const char*>(data), static_cast<ssize_t>(len));
}

esp_err_t on_page(httpd_req_t* req) {
    return send_gz(req, "text/html; charset=utf-8", generated::portal_html_gz, generated::portal_html_gz_len);
}

esp_err_t on_places(httpd_req_t* req) {
    return send_gz(req, "application/json", generated::places_json_gz, generated::places_json_gz_len);
}

esp_err_t on_scan(httpd_req_t* req) {
    static char body[3072];  // 20 networks of up to 32 escaped bytes each
    char query[32] = {};
    bool refresh = false;
    if (httpd_req_get_url_query_str(req, query, sizeof query) == ESP_OK) {
        char v[4] = {};
        refresh = httpd_query_key_value(query, "refresh", v, sizeof v) == ESP_OK && v[0] == '1';
    }
    if (!enter()) return busy(req);
    if (g_cb.scan_json) {
        g_cb.scan_json(refresh, body, sizeof body);
    } else {
        std::snprintf(body, sizeof body, "{\"scanning\":false,\"networks\":[]}");
    }
    leave();
    return send_json(req, "200 OK", body);
}

esp_err_t on_status(httpd_req_t* req) {
    char body[256];
    if (!enter()) return busy(req);
    if (g_cb.status_json) {
        g_cb.status_json(body, sizeof body);
    } else {
        std::snprintf(body, sizeof body, "{\"mode\":\"setup\",\"result\":\"none\",\"ssid\":\"\",\"location\":null}");
    }
    leave();
    return send_json(req, "200 OK", body);
}

esp_err_t on_setup(httpd_req_t* req) {
    // Form-encoded body; never logged (FR-023).
    char body[nvp::net::kMaxSubmissionBody + 1] = {};
    if (req->content_len > nvp::net::kMaxSubmissionBody) {
        return send_json(req, "400 Bad Request", "{\"error\":\"ssid\"}");
    }
    size_t got = 0;
    while (got < req->content_len) {
        const int r = httpd_req_recv(req, body + got, req->content_len - got);
        if (r <= 0) return send_json(req, "400 Bad Request", "{\"error\":\"ssid\"}");
        got += static_cast<size_t>(r);
    }
    char answer[64] = "{\"error\":\"busy\"}";
    int status = 409;
    if (g_cb.submit != nullptr) {
        if (!enter()) return busy(req);
        status = g_cb.submit(body, got, answer, sizeof answer);
        leave();
    }
    std::memset(body, 0, sizeof body);
    const char* line = status == 202 ? "202 Accepted" : status == 400 ? "400 Bad Request" : "409 Conflict";
    return send_json(req, line, answer);
}

// Every other host and path: back to the page, so phones open it by themselves (NET-03, R1).
// The body is not empty: iOS needs content, and Android counts a 200 of 4 bytes or less as "no
// internet"; a 302 with Location is a portal on both.
esp_err_t on_not_found(httpd_req_t* req, httpd_err_code_t) {
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", kPortal);
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_sendstr(req, "<a href=\"http://192.168.4.1/\">Kurulum</a>");
}

void add(const char* uri, httpd_method_t method, esp_err_t (*fn)(httpd_req_t*)) {
    httpd_uri_t u = {};
    u.uri = uri;
    u.method = method;
    u.handler = fn;
    httpd_register_uri_handler(g_server, &u);
}

}  // namespace

void start(const Callbacks& callbacks) {
    if (g_server != nullptr) return;
    sysinfo::note_peripheral_init();
    g_cb = callbacks;
    if (g_gate == nullptr) g_gate = xSemaphoreCreateMutex();
    xSemaphoreTake(g_gate, portMAX_DELAY);  // held by the worker except inside handle()
    // The redirect flood of a joining phone would fill the serial log (R5).
    esp_log_level_set("httpd_uri", ESP_LOG_ERROR);
    esp_log_level_set("httpd_txrx", ESP_LOG_ERROR);
    esp_log_level_set("httpd_parse", ESP_LOG_ERROR);
    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.server_port = 80;
    cfg.core_id = 0;  // network core (spec 001 research R8); the UI keeps core 1
    cfg.max_open_sockets = 7;  // phones open many connections (R5); LWIP_MAX_SOCKETS is 16
    cfg.lru_purge_enable = true;
    cfg.max_uri_handlers = 8;
    if (httpd_start(&g_server, &cfg) != ESP_OK) {
        g_server = nullptr;
        xSemaphoreGive(g_gate);
        return;
    }
    add("/", HTTP_GET, on_page);
    add("/places.json", HTTP_GET, on_places);
    add("/api/scan", HTTP_GET, on_scan);
    add("/api/status", HTTP_GET, on_status);
    add("/api/setup", HTTP_POST, on_setup);
    httpd_register_err_handler(g_server, HTTPD_404_NOT_FOUND, on_not_found);
}

void stop() {
    if (g_server == nullptr) return;
    xSemaphoreGive(g_gate);  // let any waiting handler finish before the server stops
    httpd_stop(g_server);
    g_server = nullptr;
}

bool running() { return g_server != nullptr; }

void handle() {
    if (g_server == nullptr) return;
    xSemaphoreGive(g_gate);
    vTaskDelay(pdMS_TO_TICKS(2));  // a waiting handler takes the gate here
    xSemaphoreTake(g_gate, portMAX_DELAY);
}

}  // namespace platform::portal
