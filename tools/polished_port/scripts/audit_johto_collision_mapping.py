#!/usr/bin/env python3
"""Audit Johto Traditional collision symbols against Emerald behavior names.

Input: JSON created by extract_johto_collision.py.
Output: review-only mapping report. Unresolved symbols are never assigned
MB_NORMAL automatically; partial source blocks remain unregistered.

Usage:
  python3 tools/polished_port/scripts/audit_johto_collision_mapping.py \
      --inventory /tmp/johto_collision.json --output /tmp/johto_mapping.json
"""
import argparse
import json
from collections import Counter
from pathlib import Path

# Only unambiguous, direct gameplay-category equivalents are listed here.
# Entries still require quadrant-by-quadrant in-game review.
CANDIDATES = {
    "FLOOR": "MB_NORMAL",
    "TALL_GRASS": "MB_TALL_GRASS",
    "LONG_GRASS": "MB_LONG_GRASS",
    "WATER": "MB_POND_WATER",
    "WATERFALL": "MB_WATERFALL",
    "ICE": "MB_ICE",
    "PUDDLE": "MB_PUDDLE",
    "LADDER": "MB_LADDER",
    "COUNTER": "MB_COUNTER",
    "PC": "MB_PC",
    "TV": "MB_TELEVISION",
    "POKECENTER_SIGN": "MB_POKEMON_CENTER_SIGN",
    "MART_SIGN": "MB_POKEMART_SIGN",
    "CURRENT_RIGHT": "MB_EASTWARD_CURRENT",
    "CURRENT_LEFT": "MB_WESTWARD_CURRENT",
    "CURRENT_UP": "MB_NORTHWARD_CURRENT",
    "CURRENT_DOWN": "MB_SOUTHWARD_CURRENT",
    "SPIN_RIGHT": "MB_SPIN_RIGHT",
    "SPIN_LEFT": "MB_SPIN_LEFT",
    "SPIN_UP": "MB_SPIN_UP",
    "SPIN_DOWN": "MB_SPIN_DOWN",
    "STOP_SPIN": "MB_STOP_SPINNING",
}

# No mapping for WALL / VOID because Emerald's collision layer is
# independent from the behavior enum; no mapping for DOOR or warp carpets
# until actual map warp events and directional entry are configured.

# Explicit physical movement classification; separate from metatile behaviors.
# These are review classifications, not final Emerald collision bitfields.
IMPASSABLE = {"WALL", "VOID", "HEADBUTT_TREE", "BUOY"}
EVENT_REQUIRED = {
    "DOOR", "CAVE", "WARP_CARPET_DOWN", "WARP_CARPET_UP",
    "WARP_CARPET_LEFT", "WARP_CARPET_RIGHT", "LADDER",
    "POKECENTER_SIGN", "MART_SIGN",
}
# Deliberately do not assign a passability default to unrecognized values.


def classify_physical(symbol):
    if symbol in IMPASSABLE:
        return "blocked"
    if symbol in EVENT_REQUIRED:
        return "requires_map_event_review"
    if symbol == "FLOOR":
        return "walkable_candidate"
    return "unresolved"


def audit(inventory):
    counts = Counter(symbol for block in inventory["blocks"]
                     for symbol in block["quadrants"].values())
    return {
        "status": "review_only_not_for_live_tileset",
        "block_count": len(inventory["blocks"]),
        "source_missing_blocks": inventory["missing_blocks_in_256_capacity"],
        "candidates": {symbol: {"emerald": CANDIDATES[symbol], "occurrences": count}
                       for symbol, count in sorted(counts.items()) if symbol in CANDIDATES},
        "physical": {symbol: {"classification": classify_physical(symbol), "occurrences": count}\n                     for symbol, count in sorted(counts.items())},\n        "unresolved": {symbol: count for symbol, count in sorted(counts.items())
                       if symbol not in CANDIDATES},
        "notes": [
            "Emerald metatile behavior and physical impassability require separate fields.",
            "Water categories require surf/depth and encounter review before acceptance.",
            "Signs, doors and directional warps require test-map script and event checks.",
            "No default behavior is emitted for unmapped symbolic collision values."
        ],
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--inventory", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    inventory = json.loads(args.inventory.read_text(encoding="utf-8"))
    result = audit(inventory)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(f"Reviewed {result['block_count']} source blocks; "
          f"{len(result['candidates'])} candidate types; "
          f"{len(result['unresolved'])} types unresolved.")
    print("Review-only: no live metatile attributes emitted.")


if __name__ == "__main__":
    main()
