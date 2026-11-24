"""Search a saved serial log for secrets (spec 004 SC-008, FR-023; quickstart §2 step 10).

Usage: python tools/scan_log_secrets.py LOGFILE

Asks for the home Wi-Fi password and the device's network password with hidden prompts (getpass);
they stay in memory only and are never printed. Reports how often each occurs in the log, then
lists every run of six or more digits that is not part of a timestamp, a time, a date or an address,
for the owner to judge (the settings PIN is never shown, so it cannot be typed in). Keep the log
outside the repository. Exit code 1 when a typed secret occurs.
"""
from __future__ import annotations

import getpass
import re
import sys
from pathlib import Path

# Runs of digits that are expected: "[   12345]" uptimes, dates, times, addresses, byte counts.
_BENIGN = re.compile(
    r"\[\s*\d+\]"                      # log timestamps
    r"|\d{4}-\d{2}-\d{2}"              # dates
    r"|\d{1,3}(?:\.\d{1,3}){3}"        # IPv4 addresses
    r"|\b\d+ (?:B|kB|MB|ms|s|free|largest|bytes)\b"
    r"|\(\d+\)"                        # district ids in "location: ... (9549)"
    r"|\b(?:psram|internal):? \d+"
)
_DIGITS = re.compile(r"\d{6,}")


def count(text: str, secret: str) -> int:
    return text.count(secret) if secret else 0


def suspicious_digit_runs(text: str) -> list[str]:
    cleaned = _BENIGN.sub(" ", text)
    found = []
    for line in cleaned.splitlines():
        for m in _DIGITS.finditer(line):
            found.append(m.group(0))
    return sorted(set(found))


def main(argv: list[str]) -> int:
    if len(argv) != 2:
        print(__doc__)
        return 2
    text = Path(argv[1]).read_text(encoding="utf-8", errors="replace")
    wifi = getpass.getpass("home Wi-Fi password (Enter to skip): ")
    ap = getpass.getpass("device network password, 8 digits from the setup screen (Enter to skip): ")
    hits = {"home Wi-Fi password": count(text, wifi), "device network password": count(text, ap)}
    del wifi, ap
    for name, n in hits.items():
        print(f"{name}: {n} occurrences")
    runs = suspicious_digit_runs(text)
    print("runs of 6+ digits to judge (none expected):", ", ".join(runs) if runs else "none")
    return 1 if any(hits.values()) else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
