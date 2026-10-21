#!/usr/bin/env python3
"""Regression tests for lossless HGSS ribbon palette-bank relocation."""
import sys
import unittest
from pathlib import Path
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from export_hgss_ribbon_gba_bg import parse_palette_map, remap_palette
from audit_hgss_ribbon_palette_slots import audit
from validate_hgss_ribbon_gba_bg import validate


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

    def test_tilemap_structure_valid(self):
        tiles = bytes(32) + bytes([0x11]) * 32
        palette = bytes(512)
        tilemap = bytearray(2048)
        tilemap[0:2] = (1 | (7 << 12)).to_bytes(2, "little")
        result = validate(tiles, palette, bytes(tilemap))
        self.assertEqual(result["tile_count"], 2)
        self.assertEqual(result["palette_banks"], [0, 7])

    def test_tilemap_rejects_invalid_tile_reference(self):
        tilemap = bytearray(2048)
        tilemap[0:2] = (2).to_bytes(2, "little")
        with self.assertRaisesRegex(ValueError, "out of range"):
            validate(bytes(64), bytes(512), bytes(tilemap))

    def test_tilemap_rejects_nonblank_padding(self):
        tilemap = bytearray(2048)
        tilemap[(20 * 32) * 2:(20 * 32) * 2 + 2] = (1).to_bytes(2, "little")
        with self.assertRaisesRegex(ValueError, "offscreen"):
            validate(bytes(64), bytes(512), bytes(tilemap))

    def test_tilemap_rejects_malformed_binary(self):
        for tiles, palette, tilemap in (
            (bytes(31), bytes(512), bytes(2048)),
            (bytes(32), bytes(510), bytes(2048)),
            (bytes(32), bytes(512), bytes(2046)),
            (bytes([1]) * 32, bytes(512), bytes(2048)),
        ):
            with self.subTest(sizes=(len(tiles), len(palette), len(tilemap))):
                with self.assertRaises(ValueError):
                    validate(tiles, palette, tilemap)

    def test_live_palette_audit(self):
        source = ("CopyPaletteIntoBufferUnfaded(foo, BG_PLTT_ID(2), 32);\n"
                  "CopyPaletteIntoBufferUnfaded(bar, BG_PLTT_ID(4), 32);")
        owners, conflicts = audit(source)
        self.assertEqual(set(owners), {2, 4})
        self.assertEqual(set(conflicts), {2, 4})


if __name__ == "__main__":
    unittest.main()
