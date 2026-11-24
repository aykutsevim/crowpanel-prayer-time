"""Tests for tools/check_tls.py (SEC-01). Run: python -m unittest discover -s tools -p "test_*.py"."""
from __future__ import annotations

import contextlib
import io
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import check_tls  # noqa: E402


def _quiet_main(argv):
    with contextlib.redirect_stdout(io.StringIO()):
        return check_tls.main(argv)


class TlsCheckTest(unittest.TestCase):
    def setUp(self) -> None:
        self._tmp = tempfile.TemporaryDirectory()
        self.root = Path(self._tmp.name)
        self._write("src/main.cpp", "int main() { return 0; }\n")

    def tearDown(self) -> None:
        self._tmp.cleanup()

    def _write(self, rel: str, text: str) -> None:
        path = self.root / rel
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8")

    def test_clean_tree_passes(self) -> None:
        self.assertEqual(check_tls.run(self.root), [])
        self.assertEqual(_quiet_main(["--root", str(self.root)]), 0)

    def test_set_insecure_fails(self) -> None:
        self._write("src/platform/net.cpp", "client.setInsecure();\n")
        self.assertTrue(check_tls.run(self.root))
        self.assertEqual(_quiet_main(["--root", str(self.root)]), 1)

    def test_one_argument_https_begin_fails(self) -> None:
        bad = (
            'http.begin("https://ezanvakti.imsakiyem.com/api");\n',
            'http.begin( "https://x" );\n',
            'http.begin(String("https://x"));\n',
            'http.begin(String( "https://x" ));\n',
        )
        for line in bad:
            with self.subTest(line=line):
                self._write("lib/providers/src/p.cpp", line)
                self.assertTrue(check_tls.run(self.root), f"not detected: {line!r}")

    def test_two_argument_begin_passes(self) -> None:
        self._write(
            "src/platform/https_probe.cpp",
            'NetworkClientSecure client;\nclient.useBuiltinCACertBundle();\n'
            'http.begin(client, "https://ezanvakti.imsakiyem.com/api");\nhttp.begin(client, url);\n',
        )
        self.assertEqual(check_tls.run(self.root), [])

    def test_all_scanned_dirs(self) -> None:
        for d in check_tls.SOURCE_DIRS:
            with self.subTest(dir=d):
                self._write(f"{d}/x/y.cpp", "c.setInsecure();\n")
                self.assertTrue(check_tls.run(self.root))
                (self.root / d / "x" / "y.cpp").unlink()

    def test_non_source_files_ignored(self) -> None:
        self._write("src/notes.md", "never call setInsecure()\n")
        self.assertEqual(check_tls.run(self.root), [])


class IdfTlsCheckTest(unittest.TestCase):
    """ESP-IDF client and configuration (research R8 L4)."""

    setUp = TlsCheckTest.setUp
    tearDown = TlsCheckTest.tearDown
    _write = TlsCheckTest._write

    def test_skip_common_name_check_fails(self) -> None:
        self._write("src/platform/https.cpp", "cfg.skip_cert_common_name_check = true;\n")
        self.assertTrue(check_tls.run(self.root))

    def test_client_without_ca_fails(self) -> None:
        self._write("src/platform/https.cpp", "esp_http_client_config_t cfg = {};\ncfg.url = kUrl;\n")
        self.assertTrue(check_tls.run(self.root))

    def test_client_with_bundle_passes(self) -> None:
        text = "esp_http_client_config_t cfg = {};\ncfg.crt_bundle_attach = esp_crt_bundle_attach;\n"
        self._write("src/platform/https.cpp", text)
        self.assertEqual(check_tls.run(self.root), [])

    def test_insecure_sdkconfig_fails(self) -> None:
        self._write("sdkconfig.defaults", "CONFIG_ESP_TLS_INSECURE=y\n")
        self.assertTrue(check_tls.run(self.root))
        self._write("sdkconfig.defaults", "CONFIG_ESP_TLS_SKIP_SERVER_CERT_VERIFY=y\n")
        self.assertTrue(check_tls.run(self.root))

    def test_secure_sdkconfig_passes(self) -> None:
        self._write("sdkconfig.defaults", "CONFIG_ESP_TLS_INSECURE=n\nCONFIG_MBEDTLS_CERTIFICATE_BUNDLE=y\n"
                                          "CONFIG_MBEDTLS_HAVE_TIME_DATE=y\n")
        self.assertEqual(check_tls.run(self.root), [])

    def test_certificate_dates_must_be_checked(self) -> None:  # spec 005 OD5-1, R3
        self._write("sdkconfig.defaults", "CONFIG_MBEDTLS_CERTIFICATE_BUNDLE=y\n")
        self.assertTrue(any("HAVE_TIME_DATE" in v for v in check_tls.run(self.root)))
        self._write("sdkconfig.defaults", "CONFIG_MBEDTLS_HAVE_TIME_DATE=n\n")
        self.assertTrue(check_tls.run(self.root))

    def test_plain_http_date_fetch_allowed_only_in_its_file(self) -> None:  # spec 005 R2, R3
        text = 'esp_http_client_config_t cfg = {};\nconst char* kHosts[] = {"http://a.example/", "http://b.example/"};\n'
        self._write("src/platform/http_date.cpp", text)
        self.assertEqual(check_tls.run(self.root), [])
        self._write("src/platform/http_date.cpp", text + 'const char* kBad = "https://c.example/";\n')
        self.assertTrue(check_tls.run(self.root))
        self._write("src/platform/http_date.cpp", "int x;\n")
        self._write("src/platform/other.cpp", text)
        self.assertTrue(check_tls.run(self.root))


if __name__ == "__main__":
    unittest.main()
