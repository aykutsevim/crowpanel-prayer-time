#include "wifi_link.h"

#include <esp_event.h>
#include <esp_mac.h>
#include <esp_netif.h>
#include <esp_random.h>
#include <esp_timer.h>
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
constexpr uint8_t kApChannel = 1;        // the AP follows the station's channel once it joins (R2)
constexpr uint8_t kApMaxClients = 2;     // NET-02
constexpr const char* kPortalUri = "http://192.168.4.1/";  // DHCP option 114 (R1)
constexpr uint8_t kLastChannel = 13;     // Türkiye: channels 1-13
constexpr uint8_t kGroup = 4;            // channels per group before a pause (R3)
constexpr uint32_t kGroupPauseUs = 120000;
constexpr uint32_t kDwellMs = 120;
constexpr size_t kAccumulated = 40;

enum class ScanState : uint8_t { Idle, Running, Done, Failed };

bool g_nvs_ok = false;
bool g_radio_on = false;
bool g_hotspot = false;
esp_netif_t* g_sta = nullptr;
esp_netif_t* g_ap = nullptr;
std::atomic<Link> g_link{Link::Down};
std::atomic<ScanState> g_scan{ScanState::Idle};
std::atomic<int> g_clients{0};
// Disconnect reasons from the event task, read by the worker (single producer, single consumer).
constexpr size_t kReasons = 16;
std::atomic<uint16_t> g_reason[kReasons];
std::atomic<uint32_t> g_reason_head{0};
uint32_t g_reason_tail = 0;

// Grouped scan, driven by scan_poll() on the worker.
bool g_grouped = false;
uint8_t g_channel = 0;
int64_t g_resume_at = 0;
nvp::net::ScanEntry g_acc[kAccumulated];
size_t g_acc_n = 0;

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
                if (d->reason == WIFI_REASON_ASSOC_LEAVE) {  // our own disconnect()
                    if (g_link.load() != Link::Connecting) g_link = Link::Down;
                } else {
                    const uint32_t h = g_reason_head.load();
                    g_reason[h % kReasons] = d->reason;
                    g_reason_head = h + 1;
                    g_link = Link::Failed;  // reconnects follow the app (NET-01, MODE-04)
                }
                break;
            }
            case WIFI_EVENT_SCAN_DONE: {
                const auto* d = static_cast<const wifi_event_sta_scan_done_t*>(data);
                g_scan = d->status == 0 ? ScanState::Done : ScanState::Failed;
                break;
            }
            case WIFI_EVENT_AP_STACONNECTED:
                g_clients.fetch_add(1);
                break;
            case WIFI_EVENT_AP_STADISCONNECTED:
                if (g_clients.load() > 0) g_clients.fetch_sub(1);
                break;
            default: break;
        }
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        g_link = Link::Up;
    }
}

bool start_channel_scan(uint8_t channel) {
    wifi_scan_config_t sc = {};
    sc.channel = channel;  // 0: all channels
    sc.show_hidden = false;
    sc.scan_type = WIFI_SCAN_TYPE_ACTIVE;
    sc.scan_time.active.min = 0;
    sc.scan_time.active.max = kDwellMs;
    g_scan = ScanState::Running;
    if (esp_wifi_scan_start(&sc, false) != ESP_OK) {
        g_scan = ScanState::Failed;
        return false;
    }
    return true;
}

// Moves the driver's current results into the accumulator (grouped scans) and frees them.
void collect() {
    uint16_t n = 0;
    esp_wifi_scan_get_ap_num(&n);
    if (n > 0) {
        auto* recs = static_cast<wifi_ap_record_t*>(std::calloc(n, sizeof(wifi_ap_record_t)));
        if (recs != nullptr && esp_wifi_scan_get_ap_records(&n, recs) == ESP_OK) {
            for (uint16_t i = 0; i < n && g_acc_n < kAccumulated; ++i) {
                nvp::net::ScanEntry& e = g_acc[g_acc_n++];
                std::snprintf(e.ssid, sizeof e.ssid, "%s", reinterpret_cast<const char*>(recs[i].ssid));
                e.rssi = recs[i].rssi;
                e.auth = nvp::net::auth_class(static_cast<int>(recs[i].authmode));
            }
        }
        std::free(recs);
    }
    esp_wifi_clear_ap_list();
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
    const bool ok = nvs_set_str(h, "pass", pass) == ESP_OK && nvs_commit(h) == ESP_OK &&
                    nvs_set_str(h, "ssid", ssid) == ESP_OK && nvs_commit(h) == ESP_OK;
    nvs_close(h);
    return ok;
}

void erase_stored() {
    if (!ensure_nvs()) return;
    nvs_handle_t h;
    if (nvs_open(kWifiNs, NVS_READWRITE, &h) == ESP_OK) {
        nvs_erase_key(h, "ssid");  // the name first: without it the details are absent
        nvs_commit(h);
        nvs_erase_key(h, "pass");
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
        if (std::strlen(out) != 8) {  // first use only; a Slice 1 password is kept (R15)
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
    g_sta = esp_netif_create_default_wifi_sta();
    g_ap = esp_netif_create_default_wifi_ap();  // 192.168.4.1/24, DHCP server with DNS (ESP-IDF default)
    wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
    if (esp_wifi_init(&init) != ESP_OK) return;
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, on_event, nullptr);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, on_event, nullptr);
    esp_wifi_set_storage(WIFI_STORAGE_RAM);  // the details live in our NVS keys only (FR-025 of spec 001)
    esp_wifi_set_mode(WIFI_MODE_STA);
    g_radio_on = esp_wifi_start() == ESP_OK;
    if (g_radio_on && g_ap != nullptr) {  // option 114: harmless, unused for http:// (R1)
        esp_netif_dhcps_stop(g_ap);
        esp_netif_dhcps_option(g_ap, ESP_NETIF_OP_SET, ESP_NETIF_CAPTIVEPORTAL_URI,
                               const_cast<char*>(kPortalUri), std::strlen(kPortalUri));
        esp_netif_dhcps_start(g_ap);
    }
}

void join(const char* ssid, const char* pass) {
    radio_on();
    if (!g_radio_on) return;
    wifi_config_t wc = {};
    std::snprintf(reinterpret_cast<char*>(wc.sta.ssid), sizeof wc.sta.ssid, "%s", ssid);
    std::snprintf(reinterpret_cast<char*>(wc.sta.password), sizeof wc.sta.password, "%s", pass);
    // Threshold OPEN for an open network, WPA2-PSK otherwise (accepts WPA2, WPA2/WPA3, WPA3; R4).
    wc.sta.threshold.authmode = pass[0] ? WIFI_AUTH_WPA2_PSK : WIFI_AUTH_OPEN;
    wc.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;  // probes every channel: hidden networks too
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

void station_ip(char out[16]) {
    out[0] = '\0';
    esp_netif_ip_info_t ip = {};
    if (g_sta == nullptr || g_link.load() != Link::Up || esp_netif_get_ip_info(g_sta, &ip) != ESP_OK) return;
    std::snprintf(out, 16, IPSTR, IP2STR(&ip.ip));
}

size_t take_reasons(uint16_t* out, size_t max) {
    const uint32_t head = g_reason_head.load();
    if (head - g_reason_tail > kReasons) g_reason_tail = head - kReasons;  // overwritten: keep the newest
    size_t n = 0;
    while (g_reason_tail != head) {
        const uint16_t r = g_reason[g_reason_tail % kReasons].load();
        ++g_reason_tail;
        if (n < max) out[n++] = r;
    }
    return n;
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
    ac.ap.pmf_cfg.required = false;  // older phones join too (R2)
    g_hotspot = esp_wifi_set_config(WIFI_IF_AP, &ac) == ESP_OK;
    std::memset(&ac, 0, sizeof ac);
    if (!g_hotspot) esp_wifi_set_mode(WIFI_MODE_STA);
    g_clients = 0;
    return g_hotspot;
}

void stop_hotspot() {
    if (!g_hotspot) return;
    esp_wifi_set_mode(WIFI_MODE_STA);
    g_hotspot = false;
    g_clients = 0;
}

bool hotspot_up() { return g_hotspot; }

const char* hotspot_address() { return "192.168.4.1"; }

uint8_t hotspot_clients() {
    const int n = g_clients.load();
    return static_cast<uint8_t>(n < 0 ? 0 : n);
}

bool scan_start(bool grouped) {
    radio_on();
    if (!g_radio_on || scan_running()) return false;
    g_grouped = grouped;
    g_acc_n = 0;
    g_resume_at = 0;
    g_channel = grouped ? 1 : 0;
    return start_channel_scan(g_channel);
}

int scan_poll() {
    const ScanState s = g_scan.load();
    if (s == ScanState::Failed) {
        g_grouped = false;
        return -2;
    }
    if (s == ScanState::Idle) return static_cast<int>(g_acc_n);
    if (!g_grouped) {
        if (s == ScanState::Running) return -1;
        return 0;  // Done; scan_take() reads the driver's list
    }
    // Grouped: collect each channel, pause after every fourth, then go on.
    if (s == ScanState::Done && g_resume_at == 0) {
        collect();
        if (g_channel >= kLastChannel) {
            g_grouped = false;
            g_scan = ScanState::Idle;
            return static_cast<int>(g_acc_n);
        }
        g_resume_at = esp_timer_get_time() + (g_channel % kGroup == 0 ? kGroupPauseUs : 0);
    }
    if (g_resume_at != 0 && esp_timer_get_time() >= g_resume_at) {
        g_resume_at = 0;
        ++g_channel;
        if (!start_channel_scan(g_channel)) return -2;
    }
    return -1;
}

size_t scan_take(nvp::net::ScanEntry* out, size_t max) {
    if (g_scan.load() == ScanState::Done && !g_grouped) collect();  // a full scan
    const size_t n = g_acc_n < max ? g_acc_n : max;
    std::memcpy(out, g_acc, n * sizeof(nvp::net::ScanEntry));
    g_acc_n = 0;
    g_scan = ScanState::Idle;
    return n;
}

bool scan_running() { return g_scan.load() == ScanState::Running || g_grouped; }

void scan_stop() {
    if (!scan_running()) return;
    esp_wifi_scan_stop();
    esp_wifi_clear_ap_list();
    g_grouped = false;
    g_acc_n = 0;
    g_resume_at = 0;
    g_scan = ScanState::Idle;
}

}  // namespace platform::wifi
