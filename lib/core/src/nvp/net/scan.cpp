#include "nvp/net/scan.h"

#include <cstring>

namespace nvp::net {

Auth auth_class(int raw) {
    switch (raw) {
        case 0: return Auth::Open;      // OPEN
        case 2:                         // WPA_PSK
        case 3:                         // WPA2_PSK
        case 4:                         // WPA_WPA2_PSK
        case 6:                         // WPA3_PSK
        case 7:                         // WPA2_WPA3_PSK
        case 11:                        // WPA3_EXT_PSK
        case 12:                        // WPA3_EXT_PSK_MIXED_MODE
            return Auth::Secured;
        default:  // 1 WEP, 5 ENTERPRISE, 8 WAPI, 9 OWE, 10 WPA3_ENT_192, 13 DPP, 14-16 enterprise, unknown
            return Auth::Unsupported;
    }
}

size_t merge_scan(ScanEntry* e, size_t n) {
    size_t m = 0;
    for (size_t i = 0; i < n; ++i) {
        if (e[i].ssid[0] == '\0') continue;
        size_t j = 0;
        while (j < m && std::strcmp(e[j].ssid, e[i].ssid) != 0) ++j;
        if (j < m) {
            if (e[i].rssi > e[j].rssi) e[j] = e[i];
        } else {
            e[m++] = e[i];
        }
    }
    // Insertion sort, strongest first; stable for equal signals.
    for (size_t i = 1; i < m; ++i) {
        const ScanEntry x = e[i];
        size_t j = i;
        while (j > 0 && e[j - 1].rssi < x.rssi) {
            e[j] = e[j - 1];
            --j;
        }
        e[j] = x;
    }
    return m < kMaxScanEntries ? m : kMaxScanEntries;
}

const char* signal_class(int rssi) {
    if (rssi >= -60) return "iyi";
    if (rssi >= -75) return "orta";
    return "zayıf";
}

const char* auth_name(Auth a) {
    switch (a) {
        case Auth::Open: return "open";
        case Auth::Secured: return "secured";
        case Auth::Unsupported: return "unsupported";
    }
    return "unsupported";
}

}  // namespace nvp::net
