# PlatformIO pre-build hook (env crowpanel7_advance): builds the setup page and the place list into
# src/generated/web_assets.{h,cpp} before the ESP-IDF CMake step reads src/ (spec 004 R7, SC-006).
# A page over its 25 600 B gzip budget, or a missing place list, stops the build.
import sys
from pathlib import Path

Import("env")  # noqa: F821  (provided by PlatformIO/SCons)

_tools = Path(env.subst("$PROJECT_DIR")) / "tools"  # noqa: F821
sys.path.insert(0, str(_tools))
import gen_web  # noqa: E402

try:
    gen_web.main()
except gen_web.WebError as e:
    sys.stderr.write(f"gen_web: {e}\n")
    env.Exit(1)  # noqa: F821
