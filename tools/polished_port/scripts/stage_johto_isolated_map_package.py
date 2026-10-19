#!/usr/bin/env python3
"""Stage an Emerald-shaped Johto test-map package outside live registration.

Outputs may be reviewed or imported manually after remaining blockers are
resolved. This command never writes data/maps, data/layouts or map_groups.json.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from prepare_johto_layout_registration import prepare, ROOT, ID, NAME
from export_johto_tileset_bundle import export as export_tileset

MAP_ID = "MAP_POLISHED_JOHTO_WALKABILITY_TEST"
MAP_NAME = "PolishedJohto_WalkabilityTest"
MISSING_HEADER_FIELDS = ("music", "region", "region_map_section",
                         "weather", "map_type", "battle_scene")

def stage(output):
    output = output.resolve()
    repo = ROOT.resolve()
    if output == repo or repo in output.parents:
        raise ValueError("Offline map package output must be outside repository")
    reports = output / "reviews"
    report = prepare(reports)
    layouts = json.loads((ROOT / "data/layouts/layouts.json").read_text(encoding="utf-8"))["layouts"]
    reference_layout = next(x for x in layouts if x["id"] == "LAYOUT_ROUTE101")
    proposed_layout = report["layout_proposal"]
    assert set(proposed_layout) == set(reference_layout), "Not a native Emerald layout record"
    assert proposed_layout["layout_version"] == "emerald"
    assert proposed_layout["id"] == ID and proposed_layout["name"] == NAME
    assert not any(x["id"] == ID or x["name"] == NAME for x in layouts)

    groups = json.loads((ROOT / "data/maps/map_groups.json").read_text(encoding="utf-8"))
    assert not any(MAP_NAME in values for values in groups.values() if isinstance(values, list))
    reference_map = json.loads((ROOT / "data/maps/Route101/map.json").read_text(encoding="utf-8"))
    header = {
        "id": MAP_ID, "name": MAP_NAME, "layout": ID,
        "music": None, "region": None, "region_map_section": None,
        "requires_flash": False, "weather": None, "map_type": None,
        "allow_cycling": False, "allow_escaping": False,
        "allow_running": True, "show_map_name": False,
        "battle_scene": None, "connections": [], "object_events": [],
        "warp_events": [], "coord_events": [], "bg_events": [],
    }
    assert set(header) == set(reference_map), "Map-header draft does not match native schema"
    assert all(header[field] is None for field in MISSING_HEADER_FIELDS)
    assert not any(header[k] for k in (
        "connections", "object_events", "warp_events", "coord_events", "bg_events"
    ))

    root = output / "OFFLINE_ONLY" / "data" / "layouts" / MAP_NAME
    root.mkdir(parents=True, exist_ok=True)
    map_raw = (reports / "johto_basic_walkability_layout.bin").read_bytes()
    border_raw = (reports / "johto_basic_border_candidate.bin").read_bytes()
    assert len(map_raw) == 8 * 8 * 2 and len(border_raw) == 2 * 2 * 2
    assert struct.unpack("<64H", map_raw) == tuple(
        cell["packed_cell"] for cell in json.loads(
            (reports / "johto_basic_walkability_layout.json").read_text()
        )["cells"]
    )
    assert struct.unpack("<4H", border_raw) == tuple(
        json.loads((reports / "johto_basic_border_candidate.json").read_text())["packed_cells"]
    )
    (root / "map.bin").write_bytes(map_raw)
    (root / "border.bin").write_bytes(border_raw)

    # Pinned real-source graphics and palettes are kept as another offline folder.
    assets = output / "OFFLINE_ONLY" / "johto_source_tileset"
    export_tileset(assets)
    (output / "OFFLINE_ONLY" / "layout_entry.review.json").write_text(
        json.dumps(proposed_layout, indent=2) + "\n", encoding="utf-8")
    (output / "OFFLINE_ONLY" / "map_header.INCOMPLETE.review.json").write_text(
        json.dumps(header, indent=2) + "\n", encoding="utf-8")

    manifest = {
        "status": "offline_package_NOT_FOR_ROM",
        "source_layout_version": "emerald",
        "map_id": MAP_ID, "layout_id": ID,
        "map_sha256": hashlib.sha256(map_raw).hexdigest(),
        "border_sha256": hashlib.sha256(border_raw).hexdigest(),
        "map_bytes": len(map_raw), "border_bytes": len(border_raw),
        "tileset_source_bundle_staged": True,
        "map_header_draft_schema_checked": True,
        "unresolved_header_fields": list(MISSING_HEADER_FIELDS),
        "no_warp_or_access_route": True,
        "full_tileset_attributes_ready": False,
        "visual_and_runtime_collision_verified": False,
        "map_registered": False,
        "tilesets_registered": False,
        "safe_to_enable": False,
    }
    (output / "OFFLINE_ONLY" / "package_manifest.json").write_text(
        json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    return manifest

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, required=True)
    manifest = stage(parser.parse_args().output)
    print(f"PASS: {manifest['map_bytes']}-byte map and {manifest['border_bytes']}-byte border staged")
    print("OFFLINE ONLY: map header unresolved, no map/tileset registration or emulator proof")
