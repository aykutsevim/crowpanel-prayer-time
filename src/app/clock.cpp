#include "clock.h"

#include <atomic>
#include <cstdio>
#include <cstring>

#include "nvp/engine/calendar.h"
#include "nvp/time/posix_zone.h"
#include "nvp/time/sim_clock.h"
#include "platform/console_io.h"
#include "platform/rtc_bm8563.h"
#include "platform/settings_store.h"
#include "platform/sysinfo.h"
#include "platform/systime.h"
#include "platform/tasks.h"
#include "platform/time_sync.h"

#ifndef NVP_BUILD_UTC
#define NVP_BUILD_UTC 0  // host builds; the device build defines it (tools/pio_build_time.py)
#endif

namespace app::clock {
namespace {

using nvp::time::RtcReading;
using nvp::time::SyncOutcome;
using nvp::time::TimeSample;
using nvp::time::TimeSource;

Source g_source = Source::Real;
nvp::time::WallClock g_wall;
nvp::time::SimClock g_sim;
nvp::time::Zone g_zone = nvp::time::default_zone();
char g_zone_text[64] = {};
RtcReading g_rtc;  // the reading at start (for the "not valid" reason)
bool g_rtc_write_failed_logged = false;
std::atomic<bool> g_valid{false};
std::atomic<uint32_t> g_network_syncs{0};

platform::tasks::Lock* g_lock = nullptr;  // guards the posted fallback sample
bool g_posted = false;
TimeSample g_post;

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

bool write_chip(int64_t utc_s) {
    uint8_t raw[nvp::time::kRtcTimeBytes];
    return nvp::time::encode(utc_s, raw) && platform::rtc::write(raw);
}

std::atomic<uint8_t> g_last_source{0};  // TimeSource, read by the network worker (T038)

void publish() {
    g_valid = g_wall.valid();
    g_last_source = static_cast<uint8_t>(g_wall.source());
}

// Applies one sample through the validity rule; logs one line (FR-014).
void apply(const TimeSample& s) {
    const int64_t up = platform::sysinfo::uptime_us();
    const nvp::time::SyncResult r = g_wall.accept(s, platform::systime::utc_us(), up, NVP_BUILD_UTC);
    char line[128];
    const unsigned long t = static_cast<unsigned long>(up / 1000);
    if (r.outcome == SyncOutcome::RefusedBeforeBuild) {
        std::snprintf(line, sizeof line, "[%8lu] time: %s%s%s refused (before build)\n", t,
                      nvp::time::source_name(s.source), s.server[0] ? " " : "", s.server);
        platform::console::write(line);
        return;
    }
    if (r.outcome == SyncOutcome::IgnoredNtpFresh) return;  // an older fallback answer: nothing to say
    platform::systime::set_utc_us(r.new_utc_us);
    if (s.source == TimeSource::Ntp || s.source == TimeSource::Http) g_network_syncs.fetch_add(1);
    const char* rtc = "";
    if (r.write_rtc) {
        if (write_chip(r.new_utc_us / 1000000)) {
            rtc = ", rtc written";
        } else if (!g_rtc_write_failed_logged) {
            g_rtc_write_failed_logged = true;  // once per boot (analysis C1)
            rtc = ", rtc write failed";
        }
    }
    const int64_t c = r.correction_us;
    const int64_t a = c < 0 ? -c : c;
    std::snprintf(line, sizeof line, "[%8lu] time: %s%s%s ok, correction %c%lld.%03lld s%s\n", t,
                  nvp::time::source_name(s.source), s.server[0] ? " " : "", s.server, c < 0 ? '-' : '+', static_cast<long long>(a / 1000000),
                  static_cast<long long>((a % 1000000) / 1000), rtc);
    platform::console::write(line);
    publish();
}

}  // namespace

void start() {
    if (g_lock == nullptr) g_lock = new platform::tasks::Lock();
    // The stored zone (TIME-04), or the default.
    if (!platform::settings::load_tz(g_zone_text, sizeof g_zone_text) || !nvp::time::parse_zone(g_zone_text, g_zone)) {
        std::snprintf(g_zone_text, sizeof g_zone_text, "%s", nvp::time::kDefaultZone);
        g_zone = nvp::time::default_zone();
    }
    g_rtc = read_chip();
    g_source = Source::Real;
    char line[112];
    if (g_rtc.valid) {
        TimeSample s;
        s.source = TimeSource::Rtc;
        s.utc_us = g_rtc.utc * 1000000;
        s.at_uptime_us = platform::sysinfo::uptime_us();
        s.server = "bm8563";
        platform::systime::set_utc_us(s.utc_us);
        g_wall.accept(s, s.utc_us, s.at_uptime_us, NVP_BUILD_UTC);
        char when[40];
        format_local(nvp::time::utc_to_local(g_zone, g_rtc.utc), when, sizeof when);
        std::snprintf(line, sizeof line, "rtc: %s local, valid\n", when);
    } else {
        // No valid time: the system clock starts at the build so time() never reads 1970; nothing is
        // shown as valid and HTTPS stays refused (R3).
        platform::systime::set_utc_us(static_cast<int64_t>(NVP_BUILD_UTC) * 1000000);
        std::snprintf(line, sizeof line, "rtc: not valid (%s)\n", nvp::time::reason_name(g_rtc.reason));
    }
    publish();
    platform::console::write(line);
}

Now now() {
    Now n;
    if (g_source == Source::Simulated) {
        n.valid = true;
        n.local = g_sim.now(platform::sysinfo::uptime_ms());
        return n;
    }
    if (!g_wall.valid()) return n;
    n.valid = true;
    n.local = nvp::time::utc_to_local(g_zone, platform::systime::utc_us() / 1000000);
    return n;
}

Source source() { return g_source; }

void apply_pending() {
    TimeSample s;
    if (platform::time_sync::take_sample(s)) apply(s);
    bool have = false;
    if (g_lock != nullptr) {
        platform::tasks::Guard guard(*g_lock);
        have = g_posted;
        if (have) s = g_post;
        g_posted = false;
    }
    if (have) apply(s);
}

void post_sample(const TimeSample& s) {
    if (g_lock == nullptr) return;
    platform::tasks::Guard guard(*g_lock);
    g_post = s;
    g_posted = true;
}

bool time_valid() { return g_valid.load(); }
uint32_t network_syncs() { return g_network_syncs.load(); }

TimeSource last_source() { return static_cast<TimeSource>(g_last_source.load()); }

RtcReading read_rtc() { return read_chip(); }

bool set_rtc_local(int64_t local, const char** why) {
    const int64_t utc = nvp::time::local_to_utc(g_zone, local);
    uint8_t raw[nvp::time::kRtcTimeBytes];
    if (!nvp::time::encode(utc, raw)) {
        *why = "outside 2000..2099";
        return false;
    }
    if (!platform::rtc::write(raw)) {
        *why = "no answer";
        return false;
    }
    const RtcReading back = read_chip();  // read back: proves the write
    if (!back.valid) {
        *why = nvp::time::reason_name(back.reason);
        return false;
    }
    TimeSample s;
    s.source = TimeSource::Rtc;
    s.utc_us = back.utc * 1000000;
    s.at_uptime_us = platform::sysinfo::uptime_us();
    s.server = "bm8563";
    platform::systime::set_utc_us(s.utc_us);
    g_wall.invalidate();  // the hand-set time replaces whatever was there
    g_wall.accept(s, s.utc_us, s.at_uptime_us, NVP_BUILD_UTC);
    g_source = Source::Real;
    publish();
    return true;
}

void invalidate() {
    g_wall.invalidate();
    platform::systime::set_utc_us(static_cast<int64_t>(NVP_BUILD_UTC) * 1000000);
    publish();
}

void forget_ntp_freshness() { g_wall.forget_ntp_freshness(); }

void use_real() { g_source = Source::Real; }

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

const char* zone() { return g_zone_text; }

bool set_zone(const char* posix) {
    nvp::time::Zone z;
    if (posix == nullptr || std::strlen(posix) >= sizeof g_zone_text || !nvp::time::parse_zone(posix, z)) return false;
    if (!platform::settings::store_tz(posix)) return false;
    g_zone = z;
    std::snprintf(g_zone_text, sizeof g_zone_text, "%s", posix);
    return true;
}

int64_t to_local(int64_t utc) { return nvp::time::utc_to_local(g_zone, utc); }
int64_t to_utc(int64_t local) { return nvp::time::local_to_utc(g_zone, local); }

void describe(char* out, size_t cap) {
    char when[40];
    const Now n = now();
    if (g_source == Source::Simulated) {
        format_local(n.local, when, sizeof when);
        std::snprintf(out, cap, "sim %s x%u%s", when, static_cast<unsigned>(g_sim.speed()),
                      g_sim.paused() ? " paused" : "");
    } else if (n.valid) {
        format_local(n.local, when, sizeof when);
        std::snprintf(out, cap, "real %s local, valid", when);
    } else {
        if (g_rtc.valid) {
            std::snprintf(out, cap, "real not valid");  // `clock invalid` (spec 005 R11)
        } else {
            std::snprintf(out, cap, "real not valid (rtc %s)", nvp::time::reason_name(g_rtc.reason));
        }
    }
}

void describe_time(char* out, size_t cap) {
    if (!g_wall.valid()) {
        std::snprintf(out, cap, "not valid (waiting for network), zone %s", g_zone_text);
        return;
    }
    if (g_wall.source() == TimeSource::Rtc) {
        std::snprintf(out, cap, "valid, source rtc (no network sync yet), zone %s", g_zone_text);
        return;
    }
    const int64_t age_min = (platform::sysinfo::uptime_us() - g_wall.last_sync_uptime_us()) / 60000000;
    const int64_t c = g_wall.last_correction_us();
    const int64_t a = c < 0 ? -c : c;
    std::snprintf(out, cap, "valid, source %s, last sync %lld min ago, correction %c%lld.%03lld s, zone %s",
                  nvp::time::source_name(g_wall.source()), static_cast<long long>(age_min), c < 0 ? '-' : '+',
                  static_cast<long long>(a / 1000000), static_cast<long long>((a % 1000000) / 1000), g_zone_text);
}

}  // namespace app::clock
