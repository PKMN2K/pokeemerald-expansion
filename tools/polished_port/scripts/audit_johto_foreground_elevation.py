#!/usr/bin/env python3
"""Cross-check authentic Johto foreground masks against map-cell elevation.

This checks source CGB priority bit 7 through translated 16x16 BG1
metatile entries for the isolated grid. Emerald OBJ occlusion is NOT
established by these data checks; a real map/sprite visual test is needed.
"""
import argparse
import json
import struct
from pathlib import Path
from audit_johto_outdoor_test_grid import audit
from test_johto_metatile_banks import load_binary_fixture
from convert_johto_traditional_blocks import convert
from stage_johto_tile_graphics import priority_overlay_tile_ids
from validate_johto_elevation_encoding import validate

def inspect():
    source_tiles = load_binary_fixture("metatiles")
    attrs = load_binary_fixture("attributes")
    geometry = convert(source_tiles, attrs)
    overlay = set(priority_overlay_tile_ids(source_tiles, attrs).values())
    grid = audit()["converted_metatile_ids_8x8"]
    cells = validate()
    records = []
    for row_index, row in enumerate(grid):
        for col_index, metatile_id in enumerate(row):
            cell = cells[row_index * 8 + col_index]
            assert cell["metatile_id"] == metatile_id
            entries = struct.unpack_from("<8H", geometry, metatile_id * 16)
            foreground = entries[4:]
            masked = [idx for idx, value in enumerate(foreground) if (value & 1023) in overlay]
            assert all((value & 1023) == 0 or (value & 1023) in overlay for value in foreground)
            assert len(masked) <= 4
            records.append({
                "x": col_index, "y": row_index,
                "metatile_id": metatile_id,
                "source_block": metatile_id // 4,
                "masked_foreground_subtiles": len(masked),
                "candidate_elevation": cell["candidate_elevation"],
                "collision_bits": cell["encoded_cell"] & 0x0c00,
            })
    flagged_blocks = {r["source_block"] for r in records if r["masked_foreground_subtiles"]}
    assert flagged_blocks == set(audit()["source_priority_blocks"])
    assert len(records) == 64
    return {
        "status": "foreground_mask_and_elevation_consistency_only",
        "cells": records,
        "flagged_source_blocks": sorted(flagged_blocks),
        "sprite_occlusion_verified_in_game": False,
        "live_map_registered": False,
        "engine_bg_layer_type_required": "METATILE_LAYER_TYPE_NORMAL",
    }
def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    report = inspect()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + "\n")
    print("PASS: source foreground masks align with 64 Johto elevation-preview cells")
    print("NOT VERIFIED: live OBJ/BG occlusion; map remains unregistered")
if __name__ == "__main__":
    main()
