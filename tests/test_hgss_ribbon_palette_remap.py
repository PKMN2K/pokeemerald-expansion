#!/usr/bin/env python3
"""Regression tests for lossless HGSS ribbon palette-bank relocation."""
import sys
import unittest
from pathlib import Path
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from export_hgss_ribbon_gba_bg import parse_palette_map, remap_palette
from audit_hgss_ribbon_palette_slots import audit


class PaletteRemappingTests(unittest.TestCase):
    def setUp(self):
        self.original = b"".join(bytes([i]) * 32 for i in range(16))

    def test_parse_valid(self):
        self.assertEqual(parse_palette_map("0:7,2:8,4:9"), {0: 7, 2: 8, 4: 9})

    def test_parse_empty_is_opt_out(self):
        self.assertEqual(parse_palette_map(""), {})

    def test_invalid_maps_rejected(self):
        for spec in ("0", "0:16", "16:0", "-1:1", "0:7,0:8", "0:7,2:7", "0:7:8"):
            with self.subTest(spec=spec):
                with self.assertRaises(ValueError):
                    parse_palette_map(spec)

    def test_relocation_is_lossless(self):
        owners = "CopyPaletteIntoBufferUnfaded(icon, BG_PLTT_ID(2), 32);"
        with patch("export_hgss_ribbon_gba_bg.SOURCE") as source:
            source.read_text.return_value = owners
            output = remap_palette(self.original, {0: 7, 2: 8, 4: 9}, {0, 2, 4})
        self.assertEqual(len(output), 512)
        for src, dst in ((0, 7), (2, 8), (4, 9)):
            self.assertEqual(output[32 * dst:32 * (dst + 1)],
                             self.original[32 * src:32 * (src + 1)])
        self.assertEqual(output[32 * 2:32 * 3], bytes(32))

    def test_collision_fails_closed(self):
        owners = "CopyPaletteIntoBufferUnfaded(icon, BG_PLTT_ID(2), 32);"
        with patch("export_hgss_ribbon_gba_bg.SOURCE") as source:
            source.read_text.return_value = owners
            with self.assertRaisesRegex(ValueError, "collide"):
                remap_palette(self.original, {0: 2, 2: 8, 4: 9}, {0, 2, 4})

    def test_incomplete_map_rejected(self):
        with self.assertRaisesRegex(ValueError, "precisely"):
            remap_palette(self.original, {0: 7}, {0, 2, 4})

    def test_live_palette_audit(self):
        source = ("CopyPaletteIntoBufferUnfaded(foo, BG_PLTT_ID(2), 32);\n"
                  "CopyPaletteIntoBufferUnfaded(bar, BG_PLTT_ID(4), 32);")
        owners, conflicts = audit(source)
        self.assertEqual(set(owners), {2, 4})
        self.assertEqual(set(conflicts), {2, 4})


if __name__ == "__main__":
    unittest.main()
