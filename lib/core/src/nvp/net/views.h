// What the setup, connecting and overlay screens show (02 §7.2-§7.4; spec 004 FR-004..FR-007,
// data-model.md). Plain structs built from the network state; lib/ui draws them.
#pragma once

#include <cstdint>

#include "nvp/net/failure.h"

namespace nvp::net {

enum class FooterColour : uint8_t { Muted, Ink, Alert };

struct SetupView {
    char ap_ssid[33] = {};
    char ap_pass_grouped[10] = {};  // "4827 1935": two groups of four (the password has no space)
    char qr_payload[96] = {};
    uint8_t phones = 0;
    FailureClass failure = FailureClass::None;  // last attempt, until the next one starts
};

SetupView make_setup_view(const char* ap_ssid, const char* ap_pass, uint8_t phones, FailureClass last);
const char* footer_text(const SetupView& v);
FooterColour footer_colour(const SetupView& v);

struct ConnectingView {
    char ssid[33 * 3 + 1] = {};  // after the glyph filter ('?' for what the font lacks)
    bool connected = false;
};

ConnectingView make_connecting_view(const char* ssid, bool connected);
const char* connecting_title(const ConnectingView& v);
const char* connecting_note(const ConnectingView& v);

struct OverlayInput {
    const char* province = nullptr;  // upper case as stored; nullptr when no location
    const char* district = nullptr;
    bool wifi_up = false;
    const char* ssid = "";
    int8_t rssi = 0;
    const char* ip = "";
    const char* version = "";
    bool show_qr = false;  // false on the device until the settings page exists (OD4-2)
};

struct OverlayView {
    char location[96] = {};
    char source[32] = {};       // "—" until Slice 6 (02 §9.5 names)
    char last_update[24] = {};  // "—" until Slice 6
    char last_day[16] = {};     // "—" until Slice 6
    char wifi[128] = {};
    char address[16] = {};
    char pin[12] = {};          // "—" until Slice 8
    char version[16] = {};
    bool show_qr = false;
    char qr_url[32] = {};
    bool confirm = false;
};

OverlayView make_overlay_view(const OverlayInput& in);

// The sample values of reference renders 15, 16 and 17 (02 §7, §11), for the host harness and the
// device's `fixture 15..17` command. The password 48271935 is the render's, not a device's.
SetupView fixture_setup_view();
ConnectingView fixture_connecting_view();
OverlayView fixture_overlay_view();

}  // namespace nvp::net
