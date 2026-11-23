#!/usr/bin/env python3
"""TLS source check (SEC-01, constitution Principle VIII).

Certificate verification stays on in every build. In Arduino-ESP32 3.3.12 the one-argument
`HTTPClient::begin(String url)` calls `setInsecure()` for https URLs when no CA is set
(research R10, specs/001-silent-board-bringup). This check therefore fails on:

  - any call to `setInsecure(`
  - a one-argument `begin("https...")` or `begin(String("https..."))`

The safe form is `http.begin(client, url)` with `client.useBuiltinCACertBundle()`.

Scanned:  src/, lib/, include/, sim/ (C, C++ and header files).
Usage:    python tools/check_tls.py [--root DIR]
Exit:     0 clean, 1 violation(s) found.
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

SOURCE_DIRS = ("src", "lib", "include", "sim")
SOURCE_SUFFIXES = {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx", ".ino", ".inc", ".ipp"}

RULES = (
    (re.compile(r"\bsetInsecure\s*\("), "setInsecure() disables certificate checks"),
    (
        re.compile(r"\bbegin\s*\(\s*(?:String\s*\(\s*)?\"https", re.IGNORECASE),
        "one-argument begin(\"https...\") skips certificate checks; use begin(client, url)",
    ),
)


def run(root: Path) -> list[str]:
    violations: list[str] = []
    for dirname in SOURCE_DIRS:
        base = root / dirname
        if not base.is_dir():
            continue
        for path in sorted(base.rglob("*")):
            if not path.is_file() or path.suffix.lower() not in SOURCE_SUFFIXES:
                continue
            rel = path.relative_to(root).as_posix()
            text = path.read_text(encoding="utf-8", errors="replace")
            for lineno, line in enumerate(text.splitlines(), start=1):
                for pattern, reason in RULES:
                    if pattern.search(line):
                        violations.append(f"{rel}:{lineno}: {reason}")
    return violations


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parent.parent)
    args = parser.parse_args(argv)
    violations = run(args.root.resolve())
    if violations:
        print("TLS CHECK FAILED (SEC-01): certificate verification must stay on.")
        for v in violations:
            print(f"  {v}")
        return 1
    print("TLS check passed (SEC-01).")
    return 0


if __name__ == "__main__":
    sys.exit(main())
