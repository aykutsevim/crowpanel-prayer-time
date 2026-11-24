// Setup page and its API on the device's network (NET-03, NET-04; spec 004 FR-011, FR-012,
// contracts/portal-http.md; extends spec 001 contracts/setup-portal.md). ESP-IDF HTTP server on
// port 80, running only while the device's network is up. start(), stop() and handle() are called
// from the network worker; the callbacks run only inside handle(), on the worker's state.
#pragma once

#include <cstddef>

namespace platform::portal {

struct Callbacks {
    // A phone submitted the form (body already bounded to 512 B). Writes the JSON answer into `out`
    // and returns the HTTP status: 202, 400 or 409. The body is wiped after the call.
    int (*submit)(const char* body, size_t len, char* out, size_t cap) = nullptr;
    // Writes the /api/status body. It never contains a password (NET-08).
    void (*status_json)(char* out, size_t cap) = nullptr;
    // Writes the /api/scan body; `refresh` asks for a new grouped scan (research R3).
    void (*scan_json)(bool refresh, char* out, size_t cap) = nullptr;
};

void start(const Callbacks& callbacks);
void stop();
bool running();
void handle();  // serve pending requests; call often while running

}  // namespace platform::portal
