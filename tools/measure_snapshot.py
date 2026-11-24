#!/usr/bin/env python3
"""Measure the main screen's geometry in a rendered snapshot (spec 003 FR-021, research R9).

Reports, from pixel colours alone:
  strip           bounding box of the day strip (non-background pixels in rows 264..293)
  current_column  bounding box of the `now`-filled column box in rows 336..463 (or null)
  marker          centre x and bounding box of the `now` marker line inside the strip (or null)

Colours are compared with a tolerance, so both the RGB565 snapshots and the 24-bit reference
renders can be measured. Standard library only.

Usage: python tools/measure_snapshot.py out/snapshots/01-main-ogle-1432.png
"""
from __future__ import annotations

import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from check_snapshots import read_png  # noqa: E402

BG = (0x0F, 0x1B, 0x3D)
NOW = (0xFF, 0xCF, 0x5C)
STRIP_Y0, STRIP_Y1 = 264, 293      # 02-ui-spec.md §4.2 (y 264..294, last row 293)
COLUMN_Y0, COLUMN_Y1 = 336, 463    # §4.5
MARKER_PROBE_Y = (270, 288)        # rows inside the strip where only the marker can be `now`


def near(p, c, tol=12):
    return abs(p[0] - c[0]) <= tol and abs(p[1] - c[1]) <= tol and abs(p[2] - c[2]) <= tol


def strip_box(rows):
    """x extent along the strip's middle row, y extent down column 30 (the night segment)."""
    mid = (STRIP_Y0 + STRIP_Y1) // 2
    xs = [x for x in range(len(rows[0])) if not near(rows[mid][x], BG, 20)]
    ys = [y for y in range(STRIP_Y0 - 8, STRIP_Y1 + 9) if not near(rows[y][30], BG, 20)]
    if not xs or not ys:
        return None
    return [min(xs), min(ys), max(xs), max(ys)]


def current_column(rows):
    """The `now`-filled box: x extent on row 340 (above the column text), y extent at its centre."""
    probe = 340
    xs = [x for x in range(len(rows[0])) if near(rows[probe][x], NOW, 6)]
    if len(xs) < 60:  # a box is about 125 px wide on that row
        return None
    cx = (min(xs) + max(xs)) // 2
    ys = [y for y in range(COLUMN_Y0 - 4, COLUMN_Y1 + 5) if near(rows[y][cx], NOW, 6) or
          (COLUMN_Y0 <= y <= COLUMN_Y1 and not near(rows[y][cx], BG, 20))]
    # The centre column crosses the text; take the outermost `now` rows instead.
    now_rows = [y for y in range(COLUMN_Y0 - 4, COLUMN_Y1 + 5) if any(near(rows[y][x], NOW, 6) for x in (min(xs) + 2, max(xs) - 2))]
    if not now_rows:
        now_rows = ys
    return [min(xs), min(now_rows), max(xs), max(now_rows)]


def marker(rows):
    xs = []
    for y in range(*MARKER_PROBE_Y):
        for x in range(len(rows[0])):
            if near(rows[y][x], NOW, 12):
                xs.append(x)
    if not xs:
        return None
    return {"x": (min(xs) + max(xs)) / 2.0, "box": [min(xs), MARKER_PROBE_Y[0], max(xs), MARKER_PROBE_Y[1] - 1]}


def measure(rows) -> dict:
    return {"strip": strip_box(rows), "current_column": current_column(rows), "marker": marker(rows)}


def main(argv: list[str] | None = None) -> int:
    args = argv if argv is not None else sys.argv[1:]
    if len(args) != 1:
        print(__doc__)
        return 2
    _, _, rows = read_png(Path(args[0]).read_bytes())
    print(json.dumps(measure(rows)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
