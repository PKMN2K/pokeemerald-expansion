#!/usr/bin/env python3
"""Classify source Johto ledges and water for an offline Emerald field audit.

Emerald ledge jumps require directional metatile behavior plus map collision;
water requires surf behavior, elevation and correct shore transitions.
This deliberately emits no live attribute table or map registration.
"""
import argparse
import json
import re
from collections import Counter
from pathlib import Path
from project_johto_collision_symbols import convert

ROOT = Path(__file__).resolve().parents[3]
BEHAVIORS = ROOT / "include/constants/metatile_behaviors.h"
LEDGES = {
    "LEDGE_DOWN": "MB_JUMP_SOUTH",
    "LEDGE_LEFT": "MB_JUMP_WEST",
    "LEDGE_RIGHT": "MB_JUMP_EAST",
    "LEDGE_DOWN_LEFT": "MB_JUMP_SOUTHWEST",
    "LEDGE_DOWN_RIGHT": "MB_JUMP_SOUTHEAST",
}
WATER = {"WATER": "MB_JOHTO_WATER"}
OBSTACLES = {"BUOY", "WHIRLPOOL"}
def classify():
    available = set(re.findall(r"^\s*(MB_[A-Z0-9_]+)\s*,", BEHAVIORS.read_text(), re.M))
    assert set(LEDGES.values()) | set(WATER.values()) <= available
    all_rows = convert()["primary"] + convert()["secondary"]
    affected = []
    for record in all_rows:
        symbol = record["source_collision_symbol"]
        if symbol not in LEDGES and symbol not in WATER and symbol not in OBSTACLES:
            continue
        category = "ledge" if symbol in LEDGES else ("water" if symbol in WATER else "water_obstacle")
        affected.append({
            "metatile_id": record["metatile_id"],
            "source_symbol": symbol,
            "category": category,
            "candidate_behavior": (LEDGES | WATER).get(symbol),
            "requires_map_collision_review": True,
            "requires_elevation_review": True,
            "live_verified": False,
        })
    counts = dict(sorted(Counter(x["source_symbol"] for x in affected).items()))
    assert len(affected) > 0
    assert set(counts) == set(LEDGES) | set(WATER) | OBSTACLES
    assert all(x["metatile_id"] < 1012 and not x["live_verified"] for x in affected)
    return {
        "status": "ledge_water_field_audit_only",
        "original_metatile_count": len(all_rows),
        "affected_metatiles": len(affected),
        "counts": counts,
        "ledge_candidates": LEDGES,
        "water_candidates": WATER,
        "obstacle_symbols": sorted(OBSTACLES),
        "records": affected,
        "attributes_generated": False,
        "live_layout_registered": False,
        "surf_and_jump_verified": False,
    }
def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    report = classify()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + "\n")
    print(f"PASS: {report['affected_metatiles']} source ledge/water quadrants reviewed")
    print("STAGING ONLY: jump direction, surfing and elevation require in-game validation")
if __name__ == "__main__":
    main()
