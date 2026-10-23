#!/usr/bin/env python3
"""Regression tests for lossless HGSS ribbon palette-bank relocation."""
import json
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from export_hgss_ribbon_gba_bg import parse_palette_map, remap_palette, export, verify_source_narc, compose_same_bank_tiles, compose_matching_palette_tiles, compose_in_original_palette
from audit_hgss_ribbon_palette_slots import audit
from validate_hgss_ribbon_gba_bg import validate


class PaletteRemappingTests(unittest.TestCase):
    def setUp(self):
        self.original = b"".join(bytes([i]) * 32 for i in range(16))

    def test_unverified_narc_is_rejected(self):
        with self.assertRaisesRegex(ValueError, "SHA-256 mismatch"):
            verify_source_narc(b"NARC" + bytes(128))

    def test_verified_source_digest_is_accepted(self):
        # Exercise the exact-match path without distributing retail ROM bytes.
        from export_hgss_ribbon_gba_bg import EXPECTED_NARC_SHA256
        with patch("export_hgss_ribbon_gba_bg.hashlib.sha256") as sha256:
            sha256.return_value.hexdigest.return_value = EXPECTED_NARC_SHA256
            self.assertIsNone(verify_source_narc(b"test fixture"))
            sha256.assert_called_once_with(b"test fixture")

    def test_source_digest_rejects_single_character_difference(self):
        from export_hgss_ribbon_gba_bg import EXPECTED_NARC_SHA256
        different = ("0" if EXPECTED_NARC_SHA256[0] != "0" else "1") + EXPECTED_NARC_SHA256[1:]
        with patch("export_hgss_ribbon_gba_bg.hashlib.sha256") as sha256:
            sha256.return_value.hexdigest.return_value = different
            with self.assertRaisesRegex(ValueError, "SHA-256 mismatch"):
                verify_source_narc(b"test fixture")

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

    def test_same_bank_transparency_preserves_original_pixel_indices(self):
        base = bytes([0x21] * 32)  # alternating source pixels 1 and 2
        overlay = bytes([0x03] * 32)  # transparent odd pixels, opaque 3 at even pixels
        output = compose_same_bank_tiles(base, overlay, 2 << 12, 2 << 12)
        self.assertEqual(output, bytes([0x23] * 32))
        self.assertEqual(len(output), 32)

    def test_same_bank_composition_resolves_tile_flips(self):
        # First row of the source overlay is opaque, all other rows transparent.
        base = bytes([0x11] * 32)
        overlay = bytes([0x22] * 4 + [0] * 28)
        output = compose_same_bank_tiles(base, overlay, 1 << 12, (1 << 12) | 0x800)
        self.assertEqual(output[:28], bytes([0x11] * 28))
        self.assertEqual(output[28:], bytes([0x22] * 4))

    def test_compositor_preserves_pixel_layout_for_transparent_overlay(self):
        base = bytes([0x21] * 32)
        overlay = bytes(32)
        self.assertEqual(
            compose_same_bank_tiles(base, overlay, 0, 0),
            base,
        )

    def test_cross_bank_exact_colour_composition(self):
        palette = bytearray(512)
        # BG3 index 5 and BG6 index 3 contain the same original BGR555 colour.
        palette[2 * 32 + 5 * 2:2 * 32 + 5 * 2 + 2] = b"\\x34\\x12"
        palette[4 * 32 + 3 * 2:4 * 32 + 3 * 2 + 2] = b"\\x34\\x12"
        base = bytes([0x11] * 32)
        overlay = bytes([0x03] * 32)
        result = compose_matching_palette_tiles(
            base, overlay, 2 << 12, 4 << 12, bytes(palette))
        self.assertEqual(result, bytes([0x15] * 32))

    def test_cross_bank_missing_colour_refuses_composition(self):
        palette = bytearray(512)
        palette[4 * 32 + 3 * 2:4 * 32 + 3 * 2 + 2] = b"\\x34\\x12"
        result = compose_matching_palette_tiles(
            bytes([0x11] * 32), bytes([0x03] * 32),
            2 << 12, 4 << 12, bytes(palette))
        self.assertIsNone(result)

    def test_cross_bank_composition_can_use_third_original_palette_bank(self):
        palette = bytearray(512)
        a, b = b"\\x34\\x12", b"\\x78\\x56"
        palette[2*32+2:2*32+4] = a
        palette[4*32+6:4*32+8] = b
        palette[6*32+10:6*32+12] = a
        palette[6*32+14:6*32+16] = b
        result = compose_in_original_palette(
            bytes([0x11])*32, bytes([0x03])*32,
            2 << 12, 4 << 12, bytes(palette))
        self.assertEqual(result, (bytes([0x57])*32, 6))

    def test_cross_bank_composition_rejects_unrepresentable_colours(self):
        palette = bytearray(512)
        palette[2*32+2:2*32+4] = b"\\x34\\x12"
        palette[4*32+6:4*32+8] = b"\\x78\\x56"
        self.assertIsNone(compose_in_original_palette(
            bytes([0x11])*32, bytes([0x03])*32,
            2 << 12, 4 << 12, bytes(palette)))

    def test_cross_bank_transparency_is_rejected(self):
        with self.assertRaisesRegex(ValueError, "different palette banks"):
            compose_same_bank_tiles(bytes(32), bytes(32), 1 << 12, 2 << 12)

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

    def test_export_roundtrip_remaps_real_screen_entries(self):
        # Synthetic Nitro structures: no copyrighted image data is bundled.
        palette = b"RLCN" + bytes(12) + b"TTLP" + bytes(20) + self.original
        self.assertEqual(len(palette), 552)
        ncgr_base = b"RGCN" + bytes(12) + b"RAHC" + bytes(44)
        self.assertEqual(len(ncgr_base), 64)
        ncgr_bg3 = ncgr_base + bytes([0x11]) * (256 * 32)
        ncgr_bg6 = ncgr_base + bytes([0x22]) * (512 * 32)
        screen = bytearray(2084)
        screen[0:4] = b"RCSN"
        screen[16:20] = b"NRCS"
        source_banks = (0, 2, 4)
        for y in range(20):
            for x in range(30):
                bank = source_banks[x % 3]
                position = 36 + (y * 32 + x) * 2
                screen[position:position + 2] = (bank << 12).to_bytes(2, "little")
        members = [b""] * 79
        members[0], members[1], members[2] = palette, ncgr_bg6, ncgr_bg3
        members[9] = members[10] = bytes(screen)
        with tempfile.TemporaryDirectory() as directory:
            with patch("export_hgss_ribbon_gba_bg.SOURCE") as source, \
                 patch("export_hgss_ribbon_gba_bg.selected", return_value=False):
                source.read_text.return_value = "CopyPaletteIntoBufferUnfaded(icon, BG_PLTT_ID(2), 32);"
                with self.assertRaisesRegex(ValueError, "requires a complete --palette-map"):
                    export(members, "header_only", 0, 0, Path(directory))
                self.assertFalse((Path(directory) / "header_only.bin").exists())
                export(members, "header_only", 0, 0, Path(directory),
                       {0: 7, 2: 8, 4: 9})
            prefix = Path(directory) / "header_only"
            tiles = prefix.with_suffix(".4bpp").read_bytes()
            mapped_palette = prefix.with_suffix(".gbapal").read_bytes()
            mapped_screen = prefix.with_suffix(".bin").read_bytes()
            result = validate(tiles, mapped_palette, mapped_screen)
            self.assertEqual(result["palette_banks"], [7, 8, 9])
            self.assertEqual(len(mapped_screen), 2048)
            for x in range(30):
                attr = int.from_bytes(mapped_screen[2*x:2*x+2], "little")
                self.assertEqual(attr >> 12, (7, 8, 9)[x % 3])
            self.assertEqual(mapped_palette[7 * 32:8 * 32], self.original[:32])
            self.assertEqual(mapped_palette[8 * 32:9 * 32], self.original[2 * 32:3 * 32])
            self.assertEqual(mapped_palette[9 * 32:10 * 32], self.original[4 * 32:5 * 32])
            for src, dst in ((0, 7), (2, 8), (4, 9)):
                bank_file = Path(directory) / f"header_only.bank_{dst:02d}.gbapal"
                self.assertEqual(bank_file.stat().st_size, 32)
                self.assertEqual(bank_file.read_bytes(),
                                 self.original[src * 32:(src + 1) * 32])
            self.assertFalse((Path(directory) / "header_only.bank_02.gbapal").exists())
            manifest = json.loads((Path(directory) / "header_only.manifest.json").read_text())
            self.assertFalse(manifest["runtime_verified"])
            self.assertFalse(manifest["composition_verified"])
            self.assertEqual(manifest["bg6_opaque_substitutions"], 0)
            self.assertEqual(manifest["bg6_same_bank_composited_cells"], 0)
            self.assertEqual(manifest["bg6_transparent_cells_not_composited"], [])
            self.assertEqual(manifest["viewport"], {"x": 0, "y": 0, "width": 240, "height": 160})
            self.assertEqual(
                [(bank["source_bank"], bank["destination_bank"], bank["size"])
                 for bank in manifest["palette_banks"]],
                [(0, 7, 32), (2, 8, 32), (4, 9, 32)],
            )
            self.assertEqual(manifest["tile_count"], len(tiles) // 32)

    def test_export_reports_same_bank_and_cross_bank_overlay_cells(self):
        palette = b"RLCN" + bytes(12) + b"TTLP" + bytes(20) + self.original
        header = b"RGCN" + bytes(12) + b"RAHC" + bytes(44)
        bg3 = header + bytes([0x11]) * (256 * 32)
        bg6 = header + bytes([0x03]) * (512 * 32)
        base_map = bytearray(2084)
        top_map = bytearray(2084)
        for screen in (base_map, top_map):
            screen[:4] = b"RCSN"
            screen[16:20] = b"NRCS"
        # The second cell uses a different bank on BG3 than BG6.
        base_map[38:40] = (2 << 12).to_bytes(2, "little")
        members = [b""] * 79
        members[0], members[1], members[2] = palette, bg6, bg3
        members[9], members[10] = bytes(top_map), bytes(base_map)
        with tempfile.TemporaryDirectory() as directory:
            with patch("export_hgss_ribbon_gba_bg.SOURCE") as source, \
                 patch("export_hgss_ribbon_gba_bg.selected",
                       side_effect=lambda layout, x, y: y == 0 and x in (0, 8)):
                source.read_text.return_value = ""
                export(members, "header_only", 0, 0, Path(directory), {0: 7, 2: 8})
            manifest = json.loads((Path(directory) / "header_only.manifest.json").read_text())
            self.assertEqual(manifest["bg6_same_bank_composited_cells"], 1)
            self.assertEqual(manifest["bg6_fully_transparent_cells_ignored"], 0)
            self.assertEqual(manifest["bg6_transparent_cells_not_composited"],
                             [{"x": 8, "y": 0, "source_x": 8, "source_y": 0,
                               "bg3_bank": 2, "bg6_bank": 0,
                               "bg6_visible_pixels": 32}])
            self.assertFalse(manifest["composition_verified"])
            tiles = (Path(directory) / "header_only.4bpp").read_bytes()
            tilemap = (Path(directory) / "header_only.bin").read_bytes()
            first = int.from_bytes(tilemap[:2], "little")
            second = int.from_bytes(tilemap[2:4], "little")
            self.assertEqual(first >> 12, 7)
            self.assertEqual(second >> 12, 8)
            self.assertEqual(tiles[(first & 0x3ff)*32:(first & 0x3ff)*32+32],
                             bytes([0x13])*32)
            self.assertEqual(tiles[(second & 0x3ff)*32:(second & 0x3ff)*32+32],
                             bytes([0x11])*32)

    def test_live_palette_audit(self):
        source = ("CopyPaletteIntoBufferUnfaded(foo, BG_PLTT_ID(2), 32);\n"
                  "CopyPaletteIntoBufferUnfaded(bar, BG_PLTT_ID(4), 32);")
        owners, conflicts = audit(source)
        self.assertEqual(set(owners), {2, 4})
        self.assertEqual(set(conflicts), {2, 4})


if __name__ == "__main__":
    unittest.main()
