#!/usr/bin/env python3
"""Build an isolated 8x8 walkability test layout from source-backed Johto tiles.

This is an offline fixture: it does not register a tileset, warps, or map.
The layout uses only FLOOR and TALL_GRASS entries with candidate attributes.
"""
import argparse
import json
import struct
from pathlib import Path
from select_johto_basic_test_terrain import select

WIDTH = HEIGHT = 8
LAND_ELEVATION = 3

def build():
    entries = select()
    floors = [x for x in entries if x["source_symbol"] == "FLOOR"]
    grass = [x for x in entries if x["source_symbol"] == "TALL_GRASS"]
    assert floors, "Source selection must contain a basic FLOOR tile"
    floor = floors[0]
    grass_tile = grass[0] if grass else None
    rows = []
    for y in range(HEIGHT):
        for x in range(WIDTH):
            choice = grass_tile if grass_tile and 2 <= x <= 5 and 2 <= y <= 5 else floor
            assert choice["attribute_word"] in (0, 2)
            rows.append({
                "x": x, "y": y,
                "metatile_id": choice["metatile_id"],
                "source_symbol": choice["source_symbol"],
                "candidate_attribute_word": choice["attribute_word"],
                "packed_cell": choice["metatile_id"] | (LAND_ELEVATION << 12),
                "in_game_verified": False,
            })
    assert len(rows) == WIDTH * HEIGHT
    assert all(r["packed_cell"] & 0x0C00 == 0 for r in rows)
    return rows

def export(out):
    out.mkdir(parents=True, exist_ok=True)
    cells = build()
    raw = struct.pack("<64H", *(r["packed_cell"] for r in cells))
    (out / "johto_basic_walkability_layout.bin").write_bytes(raw)
    result = {
        "status": "isolated_basic_walkability_layout_OFFLINE",
        "width": WIDTH, "height": HEIGHT,
        "bytes": len(raw), "elevation_candidate": LAND_ELEVATION,
        "cells": cells,
        "no_unknown_behavior_fallback": True,
        "live_tileset_registered": False,
        "live_map_registered": False,
        "movement_verified_in_emulator": False,
    }
    (out / "johto_basic_walkability_layout.json").write_text(json.dumps(result, indent=2) + "\n")
    return result

if __name__ == "__main__":
    p = argparse.ArgumentParser()
    p.add_argument("--output", type=Path, required=True)
    doc = export(p.parse_args().output)
    print("PASS: 8x8 source-backed floor/grass layout exported (128 bytes)")
    print("OFFLINE ONLY: in-game collision and rendering are not verified")
