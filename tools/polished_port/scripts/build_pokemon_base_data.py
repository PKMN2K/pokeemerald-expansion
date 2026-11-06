#!/usr/bin/env python3
"""Import default PC species data without replacing Expansion-only species.

The source BaseData INCLUDE order is joined to the same species/form indices as
the learnsets, not inferred from filenames. Cosmetic forms explicitly share the
source base row. Held items, evolution rules, artwork and ability implementations
are separate work; the manifest retains their outstanding scope.
"""
import argparse
import json
import re

import build_pokemon_learnsets as learnsets

ROOT = learnsets.ROOT
OUTPUT = ROOT / "src/data/polished_species_base_data.h"
FORMS = ROOT / "src/data/pokemon/species_info/polished_forms.h"
MANIFEST = ROOT / "tools/polished_port/manifests/pokemon_base_data.json"
EGG_GROUPS = {
    "MONSTER": "MONSTER", "WATER_1": "WATER_1", "BUG": "BUG", "FLYING": "FLYING",
    "GROUND": "FIELD", "FAIRY": "FAIRY", "PLANT": "GRASS", "HUMANSHAPE": "HUMAN_LIKE",
    "WATER_3": "WATER_3", "MINERAL": "MINERAL", "INDETERMINATE": "AMORPHOUS",
    "WATER_2": "WATER_2", "DITTO": "DITTO", "DRAGON": "DRAGON", "NONE": "NO_EGGS_DISCOVERED",
}
HATCH_RATES = ["FASTEST", "FASTER", "FAST", "MEDIUM_FAST", "MEDIUM_SLOW", "SLOW", "SLOWER", "SLOWEST"]
GENDER_FRACTIONS = ["0", "12_5", "25", "37_5", "50", "62_5", "75", "87_5", "100"]
STATS = ["HP", "Attack", "Defense", "Speed", "SpAttack", "SpDefense"]
EV_STATS = dict(zip(["HP", "Atk", "Def", "Spe", "SAt", "SDf"], STATS))
FORM_FAMILIES = {"ARBOK": "EKANS", "PIKACHU": "PIKACHU", "MAGIKARP": "MAGIKARP",
                 "GYARADOS": "MAGIKARP", "MEWTWO": "MEWTWO"}


def parse_base(text):
    lines = [s for s in learnsets.default_lines(text) if s]
    assert len(lines) == 11, lines

    def row(i, op, count=None):
        assert lines[i].startswith(op + " ") or lines[i] == op, lines[i]
        values = [s.strip() for s in lines[i][len(op):].strip().split(",") if s.strip()]
        assert count is None or len(values) == count, lines[i]
        return values

    stats = [int(x) for x in row(0, "db", 6)]
    types = row(1, "db", 2)
    catch, exp = int(row(2, "db", 1)[0]), int(row(3, "db", 1)[0])
    items = row(4, "db", 2)
    gender, hatch = row(5, "dn", 2)
    ability_species, *abilities = row(6, "abilities_for", 4)
    growth = row(7, "db", 1)[0]
    groups = row(8, "dn", 2)
    evs = {s: 0 for s in STATS}
    for item in row(9, "ev_yield"):
        match = re.fullmatch(r"([1-3]) (HP|Atk|Def|Spe|SAt|SDf)", item)
        assert match, item
        evs[EV_STATS[match[2]]] = int(match[1])
    teaching = row(10, "tmhm")
    if gender == "GENDER_UNKNOWN":
        gender_ratio = 255
    else:
        assert gender.startswith("GENDER_F") and gender[8:] in GENDER_FRACTIONS, gender
        # Source creation uses rnd(0..7) < ratio. The GBA compares all 256
        # low-byte personality values, so each eighth requires 32 values.
        # 254 and 255 remain the native female-only / genderless sentinels.
        gender_ratio = min(254, GENDER_FRACTIONS.index(gender[8:]) * 32)
    # Source daycare: ((wBaseEggSteps & 15) + 1) * 5. Unknown is nibble 15.
    hatch_index = (int(hatch) if hatch.isdecimal() else 15 if hatch == "HATCH_UNKNOWN"
                   else HATCH_RATES.index(hatch.removeprefix("HATCH_")))
    assert 0 <= hatch_index <= 15
    return dict(source_ability_species=ability_species, stats=stats, types=["TYPE_" + x for x in types], catch_rate=catch,
                base_exp=exp, source_held_items=items, gender=gender, gender_ratio=gender_ratio,
                source_hatch_rate=hatch, egg_cycles=(hatch_index + 1) * 5, friendship=70,
                abilities=["ABILITY_" + ("NONE" if x == "NO_ABILITY" else x) for x in abilities],
                growth_rate=growth, egg_groups=["EGG_GROUP_" + EGG_GROUPS[x.removeprefix("EGG_")] for x in groups],
                ev_yield=evs, source_teaching_moves=teaching)


def fields(data, indent="    "):
    rows = [f".base{s} = {v}," for s, v in zip(STATS, data["stats"])]
    rows += [".types = {" + ", ".join(data["types"]) + "},",
             f'.catchRate = {data["catch_rate"]},', f'.expYield = {data["base_exp"]},']
    rows += [f".evYield_{s} = {v}," for s, v in data["ev_yield"].items()]
    for name, key in [("genderRatio", "gender_ratio"), ("eggCycles", "egg_cycles"),
                      ("friendship", "friendship"), ("growthRate", "growth_rate")]:
        rows.append(f'.{name} = {data[key]},')
    rows += [".eggGroups = {" + ", ".join(data["egg_groups"]) + "},",
             ".abilities = {" + ", ".join(data["abilities"]) + "},"]
    return [indent + row for row in rows]


def presentation_template(text, species):
    """Keep the existing temporary native presentation, explicitly not source art."""
    match = re.search(r"    \[SPECIES_" + species + r"\] =\n    \{\n(.*?)\n    \},", text, re.S)
    assert match, species
    body = match[1]
    body = body[body.index("        .bodyColor"):]
    # Native Mega/primal/etc. transforms are not properties of these source forms.
    body = re.sub(r"^.*\.formChangeTable = .*\n?", "", body, flags=re.M)
    body = re.sub(r"^.*\.formSpeciesIdTable = .*\n?", "", body, flags=re.M)
    body += f"\n        .formSpeciesIdTable = s{species.title()}FormSpeciesIdTable,"
    return body


def build():
    files = learnsets.sources()
    source_manifest = json.loads(learnsets.build()[learnsets.MANIFEST])
    assert source_manifest["mapped_target_count"] == 391
    includes = re.findall(r'INCLUDE "([^"]+)"', files["data/pokemon/base_stats.asm"])
    assert len(includes) == len(re.findall(r"\bdw\s+\w+EvosAttacks", files["data/pokemon/evos_attacks_pointers.asm"]))
    parsed = {path: parse_base(files[path]) for path in set(includes)}
    constants = files["constants/pokemon_data_constants.asm"]
    assert "DEF BASE_HAPPINESS        EQU 70" in constants
    assert re.findall(r"\bconst GENDER_F(\w+)", constants) == GENDER_FRACTIONS
    hatch_order = re.findall(r"\bconst HATCH_(\w+)", constants)
    assert hatch_order == HATCH_RATES
    assert "DEF HATCH_UNKNOWN EQU %1111" in constants
    records = []
    for row in source_manifest["records"]:
        index = row["source_table_index"]
        if index is None:
            index = next(r["source_table_index"] for r in source_manifest["records"]
                         if r["source_species"] == row["source_species"] and r["source_form"] == "PLAIN_FORM")
        path = includes[index - 1]
        assert parsed[path]["source_ability_species"] in (
            row["source_species"], path.split("/")[-1].removesuffix(".asm").upper()), (path, row["source_species"])
        records.append({"source_species": row["source_species"], "source_form": row["source_form"],
                        "target_species": row["target_species"], "source_table_index": index,
                        "source_file": path, **parsed[path]})
    assert len(records) == 391
    available = set(re.findall(r"\b[A-Z][A-Z0-9_]+\b", "\n".join(
        p.read_text() for p in (ROOT / "include/constants").glob("*.h"))))
    for r in records:
        for symbol in r["abilities"] + r["types"] + r["egg_groups"] + [r["growth_rate"]]:
            assert symbol in available, symbol
    lines = ["/* Generated by build_pokemon_base_data.py; pinned default PC v3.2.3. */", ""]
    names = {}
    for r in records:
        if r["source_file"] in names:
            continue
        name = "sPolishedBase_" + r["source_file"].split("/")[-1].removesuffix(".asm")
        names[r["source_file"]] = name
        lines += [f"static const struct SpeciesBaseData {name} = {{"] + fields(r) + ["};", ""]
    lines += ["static const struct SpeciesBaseData *const gPolishedSpeciesBaseData[NUM_SPECIES] = {"]
    lines += [f'    [{r["target_species"]}] = &{names[r["source_file"]]},' for r in records]
    lines += ["};", ""]

    native = (ROOT / "src/data/pokemon/species_info/gen_1_families.h").read_text()
    forms = ["/* Generated data registrations. Native presentation is a documented temporary gap.",
             " * Do not report these graphics, Pokedex text or native evolution rules as imported. */", ""]
    new_forms = []
    for r in records:
        if r["source_species"] not in FORM_FAMILIES or r["source_form"] == "PLAIN_FORM":
            continue
        new_forms.append(r["target_species"])
        forms += [f'#if P_FAMILY_{FORM_FAMILIES[r["source_species"]]}', f'    [{r["target_species"]}] =',
                  "    {", "        .baseData = {"] + fields(r, "            ") + ["        },",
                  presentation_template(native, r["source_species"]), "    },", "#endif", ""]
    assert len(new_forms) == 30
    manifest = dict(source_commit=learnsets.PIN, source_variant="default; FAITHFUL undefined",
                    mapped_target_count=len(records), registered_forms=new_forms,
                    adapters={"gender_ratio": "Source rnd(0..7) < ratio maps to 32 personality values per eighth; 254 female-only, 255 genderless.",
                              "hatch_cycles": "Source daycare computes (hatch nibble + 1) * 5; HATCH_UNKNOWN = 15 => 80.",
                              "friendship": "BASE_HAPPINESS = 70 for newly created ordinary Pokemon.",
                              "save_ids": "Existing Pokemon IDs 1..1572 unchanged; 30 forms appended at 1573..1602. EGG rendering sentinel moves to 1603; saved eggs contain their actual species and the isEgg flag."},
                    remaining_gaps=["Original Pokemon graphics, palettes, Pokedex data and form animations.",
                                    "Held items and their encounter probabilities (source values retained here).",
                                    "Evolution conditions, egg moves, TM/HM/tutor compatibility and breeding rules.",
                                    "Ability effects, battle rules, IV/EV creation, hatch-step timing and source trainer attributes."],
                    records=records)
    return {OUTPUT: "\n".join(lines), FORMS: "\n".join(forms), MANIFEST: json.dumps(manifest, indent=2) + "\n"}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    for path, text in build().items():
        if args.check:
            assert path.is_file() and path.read_text() == text, f"Regenerate {path.relative_to(ROOT)}"
        else:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(text)
    print("PASS: 391 pinned source species/form base-data mappings and 30 registered forms")


if __name__ == "__main__":
    main()
