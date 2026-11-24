"""Tests for tools/check_elf_silence.py (SIL-05). Run: python -m unittest discover tools"""
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import check_elf_silence  # noqa: E402


class AudioSymbolsTest(unittest.TestCase):
    def test_clean_image_passes(self) -> None:
        names = ["app_main", "esp_lcd_new_rgb_panel", "i2c_master_transmit", "esp_wifi_start", "nvs_get_str"]
        self.assertEqual(check_elf_silence.audio_symbols(names), [])

    def test_audio_drivers_are_found(self) -> None:
        names = ["i2s_new_channel", "dac_oneshot_new_unit", "ledc_timer_config", "esp_codec_dev_open", "app_main"]
        self.assertEqual(
            check_elf_silence.audio_symbols(names),
            ["dac_oneshot_new_unit", "esp_codec_dev_open", "i2s_new_channel", "ledc_timer_config"],
        )

    def test_i2c_is_not_mistaken_for_i2s(self) -> None:
        self.assertEqual(check_elf_silence.audio_symbols(["i2c_new_master_bus", "s_i2s_hidden"]), [])


if __name__ == "__main__":
    unittest.main()
