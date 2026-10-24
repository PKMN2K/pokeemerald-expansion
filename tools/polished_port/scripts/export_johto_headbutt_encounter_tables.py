#!/usr/bin/env python3
"""Audit authentic Johto Headbutt encounter/map groups, OFFLINE ONLY.

Data and selection semantics from pinned Polished Crystal. Do not mistake
Headbutt trees for ordinary walk-in random encounters or silently invent
Emerald map identifiers/species-form remaps. City aliases Canyon; Rock Smash
uses a distinct one-table encounter group; badge levels stay symbolic.
"""
import hashlib
import json
import re
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
FIXTURES = ROOT / "tools/polished_port/fixtures"
PIN = "Rangi42/polishedcrystal@3fa43192379df5c3e7b09a08e4d5d79af4f02f42"
SET_NAMES = (
    "CITY", "CANYON", "TOWN", "ROUTE", "KANTO",
    "LAKE", "FOREST", "APRICORNS", "NOISY_FOREST", "ROCK",
)
EXPECTED_MAPS = {
    "TREEMON_SET_CITY": 10,
    "TREEMON_SET_CANYON": 7,
    "TREEMON_SET_TOWN": 1,
    "TREEMON_SET_ROUTE": 12,
    "TREEMON_SET_KANTO": 4,
    "TREEMON_SET_LAKE": 2,
    "TREEMON_SET_FOREST": 3,
    "TREEMON_SET_APRICORNS": 3,
    "TREEMON_SET_NOISY_FOREST": 1,
}
FIXTURE_MAP = {
    "encounters": "johto_headbutt_treemons.asm",
    "maps": "johto_headbutt_tree_maps.asm",
    "engine": "johto_headbutt_tree_engine.asm",
    "overworld": "johto_headbutt_overworld_flow.asm",
}


def load_sources():
    return {name: (FIXTURES / filename).read_text(encoding="utf-8")
            for name, filename in FIXTURE_MAP.items()}


def parse_entry(line):
    assert line.lstrip().startswith("tree_mon ")
    fields = [p.strip() for p in line.strip()[len("tree_mon "):].split(",")]
    assert len(fields) in (3, 4), fields
    weight = int(fields[0])
    assert 1 <= weight <= 100
    assert re.fullmatch(r"[A-Z_]+", fields[1]), fields
    species = fields[1]
    form = fields[2] if len(fields) == 4 else None
    level = fields[-1]
    if form is not None:
        assert re.fullmatch(r"[A-Z0-9_]+", form)
    assert re.fullmatch(r"\d+|LEVEL_FROM_BADGES\s*[+-]\s*\d+", level), level
    numeric = int(level) if level.isdecimal() else None
    return {
        "weight_percent": weight,
        "source_species": species,
        "source_form": form,
        "source_level_expression": level,
        "fixed_level": numeric,
        "requires_badge_level_translation": numeric is None,
    }


def parse_encounters(source):
    pointer_block = source.split("TreeMons:\n", 1)[1].split(
        "\tassert_table_length NUM_TREEMON_SETS", 1)[0]
    pointer_names = re.findall(r"(?m)^\s*dw\s+(TreeMonSet_[A-Za-z0-9]+)\b", pointer_block)
    assert len(pointer_names) == len(SET_NAMES) == 10
    assert pointer_names == [
        "TreeMonSet_" + s.title().replace("_", "") if s != "KANTO"
        else "TreeMonSet_Kanto" for s in SET_NAMES
    ]
    definitions = list(re.finditer(r"(?m)^(TreeMonSet_[A-Za-z0-9]+):[ \t]*$", source))
    assert len(definitions) == 10
    assert [m.group(1) for m in definitions] == pointer_names

    sections = {}
    for i, m in enumerate(definitions):
        start = m.end()
        stop = definitions[i + 1].start() if i + 1 < len(definitions) else len(source)
        body = source[start:stop]
        rows = re.findall(r"(?m)^[ \t]*tree_mon[ \t]+[^\n]+", body)
        splits = re.split(r"(?m)^[ \t]*db[ \t]+-1(?:[ \t]*;[^\n]*)?[ \t]*$", body)
        group = m.group(1)
        if group == "TreeMonSet_City":
            assert not rows and len(splits) == 1
            sections[group] = None
            continue
        is_rock = group == "TreeMonSet_Rock"
        assert len(splits) == (2 if is_rock else 3), (group, len(splits))
        parsed = []
        for part in splits[:1 if is_rock else 2]:
            entries = [parse_entry(line) for line in re.findall(
                r"(?m)^[ \t]*tree_mon[ \t]+[^\n]+", part)]
            assert entries and sum(x["weight_percent"] for x in entries) == 100
            parsed.append(entries)
        assert sum(len(x) for x in parsed) == len(rows)
        sections[group] = parsed

    assert sections["TreeMonSet_City"] is None
    assert len(sections["TreeMonSet_Canyon"]) == 2
    assert len(sections["TreeMonSet_Rock"]) == 1
    assert sum(sum(len(t) for t in section)
               for section in sections.values() if section is not None) == 57
    pointer_sets = []
    for index, (name, label) in enumerate(zip(SET_NAMES, pointer_names)):
        resolved = "TreeMonSet_Canyon" if name == "CITY" else label
        tables = sections[resolved]
        assert tables is not None
        pointer_sets.append({
            "index": index,
            "source_constant": "TREEMON_SET_" + name,
            "pointer_label": label,
            "effective_table_label": resolved,
            "shares_canyon_table": name == "CITY",
            "normal": tables[0],
            "rare": tables[1] if len(tables) == 2 else None,
            "is_rock_smash_table": name == "ROCK",
        })
    assert sum(p["shares_canyon_table"] for p in pointer_sets) == 1
    assert sum(p["is_rock_smash_table"] for p in pointer_sets) == 1
    return pointer_sets


def parse_maps(source):
    groups = source.split("TreeMonMaps:\n", 1)[1].split("\nRockMonMaps:", 1)[0]
    rows = re.findall(r"(?m)^[ \t]*treemon_map[ \t]+([A-Z0-9_]+),[ \t]*(TREEMON_SET_[A-Z_]+)[ \t]*$", groups)
    assert len(rows) == 43, len(rows)
    assert len({name for name, _ in rows}) == len(rows)
    assert Counter(group for _, group in rows) == Counter(EXPECTED_MAPS)
    rock = source.split("RockMonMaps:\n", 1)[1]
    rock_rows = re.findall(r"(?m)^[ \t]*treemon_map[ \t]+([A-Z0-9_]+),[ \t]*(TREEMON_SET_[A-Z_]+)[ \t]*$", rock)
    assert len(rock_rows) == 14
    assert all(g == "TREEMON_SET_ROCK" for _, g in rock_rows)
    return [{"source_map_id": mapname, "source_group": group}
            for mapname, group in rows], [name for name, _ in rock_rows]


def audit_selector(engine, overworld):
    tree = engine.split("GetTreeMon:\n", 1)[1].split("\nNoTreeMon:", 1)[0]
    assert "call GetTreeScore" in tree
    for expression in ("ld a, 10", "call RandomRange", "cp 5",
                       "cp 8", "ld a, 100", "jr nz, NoTreeMon",
                       "jr nc, NoTreeMon"):
        assert expression in tree
    score = engine.split("GetTreeScore:\n", 1)[1]
    for expression in ("call .CoordScore", "call .OTIDScore", "cp 5",
                       ".rare", ".good", ".bad"):
        assert expression in score
    reward = engine.split("TreeItemEncounter:\n", 1)[1].split(
        "\nRockItemEncounter:", 1)[0]
    for expression in ("ld a, WING_CASE", "call _CheckKeyItem",
                       "ld a, NUM_WINGS * 10", "cp NUM_WINGS",
                       "ld a, 10", "call RandomRange", "inc a",
                       "HIGH(999)"):
        assert expression in reward
    flow = overworld.split("AutoHeadbuttScript:\n", 1)[1].split(
        "\n.ShowWingIcon:", 1)[0]
    assert flow.index("callasm TreeMonEncounter") < flow.index("callasm TreeItemEncounter")
    assert "ifequalfwd TREEMON_NO_ENCOUNTER_SET, .no_item" in flow
    assert "randomwildmon" in flow and "startbattle" in flow
    assert "callasm ShakeHeadbuttTree" in flow
    assert "setflag ENGINE_HEADBUTT_ACTIVE" in overworld
    return {
        "source_tree_score_uses_facing_coordinates_and_trainer_id": True,
        "chance_to_battle_percent_by_tree_score": {"bad": 10, "good": 50, "rare": 80},
        "ordinary_table_applies_to_bad_and_good_scores": True,
        "rare_table_applies_to_rare_score": True,
        "rare_table_is_not_a_second_roll_after_normal": True,
        "tree_no_encounter_group_disables_item_reward": True,
        "tree_reward_requires_wing_case": True,
        "tree_wing_reward_percent_when_eligible": 10,
        "tree_wing_quantity_min": 1,
        "tree_wing_quantity_max": 10,
        "tree_wing_inventory_cap": 999,
        "exact_tree_score_math_requires_port": True,
        "source_headbutt_animation_and_script_require_port": True,
    }


def export(output: Path, headbutt_review: dict):
    output.mkdir(parents=True, exist_ok=True)
    assert headbutt_review["source_headbutt_quadrants"] == 58
    assert not headbutt_review["field_move_script_implemented"]
    assert not headbutt_review["headbutt_encounters_ported"]
    source = load_sources()
    pointers = parse_encounters(source["encounters"])
    maps, rock_maps = parse_maps(source["maps"])
    selection = audit_selector(source["engine"], source["overworld"])
    dynamic = sum(x["requires_badge_level_translation"]
                  for row in pointers if not row["shares_canyon_table"]
                  for table in (row["normal"], row["rare"] or [])
                  for x in table)
    assert dynamic == 6
    forms = [x for row in pointers if not row["shares_canyon_table"]
             for table in (row["normal"], row["rare"] or [])
             for x in table if x["source_form"] is not None]
    assert len(forms) == 3
    assert all(x["source_form"] == "HISUIAN_FORM" for x in forms)
    assert len({row["source_group"] for row in maps}) == 9
    assert all(p["source_constant"] in {r["source_group"] for r in maps}
               for p in pointers if not p["is_rock_smash_table"])
    assert not any(row["source_group"] == "TREEMON_SET_ROCK" for row in maps)
    report = {
        "status": "PINNED_HEADBUTT_ENCOUNTERS_REVIEW_NOT_FOR_ROM",
        "source": PIN,
        "source_files": {
            kind: {"upstream_path": {
                "encounters": "data/wild/treemons.asm",
                "maps": "data/wild/treemon_maps.asm",
                "engine": "engine/events/treemons.asm",
                "overworld": "engine/events/overworld.asm",
            }[kind], "sha256": hashlib.sha256(body.encode("utf-8")).hexdigest()}
            for kind, body in source.items()
        },
        "source_tree_map_assignments": len(maps),
        "source_rock_map_assignments_separate": len(rock_maps),
        "source_pointer_groups": len(pointers),
        "source_distinct_tree_distributions": 8,
        "source_city_aliases_canyon": True,
        "source_fixed_and_dynamic_level_expressions_preserved": True,
        "source_hisuian_forms_preserved": True,
        "source_selection_logic": selection,
        "source_tree_groups": pointers,
        "source_tree_map_groups": maps,
        "source_rock_maps": rock_maps,
        "complete_emerald_wild_encounters_emitted": False,
        "source_map_ids_remapped_to_emerald": False,
        "source_pokemon_form_ids_remapped_to_emerald": False,
        "badge_scaled_levels_implemented": False,
        "headbutt_field_move_implemented": False,
        "tree_encounter_selection_implemented": False,
        "wing_case_inventory_implemented": False,
        "live_map_registered": False,
        "safe_to_enable": False,
    }
    (output / "johto_headbutt_encounters.review.json").write_text(
        json.dumps(report, indent=2) + "\n", encoding="utf-8"
    )
    return report
