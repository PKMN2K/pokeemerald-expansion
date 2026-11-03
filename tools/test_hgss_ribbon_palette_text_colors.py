#!/usr/bin/env python3
"""Regression tests for HGSS ribbon text-color palette auditing."""
import contextlib
import io
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

import audit_hgss_ribbon_palette_slots as audit


class RibbonTextColorAuditTests(unittest.TestCase):
    def test_resolves_decimal_and_hex_text_color_constants(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            constants = root / "include" / "constants"
            constants.mkdir(parents=True)
            (constants / "characters.h").write_text(
                "#define TEXT_COLOR_DARK_GRAY 3\n"
                "#define TEXT_COLOR_RED 0x0E\n"
            )
            source = (
                "const u8 colors[] = {TEXT_COLOR_DARK_GRAY, TEXT_COLOR_RED};\n"
            )
            output = io.StringIO()
            with patch.object(audit, "ROOT", root), contextlib.redirect_stdout(output):
                audit.audit_ribbon_window_color_dependencies(source)
            result = output.getvalue()
            self.assertIn("TEXT_COLOR_DARK_GRAY = 4bpp color index 3", result)
            self.assertIn("TEXT_COLOR_RED = 4bpp color index 14", result)
            self.assertNotIn("UNRESOLVED: TEXT_COLOR_", result)

    def test_reports_unknown_text_color_constant(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            constants = root / "include" / "constants"
            constants.mkdir(parents=True)
            (constants / "characters.h").write_text(
                "#define TEXT_COLOR_RED 0x0E\n"
            )
            output = io.StringIO()
            with patch.object(audit, "ROOT", root), contextlib.redirect_stdout(output):
                audit.audit_ribbon_window_color_dependencies(
                    "const u8 color = TEXT_COLOR_UNKNOWN;\n"
                )
            self.assertIn("UNRESOLVED: TEXT_COLOR_UNKNOWN", output.getvalue())


    def test_rejects_out_of_range_four_bit_text_color(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            constants = root / "include" / "constants"
            constants.mkdir(parents=True)
            (constants / "characters.h").write_text(
                "#define TEXT_COLOR_INVALID 16U\n"
            )
            output = io.StringIO()
            with patch.object(audit, "ROOT", root), contextlib.redirect_stdout(output):
                audit.audit_ribbon_window_color_dependencies(
                    "const u8 color = TEXT_COLOR_INVALID;\n"
                )
            self.assertIn("UNRESOLVED: TEXT_COLOR_INVALID = 16", output.getvalue())



    def test_reports_out_of_range_pixel_fill_index(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            constants = root / "include" / "constants"
            constants.mkdir(parents=True)
            (constants / "characters.h").write_text("")
            source = "FillWindowPixelBuffer(windowId, PIXEL_FILL(0x10U));"
            output = io.StringIO()
            with patch.object(audit, "ROOT", root), contextlib.redirect_stdout(output):
                audit.audit_ribbon_window_color_dependencies(source)
            self.assertIn("UNRESOLVED: PIXEL_FILL index 16", output.getvalue())


    def test_flags_invalid_default_font_palette_index(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "src"
            source.mkdir()
            (source / "text.c").write_text(
                "[FONT_NORMAL] = {\n"
                "    .color.foreground = 16,\n"
                "    .color.background = 0,\n"
                "    .color.shadow = 1,\n"
                "    .color.accent = 2,\n"
                "},\n"
            )
            output = io.StringIO()
            with patch.object(audit, "ROOT", root), contextlib.redirect_stdout(output):
                audit.audit_default_ribbon_font_colors()
            self.assertIn("UNRESOLVED: default FONT_NORMAL foreground index 16", output.getvalue())


    def test_flags_hexadecimal_default_font_palette_index(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "src"
            source.mkdir()
            (source / "text.c").write_text(
                "[FONT_SMALL] = {\n"
                "    .color.foreground = 0x10U,\n"
                "    .color.background = 0x0,\n"
                "    .color.shadow = 0x1,\n"
                "    .color.accent = 0x2,\n"
                "},\n"
            )
            output = io.StringIO()
            with patch.object(audit, "ROOT", root), contextlib.redirect_stdout(output):
                audit.audit_default_ribbon_font_colors()
            self.assertIn("UNRESOLVED: default FONT_SMALL foreground index 16", output.getvalue())


    def test_reports_symbolic_pixel_fill_as_unresolved(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            constants = root / "include" / "constants"
            constants.mkdir(parents=True)
            (constants / "characters.h").write_text("")
            output = io.StringIO()
            with patch.object(audit, "ROOT", root), contextlib.redirect_stdout(output):
                audit.audit_ribbon_window_color_dependencies(
                    "FillWindowPixelBuffer(windowId, PIXEL_FILL(BG_COLOR_INDEX));"
                )
            self.assertIn("UNRESOLVED: PIXEL_FILL expression BG_COLOR_INDEX", output.getvalue())


    def test_reports_multiline_symbolic_pixel_fill_as_unresolved(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            constants = root / "include" / "constants"
            constants.mkdir(parents=True)
            (constants / "characters.h").write_text("")
            output = io.StringIO()
            with patch.object(audit, "ROOT", root), contextlib.redirect_stdout(output):
                audit.audit_ribbon_window_color_dependencies(
                    "FillWindowPixelBuffer(windowId, PIXEL_FILL(\n    BG_COLOR_INDEX\n    + 1\n));"
                )
            self.assertIn("UNRESOLVED: PIXEL_FILL expression BG_COLOR_INDEX + 1", output.getvalue())


    def test_resolves_multiline_numeric_pixel_fill_without_false_warning(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            constants = root / "include" / "constants"
            constants.mkdir(parents=True)
            (constants / "characters.h").write_text("")
            output = io.StringIO()
            with patch.object(audit, "ROOT", root), contextlib.redirect_stdout(output):
                audit.audit_ribbon_window_color_dependencies(
                    "FillWindowPixelBuffer(windowId, PIXEL_FILL(\n    0x0FU\n));"
                )
            result = output.getvalue()
            self.assertIn("4bpp color index 15", result)
            self.assertNotIn("UNRESOLVED: PIXEL_FILL", result)


    def test_rejects_negative_pixel_fill_expression(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            constants = root / "include" / "constants"
            constants.mkdir(parents=True)
            (constants / "characters.h").write_text("")
            output = io.StringIO()
            with patch.object(audit, "ROOT", root), contextlib.redirect_stdout(output):
                audit.audit_ribbon_window_color_dependencies(
                    "FillWindowPixelBuffer(windowId, PIXEL_FILL(-1));"
                )
            result = output.getvalue()
            self.assertIn("UNRESOLVED: PIXEL_FILL expression -1", result)
            self.assertNotIn("4bpp color index -1", result)


    def test_reports_arithmetic_pixel_fill_as_unresolved(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            constants = root / "include" / "constants"
            constants.mkdir(parents=True)
            (constants / "characters.h").write_text("")
            output = io.StringIO()
            with patch.object(audit, "ROOT", root), contextlib.redirect_stdout(output):
                audit.audit_ribbon_window_color_dependencies(
                    "FillWindowPixelBuffer(windowId, PIXEL_FILL(15 + 1));"
                )
            result = output.getvalue()
            self.assertIn("UNRESOLVED: PIXEL_FILL expression 15 + 1", result)
            self.assertNotIn("4bpp color index 15", result)


    def test_reports_parenthesized_pixel_fill_expression_as_unresolved(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            constants = root / "include" / "constants"
            constants.mkdir(parents=True)
            (constants / "characters.h").write_text("")
            output = io.StringIO()
            with patch.object(audit, "ROOT", root), contextlib.redirect_stdout(output):
                audit.audit_ribbon_window_color_dependencies(
                    "FillWindowPixelBuffer(windowId, PIXEL_FILL((BG_COLOR_INDEX)));"
                )
            self.assertIn("UNRESOLVED: PIXEL_FILL expression (BG_COLOR_INDEX", output.getvalue())


    def test_reports_malformed_numeric_suffix_as_unresolved(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            constants = root / "include" / "constants"
            constants.mkdir(parents=True)
            (constants / "characters.h").write_text("")
            output = io.StringIO()
            with patch.object(audit, "ROOT", root), contextlib.redirect_stdout(output):
                audit.audit_ribbon_window_color_dependencies(
                    "FillWindowPixelBuffer(windowId, PIXEL_FILL(15XYZ));"
                )
            result = output.getvalue()
            self.assertIn("UNRESOLVED: PIXEL_FILL expression 15XYZ", result)
            self.assertNotIn("4bpp color index 15", result)


    def test_resolves_indented_text_color_define(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            constants = root / "include" / "constants"
            constants.mkdir(parents=True)
            (constants / "characters.h").write_text(
                "  # define TEXT_COLOR_INDENTED 0x0FU\\n"
            )
            output = io.StringIO()
            with patch.object(audit, "ROOT", root), contextlib.redirect_stdout(output):
                audit.audit_ribbon_window_color_dependencies(
                    "const u8 color = TEXT_COLOR_INDENTED;"
                )
            result = output.getvalue()
            self.assertIn("TEXT_COLOR_INDENTED = 4bpp color index 15", result)
            self.assertNotIn("UNRESOLVED: TEXT_COLOR_INDENTED", result)


    def test_accepts_zero_palette_index_boundary(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            constants = root / "include" / "constants"
            constants.mkdir(parents=True)
            (constants / "characters.h").write_text("")
            output = io.StringIO()
            with patch.object(audit, "ROOT", root), contextlib.redirect_stdout(output):
                audit.audit_ribbon_window_color_dependencies(
                    "FillWindowPixelBuffer(windowId, PIXEL_FILL(0));"
                )
            result = output.getvalue()
            self.assertIn("4bpp color index 0", result)
            self.assertNotIn("UNRESOLVED: PIXEL_FILL", result)


    def test_accepts_maximum_palette_index_boundary(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            constants = root / "include" / "constants"
            constants.mkdir(parents=True)
            (constants / "characters.h").write_text("")
            output = io.StringIO()
            with patch.object(audit, "ROOT", root), contextlib.redirect_stdout(output):
                audit.audit_ribbon_window_color_dependencies(
                    "FillWindowPixelBuffer(windowId, PIXEL_FILL(0x0FUL));"
                )
            result = output.getvalue()
            self.assertIn("4bpp color index 15", result)
            self.assertNotIn("UNRESOLVED: PIXEL_FILL", result)


    def test_rejects_hexadecimal_palette_index_above_boundary(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            constants = root / "include" / "constants"
            constants.mkdir(parents=True)
            (constants / "characters.h").write_text("")
            output = io.StringIO()
            with patch.object(audit, "ROOT", root), contextlib.redirect_stdout(output):
                audit.audit_ribbon_window_color_dependencies(
                    "FillWindowPixelBuffer(windowId, PIXEL_FILL(0x10UL));"
                )
            result = output.getvalue()
            self.assertIn("UNRESOLVED: PIXEL_FILL index 16", result)
            self.assertNotIn("4bpp color index 16", result)


    def test_preserves_complete_nested_pixel_fill_expression(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            constants = root / "include" / "constants"
            constants.mkdir(parents=True)
            (constants / "characters.h").write_text("")
            output = io.StringIO()
            with patch.object(audit, "ROOT", root), contextlib.redirect_stdout(output):
                audit.audit_ribbon_window_color_dependencies(
                    "FillWindowPixelBuffer(windowId, PIXEL_FILL((BG_COLOR_INDEX + 1)));"
                )
            self.assertIn(
                "UNRESOLVED: PIXEL_FILL expression (BG_COLOR_INDEX + 1)",
                output.getvalue()
            )


    def test_live_hgss_ribbon_bg_palettes_match_tilemap_palette_offset(self):
        """Authentic HGSS palette files must match the BG1 tilemap bank mapping."""
        source = audit.SOURCE.read_text()
        for index in range(5):
            self.assertIn(
                f'sHgssRibbonBigPal{index}[] = INCBIN_U16('
                f'"graphics/gen4_ui/hgss_ribbons/palette_{index}.gbapal")',
                source,
            )
            self.assertRegex(
                source,
                rf"CopyPaletteIntoBufferUnfaded\s*\(\s*sHgssRibbonBigPal{index}"
                rf"\s*,\s*BG_PLTT_ID\s*\(\s*{index + 2}\s*\)",
            )
        self.assertRegex(
            source,
            r"palNum\s*=\s*sRibbonGfxData\[ribbonId\]\.palNumOffset\s*\+\s*2\s*;",
        )


    def test_authentic_hgss_ribbon_tiles_feed_grid_and_expanded_sprite(self):
        """Both live ribbon views must share the authentic, unscaled HGSS sheet."""
        source = audit.SOURCE.read_text()
        self.assertIn(
            'sRibbonIconsBig_Gfx[] = INCBIN_U32('
            '"graphics/gen4_ui/hgss_ribbons/big_icons_hgss.4bpp")',
            source,
        )
        self.assertRegex(
            source,
            r"LoadBgTiles\s*\(\s*1\s*,\s*sRibbonIconsBig_Gfx\s*,"
            r"\s*sizeof\s*\(\s*sRibbonIconsBig_Gfx\s*\)",
        )
        self.assertRegex(
            source,
            r"sSpriteSheet_RibbonIconsBig\s*=\s*\{\s*"
            r"sRibbonIconsBig_Gfx\s*,\s*sizeof\s*\(\s*sRibbonIconsBig_Gfx\s*\)",
        )
        self.assertNotRegex(
            source,
            r'(?:LoadBgTiles|INCBIN_U32)\s*\([^;]*'
            r'(?:ribbon_icons_big|ribbon_icons_small)\.4bpp',
        )


    def test_authentic_ribbon_icon_tiles_are_shared_by_bg_and_zoom_sprite(self):
        """BG grid and selected-ribbon zoom must use the same HGSS source art."""
        source = audit.SOURCE.read_text()
        self.assertIn(
            'sRibbonIconsBig_Gfx[] = INCBIN_U32('
            '"graphics/gen4_ui/hgss_ribbons/big_icons_hgss.4bpp")',
            source,
        )
        self.assertRegex(
            source,
            r"LoadBgTiles\s*\(\s*1\s*,\s*sRibbonIconsBig_Gfx\s*,"
            r"\s*sizeof\s*\(\s*sRibbonIconsBig_Gfx\s*\)",
        )
        self.assertRegex(
            source,
            r"sSpriteSheet_RibbonIconsBig\s*=\s*\{\s*"
            r"sRibbonIconsBig_Gfx\s*,\s*sizeof\s*\(\s*sRibbonIconsBig_Gfx\s*\)",
        )
        self.assertNotIn("gPokenavRibbonsIconsBig_Gfx", source)

if __name__ == "__main__":
    unittest.main()
