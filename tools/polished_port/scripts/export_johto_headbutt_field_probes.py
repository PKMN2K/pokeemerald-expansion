#!/usr/bin/env python3
"""Source-backed, unregistered Johto Headbutt-facing map-grid probes.

Recognizes all 58 authentic tree quadrants and preserves blocking collision.
The Emerald field-move dispatch, animation, tree encounters and item rewards
are explicitly NOT implemented by this staging script.
"""
import hashlib
import json
import re
import struct
from pathlib import Path
from project_johto_collision_symbols import convert
from export_johto_basic_walkability_layout import build as walkability

ROOT = Path(__file__).resolve().parents[3]
LAND, SOLID, WIDTH = 3, 3, 8
TARGETS = (("north", (3, 2)), ("south", (3, 4)),
           ("east", (4, 3)), ("west", (2, 3)))


def source_and_engine_checks():
    fixture = (ROOT / "tools/polished_port/fixtures/johto_headbutt_field_flow.asm").read_text(
        encoding="utf-8")
    required = ("COLL_HEADBUTT_TREE", "TryTileCollisionEvent:",
                "farcall TryHeadbuttOW", "TryHeadbuttFromMenu:",
                "call GetFacingTileCoord", "lb de, HEADBUTT, -1",
                "call CheckPartyMove", "checkflag ENGINE_HEADBUTT_ACTIVE",
                "callasm ShakeHeadbuttTree", "callasm TreeMonEncounter",
                "TREEMON_NO_ENCOUNTER_SET", "callasm TreeItemEncounter",
                "TreeMonEncounter:", "ld hl, TreeMonMaps")
    assert "3fa43192379df5c3e7b09a08e4d5d79af4f02f42" in fixture
    assert all(required_item in fixture for required_item in required)
    constants = (ROOT / "include/constants/metatile_behaviors.h").read_text()
    header = (ROOT / "include/metatile_behavior.h").read_text()
    behavior = (ROOT / "src/metatile_behavior.c").read_text()
    movement = (ROOT / "src/event_object_movement.c").read_text()
    field_move = (ROOT / "src/field_move.c").read_text()
    assert re.search(r"(?m)^\s*MB_JOHTO_HEADBUTT_TREE\s*,", constants)
    assert "bool8 MetatileBehavior_IsJohtoHeadbuttTree(u8 metatileBehavior);" in header
    predicate = behavior.split("bool8 MetatileBehavior_IsJohtoHeadbuttTree(", 1)[1].split("\n}", 1)[0]
    assert "return metatileBehavior == MB_JOHTO_HEADBUTT_TREE;" in predicate
    assert re.search(r"(?m)^\s*\[MB_JOHTO_HEADBUTT_TREE\]\s*=\s*0,", behavior)
    assert "GetVanillaCollision(" in movement
    assert "MapGridGetCollisionAt(x, y)" in movement
    assert "FIELD_MOVE_HEADBUTT" not in field_move
    return {
        "source_a_button_facing_tree_check": True,
        "source_party_has_headbutt_move_gate": True,
        "source_tree_encounters_and_rewards_require_separate_port": True,
        "emerald_tree_behavior_recognizer_compiled": True,
        "emerald_headbutt_field_move_not_yet_present": True,
        "ordinary_tile_encounters_not_inferred": True,
    }


def export(output: Path, coverage: dict, extended: dict, wall_rules: dict,
           original_map: bytes, primary: bytes, secondary: bytes):
    output.mkdir(parents=True, exist_ok=True)
    gates = source_and_engine_checks()
    assert len(original_map) == 128
    assert len(primary) == 512 * 16 and len(secondary) == 500 * 16
    records = convert()
    source = records["primary"] + records["secondary"]
    assert len(source) == len(coverage["metatiles"]) == 1012
    assert extended["candidate_metatiles"] == 465
    assert extended["new_headbutt_tree_candidates"] == 58
    assert extended["headbutt_field_move_scripts_pending"]
    assert wall_rules["source_wall_permission_count"] == 569
    tree_ids = [row["metatile_id"] for row in source
                if row["source_collision_symbol"] == "HEADBUTT_TREE"]
    assert len(tree_ids) == len(set(tree_ids)) == 58
    solid_ids = {row["metatile_id"] for row in wall_rules["rules"]}
    assert set(tree_ids) <= solid_ids
    assert all(extended["per_metatile_review"][i]["candidate_behavior"]
               == "MB_JOHTO_HEADBUTT_TREE" for i in tree_ids)
    floor = next(cell for cell in walkability()
                 if cell["source_symbol"] == "FLOOR"
                 and cell["candidate_attribute_word"] == 0)
    floor_word = floor["metatile_id"] | LAND << 12
    player = (3, 3)
    words = [floor_word] * 64
    cases = []
    for idx, (direction, (x, y)) in enumerate(TARGETS):
        mid = tree_ids[idx]
        bank_name = "primary" if mid < 512 else "secondary"
        local = mid if mid < 512 else mid - 512
        geom = (primary if mid < 512 else secondary)[local * 16:(local + 1) * 16]
        assert len(geom) == 16
        assert hashlib.sha256(geom).hexdigest() == coverage["metatiles"][mid]["source_geometry_sha256"]
        packed = mid | SOLID << 10 | LAND << 12
        assert (packed >> 10) & 3 == SOLID and packed >> 12 == LAND
        words[x + y * WIDTH] = packed
        cases.append({
            "direction": direction, "player_position": list(player),
            "target_position": [x, y], "metatile_id": mid,
            "source_symbol": "HEADBUTT_TREE", "bank": bank_name,
            "source_geometry_sha256": hashlib.sha256(geom).hexdigest(),
            "candidate_behavior": "MB_JOHTO_HEADBUTT_TREE",
            "candidate_mapgrid_collision": SOLID,
            "collision_should_block_walking": True,
            "field_move_prompt_or_encounter_verified": False,
            "emulator_verified": False,
        })
    assert words[player[0] + WIDTH * player[1]] == floor_word
    assert sum(((word >> 10) & 3) == 3 for word in words) == 4
    assert len({case["direction"] for case in cases}) == 4
    raw = struct.pack("<64H", *words)
    assert len(raw) == 128
    assert hashlib.sha256(raw).digest() != hashlib.sha256(original_map).digest()
    filename = "johto_headbutt_four_direction.OFFLINE.bin"
    (output / filename).write_bytes(raw)
    report = {
        "status": "JOHTO_HEADBUTT_COLLISION_PROBES_NOT_FOR_ROM",
        "source": records["source"],
        "source_headbutt_quadrants": 58,
        "facing_direction_probe_count": 4,
        "source_and_engine_checks": gates,
        "source_base_map_sha256": hashlib.sha256(original_map).hexdigest(),
        "probe_map_sha256": hashlib.sha256(raw).hexdigest(),
        "probe_filename": filename,
        "source_graphics_verified": True,
        "mapgrid_collision_rules_verified": True,
        "field_move_script_implemented": False,
        "headbutt_encounters_ported": False,
        "tree_animation_ported": False,
        "headbutt_item_rewards_ported": False,
        "map_registered": False,
        "tileset_attributes_registered": False,
        "emulator_interaction_test_passed": False,
        "safe_to_register": False,
        "cases": cases,
    }
    (output / "johto_headbutt_field_probes.review.json").write_text(
        json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return report
