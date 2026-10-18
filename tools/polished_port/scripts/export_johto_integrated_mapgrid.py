#!/usr/bin/env python3
"""Export a reproducible, offline 8x8 Emerald mapgrid integration fixture.

Combines source-backed candidate collision + elevation in one 128-byte
little-endian layout, with a machine-readable report. Deliberately does not
register a map or declare unverified field behaviors as validated.
"""
import argparse
import json
import struct
from pathlib import Path
from preview_johto_mapgrid_collision import prepare
from validate_johto_elevation_encoding import validate

def build():
    collisions, elevated = prepare(), validate()
    assert len(collisions) == len(elevated) == 64
    cells = []
    for i, (source, candidate) in enumerate(zip(collisions, elevated)):
        packed = candidate["encoded_cell"]
        assert (packed & 0x03FF) == source["metatile_id"]
        assert (packed & 0x0C00) == (source["packed_cell"] & 0x0C00)
        assert (packed & 0xF000) == (candidate["candidate_elevation"] << 12)
        cells.append({
            "index": i,
            "x": i % 8,
            "y": i // 8,
            "metatile_id": source["metatile_id"],
            "source_symbol": source["source_symbol"],
            "collision_bits": packed & 0x0C00,
            "elevation": candidate["candidate_elevation"],
            "packed_cell": packed,
            "verified_in_game": False,
        })
    return cells

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    cells = build()
    binary = struct.pack("<64H", *(row["packed_cell"] for row in cells))
    assert len(binary) == 128
    (args.output / "johto_mapgrid_candidate.bin").write_bytes(binary)
    (args.output / "johto_mapgrid_candidate.json").write_text(
        json.dumps({
            "status": "offline_integrated_mapgrid_fixture_not_live",
            "width": 8, "height": 8,
            "cells": cells,
            "bytes": 128,
            "field_behaviors_verified": False,
            "tileset_registered": False,
            "map_registered": False,
        }, indent=2) + "\n", encoding="utf-8")
    print("PASS: integrated 8x8 Johto test mapgrid written (128 bytes)")
    print("OFFLINE ONLY: field behaviors and tileset registration remain pending")
if __name__ == "__main__":
    main()
