#!/usr/bin/env python3
"""Review conservative, exact Emerald behavior candidates for Johto collision.

Only behavior-identical basic terrain is proposed. No production metatile
attributes are generated: source solidity (WALL/VOID), water access, ledges,
warps, sign interactions, and elevations need explicit engine validation.
"""
import argparse
import json
import re
from collections import Counter
from pathlib import Path
from project_johto_collision_symbols import convert

ROOT = Path(__file__).resolve().parents[3]
EMERALD_BEHAVIORS = ROOT / "include/constants/metatile_behaviors.h"
SAFE_CANDIDATES = {
    "FLOOR": "MB_NORMAL",
    "TALL_GRASS": "MB_TALL_GRASS",
}
# The source uses four quadrant-specific 16x16 collision declarations.
# Do NOT silently map impassable source tiles to MB_NORMAL, since Emerald
# map collision also depends on per-map elevation and metatile attributes.
def review():
    doc = convert()
    definitions = set(re.findall(r"^\s*(MB_[A-Z0-9_]+)\s*,", EMERALD_BEHAVIORS.read_text(), re.M))
    assert set(SAFE_CANDIDATES.values()).issubset(definitions)
    records = doc["primary"] + doc["secondary"]
    mapped = []
    unresolved = []
    for row in records:
        symbol = row["source_collision_symbol"]
        behavior = SAFE_CANDIDATES.get(symbol)
        result = {
            "metatile_id": row["metatile_id"],
            "source_symbol": symbol,
            "candidate_emerald_behavior": behavior,
            "confirmed_live_equivalent": False,
        }
        (mapped if behavior is not None else unresolved).append(result)
    assert len(records) == len(mapped) + len(unresolved) == 1012
    assert unresolved  # deliberately fail closed until full conversion is verified
    return {
        "status": "basic_behavior_candidates_only_not_live",
        "candidate_map": SAFE_CANDIDATES,
        "candidate_metatile_count": len(mapped),
        "unresolved_metatile_count": len(unresolved),
        "unresolved_symbols": dict(sorted(Counter(row["source_symbol"] for row in unresolved).items())),
        "candidates": mapped,
        "unresolved": unresolved,
        "produced_metatile_attributes": False,
        "registered": False,
        "requires_solidity_review": True,
        "requires_elevation_review": True,
        "requires_scripted_interaction_review": True,
    }
def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    report = review()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + "\n")
    print(f"PASS: {report['candidate_metatile_count']} basic-terrain candidates checked against Emerald constants")
    print(f"REVIEW: {report['unresolved_metatile_count']} metatiles require further collision/interaction mapping")
    print("STAGING ONLY: no live attributes or map registration")
if __name__ == "__main__":
    main()
