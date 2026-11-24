"""Tests for tools/check_engine_table.py (spec 002 FR-022). Run: python -m unittest discover tools"""
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import check_engine_table  # noqa: E402


class TableLinesTest(unittest.TestCase):
    def test_only_table_lines_and_no_line_end_differences(self) -> None:
        text = "# Title\r\n\r\n| a | b |\r\n|---|---|\r\n| 1 | 2 |\r\nafter\n"
        self.assertEqual(check_engine_table.table_lines(text), ["| a | b |", "|---|---|", "| 1 | 2 |"])

    def test_first_difference(self) -> None:
        self.assertIsNone(check_engine_table.first_difference(["| x |"], ["| x |"]))
        self.assertIn("line 2", check_engine_table.first_difference(["| x |", "| y |"], ["| x |", "| z |"]))
        self.assertIn("<missing>", check_engine_table.first_difference(["| x |"], []))

    def test_contract_exists_with_header_and_fifteen_rows(self) -> None:
        root = Path(__file__).resolve().parent.parent
        lines = check_engine_table.table_lines((root / check_engine_table.CONTRACT).read_text(encoding="utf-8"))
        self.assertEqual(len(lines), 17)


if __name__ == "__main__":
    unittest.main()
