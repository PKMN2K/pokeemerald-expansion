#!/usr/bin/env python3
"""Audit Johto wall/water collision families against Emerald field semantics.

A matching metatile behavior constant alone does not guarantee field parity:
WALL/VOID need collision/elevation blocking; WATER needs surf/shoreline
rules; BUOY is not freely traversable water; WHIRLPOOL needs event logic.
No live attribute table is generated.
"""
import argparse
import json
from collections import Counter
from pathlib import Path
from project_johto_collision_symbols import convert
from review_johto_emerald_behaviors import EMERALD_BEHAVIORS

FAMILIES = {
    "WALL": ("solid_obstacle", None, "requires_impassable_collision_and_elevation"),
    "VOID": ("out_of_bounds", None, "requires_impassable_collision_and_map_border"),
    "TOP_WALL": ("upper_wall", None, "requires_bg_obj_elevation_and_impassability"),
    "BOTTOM_WALL": ("lower_wall", None, "requires_elevation_and_impassability"),
    "WATER": ("surfable_water_candidate", "MB_OCEAN_WATER", "requires_surf_eligibility_and_shoreline_review"),
    "BUOY": ("water_obstacle", None, "requires_water_edge_blocking"),
    "WHIRLPOOL": ("special_water_obstacle", None, "requires_special_field_interaction"),
}
def audit():
    constants = EMERALD_BEHAVIORS.read_text()
    assert "MB_OCEAN_WATER," in constants
    source = convert()
    all_rows = source["primary"] + source["secondary"]
    entries = []
    for row in all_rows:
        symbol = row["source_collision_symbol"]
        if symbol not in FAMILIES:
            continue
        family, candidate, blocker = FAMILIES[symbol]
        entries.append({
            "metatile_id": row["metatile_id"],
            "bank": row["bank"],
            "source_symbol": symbol,
            "family": family,
            "candidate_emerald_behavior": candidate,
            "required_review": blocker,
            "eligible_for_live_registration": False,
        })
    counts = dict(sorted(Counter(row["source_symbol"] for row in entries).items()))
    assert entries and set(counts) == set(FAMILIES)
    assert all(not row["eligible_for_live_registration"] for row in entries)
    return {
        "status": "wall_water_semantics_audited_not_live",
        "symbol_counts": counts,
        "reviewed_metatiles": len(entries),
        "wall_water": entries,
        "unreviewed_metatiles": 1012 - len(entries),
        "source_symbols": sorted(FAMILIES),
        "water_candidate": "MB_OCEAN_WATER",
        "candidate_is_verified_equivalent": False,
        "produced_metatile_attributes": False,
        "registered": False,
    }
def main():
    p = argparse.ArgumentParser()
    p.add_argument("--output", required=True, type=Path)
    args = p.parse_args()
    report = audit()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + "\n")
    print("PASS: Johto wall/water source symbols classified with explicit engine blockers")
    print("STAGING ONLY: surf, buoy, impassable walls, elevation and whirlpool unresolved")
if __name__ == "__main__":
    main()
