#!/usr/bin/env python3
"""Audit source-derived Emerald layer fields for all 1,012 Johto metatiles.

The authentic 32x32 Polished Crystal blocks yield 1,012 16x16 GBA entries.
Our BG2 base / BG1 source-priority-mask conversion requires the NORMAL layer
type. Audit EVERY source 8x8 reference, palette, flip flag, BG1 entry and
masked graphics pixel before recording that field as a candidate.

This does NOT invent missing metatile BEHAVIOR attributes. Only FLOOR and
TALL_GRASS keep their existing 0/2 candidate words. Do not emit a 16-bit
tileset attribute array: encoding missing behaviors as zeros would silently
make walls and special tiles walkable.
"""
import hashlib
import json
import re
import struct
from collections import Counter
from pathlib import Path

from audit_johto_obstacle_foreground_priority import expected_entry
from export_johto_sparse_attributes import build as build_sparse, UNRESOLVED
from project_johto_collision_symbols import convert as collisions
from stage_johto_tile_graphics import (
    load_source_metatile_tables, priority_overlay_tile_ids,
    mask_source_zero_for_priority,
)

ROOT = Path(__file__).resolve().parents[3]
TOTAL = 1012
PRIMARY = 512
SECONDARY = 500
NORMAL_LAYER = "METATILE_LAYER_TYPE_NORMAL"


def source_subtile_indices(mid):
    block, quadrant = divmod(mid, 4)
    yy = (quadrant // 2) * 2
    xx = (quadrant % 2) * 2
    return tuple(block * 16 + (yy + dy) * 4 + xx + dx
                 for dy in range(2) for dx in range(2))


def audit(output: Path, primary_metatiles: bytes, secondary_metatiles: bytes,
          primary_graphics: bytes):
    output.mkdir(parents=True, exist_ok=True)
    header = (ROOT / "include/global.fieldmap.h").read_text(encoding="utf-8")
    assert re.search(r"enum\s*\{\s*METATILE_LAYER_TYPE_NORMAL\s*,", header)
    assert re.search(r"(?m)^\s*#define\s+METATILE_ATTR_LAYER_MASK\s+0xF000\b", header)
    assert re.search(r"(?m)^\s*#define\s+METATILE_ATTR_LAYER_SHIFT\s+12\b", header)
    assert len(primary_metatiles) == PRIMARY * 16
    assert len(secondary_metatiles) == SECONDARY * 16
    assert len(primary_graphics) == 512 * 32

    src_tiles, src_attrs = load_source_metatile_tables()
    assert len(src_tiles) == len(src_attrs) == 253 * 16
    overlay_ids = priority_overlay_tile_ids(src_tiles, src_attrs)
    assert len(overlay_ids) == 12

    source_doc = collisions()
    records = source_doc["primary"] + source_doc["secondary"]
    candidates, mask, known = build_sparse()
    assert len(records) == len(candidates) == TOTAL and len(mask) == 127
    assert 0 < known < TOTAL

    reviewed = []
    flags_total = 0
    unresolved_by_symbol = Counter()
    for mid, record in enumerate(records):
        assert record["metatile_id"] == mid
        bank = "primary" if mid < PRIMARY else "secondary"
        local = mid if bank == "primary" else mid - PRIMARY
        assert record["bank"] == bank and record["bank_local_index"] == local
        geometry = primary_metatiles if bank == "primary" else secondary_metatiles
        raw = geometry[local * 16:(local + 1) * 16]
        assert len(raw) == 16
        bg2 = struct.unpack("<4H", raw[:8])
        bg1 = struct.unpack("<4H", raw[8:])
        flag_count = 0
        palettes = set()
        for index, source_index in enumerate(source_subtile_indices(mid)):
            attr = src_attrs[source_index]
            assert not (attr & 0x10), "GBC attribute reserved bit set"
            expected = expected_entry(src_tiles[source_index], attr)
            assert bg2[index] == expected, f"BG2 source mismatch on metatile {mid}"
            palettes.add(attr & 7)
            assert (attr & 7) <= 6
            if attr & 0x80:
                flag_count += 1
                normal_id = expected & 0x03FF
                assert normal_id in overlay_ids
                masked_id = overlay_ids[normal_id]
                assert bg1[index] == ((expected & ~0x03FF) | masked_id)
                ordinary = primary_graphics[normal_id * 32:(normal_id + 1) * 32]
                masked = primary_graphics[masked_id * 32:(masked_id + 1) * 32]
                assert len(ordinary) == len(masked) == 32
                assert masked == mask_source_zero_for_priority(ordinary)
            else:
                assert bg1[index] == 0, f"Unflagged BG1 entry on metatile {mid}"
        flags_total += flag_count
        candidate = candidates[mid]
        has_behavior = candidate != UNRESOLVED
        assert has_behavior == bool(mask[mid // 8] & (1 << (mid % 8)))
        if has_behavior:
            assert (record["source_collision_symbol"], candidate) in (
                ("FLOOR", 0), ("TALL_GRASS", 2)
            )
        else:
            unresolved_by_symbol[record["source_collision_symbol"]] += 1
        reviewed.append({
            "metatile_id": mid,
            "bank": bank,
            "bank_local_index": local,
            "source_collision_symbol": record["source_collision_symbol"],
            "source_geometry_sha256": hashlib.sha256(raw).hexdigest(),
            "source_priority_subtiles": flag_count,
            "global_palette_slots": sorted(palettes),
            "source_derived_layer_type": NORMAL_LAYER,
            "source_derived_layer_bits": 0,
            "candidate_behavior_word": candidate if has_behavior else None,
            "behavior_requires_further_mapping": not has_behavior,
            "full_attribute_word": candidate if has_behavior else None,
            "verified_in_emulator": False,
        })
    assert flags_total == 38, "Authentic source priority count changed"
    assert sum(row["candidate_behavior_word"] is not None for row in reviewed) == known
    assert sum(row["full_attribute_word"] is None for row in reviewed) == TOTAL - known
    assert len(reviewed) == TOTAL
    assert len(reviewed[:PRIMARY]) == PRIMARY and len(reviewed[PRIMARY:]) == SECONDARY
    result = {
        "status": "SOURCE_LAYER_FIELDS_REVIEW_ONLY_BEHAVIORS_INCOMPLETE",
        "source": "Rangi42/polishedcrystal@3fa43192379df5c3e7b09a08e4d5d79af4f02f42",
        "source_metatiles_checked": TOTAL,
        "source_8x8_references_checked": TOTAL * 4,
        "primary_source_entries": PRIMARY,
        "secondary_source_entries": SECONDARY,
        "absent_secondary_source_ids": list(range(TOTAL, 1024)),
        "normal_layer_candidates": TOTAL,
        "original_priority_8x8_references_checked": flags_total,
        "original_masked_priority_graphics_checked": True,
        "source_bg2_palette_flip_bits_checked": True,
        "known_behavior_word_candidates": known,
        "unresolved_behavior_words": TOTAL - known,
        "unresolved_source_symbols": dict(sorted(unresolved_by_symbol.items())),
        "source_metatile_attribute_table_emitted": False,
        "complete_primary_behavior_attribute_table": False,
        "complete_secondary_behavior_attribute_table": False,
        "map_or_tileset_registered": False,
        "sprite_elevation_verified_in_emulator": False,
        "ready_for_live_map_registration": False,
        "metatiles": reviewed,
    }
    (output / "johto_full_layer_attribute_coverage.review.json").write_text(
        json.dumps(result, indent=2) + "\n", encoding="utf-8"
    )
    return result
