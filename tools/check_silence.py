#!/usr/bin/env python3
"""Silence check (SIL-05, handover/docs/05-architecture.md §9).

Fails if project sources reference audio APIs or if the dependency list names an
audio library. The device must never make a sound (constitution, Principle I).

Scanned:  src/, lib/, include/, sim/ (C, C++ and header files), platformio.ini,
          and every library.json / library.properties under lib/.
Allowed:  the single platform file that silences the audio path, and its test
          (ALLOW_LIST below). Nothing else may name these tokens.

Usage:    python tools/check_silence.py [--root DIR]
Exit:     0 clean, 1 violation(s) found.
"""
from __future__ import annotations

import argparse
import configparser
import re
import sys
from pathlib import Path

SOURCE_DIRS = ("src", "lib", "include", "sim")
SOURCE_SUFFIXES = {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx", ".ino", ".inc", ".ipp"}

# Banned tokens from 05-architecture.md §9. Matched as plain substrings, case-sensitive,
# except "buzzer", which is matched case-insensitively (Buzzer, BUZZER_PIN, ...).
BANNED_TOKENS = (
    "i2s",
    "I2S",
    "tone(",
    "noTone(",
    "ledcWriteTone",
    "ledcWriteNote",
    "dac_output",
    "dacWrite",
    "Audio.h",
)
BANNED_CASE_INSENSITIVE = ("buzzer",)

# Dependency names that indicate an audio library (matched case-insensitively).
BANNED_DEPENDENCY_PATTERN = re.compile(
    r"audio|i2s|buzzer|tone|talkie|mp3|wav|speaker|melody|rtttl|sam\b|tts", re.IGNORECASE
)

# The one platform file that parks the audio pins and shuts the amplifier down (SIL-02),
# and its test. Paths are relative to the project root, POSIX separators.
ALLOW_LIST = frozenset(
    {
        "src/platform/audio_off.cpp",
        "src/platform/audio_off.h",
        "test/test_audio_off/test_audio_off.cpp",
    }
)


def _rel(path: Path, root: Path) -> str:
    return path.relative_to(root).as_posix()


def scan_sources(root: Path) -> list[str]:
    violations: list[str] = []
    for dirname in SOURCE_DIRS:
        base = root / dirname
        if not base.is_dir():
            continue
        for path in sorted(base.rglob("*")):
            if not path.is_file() or path.suffix.lower() not in SOURCE_SUFFIXES:
                continue
            rel = _rel(path, root)
            if rel in ALLOW_LIST:
                continue
            text = path.read_text(encoding="utf-8", errors="replace")
            for lineno, line in enumerate(text.splitlines(), start=1):
                for token in BANNED_TOKENS:
                    if token in line:
                        violations.append(f"{rel}:{lineno}: banned token '{token}'")
                lowered = line.lower()
                for token in BANNED_CASE_INSENSITIVE:
                    if token in lowered:
                        violations.append(f"{rel}:{lineno}: banned token '{token}'")
    return violations


def _dependency_names_from_ini(ini_path: Path) -> list[str]:
    parser = configparser.ConfigParser(interpolation=None, strict=False)
    parser.read(ini_path, encoding="utf-8")
    names: list[str] = []
    for section in parser.sections():
        for key in ("lib_deps", "platform_packages"):
            if parser.has_option(section, key):
                for raw in parser.get(section, key).splitlines():
                    entry = raw.split(";", 1)[0].strip()
                    if entry:
                        names.append(f"[{section}] {key}: {entry}")
    return names


def scan_dependencies(root: Path) -> list[str]:
    violations: list[str] = []
    ini = root / "platformio.ini"
    if ini.is_file():
        for entry in _dependency_names_from_ini(ini):
            # Only the package name/URL part matters, not the version.
            if BANNED_DEPENDENCY_PATTERN.search(entry.split(":", 1)[1]):
                violations.append(f"platformio.ini: audio dependency {entry}")
    lib = root / "lib"
    if lib.is_dir():
        for manifest in sorted(list(lib.rglob("library.json")) + list(lib.rglob("library.properties"))):
            text = manifest.read_text(encoding="utf-8", errors="replace")
            for lineno, line in enumerate(text.splitlines(), start=1):
                if re.search(r"depend", line, re.IGNORECASE) and BANNED_DEPENDENCY_PATTERN.search(line):
                    violations.append(f"{_rel(manifest, root)}:{lineno}: audio dependency")
    return violations


def run(root: Path) -> list[str]:
    return scan_sources(root) + scan_dependencies(root)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parent.parent)
    args = parser.parse_args(argv)
    root = args.root.resolve()
    violations = run(root)
    if violations:
        print("SILENCE CHECK FAILED (SIL-05): the device must never make a sound.")
        for v in violations:
            print(f"  {v}")
        return 1
    print("Silence check passed (SIL-05).")
    return 0


if __name__ == "__main__":
    sys.exit(main())
