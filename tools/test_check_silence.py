"""Tests for tools/check_silence.py (SIL-05). Run: python -m unittest discover -s tools -p "test_*.py"."""
from __future__ import annotations

import contextlib
import io
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import check_silence  # noqa: E402


def _quiet_main(argv):
    with contextlib.redirect_stdout(io.StringIO()):
        return check_silence.main(argv)


class SilenceCheckTest(unittest.TestCase):
    def setUp(self) -> None:
        self._tmp = tempfile.TemporaryDirectory()
        self.root = Path(self._tmp.name)
        (self.root / "src").mkdir()
        (self.root / "src" / "main.cpp").write_text("int main() { return 0; }\n", encoding="utf-8")
        (self.root / "platformio.ini").write_text("[env:x]\nlib_deps =\n    bblanchon/ArduinoJson@7.0.0\n", encoding="utf-8")

    def tearDown(self) -> None:
        self._tmp.cleanup()

    def _write(self, rel: str, text: str) -> None:
        path = self.root / rel
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8")

    def test_clean_tree_passes(self) -> None:
        self.assertEqual(check_silence.run(self.root), [])
        self.assertEqual(_quiet_main(["--root", str(self.root)]), 0)

    def test_each_banned_token_fails(self) -> None:
        for token in check_silence.BANNED_TOKENS + ("Buzzer", "BUZZER_PIN"):
            with self.subTest(token=token):
                self._write("lib/core/src/bad.cpp", f"// {token}\n")
                violations = check_silence.run(self.root)
                self.assertTrue(violations, f"token {token!r} was not detected")
                self.assertEqual(_quiet_main(["--root", str(self.root)]), 1)
        (self.root / "lib/core/src/bad.cpp").unlink()

    def test_all_scanned_dirs(self) -> None:
        for d in check_silence.SOURCE_DIRS:
            with self.subTest(dir=d):
                self._write(f"{d}/x/y.h", "tone(5, 440);\n")
                self.assertTrue(check_silence.run(self.root))
                (self.root / d / "x" / "y.h").unlink()

    def test_allow_listed_platform_file_passes(self) -> None:
        self._write("src/platform/audio_off.cpp", "// park I2S pins, amplifier shutdown\n")
        self.assertEqual(check_silence.run(self.root), [])

    def test_audio_dependency_fails(self) -> None:
        self._write("platformio.ini", "[env:x]\nlib_deps =\n    schreibfaul1/ESP32-audioI2S@3.0.0\n")
        self.assertTrue(check_silence.run(self.root))

    def test_audio_dependency_in_library_json_fails(self) -> None:
        self._write("lib/foo/library.json", '{"dependencies": {"earlephilhower/ESP8266Audio": "1.9.7"}}\n')
        self.assertTrue(check_silence.run(self.root))

    def test_non_source_files_ignored(self) -> None:
        self._write("src/notes.txt", "I2S tone( buzzer\n")
        self.assertEqual(check_silence.run(self.root), [])


class RepositoryConfigTest(unittest.TestCase):
    """Checks on the real repository, not on a temporary tree (spec 001, tasks T027)."""

    ROOT = Path(__file__).resolve().parent.parent

    def test_device_env_ignores_core_audio_libraries(self) -> None:
        # research R3.6: the Arduino core ships ESP_I2S and ESP_SR; a stray include must fail.
        import configparser

        parser = configparser.ConfigParser(interpolation=None, strict=False)
        parser.read(self.ROOT / "platformio.ini", encoding="utf-8")
        device_envs = [s for s in parser.sections() if s.startswith("env:crowpanel7")]
        self.assertTrue(device_envs, "no device environment found in platformio.ini")
        for section in device_envs:
            with self.subTest(env=section):
                raw = parser.get(section, "lib_ignore", fallback="")
                names = {line.split(";", 1)[0].strip() for line in raw.splitlines()}
                self.assertIn("ESP_I2S", names)
                self.assertIn("ESP_SR", names)

    def test_allow_list_names_existing_files(self) -> None:
        for rel in check_silence.ALLOW_LIST:
            with self.subTest(path=rel):
                self.assertTrue((self.ROOT / rel).is_file(), f"allow-listed file missing: {rel}")


if __name__ == "__main__":
    unittest.main()
