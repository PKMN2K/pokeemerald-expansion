#!/usr/bin/env python3
"""Fail-closed offline audit of Johto test cells and Emerald map-grid encoding."""
import argparse
import json
import struct
from pathlib import Path
from export_johto_basic_walkability_layout import export as export_layout, LAND_ELEVATION
from export_johto_sparse_attributes import build as build_attributes, UNRESOLVED

BEHAVIOR = {"FLOOR": 0, "TALL_GRASS": 2}

def audit(output):
    output.mkdir(parents=True, exist_ok=True)
    layout = export_layout(output)
    attributes, mask, _ = build_attributes()
    cells = layout["cells"]
    raw = (output / "johto_basic_walkability_layout.bin").read_bytes()
    assert len(raw) == 128 and len(cells) == 64
    decoded = struct.unpack("<64H", raw)
    checks = []
    for index, (cell, word) in enumerate(zip(cells, decoded)):
        metatile = cell["metatile_id"]
        behavior = BEHAVIOR[cell["source_symbol"]]
        assert (cell["x"], cell["y"]) == (index % 8, index // 8)
        assert 0 <= metatile < len(attributes)
        assert bool(mask[metatile // 8] & (1 << (metatile % 8)))
        assert attributes[metatile] != UNRESOLVED
        assert attributes[metatile] == behavior == cell["candidate_attribute_word"]
        assert word == cell["packed_cell"]
        assert (word & 0x03FF) == metatile
        assert ((word >> 10) & 0x3) == 0
        assert ((word >> 12) & 0xF) == LAND_ELEVATION
        checks.append({"x": cell["x"], "y": cell["y"], "metatile_id": metatile,
                       "behavior_candidate": behavior, "elevation_candidate": LAND_ELEVATION})
    result = {"status": "OFFLINE_ONLY", "cells_checked": len(checks),
              "behavior_candidates": BEHAVIOR, "elevation_candidate": LAND_ELEVATION,
              "basic_map_collision_bits_clear": True,
              "source_cell_binary_round_trip_checked": True,
              "runtime_collision_verified": False, "tileset_registered": False,
              "map_registered": False, "ready_for_live_registration": False}
    (output / "johto_test_cell_encoding_audit.json").write_text(
        json.dumps(result, indent=2) + "\n", encoding="utf-8")
    return result

if __name__ == "__main__":
    p = argparse.ArgumentParser()
    p.add_argument("--output", required=True, type=Path)
    audit(p.parse_args().output)
    print("PASS: 64 Johto test cells checked against candidate attributes and packed map-grid data")
    print("OFFLINE ONLY: runtime collision and map registration remain unverified")
