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
    assert "s16 JohtoHeadbuttSelectWeightedEntry(const u8 *weights, u8 count, u8 rollOutOfHundred);" in header
    selection = cpp.split("s16 JohtoHeadbuttSelectWeightedEntry(", 1)[1].split("\n}\n", 1)[0]
    for needle in ("rollOutOfHundred >= 100", "total != 100", "remaining < weights[i]", "remaining -= weights[i]", "return i;"):
        assert needle in selection, needle
    assert "struct JohtoHeadbuttSourceEncounter" in header
    assert "bool8 JohtoHeadbuttResolveSourceEncounter(" in header
    resolver = cpp.split("bool8 JohtoHeadbuttResolveSourceEncounter(", 1)[1].split("\n}\n", 1)[0]
    for needle in ("total != 100", "rows[i].sourceSpecies == NULL",
                   "rows[i].sourceLevelExpression == NULL", "*out = rows[i]",
                   "remaining -= rows[i].weight"):
        assert needle in resolver, needle
    assert "bool8 JohtoHeadbuttGetBadgeBaseLevel(u8 badgeCount, u8 *outLevel);" in header
    badge = cpp.split("bool8 JohtoHeadbuttGetBadgeBaseLevel(", 1)[1].split("\n}\n", 1)[0]
    assert "4, 8, 12, 16, 20, 24, 28, 32, 36" in badge
    assert "40, 43, 46, 49, 52, 55, 58, 61" in badge
    assert "badgeCount >= ARRAY_COUNT(sBadgeBaseLevels)" in badge
    assert "bool8 JohtoHeadbuttResolveLevel(" in header
    level_resolver = cpp.split("bool8 JohtoHeadbuttResolveLevel(", 1)[1].split("\n}\n", 1)[0]
    for needle in ("JOHTO_HEADBUTT_LEVEL_FIXED", "JOHTO_HEADBUTT_LEVEL_BADGE_OFFSET",
                   "badgeBaseLevel + sourceValue", "level < 2", "level > 99",
                   "*outLevel = level"):
        assert needle in level_resolver, needle
    assert "bool8 JohtoHeadbuttResolveLevelFromBadgeCount(" in header
    composed = cpp.split("bool8 JohtoHeadbuttResolveLevelFromBadgeCount(", 1)[1].split("\n}\n", 1)[0]
    for needle in ("mode == JOHTO_HEADBUTT_LEVEL_FIXED",
                   "JohtoHeadbuttGetBadgeBaseLevel(badgeCount, &badgeBaseLevel)",
                   "JohtoHeadbuttResolveLevel(mode, sourceValue, badgeBaseLevel, outLevel)"):
        assert needle in composed, needle
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
    # Validate cumulative row boundaries against every pinned normal/rare table.
    for group in encounter_report["source_tree_groups"]:
        if group["is_rock_smash_table"]:
            continue
        for table in (group["normal"], group["rare"]):
            if table is None:
                continue
            weights = [entry["weight_percent"] for entry in table]
            assert all(0 < weight <= 100 for weight in weights)
            assert sum(weights) == 100
            outcomes = []
            for roll in range(100):
                remaining = roll
                for idx, weight in enumerate(weights):
                    if remaining < weight:
                        outcomes.append(idx)
                        break
                    remaining -= weight
                else:
                    raise AssertionError("unselected encounter roll")
            assert [outcomes.count(idx) for idx in range(len(weights))] == weights
    # Offline source entry resolution: retain symbolic species, forms and
    # badge-level expressions; do not invent Emerald constants or levels.
    resolved_entries = []
    seen_groups = set()
    for group in encounter_report["source_tree_groups"]:
        if group["is_rock_smash_table"]:
            continue
        effective = group["effective_table_label"]
        if effective in seen_groups:
            continue  # CITY aliases CANYON; not a new source distribution.
        seen_groups.add(effective)
        for distribution in ("normal", "rare"):
            entries = group[distribution]
            assert entries is not None
            for roll in range(100):
                remaining = roll
                for index, entry in enumerate(entries):
                    if remaining < entry["weight_percent"]:
                        assert entry["source_species"]
                        assert entry["source_level_expression"]
                        assert entry["requires_badge_level_translation"] == (entry["fixed_level"] is None)
                        resolved_entries.append({
                            "source_table": effective,
                            "distribution": distribution,
                            "roll": roll,
                            "entry_index": index,
                            "source_species": entry["source_species"],
                            "source_form": entry["source_form"],
                            "source_level_expression": entry["source_level_expression"],
                            "requires_badge_level_translation": entry["requires_badge_level_translation"],
                        })
                        break
                    remaining -= entry["weight_percent"]
                else:
                    raise AssertionError("unresolved weighted source row")
    assert len(resolved_entries) == 8 * 2 * 100
    assert any(row["source_form"] == "HISUIAN_FORM" for row in resolved_entries)
    assert any(row["requires_badge_level_translation"] for row in resolved_entries)
    # Exhaustively check all 100 rolls in each of the 16 source distributions
    # against the 17 pinned badge-base levels. Preserve species and forms,
    # while resolving only source-level arithmetic; this never calls Emerald RNG.
    badge_levels = (4, 8, 12, 16, 20, 24, 28, 32, 36,
                    40, 43, 46, 49, 52, 55, 58, 61)
    source_c = (ROOT / "src/field_move.c").read_text(encoding="utf-8")
    c_badges = source_c.split("static const u8 sBadgeBaseLevels[17] =", 1)[1].split("};", 1)[0]
    assert tuple(map(int, re.findall(r"\\d+", c_badges))) == badge_levels
    level_cases = 0
    badge_relative_cases = 0
    sample_levels = []
    for row in resolved_entries:
        expr = row["source_level_expression"]
        if row["requires_badge_level_translation"]:
            match = re.fullmatch(r"LEVEL_FROM_BADGES\\s*([+-])\\s*(\\d+)", expr)
            assert match, expr
            offset = int(match.group(2)) * (1 if match.group(1) == "+" else -1)
        else:
            assert re.fullmatch(r"\\d+", expr), expr
            fixed = int(expr)
            assert 1 <= fixed <= 100
        for badge_count, base in enumerate(badge_levels):
            level = max(2, min(99, base + offset)) if row["requires_badge_level_translation"] else fixed
            assert 1 <= level <= 100
            level_cases += 1
            badge_relative_cases += int(row["requires_badge_level_translation"])
            if row["roll"] == 0 and badge_count in (0, 8, 16) and len(sample_levels) < 48:
                sample_levels.append({
                    "source_table": row["source_table"],
                    "distribution": row["distribution"],
                    "species": row["source_species"],
                    "form": row["source_form"],
                    "expression": expr,
                    "badges": badge_count,
                    "resolved_level": level,
                })
    assert level_cases == 8 * 2 * 100 * 17
    assert badge_relative_cases > 0
    assert any(row["form"] == "HISUIAN_FORM" for row in sample_levels)
    assert any(row["expression"].startswith("LEVEL_FROM_BADGES") for row in sample_levels)
    (output / "johto_headbutt_badge_level_samples.OFFLINE.json").write_text(
        json.dumps(sample_levels, indent=2) + "\\n", encoding="utf-8")
    (output / "johto_headbutt_resolved_source_entries.OFFLINE.json").write_text(
        json.dumps(resolved_entries, indent=2) + "\n", encoding="utf-8")
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
        "source_entry_resolution_vectors": len(resolved_entries),
        "source_entry_resolution_file": "johto_headbutt_resolved_source_entries.OFFLINE.json",
        "source_entry_resolution_only": True,
        "source_badge_level_cases": level_cases,
        "source_badge_relative_cases": badge_relative_cases,
        "source_badge_level_sample_file": "johto_headbutt_badge_level_samples.OFFLINE.json",
        "source_badge_levels_exhaustively_audited_offline": True,
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
