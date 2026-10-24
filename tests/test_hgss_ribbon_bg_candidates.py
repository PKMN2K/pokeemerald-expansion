#!/usr/bin/env python3
"""Regression coverage for the source-only HGSS ribbon layout comparison."""
import json
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from audit_hgss_ribbon_bg_candidates import audit_all
from preview_hgss_ribbon_tile_layout import TRIALS


class CandidateAuditTests(unittest.TestCase):
    def test_complete_tile_aligned_coverage_and_stable_order(self):
        calls = []

        def synthetic_export(members, layout, x, y, folder, audit_only=False):
            self.assertTrue(audit_only)
            calls.append((layout, x, y))
            return {
                "bg6_transparent_cells_not_composited":
                    [{"bg6_visible_pixels": 2}] * (y // 8),
                "bg6_unresolved_palette_bank_pairs":
                    [{"bg3_bank": 0, "bg6_bank": 1,
                      "cells": y // 8, "visible_pixels": y // 4}],
                "source_palette_banks_needed": [0],
                "bg6_opaque_substitutions": 0,
                "bg6_same_bank_composited_cells": 1,
                "bg6_cross_bank_exact_color_composited_cells": 0,
            }

        with tempfile.TemporaryDirectory() as temp:
            with patch("audit_hgss_ribbon_bg_candidates.export",
                       side_effect=synthetic_export):
                report = audit_all([], Path(temp))
            expected = len(TRIALS) * 13 * 3
            self.assertEqual(report["candidate_count"], expected)
            self.assertEqual(len(calls), expected)
            self.assertEqual(len(set(calls)), expected)
            self.assertEqual({x for _, x, _ in calls}, {0, 8, 16})
            self.assertEqual({y for _, _, y in calls}, set(range(0, 97, 8)))
            self.assertTrue(all(
                (row["layout"], row["unresolved_cells"],
                 row["unresolved_visible_pixels"]) <=
                (report["candidates"][i + 1]["layout"],
                 report["candidates"][i + 1]["unresolved_cells"],
                 report["candidates"][i + 1]["unresolved_visible_pixels"])
                for i, row in enumerate(report["candidates"][:-1])
            ))
            self.assertTrue(all(row["unresolved_visible_pixels"] ==
                                row["unresolved_cells"] * 2
                                for row in report["candidates"]))
            saved = json.loads((Path(temp) / "comparison.json").read_text())
            self.assertEqual(saved, report)

    def test_rejects_inconsistent_palette_conflict_totals(self):
        inconsistent = {
            "bg6_transparent_cells_not_composited":
                [{"bg6_visible_pixels": 4}],
            "bg6_unresolved_palette_bank_pairs":
                [{"bg3_bank": 0, "bg6_bank": 1,
                  "cells": 1, "visible_pixels": 3}],
        }
        with tempfile.TemporaryDirectory() as temp:
            with patch("audit_hgss_ribbon_bg_candidates.export",
                       return_value=inconsistent):
                with self.assertRaisesRegex(ValueError, "Inconsistent"):
                    audit_all([], Path(temp))
            self.assertFalse((Path(temp) / "comparison.json").exists())

    def test_does_not_claim_visual_verification(self):
        with tempfile.TemporaryDirectory() as temp:
            with patch("audit_hgss_ribbon_bg_candidates.export",
                       return_value={
                           "bg6_transparent_cells_not_composited": [],
                           "bg6_unresolved_palette_bank_pairs": [],
                           "source_palette_banks_needed": [],
                           "bg6_opaque_substitutions": 0,
                           "bg6_same_bank_composited_cells": 0,
                           "bg6_cross_bank_exact_color_composited_cells": 0,
                       }):
                result = audit_all([], Path(temp))
        self.assertIn("not visual or runtime verification", result["purpose"])
        self.assertIn("cannot rank layouts", result["warning"])


if __name__ == "__main__":
    unittest.main()
