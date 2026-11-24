#!/usr/bin/env python3
"""TLS source check (SEC-01, constitution Principle VIII).

Certificate verification stays on in every build. In Arduino-ESP32 3.3.12 the one-argument
`HTTPClient::begin(String url)` calls `setInsecure()` for https URLs when no CA is set
(research R10, specs/001-silent-board-bringup). This check therefore fails on:

  - any call to `setInsecure(`
  - a one-argument `begin("https...")` or `begin(String("https..."))`

The safe form is `http.begin(client, url)` with `client.useBuiltinCACertBundle()`.

Since the move to plain ESP-IDF (research R8, L4) it also fails on:

  - `skip_cert_common_name_check = true`
  - a file that fills an `esp_http_client_config_t` without `crt_bundle_attach` or `cert_pem`
  - `CONFIG_ESP_TLS_INSECURE=y` or `CONFIG_ESP_TLS_SKIP_SERVER_CERT_VERIFY=y` in sdkconfig files

Scanned:  src/, lib/, include/, sim/ (C, C++ and header files), and sdkconfig* in the root.
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
    (re.compile(r"\bskip_cert_common_name_check\s*=\s*true"), "skip_cert_common_name_check disables host name checks"),
)

SDKCONFIG_RULE = re.compile(r"^\s*CONFIG_ESP_TLS_(INSECURE|SKIP_SERVER_CERT_VERIFY)\s*=\s*y", re.MULTILINE)
CLIENT_CONFIG = re.compile(r"\besp_http_client_config_t\b")
CA_SET = re.compile(r"\b(crt_bundle_attach|cert_pem)\b")


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
            if CLIENT_CONFIG.search(text) and not CA_SET.search(text):
                violations.append(f"{rel}: esp_http_client_config_t without crt_bundle_attach or cert_pem")
    for path in sorted(root.glob("sdkconfig*")):
        if path.is_file() and SDKCONFIG_RULE.search(path.read_text(encoding="utf-8", errors="replace")):
            violations.append(f"{path.name}: ESP-TLS certificate verification turned off")
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
