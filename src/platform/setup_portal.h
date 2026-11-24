// Setup page on the hotspot (contracts/setup-portal.md, research R11; FR-021, FR-022, FR-024).
// ESP-IDF HTTP server on port 80, running only while the hotspot is up. start(), stop() and
// handle() are called from the network worker; the callbacks run only inside handle().
#pragma once

#include <cstddef>

namespace platform::portal {

struct Callbacks {
    // A phone submitted details. Returns false if they are invalid (answered with 400).
    bool (*submit)(const char* ssid, const char* password) = nullptr;
    // Writes the /api/status body. It must never contain a password (NET-08).
    void (*status_json)(char* out, size_t cap) = nullptr;
    // Writes the /api/scan body.
    void (*scan_json)(char* out, size_t cap) = nullptr;
};

void start(const Callbacks& callbacks);
void stop();
bool running();
void handle();  // serve pending requests; call often while running

}  // namespace platform::portal
