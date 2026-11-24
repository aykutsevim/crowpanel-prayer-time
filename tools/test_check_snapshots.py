"""Tests for tools/check_snapshots.py. Run: python -m unittest discover -s tools -p "test_*.py"."""
from __future__ import annotations

import struct
import sys
import unittest
import zlib
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import check_snapshots as cs  # noqa: E402


def _png(rows: list[list[tuple[int, int, int]]], rgba: bool = True, filter_type: int = 0) -> bytes:
    height, width = len(rows), len(rows[0])
    raw = bytearray()
    for row in rows:
        raw.append(filter_type)  # only 0 (none) and 2 (up, with a zero previous row) are produced
        for px in row:
            raw += bytes(px) + (b"\xff" if rgba else b"")
    def chunk(kind: bytes, body: bytes) -> bytes:
        return struct.pack(">I", len(body)) + kind + body + struct.pack(">I", zlib.crc32(kind + body))
    ihdr = struct.pack(">IIBBBBB", width, height, 8, 6 if rgba else 2, 0, 0, 0)
    return b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", ihdr) + chunk(b"IDAT", zlib.compress(bytes(raw))) + chunk(b"IEND", b"")


def _good_pattern() -> list[list[tuple[int, int, int]]]:
    rows = [[cs.BLACK] * cs.WIDTH for _ in range(cs.HEIGHT)]
    for i, (_, colour) in enumerate(cs.BARS):
        for x in range(1 + i * 100, min(1 + (i + 1) * 100, cs.WIDTH - 1)):
            rows[cs.BAR_SAMPLE_Y][x] = colour
    for y in cs.FIELD_ROWS:
        for x in range(2, cs.WIDTH - 1, 2):
            rows[y][x] = cs.WHITE
    for x in range(cs.WIDTH):
        rows[0][x] = rows[cs.HEIGHT - 1][x] = cs.WHITE
    for y in range(cs.HEIGHT):
        rows[y][0] = rows[y][cs.WIDTH - 1] = cs.WHITE
    return rows


class ReadPngTest(unittest.TestCase):
    def test_reads_rgba_and_rgb(self) -> None:
        rows = [[(1, 2, 3), (4, 5, 6)], [(7, 8, 9), (10, 11, 12)]]
        for rgba in (True, False):
            w, h, got = cs.read_png(_png(rows, rgba=rgba))
            self.assertEqual((w, h), (2, 2))
            self.assertEqual(got, rows)

    def test_reads_palette(self) -> None:
        def chunk(kind: bytes, body: bytes) -> bytes:
            return struct.pack(">I", len(body)) + kind + body + struct.pack(">I", zlib.crc32(kind + body))
        ihdr = struct.pack(">IIBBBBB", 2, 1, 8, 3, 0, 0, 0)
        plte = bytes((0, 0, 0, 255, 255, 255))
        idat = zlib.compress(bytes((0, 1, 0)))  # filter 0, indices 1 and 0
        data = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", ihdr) + chunk(b"PLTE", plte) + chunk(b"IDAT", idat) + chunk(b"IEND", b"")
        _, _, got = cs.read_png(data)
        self.assertEqual(got, [[(255, 255, 255), (0, 0, 0)]])

    def test_up_filter(self) -> None:
        rows = [[(10, 20, 30)], [(0, 0, 0)]]  # with filter 2 the second row adds the first
        _, _, got = cs.read_png(_png(rows, filter_type=2))
        self.assertEqual(got[1], [(10, 20, 30)])

    def test_rejects_non_png(self) -> None:
        with self.assertRaises(ValueError):
            cs.read_png(b"not a png")


class PatternCheckTest(unittest.TestCase):
    def test_good_pattern_passes(self) -> None:
        self.assertEqual(cs.check(_good_pattern()), [])

    def test_broken_border_fails(self) -> None:
        rows = _good_pattern()
        rows[240][799] = cs.BLACK
        self.assertTrue(any("border" in e for e in cs.check(rows)))

    def test_shifted_line_field_fails(self) -> None:
        rows = _good_pattern()
        rows[cs.FIELD_ROWS[0]] = [cs.WHITE] + [cs.BLACK if x % 2 == 0 else cs.WHITE for x in range(1, cs.WIDTH - 1)] + [cs.WHITE]
        self.assertTrue(any("line field" in e for e in cs.check(rows)))

    def test_wrong_bar_colour_fails(self) -> None:
        rows = _good_pattern()
        rows[cs.BAR_SAMPLE_Y][1 + 2 * 100 + 50] = (0, 250, 255)
        self.assertTrue(any("CYAN" in e for e in cs.check(rows)))


class SetupBoxesTest(unittest.TestCase):
    """The expected boxes of the setup fixtures are the references' own (spec 004 FR-009)."""

    def test_references_meet_their_boxes(self) -> None:
        ref = Path(__file__).resolve().parent.parent / "docs" / "design" / "screens"
        for name in ("15-setup-ap", "17-status-overlay"):
            _, _, rows = cs.read_png((ref / f"{name}.png").read_bytes())
            for what, colour, region, expected in cs.SETUP_BOXES[name]:
                got = cs.colour_box(rows, colour, region)
                self.assertIsNotNone(got, f"{name} {what}")
                self.assertTrue(all(abs(a - b) <= 1 for a, b in zip(got, expected)), f"{name} {what}: {got}")


if __name__ == "__main__":
    unittest.main()
