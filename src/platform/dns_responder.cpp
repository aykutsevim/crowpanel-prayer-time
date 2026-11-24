#include "dns_responder.h"

#include <fcntl.h>
#include <lwip/inet.h>
#include <lwip/sockets.h>
#include <unistd.h>

#include <cstring>

#include "nvp/net/dns_reply.h"

namespace platform::dns {
namespace {

int g_sock = -1;
constexpr uint8_t kApIp[4] = {192, 168, 4, 1};
constexpr int kPerPoll = 8;  // a phone sends a burst of lookups when it joins

}  // namespace

bool start() {
    if (g_sock >= 0) return true;
    const int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s < 0) return false;
    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(53);
    std::memcpy(&addr.sin_addr.s_addr, kApIp, 4);  // the access point's interface only (R14)
    if (bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof addr) != 0) {
        close(s);
        return false;
    }
    fcntl(s, F_SETFL, fcntl(s, F_GETFL, 0) | O_NONBLOCK);
    g_sock = s;
    return true;
}

void stop() {
    if (g_sock < 0) return;
    close(g_sock);
    g_sock = -1;
}

bool running() { return g_sock >= 0; }

int poll() {
    if (g_sock < 0) return 0;
    int answered = 0;
    for (int i = 0; i < kPerPoll; ++i) {
        uint8_t q[512];
        uint8_t r[528];
        sockaddr_in from = {};
        socklen_t from_len = sizeof from;
        const int n = recvfrom(g_sock, q, sizeof q, 0, reinterpret_cast<sockaddr*>(&from), &from_len);
        if (n <= 0) break;  // EWOULDBLOCK: nothing waiting
        const size_t m = nvp::net::dns_reply(q, static_cast<size_t>(n), kApIp, r, sizeof r);
        if (m == 0) continue;  // malformed, a response, or not a single question: no answer
        if (sendto(g_sock, r, m, 0, reinterpret_cast<sockaddr*>(&from), from_len) > 0) ++answered;
    }
    return answered;
}

}  // namespace platform::dns
