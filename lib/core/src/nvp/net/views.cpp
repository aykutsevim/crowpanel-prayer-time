#include "nvp/net/views.h"

#include <cstdio>
#include <cstring>

#include "nvp/net/scan.h"
#include "nvp/net/wifi_qr.h"
#include "nvp/text/glyph_filter.h"
#include "nvp/text/title_case.h"
#include "nvp/text/tr.h"

namespace nvp::net {
namespace tr = nvp::text::tr;

namespace {

void copy(char* dst, size_t cap, const char* src) {
    std::snprintf(dst, cap, "%s", src != nullptr ? src : "");
}

}  // namespace

SetupView make_setup_view(const char* ap_ssid, const char* ap_pass, uint8_t phones, FailureClass last) {
    SetupView v;
    copy(v.ap_ssid, sizeof v.ap_ssid, ap_ssid);
    if (ap_pass != nullptr && std::strlen(ap_pass) == 8) {
        std::snprintf(v.ap_pass_grouped, sizeof v.ap_pass_grouped, "%.4s %.4s", ap_pass, ap_pass + 4);
    }
    wifi_qr_payload(v.ap_ssid, ap_pass != nullptr ? ap_pass : "", v.qr_payload, sizeof v.qr_payload);
    v.phones = phones;
    v.failure = last;
    return v;
}

const char* footer_text(const SetupView& v) {
    switch (v.failure) {
        case FailureClass::WrongPassword: return tr::kFooterWrongPassword;
        case FailureClass::NotFound: return tr::kFooterNotFound;
        case FailureClass::Other: return tr::kFooterOther;
        case FailureClass::None: break;
    }
    return v.phones > 0 ? tr::kFooterPhoneJoined : tr::kFooterWaiting;
}

FooterColour footer_colour(const SetupView& v) {
    if (v.failure != FailureClass::None) return FooterColour::Alert;
    return v.phones > 0 ? FooterColour::Ink : FooterColour::Muted;
}

ConnectingView make_connecting_view(const char* ssid, bool connected) {
    ConnectingView v;
    nvp::text::filter_display_glyphs(ssid, v.ssid, sizeof v.ssid);
    v.connected = connected;
    return v;
}

const char* connecting_title(const ConnectingView& v) { return v.connected ? tr::kConnectedTitle : tr::kConnectingTitle; }
const char* connecting_note(const ConnectingView& v) { return v.connected ? tr::kConnectedNote : tr::kConnectingNote; }

OverlayView make_overlay_view(const OverlayInput& in) {
    OverlayView v;
    if (in.province != nullptr && in.province[0] != '\0' && in.district != nullptr) {
        char p[41], d[41];  // names are at most 40 bytes, title case keeps the length
        nvp::text::title_case_tr(in.province, p, sizeof p);
        nvp::text::title_case_tr(in.district, d, sizeof d);
        std::snprintf(v.location, sizeof v.location, "%s, %s", p, d);
    } else {
        copy(v.location, sizeof v.location, tr::kDash);
    }
    copy(v.source, sizeof v.source, in.has_source ? source_display_name(in.source) : tr::kDash);  // spec 006
    if (in.has_last_update) {
        const int64_t t = in.last_update_local;
        const int64_t days = t >= 0 ? t / 86400 : -((-t + 86399) / 86400);
        const int32_t sod = static_cast<int32_t>(t - days * 86400);
        const engine::Date d = engine::to_date(static_cast<engine::DayNumber>(days));
        std::snprintf(v.last_update, sizeof v.last_update, "%02u.%02u.%04u %02u:%02u", static_cast<unsigned>(d.day) % 100u,
                      static_cast<unsigned>(d.month) % 100u, static_cast<unsigned>(d.year) % 10000u,
                      static_cast<unsigned>(sod / 3600) % 24u, static_cast<unsigned>(sod / 60) % 60u);
    } else {
        copy(v.last_update, sizeof v.last_update, tr::kDash);
    }
    if (in.has_last_day) {
        std::snprintf(v.last_day, sizeof v.last_day, "%02u.%02u.%04u", static_cast<unsigned>(in.last_day.day) % 100u,
                      static_cast<unsigned>(in.last_day.month) % 100u, static_cast<unsigned>(in.last_day.year) % 10000u);
    } else {
        copy(v.last_day, sizeof v.last_day, tr::kDash);
    }
    copy(v.pin, sizeof v.pin, tr::kDash);             // shown from Slice 8
    if (in.wifi_up) {
        char name[33 * 3 + 1];
        nvp::text::filter_display_glyphs(in.ssid, name, sizeof name);
        std::snprintf(v.wifi, sizeof v.wifi, "%s, %s", name, signal_class(in.rssi));
        copy(v.address, sizeof v.address, in.ip);
        std::snprintf(v.qr_url, sizeof v.qr_url, "http://%s/", in.ip != nullptr ? in.ip : "");
    } else {
        copy(v.wifi, sizeof v.wifi, tr::kWifiNotConnected);
        copy(v.address, sizeof v.address, tr::kDash);
    }
    copy(v.version, sizeof v.version, in.version);
    v.show_qr = in.show_qr && in.wifi_up;
    return v;
}

const char* source_display_name(engine::SourceId s) {
    switch (s) {
        case engine::SourceId::Diyanet: return tr::kSourceDiyanet;
        case engine::SourceId::Imsakiyem: return tr::kSourceImsakiyem;
        case engine::SourceId::Emushaf: return tr::kSourceEmushaf;
        case engine::SourceId::Aladhan: return tr::kSourceAladhan;
        case engine::SourceId::Local: return tr::kSourceLocal;
    }
    return tr::kDash;
}

void city_name(const char* province, const char* district, char* out, size_t cap) {
    if (cap == 0) return;
    out[0] = '\0';
    if (province == nullptr || district == nullptr || district[0] == '\0') return;
    const char* name = std::strcmp(province, district) == 0 ? province : district;
    nvp::text::title_case_tr(name, out, cap);
}

SetupView fixture_setup_view() { return make_setup_view("NamazVakti-7F3A", "48271935", 0, FailureClass::None); }

ConnectingView fixture_connecting_view() { return make_connecting_view("EvAgi_2.4G", false); }

OverlayView fixture_overlay_view() {
    OverlayInput in;
    in.province = "İSTANBUL";
    in.district = "SULTANBEYLİ";
    in.wifi_up = true;
    in.ssid = "EvAgi_2.4G";
    in.rssi = -54;
    in.ip = "192.168.1.37";
    in.version = "1.0.0";
    in.show_qr = true;
    OverlayView v = make_overlay_view(in);
    copy(v.source, sizeof v.source, "Diyanet (imsakiyem)");
    copy(v.last_update, sizeof v.last_update, "17.11.2026 03:00");
    copy(v.last_day, sizeof v.last_day, "31.12.2026");
    copy(v.pin, sizeof v.pin, "482 913");
    return v;
}

}  // namespace nvp::net
