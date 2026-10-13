#!/usr/bin/env python3
"""Build a *review-only* Emerald behavior/physical-collision draft.

No live metatile binary is generated: Emerald behavior and collision occupy
different bit fields, and special events need explicit map wiring.
Unresolved quadrants are represented by null, never silently MB_NORMAL.
"""
import argparse
import json
from pathlib import Path

from stage_johto_metatile_collision import stage

# A deliberately narrow subset with unambiguous source interpretation.
SAFE_BEHAVIOR = {"FLOOR": "MB_NORMAL", "TALL_GRASS": "MB_TALL_GRASS"}
SAFE_PASSABILITY = {"FLOOR": "passable", "WALL": "impassable", "VOID": "impassable"}


def draft(inventory):
    staged = stage(inventory)
    result = []
    known = 0
    for block in staged["blocks"]:
        quadrants = []
        for item in block["quadrants"]:
            symbol = item["source"]
            behavior = SAFE_BEHAVIOR.get(symbol)
            passability = SAFE_PASSABILITY.get(symbol)
            if behavior is not None and passability is not None:
                known += 1
            quadrants.append({
                "quadrant": item["quadrant"], "source_symbol": symbol,
                "behavior_candidate": behavior,
                "passability_candidate": passability,
                "ready_for_live_attribute": False,
                "needs_review": behavior is None or passability is None,
            })
        result.append({"source_block": block["source_block"], "quadrants": quadrants})
    return {
        "status": "draft_only_no_binary_output",
        "source_block_count": len(result),
        "quadrants_with_both_candidates": known,
        "quadrants": len(result) * 4,
        "source_missing_blocks": [253, 254, 255],
        "blocks": result,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--inventory", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    output = draft(json.loads(args.inventory.read_text(encoding="utf-8")))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(output, indent=2) + "\n", encoding="utf-8")
    print(f"Drafted {output['quadrants']} quadrants; "
          f"{output['quadrants_with_both_candidates']} have both conservative candidates.")
    print("No live binary emitted.")


if __name__ == "__main__":
    main()
