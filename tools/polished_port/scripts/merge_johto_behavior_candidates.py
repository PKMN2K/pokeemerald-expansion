#!/usr/bin/env python3
"""Merge source-backed Johto terrain candidates into a fail-closed review.

No unverified candidate becomes a registered Emerald attribute. Water
and ledges have explicit field-engine dependencies; walls, special
objects, warps and signage remain unmapped.
"""
import argparse
import json
from pathlib import Path
from audit_johto_metatile_attributes import audit
from audit_johto_ledge_water_fields import LEDGES, WATER

def merge():
    base = audit()
    entries = base["primary"] + base["secondary"]
    special = {**LEDGES, **WATER}
    touched = 0
    for row in entries:
        source = row["source_collision_symbol"]
        if source in special:
            assert row["candidate_behavior"] is None
            row["candidate_behavior"] = special[source]
            row["candidate_attribute_word"] = None
            row["requires_field_validation"] = True
            touched += 1
        else:
            row["requires_field_validation"] = source not in ("FLOOR", "TALL_GRASS")
        assert row["registration_ready"] is False
    unresolved = sum(row["candidate_attribute_word"] is None for row in entries)
    assert touched > 0 and unresolved > 0
    assert len(entries) == 1012
    return {
        "status": "extended_behavior_candidates_unregistered",
        "primary": entries[:512], "secondary": entries[512:],
        "directional_water_candidates": touched,
        "unresolved_attribute_words": unresolved,
        "live_attribute_binary_generated": False,
        "map_registered": False,
        "scripted_interactions_validated": False,
    }

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--output", required=True, type=Path)
    args = p.parse_args()
    report = merge()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + "\n")
    print("PASS: source-derived ledge/water candidates merged without inventing live attributes")
    print("BLOCKED: field validation and special interactions are unresolved")
if __name__ == "__main__":
    main()
