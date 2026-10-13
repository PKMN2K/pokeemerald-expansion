#!/usr/bin/env python3
"""Check a staged Johto Traditional collision inventory against pinned source facts.

Source: Rangi42/polishedcrystal at 3fa43192379df5c3e7b09a08e4d5d79af4f02f42,
data/tilesets/johto_traditional_collision.asm.
These counts are a source fingerprint, not converted Emerald behaviors.
"""
import argparse
from collections import Counter
import json
from pathlib import Path

EXPECTED = {
    "VOID": 4, "FLOOR": 323, "TALL_GRASS": 6, "WALL": 481,
    "LADDER": 4, "WHIRLPOOL": 1, "BUOY": 26, "WATER": 59,
    "WARP_CARPET_LEFT": 3, "WARP_CARPET_DOWN": 2,
    "WARP_CARPET_RIGHT": 2, "DOOR": 16, "MART_SIGN": 1,
    "POKECENTER_SIGN": 1, "HEADBUTT_TREE": 58, "LEDGE_DOWN": 7,
    "LEDGE_LEFT": 4, "LEDGE_RIGHT": 4, "LEDGE_DOWN_LEFT": 2,
    "LEDGE_DOWN_RIGHT": 2, "TOP_WALL": 4, "BOTTOM_WALL": 1,
    "CAVE": 1,
}
QUADRANTS = ("top_left", "top_right", "bottom_left", "bottom_right")


def validate(inventory):
    blocks = inventory["blocks"]
    if len(blocks) != 253 or inventory["block_count"] != 253:
        raise ValueError("expected 253 source blocks")
    if inventory["missing_blocks_in_256_capacity"] != [253, 254, 255]:
        raise ValueError("unexpected unused positions")
    seen = Counter()
    for index, block in enumerate(blocks):
        if block["block"] != index or set(block["quadrants"]) != set(QUADRANTS):
            raise ValueError(f"incorrect quadrant layout in block {index}")
        seen.update(block["quadrants"].values())
    if seen != Counter(EXPECTED):
        details = {k: {"expected": EXPECTED.get(k, 0), "actual": seen.get(k, 0)}
                   for k in sorted(set(EXPECTED) | set(seen))
                   if EXPECTED.get(k, 0) != seen.get(k, 0)}
        raise ValueError(f"source collision symbol counts mismatch: {details}")
    assert sum(seen.values()) == 253 * 4
    return seen


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--inventory", type=Path, required=True)
    args = parser.parse_args()
    counts = validate(json.loads(args.inventory.read_text(encoding="utf-8")))
    print(f"PASS: {sum(counts.values())} quadrants, {len(counts)} source symbols verified")


if __name__ == "__main__":
    main()
