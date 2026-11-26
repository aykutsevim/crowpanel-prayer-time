"""Tests for tools/gen_web.py (spec 004 FR-013, SC-006, research R7). Run: python -m unittest discover tools"""
import gzip
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import gen_web as w  # noqa: E402


def portal(tmp: Path, html: str = "<html><style>/*STYLE*/</style><script>/*SCRIPT*/</script></html>",
           js: str = "var a = 1; // keep\n// drop\n/* block */\nvar b = 'x';\n") -> Path:
    (tmp / "index.html").write_text(html, encoding="utf-8")
    (tmp / "style.css").write_text("/* c */\nbody {\n  color: red;\n}\n", encoding="utf-8")
    for n in w.SCRIPTS:
        (tmp / n).write_text(js, encoding="utf-8")
    return tmp


class PageTest(unittest.TestCase):
    def test_inlines_and_minifies_conservatively(self) -> None:
        with tempfile.TemporaryDirectory() as t:
            page = w.build_page(portal(Path(t))).decode("utf-8")
        self.assertIn("body {\ncolor: red;\n}", page)
        self.assertIn("var a = 1; // keep", page)  # a // inside a line stays
        self.assertNotIn("// drop", page)
        self.assertNotIn("block", page)
        self.assertNotIn("/*SCRIPT*/", page)

    def test_external_requests_are_refused(self) -> None:
        with tempfile.TemporaryDirectory() as t:
            with self.assertRaises(w.WebError):
                w.build_page(portal(Path(t), js="fetch('https://example.com');\n"))

    def test_placeholders_required(self) -> None:
        with tempfile.TemporaryDirectory() as t:
            with self.assertRaises(w.WebError):
                w.build_page(portal(Path(t), html="<html></html>"))

    def test_gzip_is_deterministic(self) -> None:
        self.assertEqual(w.gz(b"abc" * 100), w.gz(b"abc" * 100))
        self.assertEqual(gzip.decompress(w.gz(b"hello")), b"hello")

    def test_real_page_within_budget(self) -> None:
        page = w.build_page()
        self.assertLessEqual(len(w.gz(page)), w.PAGE_BUDGET)
        text = page.decode("utf-8")
        for s in ("Namaz Vakti kurulumu", "Wi-Fi ağı", "Yenile", "Ağım listede yok", "Wi-Fi şifresi", "Göster",
                  "Konum", "İl", "İlçe", "Kaydet ve bağlan",
                  "Cihaz Wi-Fi ağına bağlanıyor. Sonucu cihazın ekranından takip et.",
                  "Bir ağ seç", "Şifre en az 8 karakter olmalı", "İl ve ilçe seç", "desteklenmiyor"):
            self.assertIn(s, text, s)  # 02 §10 strings, byte for byte


class ArrayTest(unittest.TestCase):
    def test_c_array(self) -> None:
        text = w.c_array("x", b"\x01\xff", char=True)
        self.assertIn("extern const char x[] = {\n    0x01,0xff,0x00\n};", text)
        self.assertIn("x_len = 2;", text)


if __name__ == "__main__":
    unittest.main()


class CoordsTest(unittest.TestCase):
    """spec 007 T032: the generated coordinate table."""

    def test_hash_matches_the_device(self) -> None:
        import gen_web as w
        self.assertEqual(0x8CD8EE03, w.fnv1a32("ANKARA"))  # test/test_net/test_coords.cpp
        self.assertEqual(0x5282D4B9, w.fnv1a32("KADIKÖY"))

    def test_rows_sorted_and_unique(self) -> None:
        import gen_web as w
        rows = w.build_coords()
        self.assertEqual(1003, len(rows))
        self.assertEqual(sorted(rows), rows)
        self.assertIn((9206, 0x8CD8EE03, 3992063, 3285403), rows)
