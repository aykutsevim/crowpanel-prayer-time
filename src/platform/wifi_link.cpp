#include "wifi_link.h"

#include <esp_event.h>
#include <esp_mac.h>
#include <esp_netif.h>
#include <esp_random.h>
#include <esp_wifi.h>
#include <nvs.h>
#include <nvs_flash.h>

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "nvp/diag/ap_password.h"
#include "sysinfo.h"

namespace platform::wifi {
namespace {

constexpr const char* kWifiNs = "wifi";  // same namespace and keys the Arduino firmware wrote
constexpr const char* kApNs = "ap";
constexpr uint8_t kApChannel = 1;
constexpr uint8_t kApMaxClients = 2;  // contracts/setup-portal.md

enum class ScanState : uint8_t { Idle, Running, Done, Failed };

bool g_nvs_ok = false;
bool g_radio_on = false;
bool g_hotspot = false;
std::atomic<Link> g_link{Link::Down};
std::atomic<ScanState> g_scan{ScanState::Idle};
std::atomic<int> g_scan_count{0};

uint32_t random_word() { return esp_random(); }

// NVS holds the stored details; it is never erased here, whatever the init result.
bool ensure_nvs() {
    if (!g_nvs_ok) g_nvs_ok = nvs_flash_init() == ESP_OK;
    return g_nvs_ok;
}

bool get_str(nvs_handle_t h, const char* key, char* out, size_t cap) {
    size_t len = cap;
    return nvs_get_str(h, key, out, &len) == ESP_OK;
}

void on_event(void*, esp_event_base_t base, int32_t id, void* data) {
    if (base == WIFI_EVENT) {
        switch (id) {
            case WIFI_EVENT_STA_DISCONNECTED: {
                const auto* d = static_cast<const wifi_event_sta_disconnected_t*>(data);
                // Our own disconnect() is not a failure; the next join() sets Connecting again.
                if (d->reason == WIFI_REASON_ASSOC_LEAVE) {
                    if (g_link.load() != Link::Connecting) g_link = Link::Down;
                } else {
                    g_link = Link::Failed;  // reconnects follow the app's backoff (NET-01)
                }
                break;
            }
            case WIFI_EVENT_SCAN_DONE: {
                const auto* d = static_cast<const wifi_event_sta_scan_done_t*>(data);
                g_scan_count = d->number;
                g_scan = d->status == 0 ? ScanState::Done : ScanState::Failed;
                break;
            }
            default: break;
        }
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        g_link = Link::Up;
    }
}

}  // namespace

bool load_stored(char ssid[33], char pass[64]) {
    ssid[0] = '\0';
    pass[0] = '\0';
    if (!ensure_nvs()) return false;
    nvs_handle_t h;
    if (nvs_open(kWifiNs, NVS_READONLY, &h) != ESP_OK) return false;
    const bool has = get_str(h, "ssid", ssid, 33);
    if (has && !get_str(h, "pass", pass, 64)) pass[0] = '\0';
    nvs_close(h);
    return has && ssid[0] != '\0';
}

bool store(const char* ssid, const char* pass) {
    if (!ensure_nvs()) return false;
    nvs_handle_t h;
    if (nvs_open(kWifiNs, NVS_READWRITE, &h) != ESP_OK) return false;
    const bool ok = nvs_set_str(h, "ssid", ssid) == ESP_OK && nvs_set_str(h, "pass", pass) == ESP_OK &&
                    nvs_commit(h) == ESP_OK;
    nvs_close(h);
    return ok;
}

void erase_stored() {
    if (!ensure_nvs()) return;
    nvs_handle_t h;
    if (nvs_open(kWifiNs, NVS_READWRITE, &h) == ESP_OK) {
        nvs_erase_all(h);
        nvs_commit(h);
        nvs_close(h);
    }
}

void hotspot_ssid(char out[33]) {
    uint8_t mac[6] = {};
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    std::snprintf(out, 33, "NamazVakti-%02X%02X", mac[4], mac[5]);
}

void hotspot_password(char out[9]) {
    out[0] = '\0';
    nvs_handle_t h;
    if (ensure_nvs() && nvs_open(kApNs, NVS_READWRITE, &h) == ESP_OK) {
        if (!get_str(h, "pass", out, 9)) out[0] = '\0';
        if (std::strlen(out) != 8) {
            nvp::diag::make_ap_password(random_word, out);
            if (nvs_set_str(h, "pass", out) == ESP_OK) nvs_commit(h);
        }
        nvs_close(h);
    }
    if (std::strlen(out) != 8) nvp::diag::make_ap_password(random_word, out);  // NVS unavailable
}

void radio_on() {
    if (g_radio_on) return;
    sysinfo::note_peripheral_init();
    ensure_nvs();  // the Wi-Fi driver keeps its PHY calibration in NVS
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();
    esp_netif_create_default_wifi_ap();  // 192.168.4.1/24 with a DHCP server (ESP-IDF default)
    wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
    if (esp_wifi_init(&init) != ESP_OK) return;
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, on_event, nullptr);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, on_event, nullptr);
    esp_wifi_set_storage(WIFI_STORAGE_RAM);  // the details live in our NVS keys only (FR-025)
    esp_wifi_set_mode(WIFI_MODE_STA);
    g_radio_on = esp_wifi_start() == ESP_OK;
}

void join(const char* ssid, const char* pass) {
    radio_on();
    if (!g_radio_on) return;
    wifi_config_t wc = {};
    std::snprintf(reinterpret_cast<char*>(wc.sta.ssid), sizeof wc.sta.ssid, "%s", ssid);
    std::snprintf(reinterpret_cast<char*>(wc.sta.password), sizeof wc.sta.password, "%s", pass);
    wc.sta.threshold.authmode = pass[0] ? WIFI_AUTH_WPA2_PSK : WIFI_AUTH_OPEN;
    esp_wifi_disconnect();
    g_link = Link::Connecting;
    esp_wifi_set_config(WIFI_IF_STA, &wc);
    std::memset(&wc, 0, sizeof wc);
    if (esp_wifi_connect() != ESP_OK) g_link = Link::Failed;
}

void disconnect() {
    if (!g_radio_on) return;
    g_link = Link::Down;
    esp_wifi_disconnect();
}

Link link() { return g_radio_on ? g_link.load() : Link::Down; }

int8_t rssi() {
    wifi_ap_record_t ap = {};
    if (g_link.load() != Link::Up || esp_wifi_sta_get_ap_info(&ap) != ESP_OK) return 0;
    return ap.rssi;
}

bool start_hotspot(const char* ssid, const char* pass) {
    radio_on();
    if (!g_radio_on) return false;
    if (esp_wifi_set_mode(WIFI_MODE_APSTA) != ESP_OK) return false;
    wifi_config_t ac = {};
    std::snprintf(reinterpret_cast<char*>(ac.ap.ssid), sizeof ac.ap.ssid, "%s", ssid);
    ac.ap.ssid_len = static_cast<uint8_t>(std::strlen(ssid));
    std::snprintf(reinterpret_cast<char*>(ac.ap.password), sizeof ac.ap.password, "%s", pass);
    ac.ap.channel = kApChannel;
    ac.ap.max_connection = kApMaxClients;
    ac.ap.authmode = WIFI_AUTH_WPA2_PSK;
    ac.ap.pairwise_cipher = WIFI_CIPHER_TYPE_CCMP;
    g_hotspot = esp_wifi_set_config(WIFI_IF_AP, &ac) == ESP_OK;
    std::memset(&ac, 0, sizeof ac);
    if (!g_hotspot) esp_wifi_set_mode(WIFI_MODE_STA);
    return g_hotspot;
}

void stop_hotspot() {
    if (!g_hotspot) return;
    esp_wifi_set_mode(WIFI_MODE_STA);
    g_hotspot = false;
}

bool hotspot_up() { return g_hotspot; }

const char* hotspot_address() { return "192.168.4.1"; }

bool scan_start() {
    radio_on();
    if (!g_radio_on || g_scan.load() == ScanState::Running) return false;
    g_scan = ScanState::Running;
    if (esp_wifi_scan_start(nullptr, false) != ESP_OK) {
        g_scan = ScanState::Idle;
        return false;
    }
    return true;
}

int scan_poll() {
    switch (g_scan.load()) {
        case ScanState::Running: return -1;
        case ScanState::Done: return g_scan_count.load();
        default: return -2;
    }
}

size_t scan_take(nvp::diag::ScanEntry* out, size_t max) {
    size_t taken = 0;
    uint16_t n = static_cast<uint16_t>(max);
    auto* recs = static_cast<wifi_ap_record_t*>(std::calloc(max, sizeof(wifi_ap_record_t)));
    if (recs != nullptr && esp_wifi_scan_get_ap_records(&n, recs) == ESP_OK) {
        for (uint16_t i = 0; i < n && taken < max; ++i) {
            nvp::diag::ScanEntry& e = out[taken++];
            std::snprintf(e.ssid, sizeof e.ssid, "%s", reinterpret_cast<const char*>(recs[i].ssid));
            e.rssi = recs[i].rssi;
            e.secured = recs[i].authmode != WIFI_AUTH_OPEN;
        }
    }
    std::free(recs);
    esp_wifi_clear_ap_list();  // frees the rest of the driver's list
    g_scan = ScanState::Idle;
    return taken;
}

}  // namespace platform::wifi
