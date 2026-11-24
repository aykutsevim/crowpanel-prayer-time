#!/usr/bin/env python3
"""Side-by-side comparison of rendered fixtures with the reference renders (spec 003 FR-020,
SC-001, research R9).

For every reference in docs/design/screens/01..17 that has a snapshot in out/snapshots/, writes
out/compare/<name>.png: reference | snapshot | difference (differing pixels in white, scaled by
the size of the difference), and prints one summary line per fixture: the share of pixels that
differ by more than a small threshold, and the bounding box of the differences.

This is a review aid: glyph shapes are expected to differ slightly from the browser render
(02-ui-spec.md §12). Geometry is asserted separately by check_snapshots.py.

Usage: python tools/compare_reference.py [--only 01] [--threshold 24]
"""
from __future__ import annotations

import argparse
import struct
import sys
import zlib
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from check_snapshots import read_png  # noqa: E402  (same minimal PNG reader)

ROOT = Path(__file__).resolve().parent.parent


def write_png(path: Path, width: int, height: int, rows: list[list[tuple[int, int, int]]]) -> None:
    raw = bytearray()
    for row in rows:
        raw.append(0)
        for r, g, b in row:
            raw += bytes((r, g, b))

    def chunk(kind: bytes, body: bytes) -> bytes:
        return struct.pack(">I", len(body)) + kind + body + struct.pack(">I", zlib.crc32(kind + body) & 0xFFFFFFFF)

    data = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0))
    data += chunk(b"IDAT", zlib.compress(bytes(raw), 6)) + chunk(b"IEND", b"")
    path.write_bytes(data)


def diff_rows(ref, snap, threshold: int):
    out = []
    count = 0
    box = [10**9, 10**9, -1, -1]
    for y, (rr, sr) in enumerate(zip(ref, snap)):
        line = []
        for x, (a, b) in enumerate(zip(rr, sr)):
            d = max(abs(a[0] - b[0]), abs(a[1] - b[1]), abs(a[2] - b[2]))
            if d > threshold:
                count += 1
                box = [min(box[0], x), min(box[1], y), max(box[2], x), max(box[3], y)]
                v = min(255, 64 + d)
                line.append((v, v, v))
            else:
                line.append((0, 0, 0))
        out.append(line)
    return out, count, box


def compare(name: str, threshold: int) -> str:
    ref_path = ROOT / "docs" / "design" / "screens" / f"{name}.png"
    snap_path = ROOT / "out" / "snapshots" / f"{name}.png"
    if not snap_path.is_file():
        return f"{name}: no snapshot"
    rw, rh, ref = read_png(ref_path.read_bytes())
    sw, sh, snap = read_png(snap_path.read_bytes())
    note = ""
    if (rw, rh) == (sw, sh + 1):
        # The reference renders are 800 x 481 (one extra bottom row from the browser export);
        # 02-ui-spec.md §1 defines the canvas as 800 x 480. Compare the top 480 rows.
        ref, rh, note = ref[:sh], sh, " (reference is 481 rows; top 480 compared)"
    if (rw, rh) != (sw, sh):
        return f"{name}: size {sw}x{sh} vs reference {rw}x{rh}"
    diff, count, box = diff_rows(ref, snap, threshold)
    gap = [(255, 0, 255)] * 4
    rows = [r + gap + s + gap + d for r, s, d in zip(ref, snap, diff)]
    out_dir = ROOT / "out" / "compare"
    out_dir.mkdir(parents=True, exist_ok=True)
    write_png(out_dir / f"{name}.png", 3 * rw + 8, rh, rows)
    share = 100.0 * count / (rw * rh)
    where = f"box x {box[0]}..{box[2]} y {box[1]}..{box[3]}" if count else "identical"
    return f"{name}: {share:5.2f} % pixels differ (> {threshold}), {where}{note}"


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--only", help="prefix of one fixture, e.g. 01")
    parser.add_argument("--threshold", type=int, default=24, help="per-channel difference that counts")
    args = parser.parse_args(argv)
    refs = sorted((ROOT / "docs" / "design" / "screens").glob("[01][0-9]-*.png"))
    for ref in refs:
        num = ref.name[:2]
        if not ("01" <= num <= "17"):
            continue
        if args.only and not ref.name.startswith(args.only):
            continue
        print(compare(ref.stem, args.threshold))
    return 0


if __name__ == "__main__":
    sys.exit(main())
