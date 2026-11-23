#!/usr/bin/env python3
"""Pixel checks on the rendered diagnostic screen (spec 001, tasks T034, research R8).

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
import struct
import sys
import zlib
from pathlib import Path

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
    return 0


if __name__ == "__main__":
    sys.exit(main())
