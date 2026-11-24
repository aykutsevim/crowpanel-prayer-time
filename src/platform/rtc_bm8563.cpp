#include "rtc_bm8563.h"

#include "i2c_bus.h"
#include "nvp/time/rtc_codec.h"

namespace platform::rtc {

bool read(uint8_t raw[7]) {
    const uint8_t reg = nvp::time::kRtcFirstRegister;
    return i2c::write_read(nvp::time::kRtcAddress, &reg, 1, raw, nvp::time::kRtcTimeBytes);
}

bool write(const uint8_t raw[7]) {
    uint8_t buf[1 + nvp::time::kRtcTimeBytes];
    buf[0] = nvp::time::kRtcFirstRegister;
    for (int i = 0; i < nvp::time::kRtcTimeBytes; ++i) buf[1 + i] = raw[i];
    return i2c::write(nvp::time::kRtcAddress, buf, sizeof buf);
}

}  // namespace platform::rtc
