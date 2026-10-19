#!/usr/bin/env python3
"""Review actual source BG1 priority and GBA pixels for six obstacle fixtures.

GBC bit7 is per 8x8 tile reference, not per 16x16 collision metatile.
Test native TL/TR/BL/BR indexing, flips, palettes, converted BG2 pixels
and color-zero-masked BG1 pixels. No emulator OBJ occlusion is inferred.
"""
import hashlib
import json
import re
import struct
from pathlib import Path

from stage_johto_tile_graphics import (
    load_source_metatile_tables, priority_overlay_tile_ids,
    gba_tile_id, mask_source_zero_for_priority,
)

ROOT = Path(__file__).resolve().parents[3]
SOURCE_PRIORITY = 0x80
SOURCE_BANK = 0x08
SOURCE_HFLIP = 0x20
SOURCE_VFLIP = 0x40
SOURCE_SUBTILES_PER_BLOCK = 16
EMERALD_SUBTILES_PER_METATILE = 8
EMERALD_BANK_PRIMARY = 512
SOURCE_TOTAL = 1012
OBSTACLE_SYMBOLS = {"WALL", "VOID", "TOP_WALL", "BOTTOM_WALL", "BUOY", "WHIRLPOOL"}


def expected_entry(tile, attr):
    entry = gba_tile_id(tile, int(bool(attr & SOURCE_BANK)))
    entry |= (attr & 7) << 12
    if attr & SOURCE_HFLIP:
        entry |= 1 << 10
    if attr & SOURCE_VFLIP:
        entry |= 1 << 11
    return entry


def inspect(output: Path, obstacle_report: dict, obstacle_map: bytes,
            primary_metatiles: bytes, secondary_metatiles: bytes,
            primary_graphics: bytes):
    output.mkdir(parents=True, exist_ok=True)
    header = (ROOT / "include/global.fieldmap.h").read_text(encoding="utf-8")
    assert re.search(r"(?m)^\s*METATILE_LAYER_TYPE_NORMAL,\s*// Metatile uses middle and top bg layers", header)
    assert len(obstacle_map) == 128
    assert len(primary_metatiles) == 512 * 16
    assert len(secondary_metatiles) == 500 * 16
    assert len(primary_graphics) == 512 * 32
    assert hashlib.sha256(obstacle_map).hexdigest() == obstacle_report["review_map_sha256"]
    cells = struct.unpack("<64H", obstacle_map)
    source_tiles, source_attrs = load_source_metatile_tables()
    assert len(source_tiles) == len(source_attrs) == 253 * SOURCE_SUBTILES_PER_BLOCK
    priority = priority_overlay_tile_ids(source_tiles, source_attrs)
    assignments = obstacle_report["assignments"]
    assert {a["source_symbol"] for a in assignments} == OBSTACLE_SYMBOLS
    assert len(assignments) == len(OBSTACLE_SYMBOLS)
    details = []
    for a in assignments:
        x, y = a["x"], a["y"]
        mid = a["metatile_id"]
        assert 0 <= mid < SOURCE_TOTAL
        assert cells[x + y * 8] == a["packed_cell"]
        assert ((a["packed_cell"] >> 10) & 3) == 3
        assert ((a["packed_cell"] >> 12) & 15) == a["elevation"] == 3
        block, quadrant = divmod(mid, 4)
        start_y, start_x = (quadrant // 2) * 2, (quadrant % 2) * 2
        bank = "primary" if mid < EMERALD_BANK_PRIMARY else "secondary"
        assert a["bank"] == bank
        local = mid if bank == "primary" else mid - EMERALD_BANK_PRIMARY
        assert a["bank_local_index"] == local
        meta = primary_metatiles if bank == "primary" else secondary_metatiles
        data = meta[local * 16:(local + 1) * 16]
        assert len(data) == 16
        assert hashlib.sha256(data).hexdigest() == a["source_geometry_sha256"]
        base = struct.unpack("<4H", data[:8])
        foreground = struct.unpack("<4H", data[8:])
        source_entries = []
        for cy in range(2):
            for cx in range(2):
                index = block * 16 + (start_y + cy) * 4 + start_x + cx
                source_tile = source_tiles[index]
                attr = source_attrs[index]
                expected = expected_entry(source_tile, attr)
                base_word = base[cy * 2 + cx]
                fg_word = foreground[cy * 2 + cx]
                assert base_word == expected
                assert ((base_word >> 12) & 15) == (attr & 7)
                ordinary_id = expected & 0x03FF
                src_gfx = primary_graphics[ordinary_id * 32:(ordinary_id + 1) * 32]
                assert len(src_gfx) == 32
                if attr & SOURCE_PRIORITY:
                    assert ordinary_id in priority
                    assert fg_word == (expected & ~0x03FF) | priority[ordinary_id]
                    fg_id = fg_word & 0x03FF
                    fg_gfx = primary_graphics[fg_id * 32:(fg_id + 1) * 32]
                    assert fg_gfx == mask_source_zero_for_priority(src_gfx)
                    transparent_nibbles = sum(
                        p == 4 for raw in src_gfx for p in (raw & 15, raw >> 4)
                    )
                    assert transparent_nibbles == sum(
                        p == 0 for raw in fg_gfx for p in (raw & 15, raw >> 4)
                    )
                else:
                    assert fg_word == 0, "Non-priority tile must have transparent BG1"
                    transparent_nibbles = 0
                source_entries.append({
                    "source_subtile_index": index,
                    "source_priority_bit_set": bool(attr & SOURCE_PRIORITY),
                    "source_palette": attr & 7,
                    "source_horizontal_flip": bool(attr & SOURCE_HFLIP),
                    "source_vertical_flip": bool(attr & SOURCE_VFLIP),
                    "converted_base_word": base_word,
                    "converted_foreground_word": fg_word,
                    "foreground_color_zero_pixels_masked": transparent_nibbles,
                })
        details.append({
            "source_symbol": a["source_symbol"], "metatile_id": mid,
            "source_block": block,
            "source_quadrant": ("top_left", "top_right", "bottom_left", "bottom_right")[quadrant],
            "source_geometry_sha256": hashlib.sha256(data).hexdigest(),
            "priority_subtile_count": sum(x["source_priority_bit_set"] for x in source_entries),
            "foreground_masked_color_zero_pixels": sum(
                x["foreground_color_zero_pixels_masked"] for x in source_entries
            ),
            "candidate_collision_bits": 3,
            "candidate_elevation": 3,
            "required_emerald_layer_type": "METATILE_LAYER_TYPE_NORMAL",
            "subtiles": source_entries,
            "obj_occlusion_verified_in_emulator": False,
        })
    assert len(details) == len(assignments) == 6
    assert all(len(d["subtiles"]) == 4 for d in details)
    result = {
        "status": "OBSTACLE_FOREGROUND_PRIORITY_SOURCE_REVIEW_ONLY",
        "source": "Rangi42/polishedcrystal@3fa43192379df5c3e7b09a08e4d5d79af4f02f42",
        "obstacle_map_sha256": hashlib.sha256(obstacle_map).hexdigest(),
        "obstacle_metatiles_checked": len(details),
        "source_8x8_subtiles_checked": len(details) * 4,
        "total_priority_subtiles": sum(d["priority_subtile_count"] for d in details),
        "source_bit7_priority_references_matched": True,
        "base_bg2_geometry_and_flips_matched": True,
        "masked_bg1_graphics_matched": True,
        "candidate_metatile_layer": "METATILE_LAYER_TYPE_NORMAL",
        "metatile_attribute_words_complete": False,
        "sprite_elevation_parity_verified_in_emulator": False,
        "registered_with_engine": False,
        "safe_to_enable": False,
        "obstacles": details,
    }
    (output / "johto_obstacle_foreground_priority.review.json").write_text(
        json.dumps(result, indent=2) + "\n", encoding="utf-8"
    )
    return result
