#!/usr/bin/env python3
"""Export source-backed, sparse Johto field behavior candidate PAIRS only.

This advances the previous FLOOR/TALL_GRASS coverage by adding confirmed
generic WATER, 3 cardinal ledges and 2 two-direction corner ledges.
The binary contains (source_metatile_id, 16-bit candidate_attribute_word)
pairs with a separate bitmap; it is NOT a 512+512 table or ROM-ready data.

WALL, other obstacles, warps and scripted/special source symbols remain
unmapped. No behavior is invented or silently defaulted to MB_NORMAL.
"""
import hashlib
import json
import re
import struct
from collections import Counter
from pathlib import Path

from export_johto_sparse_attributes import build as base_sparse, UNRESOLVED
from project_johto_collision_symbols import convert
from review_johto_mapgrid_blockers import review as review_blockers, IMPASSABLE

ROOT = Path(__file__).resolve().parents[3]
BEHAVIOR_MAP = {
    "FLOOR": "MB_NORMAL",
    "TALL_GRASS": "MB_TALL_GRASS",
    "WATER": "MB_JOHTO_WATER",
    "LEDGE_DOWN": "MB_JUMP_SOUTH",
    "LEDGE_LEFT": "MB_JUMP_WEST",
    "LEDGE_RIGHT": "MB_JUMP_EAST",
    "LEDGE_DOWN_LEFT": "MB_JOHTO_LEDGE_DOWN_LEFT",
    "LEDGE_DOWN_RIGHT": "MB_JOHTO_LEDGE_DOWN_RIGHT",
}
BASIC = frozenset(("FLOOR", "TALL_GRASS"))
WATER = frozenset(("WATER",))
LEDGES = frozenset(k for k in BEHAVIOR_MAP if k.startswith("LEDGE_"))
EXPECTED_COUNTS = {
    "FLOOR": 323, "TALL_GRASS": 6,
    "WATER": 59, "LEDGE_DOWN": 7, "LEDGE_LEFT": 4,
    "LEDGE_RIGHT": 4, "LEDGE_DOWN_LEFT": 2, "LEDGE_DOWN_RIGHT": 2,
}
EXPECTED_CANDIDATES = 407
EXPECTED_WALL = 481
TOTAL = 1012


def behavior_enum_ids():
    header = (ROOT / "include/constants/metatile_behaviors.h").read_text(encoding="utf-8")
    enum = header.split("enum {", 1)[1].split("NUM_METATILE_BEHAVIORS", 1)[0]
    names = re.findall(r"(?m)^\s*(MB_[A-Z0-9_]+)\s*,", enum)
    assert len(names) == len(set(names)) and len(names) < 256
    ids = {name: i for i, name in enumerate(names)}
    assert ids["MB_NORMAL"] == 0 and ids["MB_TALL_GRASS"] == 2
    assert all(key in ids for key in BEHAVIOR_MAP.values())
    assert len(set(ids.values())) == len(ids)
    return ids


def export(output: Path, coverage: dict, primary: bytes, secondary: bytes):
    output.mkdir(parents=True, exist_ok=True)
    assert len(primary) == 512 * 16
    assert len(secondary) == 500 * 16
    assert not coverage["source_metatile_attribute_table_emitted"]
    ids = behavior_enum_ids()
    source = convert()
    records = source["primary"] + source["secondary"]
    assert len(records) == len(coverage["metatiles"]) == TOTAL
    original_words, original_valid, original_count = base_sparse()
    assert len(original_words) == TOTAL and len(original_valid) == 127
    assert original_count == 329

    blockers = review_blockers()
    blocked = blockers["primary"] + blockers["secondary"]
    assert len(blocked) == TOTAL

    symbol_counts = Counter(row["source_collision_symbol"] for row in records)
    assert {symbol: symbol_counts[symbol] for symbol in BEHAVIOR_MAP} == EXPECTED_COUNTS
    assert symbol_counts["WALL"] == EXPECTED_WALL

    pairs = []
    full_review = []
    unresolved = Counter()
    bitmap = bytearray(127)
    for mid, (src, layer, obstacle) in enumerate(zip(
        records, coverage["metatiles"], blocked
    )):
        symbol = src["source_collision_symbol"]
        assert src["metatile_id"] == layer["metatile_id"] == obstacle["metatile_id"] == mid
        assert obstacle["source_collision_symbol"] == symbol
        assert not obstacle["registration_ready"] and not obstacle["verified_in_game"]
        assert layer["source_collision_symbol"] == symbol
        assert layer["source_derived_layer_type"] == "METATILE_LAYER_TYPE_NORMAL"
        assert layer["source_derived_layer_bits"] == 0
        bank = "primary" if mid < 512 else "secondary"
        assert src["bank"] == layer["bank"] == bank
        local = mid if mid < 512 else mid - 512
        assert src["bank_local_index"] == layer["bank_local_index"] == local
        raw = (primary if mid < 512 else secondary)[local*16:(local+1)*16]
        assert len(raw) == 16
        assert hashlib.sha256(raw).hexdigest() == layer["source_geometry_sha256"]
        name = BEHAVIOR_MAP.get(symbol)
        candidate = ids[name] if name is not None else None
        original = original_words[mid]
        was_known = bool(original_valid[mid // 8] & (1 << (mid % 8)))
        assert was_known == (original != UNRESOLVED)
        if symbol in BASIC:
            assert was_known and original == candidate
            assert layer["candidate_behavior_word"] == candidate
            assert layer["full_attribute_word"] == candidate
        else:
            assert not was_known and original == UNRESOLVED
            assert layer["candidate_behavior_word"] is None
            assert layer["full_attribute_word"] is None

        if name is not None:
            assert 0 <= candidate < 256
            bitmap[mid // 8] |= 1 << (mid % 8)
            pairs.append((mid, candidate))
        else:
            unresolved[symbol] += 1
            assert candidate is None
        if symbol in IMPASSABLE:
            assert candidate is None, "Never flatten collision-only obstacles into floors"
        full_review.append({
            "metatile_id": mid,
            "source_collision_symbol": symbol,
            "bank": bank, "bank_local_index": local,
            "source_geometry_sha256": layer["source_geometry_sha256"],
            "source_priority_subtiles": layer["source_priority_subtiles"],
            "candidate_behavior": name,
            "candidate_attribute_word": candidate,
            "requires_mapcell_collision_validation": symbol in IMPASSABLE,
            "requires_water_elevation_and_field_validation": symbol in WATER,
            "requires_jump_and_elevation_validation": symbol in LEDGES,
            "requires_special_script_review": name is None and symbol not in IMPASSABLE,
            "registered_with_engine": False,
            "emulator_verified": False,
        })

    assert len(pairs) == sum(EXPECTED_COUNTS.values()) == EXPECTED_CANDIDATES
    assert len(full_review) == TOTAL
    assert len(unresolved) > 0 and sum(unresolved.values()) == TOTAL - EXPECTED_CANDIDATES
    assert unresolved["WALL"] == EXPECTED_WALL
    assert sum(x.bit_count() for x in bitmap) == EXPECTED_CANDIDATES
    assert len(bitmap) == 127
    assert all(bool(bitmap[i // 8] & (1 << (i % 8)))
               == (full_review[i]["candidate_attribute_word"] is not None)
               for i in range(TOTAL))
    assert all(0 <= mid < TOTAL and word != UNRESOLVED for mid, word in pairs)
    assert pairs == sorted(pairs)

    binary = b"".join(struct.pack("<HH", *pair) for pair in pairs)
    assert len(binary) == EXPECTED_CANDIDATES * 4
    assert len(set(pair[0] for pair in pairs)) == len(pairs)
    (output / "johto_extended_behavior_pairs.review.bin").write_bytes(binary)
    (output / "johto_extended_behavior_validity.review.bin").write_bytes(bitmap)
    report = {
        "status": "SOURCE_BACKED_EXTENDED_BEHAVIOR_PAIRS_NOT_FOR_ROM",
        "source": source["source"],
        "metatiles_checked": TOTAL,
        "source_geometry_checked": TOTAL,
        "candidate_metatiles": len(pairs),
        "candidate_primary": sum(mid < 512 for mid, _ in pairs),
        "candidate_secondary": sum(mid >= 512 for mid, _ in pairs),
        "existing_basic_candidates_retained": original_count,
        "new_water_candidates": symbol_counts["WATER"],
        "new_ledge_candidates": sum(symbol_counts[s] for s in LEDGES),
        "unresolved_metatiles": sum(unresolved.values()),
        "unresolved_wall_metatiles": unresolved["WALL"],
        "unresolved_by_symbol": dict(sorted(unresolved.items())),
        "behavior_names": BEHAVIOR_MAP,
        "behavior_ids": {key: ids[val] for key, val in BEHAVIOR_MAP.items()},
        "pairs_binary_bytes": len(binary),
        "pairs_binary_sha256": hashlib.sha256(binary).hexdigest(),
        "validity_bitmap_bytes": len(bitmap),
        "validity_bitmap_sha256": hashlib.sha256(bitmap).hexdigest(),
        "no_attribute_fallbacks_added": True,
        "complete_primary_attribute_table_emitted": False,
        "complete_secondary_attribute_table_emitted": False,
        "all_metatile_attribute_words_resolved": False,
        "full_rom_tileset_emitted": False,
        "live_map_registered": False,
        "emulator_field_semantics_verified": False,
        "safe_to_register": False,
        "pairs": [{"metatile_id": mid, "candidate_attribute_word": word}
                  for mid, word in pairs],
        "per_metatile_review": full_review,
    }
    (output / "johto_extended_behavior_pairs.review.json").write_text(
        json.dumps(report, indent=2) + "\n", encoding="utf-8"
    )
    return report
