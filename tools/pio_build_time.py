# PlatformIO pre-build hook (device build only): defines NVP_BUILD_UTC, the start (00:00:00 UTC)
# of the build day in seconds since 1970-01-01 UTC. The firmware treats a real-time clock reading
# earlier than this as not valid (TIME-03; spec 003 research R2, FR-026).
#
# Rounded down to the day on purpose: the define reaches every compiled file, so a value that
# changed on every build would force a full rebuild each time. A one-day floor is enough to reject
# a clock that lost its time.
import time

Import("env")  # noqa: F821  (provided by PlatformIO/SCons)

_day_start = int(time.time()) // 86400 * 86400
env.Append(CPPDEFINES=[("NVP_BUILD_UTC", str(_day_start))])  # noqa: F821
