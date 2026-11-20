# PlatformIO pre-build hook: runs the silence check (SIL-05) before every build and test run.
# Wired through `extra_scripts = pre:tools/pio_silence_check.py` in platformio.ini.
import subprocess
import sys
from pathlib import Path

Import("env")  # noqa: F821  (provided by PlatformIO/SCons)

_root = Path(env.subst("$PROJECT_DIR"))  # noqa: F821
_result = subprocess.run([sys.executable, str(_root / "tools" / "check_silence.py"), "--root", str(_root)])
if _result.returncode != 0:
    sys.stderr.write("Build aborted by the silence check (SIL-05).\n")
    env.Exit(1)  # noqa: F821
