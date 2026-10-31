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
                "#define TEXT_COLOR_INVALID 16\n"
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
            source = "FillWindowPixelBuffer(windowId, PIXEL_FILL(16));"
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

if __name__ == "__main__":
    unittest.main()
