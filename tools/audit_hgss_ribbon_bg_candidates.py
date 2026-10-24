#!/usr/bin/env python3
"""Compare authentic HGSS ribbon BG composition across tile-aligned GBA crops.

Diagnostic only: fewest unresolved tiles does NOT verify retail BG priority,
scroll offset, protected windows, or appearance. Requires original NARC bytes.
"""
import argparse
import json
from pathlib import Path

from export_hgss_ribbon_gba_bg import export, verify_source_narc
from render_hgss_ribbon_backgrounds import narc_members
from preview_hgss_ribbon_tile_layout import TRIALS


def audit_all(members, output):
    output.mkdir(parents=True, exist_ok=True)
    candidates = []
    for layout in TRIALS:
        for y in range(0, 97, 8):
            for x in range(0, 17, 8):
                folder = output / layout / f"x{x}_y{y}"
                result = export(members, layout, x, y, folder, audit_only=True)
                unresolved_pixels = sum(
                    cell["bg6_visible_pixels"]
                    for cell in result["bg6_transparent_cells_not_composited"]
                )
                pair_pixels = sum(pair["visible_pixels"] for pair in
                                  result["bg6_unresolved_palette_bank_pairs"])
                pair_cells = sum(pair["cells"] for pair in
                                 result["bg6_unresolved_palette_bank_pairs"])
                if (pair_pixels != unresolved_pixels or
                        pair_cells != len(result["bg6_transparent_cells_not_composited"])):
                    raise ValueError(
                        f"Inconsistent BG3/BG6 conflict accounting at {layout} "
                        f"viewport ({x}, {y})"
                    )
                candidates.append({
                    "layout": layout, "viewport_x": x, "viewport_y": y,
                    "unresolved_visible_pixels": unresolved_pixels,
                    "unresolved_cells": len(result["bg6_transparent_cells_not_composited"]),
                    "unresolved_pairs": result["bg6_unresolved_palette_bank_pairs"],
                    "source_palette_banks_needed": result["source_palette_banks_needed"],
                    "opaque_substitutions": result["bg6_opaque_substitutions"],
                    "same_bank_composites": result["bg6_same_bank_composited_cells"],
                    "cross_bank_composites": result["bg6_cross_bank_exact_color_composited_cells"],
                })
    # Compare viewport crops only within the same layout. Layouts cover
    # different BG6 regions, so a smaller region naturally has fewer conflicts.
    candidates.sort(key=lambda row: (row["layout"], row["unresolved_cells"],
                                      row["unresolved_visible_pixels"],
                                      row["viewport_y"], row["viewport_x"]))
    summary = {
        "purpose": "source-only comparison within each layout; not visual or runtime verification",
        "warning": "Conflict counts cannot rank layouts of different BG6 coverage",
        "candidate_count": len(candidates),
        "candidates": candidates,
    }
    (output / "comparison.json").write_text(json.dumps(summary, indent=2) + "\n")
    return summary


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("narc", type=Path, help="Verified retail HGSS a/1/6/2 NARC")
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    data = args.narc.read_bytes()
    verify_source_narc(data)
    members = narc_members(data)
    if len(members) != 79:
        raise ValueError("Expected 79 retail NARC members")
    summary = audit_all(members, args.output)
    print(f"Compared {summary['candidate_count']} source-only layouts.")
    print("Review comparison.json alongside original HGSS screenshots; "
          "unresolved count alone cannot establish the correct layout.")


if __name__ == "__main__":
    main()
