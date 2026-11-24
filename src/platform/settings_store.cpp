#include "settings_store.h"

#include <esp_random.h>
#include <nvs.h>
#include <nvs_flash.h>

#include <cstring>

#include "nvp/diag/ap_password.h"

namespace platform::settings {
namespace {

constexpr const char* kLocNs = "loc";
constexpr const char* kSecNs = "sec";

uint32_t random_word() { return esp_random(); }

// The clock reads the zone at start, before the network worker has opened NVS (spec 005 T034:
// a stored zone was lost across a restart). nvs_flash_init() is safe to call again.
bool ensure_nvs() {
    static bool ok = false;
    if (!ok) ok = nvs_flash_init() == ESP_OK;
    return ok;
}

bool get_str(nvs_handle_t h, const char* key, char* out, size_t cap) {
    size_t len = cap;
    return nvs_get_str(h, key, out, &len) == ESP_OK;
}

}  // namespace

bool load_location(nvp::net::Location& out) {
    out = nvp::net::Location{};
    nvs_handle_t h;
    if (nvs_open(kLocNs, NVS_READONLY, &h) != ESP_OK) return false;
    uint32_t id = 0;
    const bool ok = nvs_get_u32(h, "district_id", &id) == ESP_OK && id != 0 &&
                    get_str(h, "province", out.province, sizeof out.province) &&
                    get_str(h, "district", out.district, sizeof out.district);
    nvs_close(h);
    if (!ok) {
        out = nvp::net::Location{};
        return false;
    }
    out.district_id = id;
    return true;
}

bool store_location(const nvp::net::Location& loc) {
    nvs_handle_t h;
    if (nvs_open(kLocNs, NVS_READWRITE, &h) != ESP_OK) return false;
    // The names first, the id last: the location counts as present only with an id (R10).
    const bool ok = nvs_set_str(h, "province", loc.province) == ESP_OK &&
                    nvs_set_str(h, "district", loc.district) == ESP_OK && nvs_commit(h) == ESP_OK &&
                    nvs_set_u32(h, "district_id", loc.district_id) == ESP_OK && nvs_commit(h) == ESP_OK;
    nvs_close(h);
    return ok;
}

bool ensure_pin() {
    nvs_handle_t h;
    if (nvs_open(kSecNs, NVS_READWRITE, &h) != ESP_OK) return false;
    char pin[7] = {};
    bool ok = get_str(h, "pin", pin, sizeof pin) && std::strlen(pin) == 6;
    if (!ok) {
        nvp::diag::make_settings_pin(random_word, pin);
        ok = nvs_set_str(h, "pin", pin) == ESP_OK && nvs_commit(h) == ESP_OK;
    }
    std::memset(pin, 0, sizeof pin);
    nvs_close(h);
    return ok;
}

bool load_tz(char* out, size_t cap) {
    if (cap > 0) out[0] = '\0';
    nvs_handle_t h;
    if (!ensure_nvs() || nvs_open("set", NVS_READONLY, &h) != ESP_OK) return false;
    const bool ok = get_str(h, "tz", out, cap) && out[0] != '\0';
    nvs_close(h);
    if (!ok && cap > 0) out[0] = '\0';
    return ok;
}

bool store_tz(const char* tz) {
    nvs_handle_t h;
    if (!ensure_nvs() || nvs_open("set", NVS_READWRITE, &h) != ESP_OK) return false;
    const bool ok = nvs_set_str(h, "tz", tz) == ESP_OK && nvs_commit(h) == ESP_OK;
    nvs_close(h);
    return ok;
}

}  // namespace platform::settings
