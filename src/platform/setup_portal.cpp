#include "setup_portal.h"

#include <esp_http_server.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include <cstdio>
#include <cstring>

#include "nvp/diag/form.h"

#include "sysinfo.h"

namespace platform::portal {
namespace {

// The single setup page: inline HTML, CSS and JS, under 4 kB, no external requests.
// Engineering text in English (spec 001 assumptions); Slice 4 replaces it.
const char kPage[] = R"HTML(<!doctype html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1"><title>Namaz Vakti Panel setup</title>
<style>body{font:16px sans-serif;margin:16px;max-width:480px}button,input{font:inherit;padding:8px;width:100%;box-sizing:border-box;margin:4px 0}
#nets button{text-align:left;background:#eee;border:1px solid #ccc}#st{margin-top:12px;font-weight:bold}</style></head>
<body><h2>Wi-Fi setup</h2><p>Pick your home network or type its name.</p><div id="nets">Scanning...</div>
<form id="f"><input id="s" name="ssid" placeholder="Network name" maxlength="32" required>
<input id="p" name="password" type="password" placeholder="Password (empty for an open network)" maxlength="63">
<button type="submit">Connect</button></form><div id="st"></div>
<script>
const $=i=>document.getElementById(i);
function esc(t){return t.replace(/[&<>"]/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'}[c]))}
async function scan(){try{const r=await fetch('/api/scan');const j=await r.json();
$('nets').innerHTML=j.networks.length?j.networks.map(n=>'<button type="button" data-s="'+esc(n.ssid)+'">'+esc(n.ssid)+' ('+n.rssi+' dBm'+(n.secured?'':', open')+')</button>').join(''):'No networks found yet.';
for(const b of $('nets').querySelectorAll('button'))b.onclick=()=>{$('s').value=b.dataset.s;$('p').focus()};}catch(e){}}
async function status(){try{const r=await fetch('/api/status');const j=await r.json();
const m={setup:'',trying:'Connecting to '+j.ssid+'...',connected:'Connected to '+j.ssid+'. You can close this page.',failed:'Connection failed. Check the password and try again.'};
$('st').textContent=m[j.state]||'';}catch(e){}}
$('f').onsubmit=async e=>{e.preventDefault();const b=new URLSearchParams(new FormData($('f')));
const r=await fetch('/api/setup',{method:'POST',body:b});$('st').textContent=r.status==202?'Connecting...':'Check the network name and password.';};
scan();setInterval(scan,15000);setInterval(status,2000);
</script></body></html>)HTML";

static_assert(sizeof kPage < 4096, "setup page must stay under 4 kB (contracts/setup-portal.md)");

// The server runs its own task. Callbacks touch the network worker's state, so they run only
// while the worker sits in handle(): the worker holds g_gate the rest of the time.
httpd_handle_t g_server = nullptr;
SemaphoreHandle_t g_gate = nullptr;
Callbacks g_cb;

constexpr TickType_t kGateWait = pdMS_TO_TICKS(2000);
constexpr size_t kMaxBody = 256;  // ssid (32) and password (63), encoded, with room to spare

bool enter() { return xSemaphoreTake(g_gate, kGateWait) == pdTRUE; }
void leave() { xSemaphoreGive(g_gate); }

esp_err_t send_json(httpd_req_t* req, const char* status, const char* body) {
    httpd_resp_set_status(req, status);
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_sendstr(req, body);
}

esp_err_t busy(httpd_req_t* req) { return send_json(req, "503 Service Unavailable", "{\"error\":\"busy\"}"); }

esp_err_t on_page(httpd_req_t* req) {
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    return httpd_resp_send(req, kPage, sizeof kPage - 1);
}

esp_err_t on_scan(httpd_req_t* req) {
    static char body[2048];
    if (!enter()) return busy(req);
    if (g_cb.scan_json) {
        g_cb.scan_json(body, sizeof body);
    } else {
        std::snprintf(body, sizeof body, "{\"networks\":[]}");
    }
    leave();
    return send_json(req, "200 OK", body);
}

esp_err_t on_status(httpd_req_t* req) {
    char body[160];
    if (!enter()) return busy(req);
    if (g_cb.status_json) {
        g_cb.status_json(body, sizeof body);
    } else {
        std::snprintf(body, sizeof body, "{\"state\":\"setup\",\"ssid\":\"\"}");
    }
    leave();
    return send_json(req, "200 OK", body);
}

esp_err_t on_setup(httpd_req_t* req) {
    // Form-encoded body (the page sends URLSearchParams). Never logged (FR-024).
    char body[kMaxBody + 1] = {};
    if (req->content_len > kMaxBody) return send_json(req, "400 Bad Request", "{\"accepted\":false}");
    size_t got = 0;
    while (got < req->content_len) {
        const int r = httpd_req_recv(req, body + got, req->content_len - got);
        if (r <= 0) return send_json(req, "400 Bad Request", "{\"accepted\":false}");
        got += static_cast<size_t>(r);
    }
    char ssid[33] = {};
    char pass[64] = {};
    const bool parsed = nvp::diag::form_value(body, "ssid", ssid, sizeof ssid) &&
                        nvp::diag::form_value(body, "password", pass, sizeof pass);  // both always sent
    bool accepted = false;
    if (parsed && g_cb.submit != nullptr) {
        if (!enter()) return busy(req);
        accepted = g_cb.submit(ssid, pass);
        leave();
    }
    std::memset(pass, 0, sizeof pass);
    std::memset(body, 0, sizeof body);
    return accepted ? send_json(req, "202 Accepted", "{\"accepted\":true}")
                    : send_json(req, "400 Bad Request", "{\"accepted\":false}");
}

esp_err_t on_not_found(httpd_req_t* req, httpd_err_code_t) {
    httpd_resp_set_status(req, "404 Not Found");
    httpd_resp_set_type(req, "text/plain");
    return httpd_resp_sendstr(req, "not found");
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
    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.server_port = 80;
    cfg.core_id = 0;  // network core (research R8); the UI keeps core 1
    cfg.max_open_sockets = 4;
    cfg.lru_purge_enable = true;
    if (httpd_start(&g_server, &cfg) != ESP_OK) {
        g_server = nullptr;
        xSemaphoreGive(g_gate);
        return;
    }
    add("/", HTTP_GET, on_page);
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
