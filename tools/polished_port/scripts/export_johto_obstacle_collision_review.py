#!/usr/bin/env python3
"""Stage a separate Johto obstacle-collision map-grid review fixture.

Source symbols and graphics come from the pinned Polished Crystal blocks;
0x0C00 comes from Emerald's MAPGRID_IMPASSABLE encoding. This cannot
substitute for unmapped metatile attributes or emulator movement tests.
The original floor/grass walkability map is never modified.
"""
import hashlib
import json
import re
import struct
from pathlib import Path

from export_johto_basic_walkability_layout import build as walkability
from export_johto_sparse_attributes import build as attributes, UNRESOLVED
from project_johto_collision_symbols import convert as source_collisions

ROOT = Path(__file__).resolve().parents[3]
SYMBOLS = ("WALL", "VOID", "TOP_WALL", "BOTTOM_WALL", "BUOY", "WHIRLPOOL")
WIDTH = HEIGHT = 8
ELEVATION = 3
COLLISION = 3


def verify_engine_encoding():
    header = (ROOT / "include/global.fieldmap.h").read_text(encoding="utf-8")
    fieldmap = (ROOT / "src/fieldmap.c").read_text(encoding="utf-8")
    expected = {
        "MAPGRID_METATILE_ID_MASK": "0x03FF",
        "MAPGRID_COLLISION_MASK": "0x0C00",
        "MAPGRID_ELEVATION_MASK": "0xF000",
        "MAPGRID_COLLISION_SHIFT": "10",
        "MAPGRID_ELEVATION_SHIFT": "12",
        "MAPGRID_IMPASSABLE": "MAPGRID_COLLISION_MASK",
    }
    for key, value in expected.items():
        assert re.search(r"(?m)^\s*#define\s+" + key + r"\s+" + value + r"\b", header), key
    assert "return UNPACK_COLLISION(block);" in fieldmap
    assert "gBackupMapLayout.map[x + gBackupMapLayout.width * y] |= MAPGRID_COLLISION_MASK;" in fieldmap
    return "include/global.fieldmap.h and src/fieldmap.c"


def export(output, base_map: bytes, primary_metatiles: bytes,
           secondary_metatiles: bytes):
    output.mkdir(parents=True, exist_ok=True)
    engine_provenance = verify_engine_encoding()
    assert len(base_map) == WIDTH * HEIGHT * 2
    assert len(primary_metatiles) == 512 * 16
    assert len(secondary_metatiles) == 500 * 16
    base_words = struct.unpack("<64H", base_map)
    source_cells = walkability()
    assert base_words == tuple(row["packed_cell"] for row in source_cells)
    assert all(((word >> 10) & 3) == 0 and (word >> 12) == ELEVATION
               for word in base_words)

    source = source_collisions()
    records = source["primary"] + source["secondary"]
    candidate_attrs, bitmap, _ = attributes()
    assert len(records) == len(candidate_attrs) == 1012 and len(bitmap) == 127
    assert len({row["metatile_id"] for row in records}) == 1012

    changed = list(base_words)
    assignments = []
    for i, symbol in enumerate(SYMBOLS):
        x, y = 2 + i, 1
        at = x + y * WIDTH
        original = source_cells[at]
        assert original["source_symbol"] == "FLOOR"
        matches = [row for row in records if row["source_collision_symbol"] == symbol]
        assert matches, f"No authentic source metatile for {symbol}"
        row = matches[0]
        metatile = row["metatile_id"]
        assert 0 <= metatile < 1012
        assert candidate_attrs[metatile] == UNRESOLVED
        assert not (bitmap[metatile // 8] & (1 << (metatile % 8)))
        bank = row["bank"]
        local = row["bank_local_index"]
        assert (bank == "primary" and local == metatile) or (
            bank == "secondary" and local == metatile - 512)
        geometry = primary_metatiles if bank == "primary" else secondary_metatiles
        shape = geometry[local * 16:(local + 1) * 16]
        assert len(shape) == 16
        word = metatile | (COLLISION << 10) | (ELEVATION << 12)
        assert (word & 0x03FF) == metatile
        assert (word & 0x0C00) == 0x0C00
        assert (word & 0xF000) == ELEVATION << 12
        changed[at] = word
        assignments.append({
            "x": x, "y": y, "source_symbol": symbol,
            "metatile_id": metatile, "bank": bank, "bank_local_index": local,
            "source_geometry_sha256": hashlib.sha256(shape).hexdigest(),
            "packed_cell": word, "collision": COLLISION, "elevation": ELEVATION,
            "candidate_attribute_word": None,
            "source_emerald_behavior_mapped": False,
            "sprite_priority_verified": False,
            "movement_verified_in_emulator": False,
        })

    assert sum(before != after for before, after in zip(base_words, changed)) == len(SYMBOLS)
    assert changed[1 + WIDTH] == base_words[1 + WIDTH], "Preserve proposed debug spawn"
    assert all(changed[i] == base_words[i] for i in range(64)
               if i not in {entry["x"] + WIDTH * entry["y"] for entry in assignments})
    raw = struct.pack("<64H", *changed)
    assert struct.unpack("<64H", raw) == tuple(changed)
    (output / "johto_obstacle_collision.review.bin").write_bytes(raw)
    report = {
        "status": "OFFLINE_OBSTACLE_MAPGRID_REVIEW_NOT_FOR_ROM",
        "map_width": WIDTH, "map_height": HEIGHT,
        "changed_cells": len(assignments),
        "source_symbols": list(SYMBOLS),
        "collision_bits_engine_verified": True,
        "engine_provenance": engine_provenance,
        "collision_value": COLLISION,
        "source_wall_and_obstacle_graphics_verified": True,
        "base_map_sha256": hashlib.sha256(base_map).hexdigest(),
        "review_map_sha256": hashlib.sha256(raw).hexdigest(),
        "review_map_bytes": len(raw),
        "remaining_source_attribute_words_unresolved": True,
        "all_untouched_map_cells_preserved": True,
        "debug_spawn_untouched": True,
        "registered_with_engine": False,
        "emulator_collision_verified": False,
        "safe_to_register": False,
        "assignments": assignments,
    }
    (output / "johto_obstacle_collision.review.json").write_text(
        json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return report
