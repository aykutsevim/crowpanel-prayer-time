#include "clock.h"

#include <cstdio>

#include "nvp/engine/calendar.h"
#include "nvp/time/sim_clock.h"
#include "nvp/time/zone.h"
#include "platform/console_io.h"
#include "platform/rtc_bm8563.h"
#include "platform/sysinfo.h"

#ifndef NVP_BUILD_UTC
#define NVP_BUILD_UTC 0  // host builds; the device build defines it (tools/pio_build_time.py)
#endif

namespace app::clock {
namespace {

using nvp::time::RtcInvalid;
using nvp::time::RtcReading;

Source g_source = Source::Rtc;
RtcReading g_rtc;           // the reading the Rtc source is based on
int64_t g_rtc_base_us = 0;  // uptime when it was read
nvp::time::SimClock g_sim;

void format_local(int64_t local, char* out, size_t cap) {
    const int64_t days = local >= 0 ? local / 86400 : -((-local + 86399) / 86400);
    const int64_t sod = local - days * 86400;
    const nvp::engine::Date d = nvp::engine::to_date(static_cast<nvp::engine::DayNumber>(days));
    const int s = static_cast<int>(sod);  // 0..86399
    std::snprintf(out, cap, "%04d-%02u-%02u %02d:%02d:%02d", static_cast<int>(d.year) % 10000, d.month % 100u,
                  d.day % 100u, (s / 3600) % 24, (s / 60) % 60, s % 60);
}

RtcReading read_chip() {
    uint8_t raw[nvp::time::kRtcTimeBytes];
    if (!platform::rtc::read(raw)) return RtcReading{};  // reason NoAnswer
    return nvp::time::decode(raw, NVP_BUILD_UTC);
}

}  // namespace

void start() {
    g_rtc = read_chip();
    g_rtc_base_us = platform::sysinfo::uptime_us();
    g_source = Source::Rtc;
    char line[96];
    if (g_rtc.valid) {
        char when[40];
        format_local(nvp::time::utc_to_local(g_rtc.utc), when, sizeof when);
        std::snprintf(line, sizeof line, "rtc: %s local, valid\n", when);
    } else {
        std::snprintf(line, sizeof line, "rtc: not valid (%s)\n", nvp::time::reason_name(g_rtc.reason));
    }
    platform::console::write(line);
}

Now now() {
    Now n;
    if (g_source == Source::Simulated) {
        n.valid = true;
        n.local = g_sim.now(platform::sysinfo::uptime_ms());
        return n;
    }
    if (!g_rtc.valid) return n;
    const int64_t elapsed_s = (platform::sysinfo::uptime_us() - g_rtc_base_us) / 1000000;
    n.valid = true;
    n.local = nvp::time::utc_to_local(g_rtc.utc + elapsed_s);
    return n;
}

Source source() { return g_source; }

RtcReading read_rtc() { return read_chip(); }

bool set_rtc_local(int64_t local, const char** why) {
    uint8_t raw[nvp::time::kRtcTimeBytes];
    const int64_t utc = nvp::time::local_to_utc(local);
    if (!nvp::time::encode(utc, raw)) {
        *why = "outside 2000..2099";
        return false;
    }
    if (!platform::rtc::write(raw)) {
        *why = "no answer";
        return false;
    }
    g_rtc = read_chip();  // read back: proves the write and re-bases the running time
    g_rtc_base_us = platform::sysinfo::uptime_us();
    g_source = Source::Rtc;
    if (!g_rtc.valid) {
        *why = nvp::time::reason_name(g_rtc.reason);
        return false;
    }
    return true;
}

void use_rtc() { g_source = Source::Rtc; }

void use_sim(int64_t local, bool paused) {
    const uint32_t ms = platform::sysinfo::uptime_ms();
    g_sim.set(local, ms);
    if (paused) g_sim.pause(ms);
    g_source = Source::Simulated;
}

bool set_speed(uint16_t speed) {
    if (g_source != Source::Simulated) return false;
    return g_sim.set_speed(speed, platform::sysinfo::uptime_ms());
}

void pause() { g_sim.pause(platform::sysinfo::uptime_ms()); }
void run() { g_sim.run(platform::sysinfo::uptime_ms()); }

void describe(char* out, size_t cap) {
    char when[40];
    const Now n = now();
    if (g_source == Source::Simulated) {
        format_local(n.local, when, sizeof when);
        std::snprintf(out, cap, "sim %s x%u%s", when, static_cast<unsigned>(g_sim.speed()),
                      g_sim.paused() ? " paused" : "");
    } else if (n.valid) {
        format_local(n.local, when, sizeof when);
        std::snprintf(out, cap, "rtc %s local, valid", when);
    } else {
        std::snprintf(out, cap, "rtc not valid (%s)", nvp::time::reason_name(g_rtc.reason));
    }
}

}  // namespace app::clock
