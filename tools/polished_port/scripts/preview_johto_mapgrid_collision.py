#!/usr/bin/env python3
"""Export an OFFLINE Johto map-grid collision proof, not a playable layout.

Emerald stores collision in map cells (bits 10-11), not the tileset
behavior field. Source WALL/VOID are provisionally blocked with 0x0C00.
This is a conservative diagnostic for only the 8x8 sample grid, and
does NOT assert verified in-game collision/elevation or make water surfable.
"""
import argparse
import json
import struct
from pathlib import Path
from audit_johto_outdoor_test_grid import audit
from project_johto_collision_symbols import convert

IMPASSABLE = frozenset({"WALL", "VOID", "TOP_WALL", "BOTTOM_WALL", "BUOY", "WHIRLPOOL"})
WATER = "WATER"
COLLISION_MASK = 0x0C00
METATILE_MASK = 0x03FF
def prepare():
    grid = audit()["converted_metatile_ids_8x8"]
    rows = convert()["primary"] + convert()["secondary"]
    assert len(rows) == 1012
    cells = []
    for row in grid:
        for index in row:
            symbol = rows[index]["source_collision_symbol"]
            blocking = symbol in IMPASSABLE
            water = symbol == WATER
            cells.append({"metatile_id": index, "source_symbol": symbol,
                          "provisional_impassable": blocking,
                          "water_review_required": water,
                          "packed_cell": index | (COLLISION_MASK if blocking else 0)})
    assert len(cells) == 64
    assert all(c["packed_cell"] & METATILE_MASK == c["metatile_id"] for c in cells)
    assert all(c["packed_cell"] & COLLISION_MASK in (0, COLLISION_MASK) for c in cells)
    # A positive classification should never silently enable surfing.
    assert all(not c["water_review_required"] or not c["provisional_impassable"] for c in cells)
    return cells
def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    cells = prepare()
    args.output.mkdir(parents=True, exist_ok=True)
    (args.output / "johto_collision_preview.bin").write_bytes(
        struct.pack("<64H", *(cell["packed_cell"] for cell in cells)))
    (args.output / "johto_collision_preview.json").write_text(json.dumps({
        "status": "offline_collision_preview_not_registered",
        "width": 8, "height": 8, "cells": cells,
        "source_symbols_blocked_provisionally": sorted(IMPASSABLE),
        "water_surfing_verified": False,
        "map_elevation_verified": False,
        "metatile_attribute_tables_emitted": False,
        "map_registered": False,
    }, indent=2) + "\n")
    print("PASS: source-backed 8x8 map-grid collision preview, 128 bytes")
    print("OFFLINE ONLY: surf, elevation and field behavior remain unresolved")
if __name__ == "__main__":
    main()
