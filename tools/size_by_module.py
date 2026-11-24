#!/usr/bin/env python3
"""Size of the diagnostic mode inside the firmware image (FR-032, spec 001).

The diagnostic and settings screens ship with the product (owner decision 2026-11-20), so
their flash cost is reported separately after every build. This reads the GNU ld map file
and sums the input sections that occupy flash or RAM images, per module:

  diag ui     lib/ui/src/diag/*         (archive libui.a, members named after these files)
  diag core   lib/core/src/nvp/diag/*   (archive libcore.a, members named after these files)
  app         src/app/*                 (object paths under src/app/)

It also reports the main screen's fonts (lib/ui/src/fonts/*, spec 003 FR-003, FR-025) against
their 300 kB budget (05-architecture.md §5) and fails above it.

Counted: sections at a non-zero address, except .bss, .noinit, COMMON and debug sections.
Not counted: merged string-literal sections (`*.str1.*`), because the linker attributes all
merged strings to one object; the figures are therefore a slight underestimate.

Usage: python tools/size_by_module.py <firmware.map> [--root DIR]
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

SECTION_INLINE = re.compile(r"^ (\.\S+)\s+0x([0-9a-fA-F]+)\s+0x([0-9a-fA-F]+)\s+(\S.*)$")
SECTION_NAME_ONLY = re.compile(r"^ (\.\S+)\s*$")
SECTION_CONT = re.compile(r"^\s+0x([0-9a-fA-F]+)\s+0x([0-9a-fA-F]+)\s+(\S.*)$")
SKIP_PREFIXES = (".bss", ".noinit", ".debug", ".comment", ".xt.", ".xtensa", ".note", ".stab")


def _member_names(root: Path, rel_dir: str) -> set[str]:
    base = root / rel_dir
    if not base.is_dir():
        return set()
    # Arduino builds named members "x.cpp.o", ESP-IDF builds "x.o": accept both.
    names: set[str] = set()
    for p in base.rglob("*"):
        if p.suffix in {".c", ".cpp"}:
            names.add(p.name + ".o")
            names.add(p.stem + ".o")
    return names


def _counted(name: str, address: int) -> bool:
    if address == 0 or name.startswith(SKIP_PREFIXES) or ".str1." in name:
        return False
    return True


def input_sections(map_text: str):
    """Yield (section name, address, size, object) for each input section of the memory map."""
    marker = "Linker script and memory map"
    start = map_text.find(marker)
    lines = map_text[start:].splitlines() if start >= 0 else []
    pending = None
    for line in lines:
        if pending is not None:
            m = SECTION_CONT.match(line)
            if m:
                yield pending, int(m.group(1), 16), int(m.group(2), 16), m.group(3).strip()
            pending = None
            continue
        m = SECTION_INLINE.match(line)
        if m:
            yield m.group(1), int(m.group(2), 16), int(m.group(3), 16), m.group(4).strip()
            continue
        m = SECTION_NAME_ONLY.match(line)
        if m:
            pending = m.group(1)


def module_sizes(map_text: str, root: Path) -> dict[str, int]:
    ui = _member_names(root, "lib/ui/src/diag")
    core = _member_names(root, "lib/core/src/nvp/diag")
    sizes = {"diag ui": 0, "diag core": 0, "app": 0}
    for name, address, size, obj in input_sections(map_text):
        if not _counted(name, address):
            continue
        norm = obj.replace("\\", "/")
        # PlatformIO names the archive after the library folder (libcore.a, libui.a).
        m = re.search(r"/lib(nvp_ui|nvp_core|ui|core)\.a\(([^)]+)\)", norm)
        if m:
            if m.group(1) in ("ui", "nvp_ui") and m.group(2) in ui:
                sizes["diag ui"] += size
            elif m.group(1) in ("core", "nvp_core") and m.group(2) in core:
                sizes["diag core"] += size
        elif "/src/app/" in norm:
            sizes["app"] += size
    return sizes


FONT_BUDGET = 300 * 1024


def font_bytes(map_text: str, root: Path) -> int:
    fonts = _member_names(root, "lib/ui/src/fonts")
    total = 0
    for name, address, size, obj in input_sections(map_text):
        if not _counted(name, address):
            continue
        m = re.search(r"/lib(nvp_ui|ui)\.a\(([^)]+)\)", obj.replace("\\", "/"))
        if m and m.group(2) in fonts:
            total += size
    return total


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("map", type=Path)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parent.parent)
    args = parser.parse_args(argv)
    if not args.map.is_file():
        print(f"size by module: map file not found: {args.map}")
        return 1
    sizes = module_sizes(args.map.read_text(encoding="utf-8", errors="replace"), args.root.resolve())
    total = sum(sizes.values())
    parts = ", ".join(f"{k} {v}" for k, v in sizes.items())
    print(f"Diagnostic mode size: {total} bytes ({parts}); ships with the product (FR-032).")
    fonts = font_bytes(args.map.read_text(encoding="utf-8", errors="replace"), args.root.resolve())
    print(f"Fonts: {fonts} bytes = {fonts / 1024:.1f} kB of the {FONT_BUDGET // 1024} kB font budget (spec 003 FR-003).")
    if fonts > FONT_BUDGET:
        print("SIZE CHECK FAILED: fonts over their 300 kB budget.")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
