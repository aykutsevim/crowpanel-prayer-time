# PlatformIO pre-build hook: tells LVGL and lib/ui where include/lv_conf.h is.
# In ESP-IDF builds PlatformIO passes -D but not -I build flags to libraries, and a path written in
# platformio.ini loses its Windows backslashes, so the define is built here with forward slashes.
Import("env")  # noqa: F821  (provided by PlatformIO/SCons)

_path = env.subst("$PROJECT_DIR").replace("\\", "/") + "/include/lv_conf.h"  # noqa: F821
env.Append(CPPDEFINES=[("LV_CONF_PATH", env.StringifyMacro(_path))])  # noqa: F821
