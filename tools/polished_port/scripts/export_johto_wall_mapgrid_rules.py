#!/usr/bin/env python3
"""Source-verified map-cell collision rules for all Johto WALL quadrants.

The Emerald engine stores impassability in each MAP GRID CELL (bits 10-11),
not in the 16-bit metatile attribute. Every WALL quadrant can therefore
be associated with an impassable-cell rule while its Emerald *behavior* and
sprite elevation remain unresolved.

The review binary is an ID -> collision bit mask audit, NOT a tileset
attribute table. No live maps are patched or registered.
"""
import hashlib
import json
import re
import struct
from pathlib import Path

from project_johto_collision_symbols import convert
from export_johto_sparse_attributes import build as sparse, UNRESOLVED

ROOT = Path(__file__).resolve().parents[3]
COLLISION_MASK = 0x0C00
ELEVATION_MASK = 0xF000
METATILE_MASK = 0x03FF
WALL_SYMBOL = "WALL"
EXPECTED_WALL_QUADRANTS = 481


def verify_native_movement():
    header = (ROOT / "include/global.fieldmap.h").read_text(encoding="utf-8")
    fieldmap = (ROOT / "src/fieldmap.c").read_text(encoding="utf-8")
    movement = (ROOT / "src/event_object_movement.c").read_text(encoding="utf-8")
    constants = {
        "MAPGRID_METATILE_ID_MASK": "0x03FF",
        "MAPGRID_COLLISION_MASK": "0x0C00",
        "MAPGRID_ELEVATION_MASK": "0xF000",
        "MAPGRID_COLLISION_SHIFT": "10",
        "MAPGRID_ELEVATION_SHIFT": "12",
        "MAPGRID_IMPASSABLE": "MAPGRID_COLLISION_MASK",
    }
    for key, value in constants.items():
        assert re.search(r"(?m)^\s*#define\s+" + key + r"\s+" + value + r"\b", header), key
    assert "return UNPACK_COLLISION(block);" in fieldmap
    assert ("gBackupMapLayout.map[x + gBackupMapLayout.width * y] "
            "|= MAPGRID_COLLISION_MASK;") in fieldmap
    start = movement.index("static enum Collision GetVanillaCollision(")
    stop = movement.index("\n}", start)
    function = movement[start:stop]
    assert re.search(
        r"else\s+if\s*\(\s*MapGridGetCollisionAt\(x,\s*y\)\s*\|\|", function
    ), "Native movement must gate map-cell collision before behavior checks"
    assert "return COLLISION_IMPASSABLE;" in function
    assert function.index("MapGridGetCollisionAt(x, y)") < function.index(
        "IsMetatileDirectionallyImpassable("
    )
    return [
        "include/global.fieldmap.h",
        "src/fieldmap.c",
        "src/event_object_movement.c:GetVanillaCollision",
    ]


def combine_with_existing_cell(existing_word: int, source_metatile_id: int) -> int:
    """Replace original cell ID, preserve its height, set collision to 3."""
    assert 0 <= existing_word <= 65535
    assert 0 <= source_metatile_id < 1012
    result = (existing_word & ELEVATION_MASK) | source_metatile_id | COLLISION_MASK
    assert (result & ELEVATION_MASK) == (existing_word & ELEVATION_MASK)
    assert (result & METATILE_MASK) == source_metatile_id
    assert (result & COLLISION_MASK) == COLLISION_MASK
    return result


def export(output: Path, layer_coverage: dict):
    output.mkdir(parents=True, exist_ok=True)
    provenance = verify_native_movement()
    source = convert()
    all_rows = source["primary"] + source["secondary"]
    sparse_words, bitmap, _ = sparse()
    audited = layer_coverage["metatiles"]
    assert len(all_rows) == len(audited) == len(sparse_words) == 1012
    assert len(bitmap) == 127
    assert layer_coverage["normal_layer_candidates"] == 1012
    assert not layer_coverage["source_metatile_attribute_table_emitted"]

    rows = []
    for mid, source_row in enumerate(all_rows):
        assert source_row["metatile_id"] == mid
        if source_row["source_collision_symbol"] != WALL_SYMBOL:
            continue
        layer = audited[mid]
        assert layer["metatile_id"] == mid
        assert layer["source_collision_symbol"] == WALL_SYMBOL
        assert layer["source_derived_layer_type"] == "METATILE_LAYER_TYPE_NORMAL"
        assert layer["candidate_behavior_word"] is None
        assert layer["full_attribute_word"] is None
        assert layer["behavior_requires_further_mapping"] is True
        assert sparse_words[mid] == UNRESOLVED
        assert not bitmap[mid // 8] & (1 << (mid % 8))
        bank = "primary" if mid < 512 else "secondary"
        assert source_row["bank"] == layer["bank"] == bank
        local = mid if bank == "primary" else mid - 512
        assert source_row["bank_local_index"] == layer["bank_local_index"] == local
        examples = [combine_with_existing_cell(3 << 12, mid),
                    combine_with_existing_cell(1 << 12, mid)]
        assert examples[0] >> 12 == 3 and examples[1] >> 12 == 1
        assert examples[0] & METATILE_MASK == mid
        rows.append({
            "metatile_id": mid, "bank": bank, "bank_local_index": local,
            "source_block": source_row["source_block"],
            "source_quadrant": source_row["quadrant"],
            "source_collision_symbol": WALL_SYMBOL,
            "source_geometry_sha256": layer["source_geometry_sha256"],
            "source_priority_subtiles": layer["source_priority_subtiles"],
            "candidate_mapcell_collision_mask": COLLISION_MASK,
            "example_land_cell_elevation_3": examples[0],
            "example_surf_cell_elevation_1": examples[1],
            "preserves_existing_mapcell_elevation": True,
            "candidate_emerald_layer": "METATILE_LAYER_TYPE_NORMAL",
            "candidate_emerald_behavior": None,
            "candidate_full_metatile_attribute_word": None,
            "field_movement_verified_in_emulator": False,
            "source_placement_elevation_verified": False,
        })
    assert len(rows) == EXPECTED_WALL_QUADRANTS
    assert [row["metatile_id"] for row in rows] == sorted(row["metatile_id"] for row in rows)
    assert any(row["metatile_id"] < 512 for row in rows)
    assert any(row["metatile_id"] >= 512 for row in rows)
    priority_count = sum(row["source_priority_subtiles"] > 0 for row in rows)
    assert priority_count == 3, "Original pinned wall foreground inventory changed"

    # Strictly NOT a map file or a tileset attribute file. This encodes
    # source global metatile ID and required map-cell collision bits only.
    raw = b"".join(struct.pack("<HH", row["metatile_id"],
                               row["candidate_mapcell_collision_mask"])
                   for row in rows)
    assert len(raw) == EXPECTED_WALL_QUADRANTS * 4
    (output / "johto_wall_collision_rules.review.bin").write_bytes(raw)
    report = {
        "status": "WALL_MAPGRID_COLLISION_RULES_NOT_FOR_ROM",
        "source": source["source"],
        "engine_sources_verified": provenance,
        "source_wall_metatiles": len(rows),
        "source_wall_primary": sum(r["bank"] == "primary" for r in rows),
        "source_wall_secondary": sum(r["bank"] == "secondary" for r in rows),
        "source_priority_wall_metatiles": priority_count,
        "review_pairs": len(rows),
        "review_binary_bytes": len(raw),
        "review_binary_sha256": hashlib.sha256(raw).hexdigest(),
        "mapcell_collision_mask": COLLISION_MASK,
        "mapcell_elevation_preserved_by_rule": True,
        "movement_gate_checked_against_native_engine": True,
        "wall_behavior_words_still_unresolved": len(rows),
        "wall_attribute_fallbacks_added": 0,
        "complete_tileset_attributes_exported": False,
        "live_map_registered": False,
        "wall_sprite_occlusion_verified_in_emulator": False,
        "safe_for_live_registration": False,
        "walls": rows,
    }
    (output / "johto_wall_collision_rules.review.json").write_text(
        json.dumps(report, indent=2) + "\n", encoding="utf-8"
    )
    return report
