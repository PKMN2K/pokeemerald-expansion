#!/usr/bin/env python3
"""Audit a staging-only Johto outdoor rendering test grid; never register a map.

Representative source blocks expose the seven original CGB priority-flag
blocks alongside ordinary blocks. This is NOT a walkable Johto map: source
collision, elevation and map events still require engine-side verification.
"""
import json
from pathlib import Path
from test_johto_metatile_banks import load_binary_fixture
from convert_johto_traditional_blocks import convert, BLOCK_COUNT
from stage_johto_tile_graphics import build_graphics

ROOT = Path(__file__).resolve().parents[3]
GRID_BLOCKS = (
    (0, 20, 145, 146),
    (1, 210, 211, 2),
    (3, 214, 215, 4),
    (5, 6, 7, 8),
)

def audit():
    tiles = load_binary_fixture("metatiles")
    attrs = load_binary_fixture("attributes")
    gba_metatiles = convert(tiles, attrs)
    primary, secondary, _ = build_graphics()
    assert len(gba_metatiles) == BLOCK_COUNT * 4 * 16
    assert len(primary) == len(secondary) == 16384
    assert len(GRID_BLOCKS) == 4 and all(len(row) == 4 for row in GRID_BLOCKS)
    metatile_rows = []
    for row in GRID_BLOCKS:
        upper, lower = [], []
        for source_block in row:
            assert 0 <= source_block < BLOCK_COUNT
            first = source_block * 4
            upper.extend((first, first + 1))
            lower.extend((first + 2, first + 3))
        metatile_rows.extend((upper, lower))
    assert len(metatile_rows) == 8 and all(len(row) == 8 for row in metatile_rows)
    flagged = sorted({
        block for row in GRID_BLOCKS for block in row
        if any(a & 0x80 for a in attrs[block * 16:(block + 1) * 16])
    })
    assert flagged == [20, 145, 146, 210, 211, 214, 215]
    return {
        "status": "diagnostic_grid_only_not_registered",
        "purpose": "BG2/BG1 mask and animation visual test preparation",
        "source_blocks_4x4": GRID_BLOCKS,
        "converted_metatile_ids_8x8": metatile_rows,
        "source_priority_blocks": flagged,
        "source_metatiles": BLOCK_COUNT * 4,
        "source_gba_tiles": 288,
        "scroll_tile_id": 182,
        "animation_tile_ids": [132, 149, 157, 158, 178, 179, 194, 195],
        "live_map_registered": False,
        "requires_collision_validation": True,
        "requires_sprite_elevation_validation": True,
        "requires_animation_visual_validation": True,
    }

def main():
    doc = audit()
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(doc, indent=2) + "\n", encoding="utf-8")
    print("PASS: authentic Johto 8x8 diagnostic grid prepared from 16 source blocks")
    print("PASS: all seven original priority-bearing blocks covered")
    print("STAGED ONLY: no walkable layout or map registration")

if __name__ == "__main__":
    main()
