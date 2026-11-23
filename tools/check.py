#!/usr/bin/env python3
"""`check`: the one command every slice must pass (05-architecture.md §11).

Steps, in order, stopping at the first failure:
  1. unit tests of the repository tools (silence and TLS check self-tests)
  2. silence check (SIL-05)
  3. TLS source check (SEC-01)
  4. host unit tests:   pio test -e native
  5. firmware build:    pio run -e crowpanel7_advance
  6. size report against the 2.9 MiB budget, then the diagnostic mode's share (FR-032)
  7. UI snapshots:      pio run -e sim -t exec  (engineering screens, FR-008)

Usage: python tools/check.py [--skip-firmware]
"""
from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
FIRMWARE_ENV = "crowpanel7_advance"
BUDGET_BYTES = int(2.9 * 1024 * 1024)


def find_pio() -> str:
    found = shutil.which("pio") or shutil.which("platformio")
    if found:
        return found
    scripts = "Scripts" if os.name == "nt" else "bin"
    exe = "pio.exe" if os.name == "nt" else "pio"
    candidate = Path.home() / ".platformio" / "penv" / scripts / exe
    if candidate.is_file():
        return str(candidate)
    sys.exit("PlatformIO Core not found. Install it: https://docs.platformio.org/en/latest/core/installation/")


def step(title: str, cmd: list[str]) -> None:
    print(f"\n=== {title}\n$ {' '.join(cmd)}", flush=True)
    start = time.monotonic()
    env = dict(os.environ, PYTHONIOENCODING="utf-8", PYTHONUTF8="1")
    result = subprocess.run(cmd, cwd=ROOT, env=env)
    elapsed = time.monotonic() - start
    if result.returncode != 0:
        print(f"\nCHECK FAILED at: {title} (exit {result.returncode}, {elapsed:.0f} s)")
        sys.exit(result.returncode)
    print(f"--- ok: {title} ({elapsed:.0f} s)")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--skip-firmware", action="store_true", help="host-only run")
    args = parser.parse_args()

    py = sys.executable
    step("tool tests", [py, "-m", "unittest", "discover", "-s", "tools", "-p", "test_*.py"])
    step("silence check", [py, "tools/check_silence.py"])
    step("tls check", [py, "tools/check_tls.py"])
    pio = find_pio()
    step("host tests", [pio, "test", "-e", "native"])
    if args.skip_firmware:
        print("\nFirmware build skipped (--skip-firmware).")
        return
    step("firmware build", [pio, "run", "-e", FIRMWARE_ENV])

    image = ROOT / ".pio" / "build" / FIRMWARE_ENV / "firmware.bin"
    size = image.stat().st_size
    print(
        f"\n=== size report\nfirmware.bin: {size} bytes ({size / 1048576:.3f} MiB), "
        f"{100.0 * size / BUDGET_BYTES:.1f}% of the 2.9 MiB budget ({BUDGET_BYTES} bytes), "
        f"{BUDGET_BYTES - size} bytes left."
    )
    if size > BUDGET_BYTES:
        sys.exit("CHECK FAILED: firmware over budget.")
    step("size by module", [py, "tools/size_by_module.py", str(ROOT / ".pio" / "build" / FIRMWARE_ENV / "firmware.map")])
    step("ui snapshots", [pio, "run", "-e", "sim", "-t", "exec"])
    step("snapshot pixels", [py, "tools/check_snapshots.py"])
    print("\nCHECK PASSED")


if __name__ == "__main__":
    main()
