#!/usr/bin/env python3
"""Prepare a schema-matched *draft* layout entry for the isolated Johto test.

This emits a standalone proposal, not data/layouts/layouts.json. Incomplete
attribute tables and unverified field interactions block live registration.
"""
import argparse
import json
from pathlib import Path
from export_johto_basic_walkability_layout import export as export_layout
from export_johto_sparse_attributes import build as build_attributes

ROOT = Path(__file__).resolve().parents[3]
LAYOUTS = ROOT / "data/layouts/layouts.json"
ID = "LAYOUT_POLISHED_JOHTO_WALKABILITY_TEST"
NAME = "PolishedJohto_WalkabilityTest_Layout"

def prepare(out):
    out.mkdir(parents=True, exist_ok=True)
    walk = export_layout(out)
    current = json.loads(LAYOUTS.read_text(encoding="utf-8"))
    assert isinstance(current.get("layouts"), list)
    existing = current["layouts"]
    assert not any(x.get("id") == ID or x.get("name") == NAME for x in existing)
    attrs, mask, _ = build_attributes()
    assert len(attrs) == 1012 and len(mask) == 127
    used = sorted({x["metatile_id"] for x in walk["cells"]})
    assert used and all(attrs[i] != 0xFFFF for i in used)
    proposal = {
        "id": ID, "name": NAME, "width": 8, "height": 8,
        "border_width": 2, "border_height": 2,
        "primary_tileset": "gTileset_PolishedJohtoOutdoorPrimary",
        "secondary_tileset": "gTileset_PolishedJohtoOutdoorSecondary",
        "border_filepath": "data/layouts/PolishedJohto_WalkabilityTest/border.bin",
        "blockdata_filepath": "data/layouts/PolishedJohto_WalkabilityTest/map.bin",
    }
    required = {"id", "name", "width", "height", "border_width",
                "border_height", "primary_tileset", "secondary_tileset",
                "border_filepath", "blockdata_filepath"}
    assert required <= proposal.keys()
    report = {
        "status": "proposed_layout_not_registered",
        "layout_proposal": proposal,
        "currently_registered": False,
        "existing_layouts_modified": False,
        "referenced_test_metatile_ids": used,
        "all_test_cells_have_basic_candidate_attributes": True,
        "complete_tileset_attribute_tables": False,
        "movement_verified_in_emulator": False,
        "map_header_and_warp_defined": False,
        "border_art_verified": False,
    }
    (out / "johto_layout_registration_proposal.json").write_text(
        json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return report

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, required=True)
    result = prepare(parser.parse_args().output)
    print("PASS: proposed isolated Johto layout matches expected schema")
    print("NOT REGISTERED: complete attributes, border, map header/warp and field testing pending")
