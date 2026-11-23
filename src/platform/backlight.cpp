#include "backlight.h"

#include "audio_off.h"
#include "sysinfo.h"

namespace platform::backlight {

bool full_on() {
    sysinfo::note_peripheral_init();
    return silence::aux_send(silence::kAuxBacklightFull);
}

}  // namespace platform::backlight
