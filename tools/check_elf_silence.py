#!/usr/bin/env python3
"""Linked-image silence check (SIL-05, constitution Principle I).

The source check (check_silence.py) cannot see what the framework links in. This check reads the
symbol table of the linked firmware and fails if any audio-capable driver made it into the image:
I2S, DAC, LEDC (PWM, which can drive a tone) or a codec.

Usage:  python tools/check_elf_silence.py path/to/firmware.elf [--nm path/to/nm]
Exit:   0 clean, 1 audio symbols found, 2 could not read the image.
"""
from __future__ import annotations

import argparse
import re
import shutil
import subprocess
import sys
from pathlib import Path

# Driver entry points, matched at the start of a symbol name.
BANNED = re.compile(r"^(i2s_|dac_|ledc_|esp_codec|audio_|es8311|es7210)", re.IGNORECASE)


def audio_symbols(names: list[str]) -> list[str]:
    return sorted({n for n in names if BANNED.match(n)})


def read_symbols(elf: Path, nm: str) -> list[str]:
    out = subprocess.run([nm, "--defined-only", str(elf)], capture_output=True, text=True, check=True).stdout
    names = []
    for line in out.splitlines():
        parts = line.split()
        if len(parts) >= 3:
            names.append(parts[2])
    return names


def default_nm() -> str | None:
    candidates = [
        Path.home() / ".platformio/packages/toolchain-xtensa-esp-elf/bin/xtensa-esp32s3-elf-nm.exe",
        Path.home() / ".platformio/packages/toolchain-xtensa-esp-elf/bin/xtensa-esp32s3-elf-nm",
    ]
    for c in candidates:
        if c.is_file():
            return str(c)
    return shutil.which("xtensa-esp32s3-elf-nm")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("elf", type=Path)
    parser.add_argument("--nm", default=None)
    args = parser.parse_args(argv)
    nm = args.nm or default_nm()
    if nm is None or not args.elf.is_file():
        print("ELF SILENCE CHECK: cannot read the image (nm or ELF missing).")
        return 2
    found = audio_symbols(read_symbols(args.elf, nm))
    if found:
        print("ELF SILENCE CHECK FAILED (SIL-05): audio-capable code is linked into the firmware:")
        for name in found[:20]:
            print(f"  {name}")
        return 1
    print("ELF silence check passed (SIL-05): no I2S, DAC, LEDC or codec symbols.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
