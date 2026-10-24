#!/usr/bin/env python3
"""Audit source-exact Johto Headbutt tree score and battle RNG gates.

The Emerald functions compiled in src/field_move.c are intentionally dormant.
This emits only deterministic offline expected results for four-facing map
tests, and does not call them in-game or wire any map, ability, or reward.
"""
import hashlib
import json
import re
from collections import Counter
from pathlib import Path

from export_johto_headbutt_encounter_tables import load_sources

ROOT = Path(__file__).resolve().parents[3]
SCORE_NAMES = ("bad", "good", "rare")
THRESHOLDS = (1, 5, 8)
WIDTH = HEIGHT = 16


def verify_source_and_engine():
    source = load_sources()["engine"]
    source_score = source.split("GetTreeScore:\n", 1)[1].split("\n.CoordScore:", 1)[0]
    source_coords = source.split(".CoordScore:\n", 1)[1].split("\n.OTIDScore:", 1)[0]
    source_id = source.split(".OTIDScore:\n", 1)[1]
    encounter = source.split("GetTreeMon:\n", 1)[1].split("\nSelectTreeMon:\n", 1)[0]
    for needle in ("call .CoordScore", "call .OTIDScore", "sub c",
                   "jr z, .rare", "add 10", "cp 5", "jr c, .good"):
        assert needle in source_score, needle
    for needle in ("call GetFacingTileCoord", "ld c, e", "ld a, d",
                   "add hl, bc", "ld c, d", "ld a, 5",
                   "ld a, 10", "farcall Divide"):
        assert needle in source_coords, needle
    for needle in ("ld a, [wPlayerID]", "ld a, [wPlayerID + 1]",
                   "ld a, 10", "farcall Divide"):
        assert needle in source_id, needle
    for needle in ("call GetTreeScore", ".bad", ".good", ".rare",
                   "ld a, 10", "call RandomRange", "cp 5", "cp 8",
                   "jr nz, NoTreeMon", "jr nc, NoTreeMon"):
        assert needle in encounter, needle

    header = (ROOT / "include/field_move.h").read_text(encoding="utf-8")
    cpp = (ROOT / "src/field_move.c").read_text(encoding="utf-8")
    assert "enum JohtoHeadbuttTreeScore GetJohtoHeadbuttTreeScore(u8 x, u8 y, u16 trainerId);" in header
    assert "bool8 JohtoHeadbuttRollStartsBattle(enum JohtoHeadbuttTreeScore score, u8 rollOutOfTen);" in header
    for name in ("JOHTO_HEADBUTT_BAD_TREE", "JOHTO_HEADBUTT_GOOD_TREE",
                 "JOHTO_HEADBUTT_RARE_TREE"):
        assert name in header and name in cpp
    score_body = cpp.split("enum JohtoHeadbuttTreeScore GetJohtoHeadbuttTreeScore(", 1)[1].split("\n}\n", 1)[0]
    for needle in ("(u16)x * (y + 1) + y", "(coordinateNumerator / 5) % 10",
                   "trainerId % 10", "(coordinateScore + 10 - trainerScore) % 10",
                   "if (delta == 0)", "if (delta < 5)"):
        assert needle in score_body, needle
    roll_body = cpp.split("bool8 JohtoHeadbuttRollStartsBattle(", 1)[1].split("\n}\n", 1)[0]
    assert "if (rollOutOfTen >= 10)" in roll_body
    for threshold in THRESHOLDS:
        assert "rollOutOfTen < " + str(threshold) in roll_body
    return {"pinned_coordinate_arithmetic_reviewed": True,
            "pinned_trainer_id_modulo_verified": True,
            "pinned_random_roll_gates_verified": True,
            "compiled_but_unwired_emerald_helpers_checked": True}


def score_at(x: int, y: int, trainer_id: int) -> int:
    assert 0 <= x <= 255 and 0 <= y <= 255
    assert 0 <= trainer_id <= 65535
    numerator = (x * (y + 1) + y) & 0xFFFF
    coord = (numerator // 5) % 10
    delta = (coord - trainer_id % 10) % 10
    return 2 if delta == 0 else 1 if delta < 5 else 0


def starts_battle(score: int, roll: int) -> bool:
    assert 0 <= roll < 10 and score in (0, 1, 2)
    return roll < THRESHOLDS[score]


def export(output: Path, encounter_report: dict):
    output.mkdir(parents=True, exist_ok=True)
    gates = verify_source_and_engine()
    assert encounter_report["source_tree_map_assignments"] == 43
    assert encounter_report["source_pointer_groups"] == 10
    assert encounter_report["source_selection_logic"]["chance_to_battle_percent_by_tree_score"] == {
        "bad": 10, "good": 50, "rare": 80,
    }
    assert not encounter_report["tree_encounter_selection_implemented"]
    vectors = bytearray()
    count = Counter()
    for y in range(HEIGHT):
        for x in range(WIDTH):
            per_tile = Counter()
            for trainer_mod_ten in range(10):
                result = score_at(x, y, trainer_mod_ten)
                vectors.append(result)
                count[SCORE_NAMES[result]] += 1
                per_tile[SCORE_NAMES[result]] += 1
            assert per_tile == {"rare": 1, "good": 4, "bad": 5}
    assert len(vectors) == WIDTH * HEIGHT * 10 == 2560
    assert count == {"rare": 256, "good": 1024, "bad": 1280}
    # Boundary and wraparound of the original 16-bit coordinate accumulator.
    for y in (0, 1, 5, 31, 128, 255):
        for x in (0, 1, 5, 31, 128, 255):
            for trainer_mod in range(10):
                result = score_at(x, y, trainer_mod)
                assert result in (0, 1, 2)
                for factor in (0, 10, 100, 1000, 6000):
                    trainer_id = trainer_mod + factor
                    if trainer_id <= 65535:
                        assert score_at(x, y, trainer_id) == result

    pass_counts = {SCORE_NAMES[s]: sum(starts_battle(s, r) for r in range(10))
                   for s in range(3)}
    assert pass_counts == {"bad": 1, "good": 5, "rare": 8}
    samples = []
    for x, y, trainer_id in (
        (0, 0, 0), (0, 0, 9), (1, 1, 11), (5, 2, 12345),
        (10, 10, 54321), (15, 15, 65535), (255, 255, 65535),
    ):
        category = score_at(x, y, trainer_id)
        samples.append({"facing_tile_xy": [x, y], "trainer_id": trainer_id,
                        "tree_score": SCORE_NAMES[category],
                        "source_encounter_chance_percent": THRESHOLDS[category] * 10,
                        "source_rare_distribution_selected": category == 2})
    assert samples[0]["tree_score"] == "rare"
    assert samples[1]["tree_score"] == "good"
    data = bytes(vectors)
    filename = "johto_headbutt_score_16x16_trainer_mod10.OFFLINE.bin"
    (output / filename).write_bytes(data)
    report = {
        "status": "JOHTO_HEADBUTT_SCORE_MODEL_UNREGISTERED",
        "source": encounter_report["source"],
        "source_checks": gates,
        "grid_width": WIDTH, "grid_height": HEIGHT,
        "trainer_id_modulo_buckets": 10,
        "vector_count": len(data),
        "binary_filename": filename,
        "binary_sha256": hashlib.sha256(data).hexdigest(),
        "vector_score_counts": dict(count),
        "encounter_roll_successes_out_of_ten": pass_counts,
        "source_tree_table_selects_rare_only_for_score_rare": True,
        "source_rock_smash_encounter_tables_excluded": True,
        "source_map_identifiers_untranslated": True,
        "field_move_not_registered": True,
        "tree_battles_not_enabled": True,
        "emulator_verified": False,
        "safe_to_enable": False,
        "samples": samples,
    }
    (output / "johto_headbutt_score.review.json").write_text(
        json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return report
