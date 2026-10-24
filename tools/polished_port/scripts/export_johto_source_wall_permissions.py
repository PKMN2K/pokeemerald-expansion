#!/usr/bin/env python3
"""Source-pinned Crystal WALL_TILE permission mask candidates, OFFLINE ONLY.

Distinguish ordinary WALL, VOID, HEADBUTT_TREE and BUOY from water-permission
WHIRLPOOL and land-permission TOP_WALL/BOTTOM_WALL. Do not convert map-cell
collision bit masks to Emerald behavior attributes or claim field interactions.
"""
import hashlib
import json
import re
import struct
from collections import Counter
from pathlib import Path
from export_johto_sparse_attributes import build as sparse, UNRESOLVED
from project_johto_collision_symbols import convert
from export_johto_wall_mapgrid_rules import COLLISION_MASK, verify_native_movement

ROOT = Path(__file__).resolve().parents[3]
SOURCE_PERMISSIONS = {
    "VOID": "WALL_TILE", "WALL": "WALL_TILE",
    "HEADBUTT_TREE": "WALL_TILE", "BUOY": "WALL_TILE",
    "WHIRLPOOL": "WATER_TILE", "TOP_WALL": "LAND_TILE",
    "BOTTOM_WALL": "LAND_TILE",
}
EXPECTED = {
    "WALL": 481, "HEADBUTT_TREE": 58, "BUOY": 26, "VOID": 4,
    "WHIRLPOOL": 1, "TOP_WALL": 4, "BOTTOM_WALL": 1,
}
WALL_PERMISSION = {s for s, permission in SOURCE_PERMISSIONS.items()
                   if permission == "WALL_TILE"}


def validate_source_permissions():
    fixture = (
        ROOT / "tools/polished_port/fixtures/johto_source_wall_permissions.asm"
    ).read_text(encoding="utf-8")
    assert "3fa43192379df5c3e7b09a08e4d5d79af4f02f42" in fixture
    matches = re.findall(
        r"(?m)^\s*db\s+(LAND_TILE|WATER_TILE|WALL_TILE)\s*;\s*COLL_([A-Z0-9_]+)\s*$",
        fixture,
    )
    found = {}
    for permission, symbol in matches:
        assert symbol not in found
        found[symbol] = permission
    assert found == SOURCE_PERMISSIONS
    assert WALL_PERMISSION == {"WALL", "HEADBUTT_TREE", "BUOY", "VOID"}
    return found


def export(output: Path, coverage: dict, prior_wall: dict, extended: dict):
    output.mkdir(parents=True, exist_ok=True)
    source_permissions = validate_source_permissions()
    engine_sources = verify_native_movement()
    source = convert()
    rows = source["primary"] + source["secondary"]
    original, bitmap, _ = sparse()
    assert len(rows) == len(coverage["metatiles"]) == len(original) == 1012
    assert len(bitmap) == 127
    frequencies = Counter(row["source_collision_symbol"] for row in rows)
    assert {sym: frequencies[sym] for sym in EXPECTED} == EXPECTED
    assert prior_wall["source_wall_metatiles"] == 481
    assert extended["candidate_metatiles"] == 465
    assert extended["new_headbutt_tree_candidates"] == 58
    assert extended["unresolved_wall_metatiles"] == 481
    assert extended["unresolved_metatiles"] == 547
    assert not coverage["source_metatile_attribute_table_emitted"]

    pairs = []
    report_rows = []
    for row, layer in zip(rows, coverage["metatiles"]):
        symbol = row["source_collision_symbol"]
        if symbol not in WALL_PERMISSION:
            continue
        mid = row["metatile_id"]
        assert layer["metatile_id"] == mid and layer["source_collision_symbol"] == symbol
        assert layer["source_geometry_sha256"]
        assert layer["source_derived_layer_type"] == "METATILE_LAYER_TYPE_NORMAL"
        assert layer["candidate_behavior_word"] is None
        assert layer["full_attribute_word"] is None
        assert original[mid] == UNRESOLVED
        assert not (bitmap[mid // 8] & (1 << (mid % 8)))
        assert source_permissions[symbol] == "WALL_TILE"
        pairs.append((mid, COLLISION_MASK))
        report_rows.append({
            "metatile_id": mid, "source_symbol": symbol,
            "source_block": row["source_block"],
            "source_quadrant": row["quadrant"],
            "source_geometry_sha256": layer["source_geometry_sha256"],
            "source_priority_subtiles": layer["source_priority_subtiles"],
            "source_collision_permission": "WALL_TILE",
            "candidate_mapgrid_collision_mask": COLLISION_MASK,
            "preserve_existing_elevation": True,
            "requires_field_interaction": symbol == "HEADBUTT_TREE",
            "requires_water_object_placement_review": symbol == "BUOY",
            "requires_map_border_review": symbol == "VOID",
            "no_behavior_attribute_assigned": True,
            "emulator_verified": False,
        })
    assert pairs == sorted(pairs)
    assert len(pairs) == sum(EXPECTED[s] for s in WALL_PERMISSION) == 569
    assert len({mid for mid, _ in pairs}) == 569
    old = [(r["metatile_id"], r["candidate_mapcell_collision_mask"])
           for r in prior_wall["walls"]]
    assert [(r["metatile_id"], r["candidate_mapgrid_collision_mask"])
            for r in report_rows if r["source_symbol"] == "WALL"] == old
    assert all((mask & 0x0C00) == 0x0C00 for _, mask in pairs)
    assert not any(r["source_symbol"] in ("WHIRLPOOL", "TOP_WALL", "BOTTOM_WALL")
                   for r in report_rows)
    raw = b"".join(struct.pack("<HH", mid, mask) for mid, mask in pairs)
    assert len(raw) == 569 * 4
    (output / "johto_source_wall_permissions.review.bin").write_bytes(raw)
    report = {
        "status": "SOURCE_PINNED_WALL_PERMISSIONS_NOT_FOR_ROM",
        "source": source["source"],
        "source_permission_fixture_verified": True,
        "native_collision_engine_sources_verified": engine_sources,
        "source_permissions": source_permissions,
        "counts": dict(EXPECTED),
        "source_wall_permission_count": 569,
        "existing_wall_subset_count": 481,
        "new_wall_permission_quadrants": 88,
        "headbutt_interaction_pending": True,
        "buoy_and_void_placement_pending": True,
        "whirlpool_is_water_not_wall": True,
        "directional_wall_edges_are_land_not_wall": True,
        "binary_bytes": len(raw),
        "binary_sha256": hashlib.sha256(raw).hexdigest(),
        "source_geometry_hashes_verified": True,
        "complete_attribute_table_emitted": False,
        "mapgrid_map_emitted": False,
        "source_interaction_script_ported": False,
        "live_tileset_registered": False,
        "live_map_registered": False,
        "safe_to_register": False,
        "rules": report_rows,
    }
    (output / "johto_source_wall_permissions.review.json").write_text(
        json.dumps(report, indent=2) + "\n", encoding="utf-8"
    )
    return report
