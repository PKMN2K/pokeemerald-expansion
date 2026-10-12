#!/usr/bin/env python3
"""Produce review-only, quadrant-aligned Emerald collision planning data.

Consumes the source-faithful JSON inventory from extract_johto_collision.py.
NEVER writes live metatile attributes: physical collision in Emerald is a
separate concern from behavioral metatile IDs, and a source block can contain
four different collision symbols.
"""
import argparse
import json
from pathlib import Path

from audit_johto_collision_mapping import CANDIDATES, classify_physical

QUADRANTS = ("top_left", "top_right", "bottom_left", "bottom_right")


def stage(inventory):
    blocks = inventory["blocks"]
    if inventory["block_count"] != len(blocks):
        raise ValueError("inventory block_count disagrees with blocks")
    if len(blocks) != 253:
        raise ValueError("expected 253 authentic Johto Traditional source blocks")
    if inventory["missing_blocks_in_256_capacity"] != [253, 254, 255]:
        raise ValueError("unexpected missing block indices")
    result = []
    unresolved = 0
    for index, block in enumerate(blocks):
        if block["block"] != index or set(block["quadrants"]) != set(QUADRANTS):
            raise ValueError(f"invalid block ordering/quadrants at {index}")
        quarters = []
        for quadrant in QUADRANTS:
            symbol = block["quadrants"][quadrant]
            physical = classify_physical(symbol)
            behavior = CANDIDATES.get(symbol)
            review = physical != "walkable_candidate" or behavior is None
            unresolved += int(review)
            quarters.append({"quadrant": quadrant, "source": symbol,
                             "physical": physical, "emerald_behavior_candidate": behavior,
                             "requires_review": review})
        result.append({"source_block": index, "quadrants": quarters})
    return {"status": "review_only_not_for_live_tileset",
            "source_block_count": len(result), "source_missing_blocks": [253, 254, 255],
            "quadrants_requiring_review": unresolved, "blocks": result}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--inventory", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    result = stage(json.loads(args.inventory.read_text(encoding="utf-8")))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(f"Staged {result['source_block_count']} blocks; "
          f"{result['quadrants_requiring_review']} quadrants require review.")
    print("No live Emerald metatile attribute output produced.")


if __name__ == "__main__":
    main()
