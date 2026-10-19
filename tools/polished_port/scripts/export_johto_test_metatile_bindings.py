#!/usr/bin/env python3
"""Bind only the Johto test layout's referenced metatiles to source candidates.

This writes a compact review-only list of (original global metatile ID,
candidate attribute word). It is NOT an Emerald tileset attribute table:
unreferenced source records remain unresolved, and the 12 absent secondary
slots are never fabricated.
"""
import hashlib
import json
import struct
from pathlib import Path

from export_johto_basic_walkability_layout import build as build_walkability
from export_johto_sparse_attributes import build as build_attributes, UNRESOLVED
from project_johto_collision_symbols import convert as source_collisions

BEHAVIORS = {"FLOOR": 0, "TALL_GRASS": 2}
BANKS = (("primary", 0, 512), ("secondary", 512, 1012))


def export(output: Path, map_data: bytes, border_data: bytes,
           primary_metatiles: bytes, secondary_metatiles: bytes):
    output.mkdir(parents=True, exist_ok=True)
    assert len(map_data) == 128
    assert len(border_data) == 8
    assert len(primary_metatiles) == 512 * 16
    assert len(secondary_metatiles) == 500 * 16

    map_words = struct.unpack("<64H", map_data)
    border_words = struct.unpack("<4H", border_data)
    source_cells = build_walkability()
    assert map_words == tuple(cell["packed_cell"] for cell in source_cells)
    assert all((word & 0x0C00) == 0 and (word >> 12) == 3
               for word in map_words + border_words)

    attributes, valid, _ = build_attributes()
    source = source_collisions()
    rows = source["primary"] + source["secondary"]
    assert len(attributes) == len(rows) == 1012
    assert len(valid) == 127

    floor = source_cells[0]
    assert floor["source_symbol"] == "FLOOR"
    assert all(word == floor["packed_cell"] for word in border_words)

    used = sorted({word & 0x03FF for word in map_words + border_words})
    bindings = []
    for metatile_id in used:
        assert 0 <= metatile_id < 1012, "Do not invent absent secondary slots"
        row = rows[metatile_id]
        assert row["metatile_id"] == metatile_id
        symbol = row["source_collision_symbol"]
        assert symbol in BEHAVIORS, "Unreviewed source behavior cannot be bound"
        candidate = attributes[metatile_id]
        assert candidate != UNRESOLVED
        assert candidate == BEHAVIORS[symbol]
        assert bool(valid[metatile_id // 8] & (1 << (metatile_id % 8)))
        bank, start, end = next(item for item in BANKS
                                if item[1] <= metatile_id < item[2])
        assert row["bank"] == bank
        bank_index = metatile_id - start
        geometry_bank = primary_metatiles if bank == "primary" else secondary_metatiles
        tile_geometry = geometry_bank[bank_index * 16:(bank_index + 1) * 16]
        assert len(tile_geometry) == 16
        bindings.append({
            "metatile_id": metatile_id,
            "bank": bank,
            "bank_local_index": bank_index,
            "source_collision_symbol": symbol,
            "candidate_attribute_word": candidate,
            "source_geometry_sha256": hashlib.sha256(tile_geometry).hexdigest(),
            "map_cell_uses": sum((word & 0x03FF) == metatile_id for word in map_words),
            "border_cell_uses": sum((word & 0x03FF) == metatile_id for word in border_words),
        })
    assert sum(row["map_cell_uses"] for row in bindings) == 64
    assert sum(row["border_cell_uses"] for row in bindings) == 4
    assert all(row["candidate_attribute_word"] in (0, 2) for row in bindings)

    # Small audit fixture; NOT a 512/512 Emerald attribute table.
    raw = b"".join(struct.pack("<HH", row["metatile_id"],
                               row["candidate_attribute_word"]) for row in bindings)
    (output / "johto_test_attribute_bindings.review.bin").write_bytes(raw)
    report = {
        "status": "source_backed_test_only_NOT_FOR_ROM",
        "binding_count": len(bindings),
        "map_cells_checked": 64,
        "border_cells_checked": 4,
        "binary_bytes": len(raw),
        "binary_sha256": hashlib.sha256(raw).hexdigest(),
        "original_metatile_ids_preserved": True,
        "source_geometry_bytes_verified": True,
        "no_unresolved_attribute_fallback": True,
        "complete_primary_attribute_table": False,
        "complete_secondary_attribute_table": False,
        "live_registration_allowed": False,
        "bindings": bindings,
    }
    (output / "johto_test_attribute_bindings.review.json").write_text(
        json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return report
