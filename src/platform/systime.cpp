#include "systime.h"

#include <sys/time.h>

namespace platform::systime {

int64_t utc_us() {
    timeval tv = {};
    gettimeofday(&tv, nullptr);
    return static_cast<int64_t>(tv.tv_sec) * 1000000 + tv.tv_usec;
}

void set_utc_us(int64_t utc_us) {
    timeval tv = {};
    tv.tv_sec = static_cast<time_t>(utc_us / 1000000);
    tv.tv_usec = static_cast<suseconds_t>(utc_us % 1000000);
    settimeofday(&tv, nullptr);
}

}  // namespace platform::systime
