#!/usr/bin/env python3
"""Emit a fail-closed Johto Emerald metatile-attribute REVIEW, never a live binary.

Attributes are 16-bit in Emerald: low 8 bits behavior, high 4 bits
layer (NORMAL=0). Only FLOOR and TALL_GRASS have basic candidate behaviors.
All other symbols remain unassigned; no fallback MB_NORMAL is permitted.
"""
import argparse
import json
import re
from pathlib import Path
from project_johto_collision_symbols import convert
from review_johto_emerald_behaviors import EMERALD_BEHAVIORS, SAFE_CANDIDATES

def audit():
    header = EMERALD_BEHAVIORS.read_text(encoding="utf-8")
    members = re.search(r"enum\s*\{(.*?)NUM_METATILE_BEHAVIORS", header, re.S).group(1)
    names = re.findall(r"^\s*(MB_[A-Z0-9_]+)\s*,", members, re.M)
    ids = {name: index for index, name in enumerate(names)}
    assert ids["MB_NORMAL"] == 0 and ids["MB_TALL_GRASS"] == 2
    assert all(ids[name] < 256 for name in SAFE_CANDIDATES.values())
    data = convert()
    entries = []
    for row in data["primary"] + data["secondary"]:
        symbol = row["source_collision_symbol"]
        candidate = SAFE_CANDIDATES.get(symbol)
        entries.append({
            "metatile_id": row["metatile_id"],
            "source_collision_symbol": symbol,
            "candidate_behavior": candidate,
            "candidate_attribute_word": ids[candidate] if candidate is not None else None,
            "layer": "METATILE_LAYER_TYPE_NORMAL",
            "registration_ready": False,
        })
    assert len(entries) == 1012
    assert any(x["candidate_attribute_word"] is None for x in entries)
    return {
        "status": "partial_metatile_attribute_review_not_registered",
        "primary": entries[:512],
        "secondary": entries[512:],
        "unknown_entries": sum(x["candidate_attribute_word"] is None for x in entries),
        "binary_exported": False,
        "map_registered": False,
        "sprite_priority_verified": False,
    }

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    report = audit()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print("PASS: 1012 source metatiles reviewed; unknown attributes remain unassigned")
    print("NOT LIVE: no binary or tileset registration")
if __name__ == "__main__":
    main()
