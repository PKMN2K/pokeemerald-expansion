#!/usr/bin/env python3
"""Audit authentic Johto Headbutt encounter/map groups, OFFLINE ONLY.

Data and selection semantics from pinned Polished Crystal. Generate dormant
typed C tables with explicit species/form translation and original map IDs.
City aliases Canyon; Rock Smash stays separate. The review preserves source
level expressions and reports unresolved mappings without dropping rows.
"""
import argparse
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
RUNTIME_TABLES = ROOT / "src/data/polished_johto_headbutt_tables.h"
# These are species identities, not transferable numeric form indexes.
# Polished Crystal HISUIAN_FORM=4 is NOT Expansion form index 4.
FORM_SPECIES = {
    ("VOLTORB", "HISUIAN_FORM"): "SPECIES_VOLTORB_HISUI",
    ("ELECTRODE", "HISUIAN_FORM"): "SPECIES_ELECTRODE_HISUI",
}


def species_candidates(pointers, constants=None):
    if constants is None:
        constants = (ROOT / "include/constants/species.h").read_text()
    declared = set(re.findall(r"(?m)^\s*(SPECIES_[A-Z0-9_]+)\s*=", constants))
    entries = [entry for pointer in pointers if not pointer["shares_canyon_table"]
               for table in (pointer["normal"], pointer["rare"] or []) for entry in table]
    candidates = []
    for name in sorted({entry["source_species"] for entry in entries}):
        forms = sorted({e["source_form"] for e in entries
                        if e["source_species"] == name and e["source_form"] is not None})
        base = "SPECIES_" + name
        mapped = [FORM_SPECIES[(name, form)] for form in forms
                  if FORM_SPECIES.get((name, form)) in declared]
        unresolved = [form for form in forms
                      if FORM_SPECIES.get((name, form)) not in declared]
        candidates.append({
            "source_species": name,
            "source_forms": forms,
            "base_constant": base if base in declared else None,
            "source_form_constant_candidates": mapped,
            "unresolved_forms": unresolved,
            "runtime_mapping_verified": False,
        })
    # An audit must still report missing identities; only code emission rejects them.
    return candidates


def target_species(entry):
    name, form = entry["source_species"], entry["source_form"]
    if form is None:
        return "SPECIES_" + name
    if (name, form) not in FORM_SPECIES:
        raise ValueError(f"Unresolved Headbutt form: {name}/{form}")
    return FORM_SPECIES[(name, form)]


def verify_form_identity():
    """Verify the two regional identities; do not claim stat/learnset parity."""
    info = (ROOT / "src/data/pokemon/species_info/gen_1_families.h").read_text()
    tables = (ROOT / "src/data/pokemon/form_species_tables.h").read_text()
    for name in ("VOLTORB", "ELECTRODE"):
        fixture = (FIXTURES / f"johto_headbutt_{name.lower()}_hisuian.asm").read_text()
        assert f"abilities_for {name}_HISUIAN," in fixture
        assert "db ELECTRIC, GRASS ; type" in fixture
        constant = FORM_SPECIES[(name, "HISUIAN_FORM")]
        block = info.split(f"[{constant}] =", 1)[1].split("\n    },", 1)[0]
        assert ".isHisuianForm = TRUE" in block
        assert f".natDexNum = NATIONAL_DEX_{name}" in block
        assert "MON_TYPES(TYPE_ELECTRIC, TYPE_GRASS)" in block
        table = f"s{name.title()}FormSpeciesIdTable"
        assert f".formSpeciesIdTable = {table}" in block
        body = tables.split(f"{table}[] = {{", 1)[1].split("};", 1)[0]
        assert re.findall(r"\bSPECIES_[A-Z_]+", body) == ["SPECIES_" + name, constant]
        assert "#if P_HISUIAN_FORMS" in body


def source_map_assignments(maps):
    from route_johto_palette import parse_source_constants
    source_ids = {name: ids for ids, name in parse_source_constants(
        (FIXTURES / "johto_map_constants.asm").read_text()).items()}
    return [dict(row, source_map_group=source_ids[row["source_map_id"]][0],
                 source_map_number=source_ids[row["source_map_id"]][1]) for row in maps]


def render_runtime_tables(pointers, maps, candidates=None):
    if candidates is None:
        candidates = species_candidates(pointers)
    unresolved = [c for c in candidates if c["base_constant"] is None or c["unresolved_forms"]]
    if unresolved:
        raise ValueError("Unresolved Headbutt species/form mappings: " + json.dumps(unresolved))
    verify_form_identity()
    lines = ["// Generated by tools/polished_port/scripts/export_johto_headbutt_encounter_tables.py",
             "// Source: " + PIN,
             "// Dormant only. Map IDs are ORIGINAL Crystal group/number, not Emerald IDs.", ""]
    trees = [p for p in pointers if not p["is_rock_smash_table"]]
    for group in trees:
        if group["shares_canyon_table"]:
            continue
        for kind in ("normal", "rare"):
            label = group["effective_table_label"] + "_" + kind
            lines.append(f"static const struct JohtoHeadbuttEntry s{label}[] =")
            lines.append("{")
            for row in group[kind]:
                if row["fixed_level"] is None:
                    match = re.fullmatch(r"LEVEL_FROM_BADGES\s*([+-])\s*(\d+)", row["source_level_expression"])
                    assert match, row
                    value = int(match[2]) * (1 if match[1] == "+" else -1)
                    mode = "JOHTO_HEADBUTT_LEVEL_BADGE_OFFSET"
                else:
                    value, mode = row["fixed_level"], "JOHTO_HEADBUTT_LEVEL_FIXED"
                lines.append(f"    {{ {target_species(row)}, {value}, {row['weight_percent']}, {mode} }},")
            lines += ["};", ""]
    lines += ["static const struct JohtoHeadbuttGroup sJohtoHeadbuttGroups[] =", "{"]
    for group in trees:
        label = "s" + group["effective_table_label"]
        lines += [f"    // {group['source_constant']}", "    {",
                  f"        {{ {label}_normal, ARRAY_COUNT({label}_normal) }},",
                  f"        {{ {label}_rare, ARRAY_COUNT({label}_rare) }},", "    },"]
    lines += ["};", "", "static const struct JohtoHeadbuttMap sJohtoHeadbuttMaps[] =", "{"]
    indices = {p["source_constant"]: p["index"] for p in trees}
    for row in source_map_assignments(maps):
        lines.append(f"    {{ {row['source_map_group']}, {row['source_map_number']}, "
                     f"{indices[row['source_group']]} }}, // {row['source_map_id']}")
    lines += ["};", ""]
    return "\n".join(lines)


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
        "exact_tree_score_math_requires_port": False,
        "tree_score_live_gameplay_dispatch_pending": True,
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
    assert len(forms) == 4
    assert all(x["source_form"] == "HISUIAN_FORM" for x in forms)
    assert len({row["source_group"] for row in maps}) == 9
    assert all(p["source_constant"] in {r["source_group"] for r in maps}
               for p in pointers if not p["is_rock_smash_table"])
    assert not any(row["source_group"] == "TREEMON_SET_ROCK" for row in maps)
    candidates = species_candidates(pointers)
    unresolved = [c for c in candidates if c["base_constant"] is None or c["unresolved_forms"]]
    runtime_tables_match = False
    if not unresolved:
        runtime_tables_match = (RUNTIME_TABLES.exists() and
                                RUNTIME_TABLES.read_text() == render_runtime_tables(pointers, maps, candidates))
    tree_species = {entry["source_species"] for pointer in pointers
                    if not pointer["is_rock_smash_table"]
                    for table in (pointer["normal"], pointer["rare"]) for entry in table}
    for candidate in candidates:
        candidate["used_by_headbutt"] = candidate["source_species"] in tree_species
        candidate["runtime_mapping_verified"] = runtime_tables_match and candidate["used_by_headbutt"]
    map_groups = json.loads((ROOT / "data/maps/map_groups.json").read_text(encoding="utf-8"))
    # The custom garden has no source map identity. New Bark has a City set,
    # Route 29 has a Route set, and their imported interiors have none.
    # Keep rejecting unreviewed registrations; a "Johto" name or matching
    # numeric map ID must never imply an Emerald-to-source encounter bridge.
    from build_new_bark import MAPS as new_bark_maps
    registered_source_sets = {row["source_map_id"]: row["source_group"] for row in maps
                              if row["source_map_id"] in new_bark_maps.values()}
    assert registered_source_sets == {"NEW_BARK_TOWN": "TREEMON_SET_CITY", "ROUTE_29": "TREEMON_SET_ROUTE", "CHERRYGROVE_CITY": "TREEMON_SET_CITY"}
    # Production still contains only the dormant selector's definition, with
    # no live caller or function-pointer binding. A future field phase must
    # replace this gate with explicit mapping and gameplay validation.
    selector = r"\bJohtoHeadbuttSelectEncounterFromSourceMap\b"
    references = [str(path.relative_to(ROOT)) for path in (ROOT / "src").rglob("*.c")
                  if re.search(selector, path.read_text())]
    assert references == ["src/polished_johto_headbutt.c"]
    johto_groups = {name: map_groups[name] for name in map_groups["group_order"] if "Johto" in name}
    assert set(johto_groups) <= {"gMapGroup_PolishedJohtoDemo"}
    visual_demo_maps = johto_groups.get("gMapGroup_PolishedJohtoDemo", [])
    expected_visual_maps = ["PolishedJohtoDemoOutdoor", "PolishedJohtoDemoLab"]
    expected_visual_maps += ["Polished" + name for name in new_bark_maps]
    assert not visual_demo_maps or visual_demo_maps == expected_visual_maps

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
        "source_species_constant_candidates": candidates,
        "source_species_candidates_checked": len(candidates),
        "dormant_tree_species_count": len(tree_species),
        "source_form_runtime_semantics_verified": runtime_tables_match,
        "unresolved_species_form_mappings": unresolved,
        "dormant_encounter_data_complete": runtime_tables_match,
        "dormant_generated_tables_match_source": runtime_tables_match,
        "dormant_source_map_assignments": source_map_assignments(maps),
        "dormant_source_map_dispatch_implemented": runtime_tables_match,
        "identity_mapping_only_engine_stats_and_learnsets_retained": True,
        "source_map_groups_registered_by_name": False,
        "registered_visual_demo_maps_without_headbutt_mapping": visual_demo_maps,
        "registered_source_tree_sets_not_wired": registered_source_sets,
        "johto_source_map_dispatch_verified": runtime_tables_match,
        "emerald_to_source_map_dispatch_verified": False,
        "headbutt_field_dispatch_verified": False,
        "complete_emerald_wild_encounters_emitted": False,
        "source_map_ids_remapped_to_emerald": False,
        "source_pokemon_form_ids_remapped_to_emerald": runtime_tables_match,
        "badge_scaled_levels_implemented": runtime_tables_match,
        "headbutt_field_move_implemented": False,
        "tree_encounter_selection_implemented": runtime_tables_match,
        "wing_case_inventory_implemented": False,
        "live_map_registered": False,
        "safe_to_enable": False,
    }
    (output / "johto_headbutt_encounters.review.json").write_text(
        json.dumps(report, indent=2) + "\n", encoding="utf-8"
    )
    return report


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="verify checked-in dormant C tables")
    args = parser.parse_args()
    source = load_sources()
    generated = render_runtime_tables(parse_encounters(source["encounters"]), parse_maps(source["maps"])[0])
    if args.check:
        if RUNTIME_TABLES.read_text() != generated:
            raise SystemExit("Headbutt C tables are stale; regenerate with this script")
    else:
        RUNTIME_TABLES.write_text(generated)
    print("PASS: 16 Headbutt distributions and 43 source map assignments; no live registration")
