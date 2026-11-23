"""Checks on the repository itself. Run: python -m unittest discover -s tools -p "test_*.py".

The PlatformIO VS Code integration overwrites `.gitignore` with its own five-line template
whenever it regenerates the project (platformio/project/integration/generator.py, generate()).
That silently drops the rules that keep secrets and factory flash images out of git (SEC-04).
This test fails when that happens; restore the file with `git checkout -- .gitignore`.
"""
from __future__ import annotations

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import check_silence  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent

REQUIRED_IGNORES = (
    ".pio/",
    "out/",
    "__pycache__/",
    ".vscode/",
    ".claude/settings.local.json",
    "*.secret",
    "secrets.ini",
    ".env",
    "backup/",
)


class GitignoreTest(unittest.TestCase):
    def test_root_gitignore_keeps_required_rules(self) -> None:
        path = ROOT / ".gitignore"
        self.assertTrue(path.is_file(), ".gitignore is missing")
        lines = {line.strip() for line in path.read_text(encoding="utf-8").splitlines()}
        missing = [rule for rule in REQUIRED_IGNORES if rule not in lines]
        self.assertEqual(
            missing, [],
            ".gitignore lost required rules (overwritten by the PlatformIO IDE?). "
            "Restore it with: git checkout -- .gitignore",
        )

    def test_backup_folder_ignores_itself(self) -> None:
        # Second line of defence for factory flash images, independent of the root file.
        path = ROOT / "backup" / ".gitignore"
        self.assertTrue(path.is_file(), "backup/.gitignore is missing")
        lines = [line.strip() for line in path.read_text(encoding="utf-8").splitlines()]
        self.assertIn("*", lines)


class DeviceBuildTest(unittest.TestCase):
    """Spec 001, tasks T027."""

    ROOT = ROOT

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
