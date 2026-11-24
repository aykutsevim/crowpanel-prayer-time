#!/usr/bin/env python3
"""Pixel checks on the rendered screens.

Spec 003 (FR-021, SC-002): for each main-screen fixture 01..14 in out/snapshots, the strip box
must be 24..775 x 264..293, the now marker's centre must be X(now) +-1 px (02-ui-spec.md §3), and
the current column box must match the one measured in the reference render +-1 px (§4.5).

Spec 001 (tasks T034, research R8): the diagnostic screen's test pattern, below.

Reads out/snapshots/diag_idle.png (written by `pio run -e sim -t exec`) and checks, pixel by
pixel, the parts of the test pattern the owner later judges by eye:

  - the one-pixel border is white on all four edges (x=0, x=799, y=0, y=479)
  - the line field alternates exactly: white on even x, black on odd x (rows 182 and 276)
  - each colour bar is its pure RGB565 primary or mix, in the contract order

No third-party packages: a minimal PNG reader for the 8-bit RGB, RGBA and palette,
non-interlaced files LodePNG writes (it picks a palette when the image has few colours).

Usage: python tools/check_snapshots.py [--dir out/snapshots]
Exit:  0 all checks pass, 1 a check failed, 2 the file could not be read.
"""
from __future__ import annotations

import argparse
import sys as _sys
import struct
import sys
import zlib
from pathlib import Path

_sys.path.insert(0, str(Path(__file__).resolve().parent))

WIDTH, HEIGHT = 800, 480
WHITE, BLACK = (255, 255, 255), (0, 0, 0)
BARS = (  # name, colour, sample x (bar centre)
    ("WHITE", (255, 255, 255)),
    ("YELLOW", (255, 255, 0)),
    ("CYAN", (0, 255, 255)),
    ("GREEN", (0, 255, 0)),
    ("MAGENTA", (255, 0, 255)),
    ("RED", (255, 0, 0)),
    ("BLUE", (0, 0, 255)),
    ("BLACK", (0, 0, 0)),
)
FIELD_ROWS = (182, 276)  # inside the line field, clear of the labels on it
BAR_SAMPLE_Y = 60        # below the ticks, above the bar labels


def read_png(data: bytes) -> tuple[int, int, list[list[tuple[int, int, int]]]]:
    """Returns width, height and rows of RGB tuples."""
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError("not a PNG file")
    pos, idat, palette = 8, b"", b""
    width = height = bit_depth = colour_type = interlace = 0
    while pos < len(data):
        length, kind = struct.unpack(">I4s", data[pos:pos + 8])
        body = data[pos + 8:pos + 8 + length]
        pos += 12 + length
        if kind == b"IHDR":
            width, height, bit_depth, colour_type, _, _, interlace = struct.unpack(">IIBBBBB", body)
        elif kind == b"PLTE":
            palette = body
        elif kind == b"IDAT":
            idat += body
        elif kind == b"IEND":
            break
    channels = {2: 3, 3: 1, 6: 4}.get(colour_type)
    if bit_depth != 8 or channels is None or interlace != 0:
        raise ValueError(f"unsupported PNG (depth {bit_depth}, colour type {colour_type}, interlace {interlace})")
    raw = zlib.decompress(idat)
    stride = width * channels
    rows: list[list[tuple[int, int, int]]] = []
    prev = bytearray(stride)
    i = 0
    for _ in range(height):
        ftype = raw[i]
        line = bytearray(raw[i + 1:i + 1 + stride])
        i += 1 + stride
        for x in range(stride):
            a = line[x - channels] if x >= channels else 0
            b = prev[x]
            c = prev[x - channels] if x >= channels else 0
            if ftype == 1:
                line[x] = (line[x] + a) & 0xFF
            elif ftype == 2:
                line[x] = (line[x] + b) & 0xFF
            elif ftype == 3:
                line[x] = (line[x] + ((a + b) >> 1)) & 0xFF
            elif ftype == 4:
                p = a + b - c
                pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                pred = a if pa <= pb and pa <= pc else (b if pb <= pc else c)
                line[x] = (line[x] + pred) & 0xFF
        if colour_type == 3:
            rows.append([tuple(palette[3 * line[x]:3 * line[x] + 3]) for x in range(stride)])
        else:
            rows.append([tuple(line[x:x + 3]) for x in range(0, stride, channels)])
        prev = line
    return width, height, rows


def check(rows: list[list[tuple[int, int, int]]]) -> list[str]:
    errors: list[str] = []
    for x in range(WIDTH):
        for y in (0, HEIGHT - 1):
            if rows[y][x] != WHITE:
                errors.append(f"border: pixel ({x},{y}) is {rows[y][x]}, expected white")
    for y in range(HEIGHT):
        for x in (0, WIDTH - 1):
            if rows[y][x] != WHITE:
                errors.append(f"border: pixel ({x},{y}) is {rows[y][x]}, expected white")
    for y in FIELD_ROWS:
        for x in range(1, WIDTH - 1):
            expected = WHITE if x % 2 == 0 else BLACK
            if rows[y][x] != expected:
                errors.append(f"line field: pixel ({x},{y}) is {rows[y][x]}, expected {expected}")
    for i, (name, colour) in enumerate(BARS):
        x = 1 + i * 100 + 50
        if rows[BAR_SAMPLE_Y][x] != colour:
            errors.append(f"bar {name}: pixel ({x},{BAR_SAMPLE_Y}) is {rows[BAR_SAMPLE_Y][x]}, expected {colour}")
    return errors


# Main-screen fixtures (spec 003 research R10): name -> second of day (None: no marker).
FIXTURES = {
    "01-main-ogle-1432": 52327, "02-main-gece-0330": 12600, "03-main-imsak-0650": 24600,
    "04-main-kusluk-1000": 36000, "05-main-vakit-girdi-1255": 46500, "06-main-ikindi-1620": 58800,
    "07-main-az-kaldi-1742": 63720, "08-main-aksam-1820": 66000,
    "09-main-yatsi-2130": 77400, "10-state-offline": 52327, "11-state-cache-ending": 52327,
    "12-state-calculated": 52327, "13-state-time-not-set": None, "14-state-no-data": 52327,
}


def check_main(snap_dir: Path, ref_dir: Path) -> list[str]:
    import measure_snapshot as ms

    errors: list[str] = []
    for name, second in FIXTURES.items():
        path = snap_dir / f"{name}.png"
        try:
            _, _, rows = read_png(path.read_bytes())
            _, _, ref_rows = read_png((ref_dir / f"{name}.png").read_bytes())
        except (OSError, ValueError, zlib.error) as exc:
            errors.append(f"{name}: cannot read snapshot or reference ({exc})")
            continue
        got, ref = ms.measure(rows), ms.measure(ref_rows)
        strip = got["strip"]
        if strip is None or any(abs(a - b) > 1 for a, b in zip(strip, [24, 264, 775, 293])):
            errors.append(f"{name}: strip box {strip}, expected [24, 264, 775, 293] +-1")
        if second is None:
            if got["marker"] is not None:
                errors.append(f"{name}: marker shown, expected none")
        else:
            expected_x = 24 + second / 86400 * 752
            if got["marker"] is None or abs(got["marker"]["x"] - expected_x) > 1:
                errors.append(f"{name}: marker at {got['marker'] and got['marker']['x']}, expected {expected_x:.1f} +-1")
        a, b = got["current_column"], ref["current_column"]
        if (a is None) != (b is None) or (a and any(abs(x - y) > 1 for x, y in zip(a, b))):
            errors.append(f"{name}: current column box {a}, reference {b}")
    return errors


# Setup screens (spec 004 FR-009, contracts/screen-api.md): name -> [(what, colour, search region,
# expected box x0, y0, x1, y1)], each within 1 px. Colours are 02 §2.1 tokens.
INK, LINE, PANEL, ALERT = (0xF5, 0xF1, 0xE6), (0x3A, 0x4C, 0x8A), (0x17, 0x25, 0x52), (0xFF, 0x8F, 0x66)
SETUP_BOXES = {
    "15-setup-ap": [
        ("QR square", INK, (480, 60, 799, 370), (508, 86, 775, 353)),
        ("step circle 1", LINE, (18, 86, 66, 134), (24, 92, 59, 127)),
        ("step circle 2", LINE, (18, 246, 66, 294), (24, 252, 59, 287)),
        ("step circle 3", LINE, (18, 312, 66, 360), (24, 318, 59, 353)),
    ],
    "17-status-overlay": [
        ("panel", PANEL, (60, 20, 740, 460), (101, 53, 698, 426)),
        ("QR square", INK, (520, 100, 699, 270), (544, 122, 675, 253)),
    ],
    "17b-overlay-confirm": [
        ("Baştan kur button", ALERT, (100, 300, 699, 427), (474, 350, 673, 405)),
    ],
}


def colour_box(rows, colour, region) -> tuple[int, int, int, int] | None:
    x0, y0, x1, y1 = region
    xs, ys = [], []
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            if rows[y][x] == colour:
                xs.append(x)
                ys.append(y)
    return (min(xs), min(ys), max(xs), max(ys)) if xs else None


def check_setup(snap_dir: Path) -> list[str]:
    errors: list[str] = []
    for name, boxes in SETUP_BOXES.items():
        try:
            _, _, rows = read_png((snap_dir / f"{name}.png").read_bytes())
        except (OSError, ValueError, zlib.error) as exc:
            errors.append(f"{name}: cannot read snapshot ({exc})")
            continue
        for what, colour, region, expected in boxes:
            got = colour_box(rows, colour, region)
            if got is None or any(abs(a - b) > 1 for a, b in zip(got, expected)):
                errors.append(f"{name}: {what} box {got}, expected {expected} +-1")
    return errors


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--dir", type=Path, default=Path(__file__).resolve().parent.parent / "out" / "snapshots")
    args = parser.parse_args(argv)
    path = args.dir / "diag_idle.png"
    try:
        width, height, rows = read_png(path.read_bytes())
    except (OSError, ValueError, zlib.error) as exc:
        print(f"snapshot check: cannot read {path}: {exc}")
        return 2
    if (width, height) != (WIDTH, HEIGHT):
        print(f"snapshot check: {path} is {width}x{height}, expected {WIDTH}x{HEIGHT}")
        return 1
    errors = check(rows)
    if errors:
        print(f"SNAPSHOT CHECK FAILED: {len(errors)} pixel(s) wrong in {path.name}")
        for e in errors[:20]:
            print(f"  {e}")
        return 1
    print("Snapshot check passed: border white on all four edges, line field alternates, bar colours exact.")
    main_errors = check_main(args.dir, Path(__file__).resolve().parent.parent / "docs" / "design" / "screens")
    if main_errors:
        print(f"GEOMETRY CHECK FAILED: {len(main_errors)} problem(s) in the main-screen fixtures")
        for e in main_errors:
            print(f"  {e}")
        return 1
    print(f"Geometry check passed: {len(FIXTURES)} main-screen fixtures (strip, marker, current column) within 1 px.")
    setup_errors = check_setup(args.dir)
    if setup_errors:
        print(f"GEOMETRY CHECK FAILED: {len(setup_errors)} problem(s) in the setup-screen fixtures")
        for e in setup_errors:
            print(f"  {e}")
        return 1
    print(f"Geometry check passed: setup fixtures 15, 17, 17b (QR squares, circles, panel, button) within 1 px.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
