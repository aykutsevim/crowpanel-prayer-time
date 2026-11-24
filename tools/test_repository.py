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
    "managed_components/",
    "sdkconfig.*",
    "!sdkconfig.defaults",
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

    def _device_envs(self):
        import configparser

        parser = configparser.ConfigParser(interpolation=None, strict=False)
        parser.read(self.ROOT / "platformio.ini", encoding="utf-8")
        envs = [s for s in parser.sections() if s.startswith("env:crowpanel7")]
        self.assertTrue(envs, "no device environment found in platformio.ini")
        return parser, envs

    def test_device_env_is_pinned_espidf_without_arduino(self) -> None:
        # research R8 L4 and R9: plain ESP-IDF, framework package pinned exactly (Principle V).
        # The Arduino core shipped audio libraries (ESP_I2S, ESP_SR); it is no longer used.
        parser, envs = self._device_envs()
        for section in envs:
            with self.subTest(env=section):
                self.assertEqual(parser.get(section, "framework").strip(), "espidf")
                packages = parser.get(section, "platform_packages", fallback="")
                self.assertRegex(packages, r"framework-espidf@\d+\.\d+\.\d+")
                self.assertNotIn("arduino", packages.lower())

    def test_device_env_runs_the_image_silence_check(self) -> None:
        # SIL-05: tools/pio_size_budget.py runs tools/check_elf_silence.py on every device build.
        parser, envs = self._device_envs()
        hook = (self.ROOT / "tools" / "pio_size_budget.py").read_text(encoding="utf-8")
        self.assertIn("check_elf_silence.py", hook)
        for section in envs:
            with self.subTest(env=section):
                self.assertIn("post:tools/pio_size_budget.py", parser.get(section, "extra_scripts"))

    def test_managed_components_are_pinned_and_silent(self) -> None:
        # Principle V (exact versions) and Principle I (no audio component).
        manifest = (self.ROOT / "src" / "idf_component.yml").read_text(encoding="utf-8")
        for line in manifest.splitlines():
            stripped = line.split("#", 1)[0].strip()
            if ":" not in stripped or stripped.startswith("dependencies"):
                continue
            name, version = (part.strip().strip('"') for part in stripped.split(":", 1))
            with self.subTest(component=name):
                self.assertRegex(version, r"^==\d+\.\d+\.\d+$")
                self.assertIsNone(check_silence.BANNED_DEPENDENCY_PATTERN.search(name))

    def test_allow_list_names_existing_files(self) -> None:
        for rel in check_silence.ALLOW_LIST:
            with self.subTest(path=rel):
                self.assertTrue((self.ROOT / rel).is_file(), f"allow-listed file missing: {rel}")


if __name__ == "__main__":
    unittest.main()
