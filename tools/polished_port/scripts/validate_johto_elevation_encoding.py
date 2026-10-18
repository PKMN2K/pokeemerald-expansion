#!/usr/bin/env python3
"""Validate Emerald map-cell elevation packing for the isolated Johto grid.

This produces a candidate-only 8x8 map fixture. Elevation 3 is the
Emerald default for land, elevation 1 the surf plane; neither is proof
of equivalent Polished Crystal traversal or OBJ/BG priority.
"""
import argparse
import json
import re
import struct
from pathlib import Path
from preview_johto_mapgrid_collision import prepare

ROOT = Path(__file__).resolve().parents[3]
HEADER = ROOT / "include/global.fieldmap.h"
ID_MASK = 0x03ff
COLLISION_MASK = 0x0c00
ELEVATION_MASK = 0xf000
LAND_ELEVATION = 3
WATER_ELEVATION = 1

def validate():
    header = HEADER.read_text()
    for constant, value in (
        ("MAPGRID_METATILE_ID_MASK", "0x03FF"),
        ("MAPGRID_COLLISION_MASK", "0x0C00"),
        ("MAPGRID_ELEVATION_MASK", "0xF000"),
        ("ELEVATION_DEFAULT", "3"),
        ("ELEVATION_SURF", "1"),
    ):
        assert re.search(r"^\s*(?:#define\s+)?" + constant + r"\s*(?:=\s*)?" + value + r"\b", header, re.M), constant
    source = prepare()
    output = []
    for item in source:
        elevation = WATER_ELEVATION if item["source_symbol"] == "WATER" else LAND_ELEVATION
        packed = item["packed_cell"] | (elevation << 12)
        assert packed & ID_MASK == item["metatile_id"]
        assert packed & COLLISION_MASK == item["packed_cell"] & COLLISION_MASK
        assert (packed & ELEVATION_MASK) >> 12 == elevation
        output.append({
            "metatile_id": item["metatile_id"],
            "source_symbol": item["source_symbol"],
            "candidate_elevation": elevation,
            "encoded_cell": packed,
            "candidate_only": True,
        })
    assert len(output) == 64
    return output

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--output", required=True, type=Path)
    args = p.parse_args()
    cells = validate()
    args.output.mkdir(parents=True, exist_ok=True)
    (args.output / "johto_elevation_candidate.bin").write_bytes(
        struct.pack("<64H", *(c["encoded_cell"] for c in cells)))
    (args.output / "johto_elevation_candidate.json").write_text(
        json.dumps({
            "status": "elevation_encoding_validated_semantics_unverified",
            "width": 8, "height": 8,
            "cells": cells,
            "surf_eligibility_verified": False,
            "object_bg_priority_verified": False,
            "ledge_traversal_verified": False,
            "registered": False,
        }, indent=2) + "\n")
    print("PASS: 64 map cells preserve source ID and collision bits with valid elevation packing")
    print("NOT LIVE: land/surf plane elevation choices remain provisional")
if __name__ == "__main__":
    main()
