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
from export_johto_basic_border_candidate import export as export_border
from audit_johto_border_graphics import audit as audit_border_graphics
from audit_johto_test_cell_encoding import audit as audit_cell_encoding

ROOT = Path(__file__).resolve().parents[3]
LAYOUTS = ROOT / "data/layouts/layouts.json"
ID = "LAYOUT_POLISHED_JOHTO_WALKABILITY_TEST"
NAME = "PolishedJohto_WalkabilityTest_Layout"

def prepare(out):
    out.mkdir(parents=True, exist_ok=True)
    walk = export_layout(out)
    cell_audit = audit_cell_encoding(out)
    assert cell_audit["cells_checked"] == 64
    assert cell_audit["source_cell_binary_round_trip_checked"]
    assert not cell_audit["ready_for_live_registration"]
    border = export_border(out)
    graphics = audit_border_graphics(out)
    assert graphics["metatile_id"] == border["metatile_id"]
    assert graphics["geometry_checked"] and not graphics["border_art_verified"]
    assert border["width"] == border["height"] == 2
    assert border["bytes"] == 8 and len(border["packed_cells"]) == 4
    assert border["source_backed"] and not border["registered"]
    current = json.loads(LAYOUTS.read_text(encoding="utf-8"))
    assert isinstance(current.get("layouts"), list)
    existing = current["layouts"]
    assert not any(x.get("id") == ID or x.get("name") == NAME for x in existing)
    attrs, mask, _ = build_attributes()
    assert len(attrs) == 1012 and len(mask) == 127
    cells = walk["cells"]
    assert walk["width"] == walk["height"] == 8
    assert len(cells) == 64 and walk["bytes"] == 128
    assert [(cell["x"], cell["y"]) for cell in cells] == [
        (x, y) for y in range(8) for x in range(8)
    ]
    used = sorted({cell["metatile_id"] for cell in cells})
    assert used and all(0 <= i < len(attrs) and i < 1024 for i in used)
    coverage = all(
        attrs[cell["metatile_id"]] != 0xFFFF
        and bool(mask[cell["metatile_id"] // 8] & (1 << (cell["metatile_id"] % 8)))
        and attrs[cell["metatile_id"]] == cell["candidate_attribute_word"]
        and cell["packed_cell"] == (cell["metatile_id"] | (3 << 12))
        and cell["source_symbol"] in ("FLOOR", "TALL_GRASS")
        for cell in cells
    )
    assert coverage, "Test map contains unsupported or mismatched metatile attributes"
    proposal = {
        "id": ID, "name": NAME, "width": 8, "height": 8,
        "primary_tileset": "gTileset_PolishedJohtoOutdoorPrimary",
        "secondary_tileset": "gTileset_PolishedJohtoOutdoorSecondary",
        "layout_version": "emerald",
        "border_filepath": "data/layouts/PolishedJohto_WalkabilityTest/border.bin",
        "blockdata_filepath": "data/layouts/PolishedJohto_WalkabilityTest/map.bin",
    }
    required = {"id", "name", "width", "height",
                "primary_tileset", "secondary_tileset",
                "border_filepath", "blockdata_filepath", "layout_version"}
    assert required <= proposal.keys()
    report = {
        "status": "proposed_layout_not_registered",
        "layout_proposal": proposal,
        "currently_registered": False,
        "existing_layouts_modified": False,
        "referenced_test_metatile_ids": used,
        "all_test_cells_have_basic_candidate_attributes": coverage,
        "complete_tileset_attribute_tables": False,
        "movement_verified_in_emulator": False,
        "map_header_and_warp_defined": False,
        "border_art_verified": False,
        "offline_border_candidate_generated": True,
        "test_cell_binary_encoding_checked": cell_audit["source_cell_binary_round_trip_checked"],
        "border_candidate_metatile_id": border["metatile_id"],
        "border_source_geometry_checked": graphics["geometry_checked"],
        "border_runtime_priority_verified": graphics["runtime_priority_verified"],
        "border_converted_4bpp_pixels_checked": graphics["converted_4bpp_pixels_checked"],
        "emulator_test_readiness": {
            "source_geometry_checked": graphics["geometry_checked"],
            "source_pixels_checked": graphics["converted_4bpp_pixels_checked"],
            "candidate_collision_coverage": coverage,
            "test_cell_binary_encoding_checked": cell_audit["source_cell_binary_round_trip_checked"],
            "complete_tileset_attributes": False,
            "visual_border_verified": False,
            "object_layer_priority_verified": False,
            "map_header_and_warp_defined": False,
            "ready_for_live_map_registration": False,
        },
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
