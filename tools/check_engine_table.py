#!/usr/bin/env python3
"""Engine table check (spec 002 FR-022, SC-005).

Runs the host program built by `pio run -e engine_table`, prints its Markdown table, and fails
unless the table equals the one in specs/002-prayer-engine/contracts/engine-table.md (the lines
starting with '|'), ignoring Windows line ends. The program is run directly rather than through
`pio run -t exec`, whose console echo cannot print Turkish letters on a cp1252 console.

Usage:  python tools/check_engine_table.py [--root DIR]
Exit:   0 equal, 1 different, 2 program or contract missing.
"""
from __future__ import annotations

import argparse
import os
import subprocess
import sys
from pathlib import Path

CONTRACT = Path("specs") / "002-prayer-engine" / "contracts" / "engine-table.md"


def table_lines(text: str) -> list[str]:
    return [line.rstrip("\r") for line in text.split("\n") if line.startswith("|")]


def first_difference(expected: list[str], actual: list[str]) -> str | None:
    for i in range(max(len(expected), len(actual))):
        e = expected[i] if i < len(expected) else "<missing>"
        a = actual[i] if i < len(actual) else "<missing>"
        if e != a:
            return f"line {i + 1}:\n  expected: {e}\n  actual:   {a}"
    return None


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parent.parent)
    args = parser.parse_args(argv)
    root = args.root.resolve()
    exe = root / ".pio" / "build" / "engine_table" / ("program.exe" if os.name == "nt" else "program")
    contract = root / CONTRACT
    if not exe.is_file() or not contract.is_file():
        print(f"ENGINE TABLE CHECK: missing {exe if not exe.is_file() else contract}")
        return 2
    run = subprocess.run([str(exe)], capture_output=True)
    actual_text = run.stdout.decode("utf-8")
    sys.stdout.buffer.write(actual_text.replace("\r\n", "\n").encode("utf-8"))
    sys.stdout.flush()
    if run.returncode != 0:
        print(f"ENGINE TABLE CHECK FAILED: the program exited with {run.returncode}")
        return 1
    diff = first_difference(table_lines(contract.read_text(encoding="utf-8")), table_lines(actual_text))
    if diff is not None:
        print("ENGINE TABLE CHECK FAILED (FR-022): output differs from contracts/engine-table.md, " + diff)
        return 1
    print("Engine table check passed (FR-022): identical to contracts/engine-table.md.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
