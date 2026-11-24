"""Tests for tools/scan_log_secrets.py (spec 004 SC-008). Run: python -m unittest discover tools"""
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import scan_log_secrets as s  # noqa: E402

LOG = """[   12345] mode: setup (no location)
[  123456] wifi: device network NamazVakti-83EC open at 192.168.4.1
clock: rtc 2026-11-24 13:37:52 local, valid
heap internal: 51323 free, 31744 largest
heap psram: 5986632 free
location: İSTANBUL / SULTANBEYLİ (9549)
[ 1234567] attempt: ok 192.168.1.37; stored wifi + location 9549
"""


class ScanTest(unittest.TestCase):
    def test_counts_typed_secrets(self) -> None:
        self.assertEqual(s.count(LOG + "x48271935y", "48271935"), 1)
        self.assertEqual(s.count(LOG, ""), 0)

    def test_benign_digit_runs_are_not_listed(self) -> None:
        self.assertEqual(s.suspicious_digit_runs(LOG), [])

    def test_a_pin_like_run_is_listed(self) -> None:
        self.assertEqual(s.suspicious_digit_runs(LOG + "debug value 482913\n"), ["482913"])


if __name__ == "__main__":
    unittest.main()
