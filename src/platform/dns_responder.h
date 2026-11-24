// Captive-portal DNS responder (NET-03; spec 004 FR-011, contracts/portal-http.md, research R1, R5).
// One non-blocking UDP socket on port 53 of the access point's address; the answers come from
// nvp::net::dns_reply. Polled by the network worker while the device's network is up.
#pragma once

#include <cstdint>

namespace platform::dns {

bool start();   // binds 192.168.4.1:53
void stop();
bool running();
// Answers the queries waiting on the socket (at most a few per call). Returns how many it answered.
int poll();

}  // namespace platform::dns
