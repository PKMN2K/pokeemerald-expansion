#!/usr/bin/env python3
"""Build a source-faithful Johto collision/elevation registration blocker matrix.

This is a per-metatile actionable review, NOT a GBA mapgrid or attribute
binary: actual collision and elevation live in map cells and require testing.
"""
import argparse
import json
from collections import Counter
from pathlib import Path
from merge_johto_behavior_candidates import merge

# Exact WALL_TILE permissions from the pinned Crystal collision table.
# WHIRLPOOL is WATER_TILE; TOP_WALL and BOTTOM_WALL are LAND_TILE.
IMPASSABLE = {"WALL", "VOID", "HEADBUTT_TREE", "BUOY"}
WATER = {"WATER"}
LEDGES = {"LEDGE_DOWN", "LEDGE_LEFT", "LEDGE_RIGHT", "LEDGE_DOWN_LEFT", "LEDGE_DOWN_RIGHT"}
BASIC = {"FLOOR", "TALL_GRASS"}

def classify(row):
    symbol = row["source_collision_symbol"]
    blockers = []
    if symbol in IMPASSABLE:
        blockers += ["map_cell_collision_block", "elevation_and_object_occlusion_review"]
    elif symbol in WATER:
        blockers += ["map_cell_surf_elevation", "surf_and_shore_transition_test"]
    elif symbol in LEDGES:
        blockers += ["jump_direction_and_collision_test", "map_cell_elevation_test"]
    elif symbol in BASIC:
        blockers += ["map_cell_walkability_test"]
    else:
        blockers += ["source_special_behavior_translation", "field_interaction_or_script_test"]
    if symbol == "HEADBUTT_TREE":
        blockers += ["headbutt_field_interaction_test"]
    if symbol == "BUOY":
        blockers += ["buoy_water_placement_and_occlusion_test"]
    if symbol == "WHIRLPOOL":
        blockers += ["water_tile_permission_test", "whirlpool_field_move_test",
                     "not_an_ordinary_impassable_wall"]
    if symbol == "VOID":
        blockers += ["map_border_test"]
    if symbol in {"TOP_WALL", "BOTTOM_WALL"}:
        blockers += ["foreground_priority_test", "directional_edge_collision_test",
                     "source_permission_is_land_not_wall"]
    return {**row, "required_live_tests": blockers, "verified_in_game": False}

def review():
    data = merge()
    rows = [classify(x) for x in data["primary"] + data["secondary"]]
    assert len(rows) == 1012 and [x["metatile_id"] for x in rows] == list(range(1012))
    assert all(not x["verified_in_game"] and not x["registration_ready"] for x in rows)
    symbols = Counter(x["source_collision_symbol"] for x in rows)
    categories = {
        "impassable": sum(symbols[x] for x in IMPASSABLE),
        "water": sum(symbols[x] for x in WATER),
        "ledge": sum(symbols[x] for x in LEDGES),
        "basic": sum(symbols[x] for x in BASIC),
    }
    categories["special_other"] = 1012 - sum(categories.values())
    assert all(v >= 0 for v in categories.values())
    assert sum(categories.values()) == 1012
    assert categories["impassable"] and categories["water"] and categories["ledge"]
    return {
        "status": "per_metatile_mapgrid_blockers_review_only",
        "categories": categories,
        "primary": rows[:512], "secondary": rows[512:],
        "generated_mapgrid_cells": False,
        "generated_attribute_binary": False,
        "live_map_registered": False,
    }

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    result = review()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print("PASS: 1012 metatiles classified with explicit live collision/elevation test blockers")
    print("NOT LIVE: no generated mapgrid, attributes or map registration")
if __name__ == "__main__":
    main()
