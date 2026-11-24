#!/usr/bin/env python3
"""Regenerate the LVGL fonts of the main screen (spec 003 FR-003, research R3).

1. Downloads, if missing, the four Barlow TTFs and OFL.txt from google/fonts into src/, pinned to
   one google/fonts commit, and records that commit in SOURCES.md.
2. Converts the 14 fonts of research R3 into lib/ui/src/fonts/<name>.c with lv_font_conv 1.5.3, using the same
   options LVGL 9.6.0 uses for its own built-in fonts (see the header of lv_font_montserrat_14.c):
   --no-compress --no-prefilter --bpp 4 --format lvgl --force-fast-kern-format.
3. Prints the size of each generated file's glyph bitmap and the total, against the 300 kB budget
   of docs/specification/05-architecture.md §5.

Usage: python assets/fonts/gen_fonts.py [--commit SHA]
Needs Node.js (npx) for the conversion. The output is deterministic: running it again without a
change rewrites identical files.
"""
from __future__ import annotations

import argparse
import json
import re
import shutil
import ssl
import subprocess
import sys
import urllib.request
from datetime import date
from pathlib import Path

HERE = Path(__file__).resolve().parent
SRC = HERE / "src"
OUT = HERE.parent.parent / "lib" / "ui" / "src" / "fonts"  # compiled as part of lib/ui
TOOL = "lv_font_conv@1.5.3"
OPTIONS = ["--no-compress", "--no-prefilter", "--bpp", "4", "--format", "lvgl", "--force-fast-kern-format"]

FILES = {  # local name -> path inside google/fonts
    "Barlow-Regular.ttf": "ofl/barlow/Barlow-Regular.ttf",
    "Barlow-Medium.ttf": "ofl/barlow/Barlow-Medium.ttf",
    "Barlow-SemiBold.ttf": "ofl/barlow/Barlow-SemiBold.ttf",
    "BarlowCondensed-SemiBold.ttf": "ofl/barlowcondensed/BarlowCondensed-SemiBold.ttf",
    "OFL.txt": "ofl/barlow/OFL.txt",
}

# 02-ui-spec.md §2.2. Turkish set: space, 0-9, A-Z, a-z, ÇĞİÖŞÜçğıöşü, . , : ; - ' ( ) / % ! ? +,
# U+2019 (’) and U+2026 (…).
TURKISH = "0x20-0x7E,0xC7,0xD6,0xDC,0xE7,0xF6,0xFC,0x11E,0x11F,0x130,0x131,0x15E,0x15F,0x2019,0x2026"
NUM_DASH = "0x30-0x3A,0x2D"   # 0-9 : -
NUM_ONLY = "0x30-0x3A"        # 0-9 :
NUM_BOUNDS = "0x30-0x3A,0x2E,0x20"  # 0-9 : . space
# Network and place names on the setup screens (spec 004 research R13): the Turkish set plus
# Â â Î î Û û, which Diyanet's place names use (KÂĞITHANE). Must match nvp/text/glyph_filter.cpp.
DISPLAY = TURKISH + ",0xC2,0xE2,0xCE,0xEE,0xDB,0xFB"

FONTS = (  # name, file, size, ranges
    ("nvp_num_114", "BarlowCondensed-SemiBold.ttf", 114, NUM_DASH),
    ("nvp_num_96", "BarlowCondensed-SemiBold.ttf", 96, NUM_DASH),
    ("nvp_num_48", "BarlowCondensed-SemiBold.ttf", 48, NUM_DASH),
    ("nvp_num_46", "BarlowCondensed-SemiBold.ttf", 46, NUM_ONLY),
    ("nvp_num_44", "BarlowCondensed-SemiBold.ttf", 44, NUM_ONLY),
    ("nvp_num_25", "BarlowCondensed-SemiBold.ttf", 25, NUM_BOUNDS),
    # Setup address and overlay PIN: references 15 and 17 show 15 px digits, a 21 px font (spec 004 R13).
    ("nvp_num_21", "BarlowCondensed-SemiBold.ttf", 21, NUM_BOUNDS),
    ("nvp_cond_600_38", "BarlowCondensed-SemiBold.ttf", 38, DISPLAY),  # setup SSID, password; connecting SSID
    ("nvp_txt_600_26", "Barlow-SemiBold.ttf", 26, TURKISH),
    ("nvp_txt_600_18", "Barlow-SemiBold.ttf", 18, TURKISH),
    ("nvp_txt_500_21", "Barlow-Medium.ttf", 21, TURKISH + ",0x2248"),  # + ≈ for the calculated state
    ("nvp_txt_500_19", "Barlow-Medium.ttf", 19, TURKISH),
    ("nvp_txt_400_20", "Barlow-Regular.ttf", 20, TURKISH),
    ("nvp_txt_400_17", "Barlow-Regular.ttf", 17, TURKISH),
    ("nvp_txt_400_15", "Barlow-Regular.ttf", 15, TURKISH),
    ("nvp_txt_600_19", "Barlow-SemiBold.ttf", 19, "0x31-0x33"),       # setup step numbers 1-3
    ("nvp_txt_400_18", "Barlow-Regular.ttf", 18, DISPLAY + ",0x2014"),  # overlay values; — for "not yet"
    ("nvp_txt_400_14", "Barlow-Regular.ttf", 14, TURKISH),  # also the hour labels (02 §2.2 consolidation)
)
BUDGET = 300 * 1024


def tls_context() -> ssl.SSLContext:
    """Certificate verification stays on (constitution VIII); use certifi's CA bundle when the
    Python install has no usable system store."""
    try:
        import certifi

        return ssl.create_default_context(cafile=certifi.where())
    except ImportError:
        return ssl.create_default_context()


def fetch(url: str, timeout: int) -> bytes:
    with urllib.request.urlopen(url, timeout=timeout, context=tls_context()) as r:
        return r.read()


def latest_commit() -> str:
    return json.loads(fetch("https://api.github.com/repos/google/fonts/commits/main", 30))["sha"]


def download(commit: str) -> None:
    SRC.mkdir(exist_ok=True)
    for name, path in FILES.items():
        target = SRC / name
        if target.is_file():
            continue
        url = f"https://raw.githubusercontent.com/google/fonts/{commit}/{path}"
        print(f"download {url}")
        target.write_bytes(fetch(url, 60))
    (HERE / "SOURCES.md").write_text(
        "# Font sources\n\n"
        "Barlow and Barlow Condensed by Jeremy Tribby, SIL Open Font License 1.1 (`src/OFL.txt`).\n\n"
        f"Downloaded from google/fonts at commit `{commit}` on {date.today().isoformat()}:\n\n"
        + "".join(f"- `src/{n}` from `{p}`\n" for n, p in FILES.items())
        + "\nUpstream: github.com/jpt/barlow (google/fonts METADATA.pb names upstream commit `b4726dd`, tag v1.408).\n"
        "Converted by `gen_fonts.py` with `npx lv_font_conv@1.5.3` (spec 003 research R3).\n",
        encoding="utf-8",
    )


def convert() -> int:
    OUT.mkdir(exist_ok=True)
    npx = shutil.which("npx") or shutil.which("npx.cmd")
    if npx is None:
        sys.exit("npx not found: install Node.js to regenerate the fonts")
    total = 0
    for name, ttf, size, ranges in FONTS:
        out = OUT / f"{name}.c"
        cmd = [npx, "--yes", TOOL, *OPTIONS, "--size", str(size), "--font", str(SRC / ttf), "-r", ranges,
               "--lv-font-name", name, "-o", str(out)]
        subprocess.run(cmd, check=True)
        # lv_font_conv writes the command line (with absolute paths) into the header; make it
        # machine-independent so regenerating on another computer gives identical files.
        text = out.read_text(encoding="utf-8")
        text = text.replace(str(SRC) + "\\", "").replace(str(SRC) + "/", "").replace(str(SRC), "")
        text = re.sub(r"-o \S+\.c", f"-o {name}.c", text)
        out.write_text(text, encoding="utf-8", newline="\n")
        bitmap = bitmap_bytes(text)
        total += bitmap
        print(f"{name:16s} {size:4d}px  glyph bitmaps {bitmap:7d} B")
    print(f"total glyph bitmaps {total} B = {total / 1024:.1f} kB of the {BUDGET // 1024} kB font budget")
    return total


def bitmap_bytes(text: str) -> int:
    m = re.search(r"glyph_bitmap\[\]\s*=\s*\{(.*?)\};", text, re.S)
    return len(re.findall(r"0x[0-9a-fA-F]{1,2}", m.group(1))) if m else 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--commit", help="google/fonts commit to download from (default: current main)")
    args = parser.parse_args()
    if not all((SRC / n).is_file() for n in FILES):
        download(args.commit or latest_commit())
    total = convert()
    return 0 if total <= BUDGET else 1


if __name__ == "__main__":
    sys.exit(main())
