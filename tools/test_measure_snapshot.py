"""Tests for tools/measure_snapshot.py (spec 003 FR-021). Run: python -m unittest discover tools"""
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import measure_snapshot as m  # noqa: E402


def canvas():
    return [[m.BG for _ in range(800)] for _ in range(480)]


def rect(rows, x0, y0, x1, y1, colour):
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            rows[y][x] = colour


class MeasureTest(unittest.TestCase):
    def test_strip_box(self) -> None:
        rows = canvas()
        rect(rows, 24, 264, 775, 293, (0x2A, 0x3B, 0x78))
        self.assertEqual(m.measure(rows)["strip"], [24, 264, 775, 293])

    def test_current_column_and_marker(self) -> None:
        rows = canvas()
        rect(rows, 24, 264, 775, 293, (0x2A, 0x3B, 0x78))
        rect(rows, 276, 336, 400, 463, m.NOW)  # column 2
        rect(rows, 474, 259, 476, 299, m.NOW)  # marker line, centre 475
        got = m.measure(rows)
        self.assertEqual(got["current_column"], [276, 336, 400, 463])
        self.assertEqual(got["marker"]["x"], 475.0)
        self.assertEqual(got["strip"], [24, 264, 775, 293])  # marker colour excluded from the strip

    def test_nothing_current(self) -> None:
        rows = canvas()
        rect(rows, 60, 426, 110, 436, m.NOW)  # a yellow note is not a column box
        got = m.measure(rows)
        self.assertIsNone(got["current_column"])
        self.assertIsNone(got["marker"])


if __name__ == "__main__":
    unittest.main()
