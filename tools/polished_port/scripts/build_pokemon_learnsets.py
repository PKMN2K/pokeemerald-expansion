#!/usr/bin/env python3
"""Import the pinned default Polished Crystal level-up and evolution moves.

The source species/form pointer tables, rather than similar labels or numeric
IDs, define which learnset belongs to each Pokemon. Missing target forms stay
explicitly unresolved in the manifest; they are never replaced by a base form.
"""
import argparse
import base64
import hashlib
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
FIXTURES = ROOT / "tools/polished_port/fixtures/pokemon_data"
PIN = "3fa43192379df5c3e7b09a08e4d5d79af4f02f42"
MANIFEST = ROOT / "tools/polished_port/manifests/pokemon_learnsets.json"
OUTPUT = ROOT / "src/data/polished_level_up_learnsets.h"
CUSTOM_MOVES = ROOT / "src/data/polished_custom_moves.h"
MOVE_ALIASES = {
    "PSYCHIC_M": "PSYCHIC", "DAZZLINGLEAM": "DAZZLING_GLEAM",
    "DISARM_VOICE": "DISARMING_VOICE",
    "FURY_STRIKES": "POLISHED_FURY_STRIKES",
    "HEALINGLIGHT": "POLISHED_HEALING_LIGHT",
    "FRESH_SNACK": "POLISHED_FRESH_SNACK",
}
SPECIES_ALIASES = {
    "FARFETCH_D": "FARFETCHD", "SIRFETCH_D": "SIRFETCHD",
    "MR__MIME": "MR_MIME", "MR__RIME": "MR_RIME", "MIME_JR_": "MIME_JR",
}
SPECIAL_FORMS = {
    ("TAUROS", "PALDEAN_FORM"): "TAUROS_PALDEA_COMBAT",
    ("TAUROS", "TAUROS_PALDEAN_FIRE_FORM"): "TAUROS_PALDEA_BLAZE",
    ("TAUROS", "TAUROS_PALDEAN_WATER_FORM"): "TAUROS_PALDEA_AQUA",
    ("URSALUNA", "URSALUNA_BLOODMOON_FORM"): "URSALUNA_BLOODMOON",
    ("DUDUNSPARCE", "DUDUNSPARCE_THREE_SEGMENT_FORM"): "DUDUNSPARCE_THREE_SEGMENT",
    ("PICHU", "PICHU_SPIKY_EARED_FORM"): "PICHU_SPIKY_EARED",
}
REGIONS = {"ALOLAN_FORM": "ALOLA", "GALARIAN_FORM": "GALAR",
           "HISUIAN_FORM": "HISUI", "PALDEAN_FORM": "PALDEA"}


def custom_moves(files):
    rows = [line.removeprefix("move ").split(",") for line in
            default_lines(files["data/moves/moves.asm"].split("Moves::", 1)[1])
            if line.startswith("move ")]
    rows = [[x.strip() for x in row] for row in rows]
    names = re.findall(r'\bli "([^"]+)"', "\n".join(default_lines(files["data/moves/names.asm"])))
    pointer_body = files["data/moves/descriptions.asm"].split("\tassert_table_length NUM_ATTACKS", 1)[0]
    pointers = re.findall(r"\bdw (\w+Description)", "\n".join(default_lines(pointer_body)))
    assert len(rows) == len(names) == len(pointers)
    lines = ["/* Generated from pinned default Polished Crystal move data. */"]
    metadata = []
    adapters = {
        "FURY_STRIKES": ("EFFECT_HIT", "TARGET_SELECTED", "FurySwipes"),
        "HEALINGLIGHT": ("EFFECT_MORNING_SUN", "TARGET_USER", "MorningSun"),
        "FRESH_SNACK": ("EFFECT_RESTORE_HP", "TARGET_USER", "SoftBoiled"),
    }
    for row, name, description in zip(rows, names, pointers):
        source, effect, power, move_type, accuracy, pp, chance, category = row
        if source not in adapters:
            continue
        target_effect, target, animation = adapters[source]
        body = files["data/moves/descriptions.asm"].split(description + ":", 1)[1].split("\tdone", 1)[0]
        text = "\\n".join(re.findall(r'(?:text|next) "([^"]+)"', body))
        assert text and chance == "0"
        metadata.append({"source_move": source, "source_effect": effect, "target_move": "MOVE_" + MOVE_ALIASES[source],
                         "name": name, "type": move_type, "power": int(power), "accuracy": int(accuracy),
                         "pp": int(pp), "category": category, "source_description": text,
                         "presentation_gap": "Native animation adapter; source battle graphics/animation still pending."})
        lines.extend([f"    [MOVE_{MOVE_ALIASES[source]}] =", "    {",
            f'        .name = COMPOUND_STRING("{name}"),',
            f'        .description = COMPOUND_STRING("{text}"),',
            f"        .effect = {target_effect},", f"        .power = {power},",
            f"        .type = TYPE_{move_type},", f"        .accuracy = {0 if accuracy == '-1' else accuracy},",
            f"        .pp = {pp},", f"        .target = {target},", f"        .category = DAMAGE_CATEGORY_{category},"])
        if category == "STATUS":
            lines.extend(["        .healingMove = TRUE,", "        .snatchAffected = TRUE,",
                          "        .ignoresProtect = TRUE,", "        .mirrorMoveBanned = TRUE,"])
        else:
            lines.extend(["        .multiHit = TRUE,", "        .makesContact = TRUE,"])
        lines.extend(["        .metronomeBanned = !POLISHED_JOHTO_DEMO,",
                      f"        .battleAnimScript = gBattleAnimMove_{animation},", "    },"])
    assert len(metadata) == 3
    return "\n".join(lines) + "\n", metadata


def sources():
    manifest = json.loads((FIXTURES / "sources.json").read_text())
    assert manifest["commit"] == PIN
    result = {}
    for entry in manifest["files"]:
        data = base64.b64decode((FIXTURES / entry["fixture"]).read_text(), validate=False)
        sha = hashlib.sha1(b"blob " + str(len(data)).encode() + b"\0" + data).hexdigest()
        assert sha == entry["git_blob_sha1"], entry["path"]
        result[entry["path"]] = data.decode()
    return result


def default_lines(text):
    """Resolve release-default FAITHFUL conditionals and reject unknown ones."""
    enabled = [True]
    for raw in text.splitlines():
        line = raw.split(";", 1)[0].strip()
        if line.startswith("if "):
            assert line in ("if DEF(FAITHFUL)", "if !DEF(FAITHFUL)"), line
            enabled.append(line == "if !DEF(FAITHFUL)")
        elif line == "else":
            assert len(enabled) > 1
            enabled[-1] = not enabled[-1]
        elif line == "endc":
            assert len(enabled) > 1
            enabled.pop()
        elif all(enabled):
            yield line
    assert enabled == [True]


def target_species(species, form, available):
    name = SPECIES_ALIASES.get(species, species)
    if form == "PLAIN_FORM":
        target = name
    elif (species, form) in SPECIAL_FORMS:
        target = SPECIAL_FORMS[species, form]
    elif form in REGIONS:
        target = name + "_" + REGIONS[form]
    elif species == "UNOWN":
        target = form.removesuffix("_FORM")
    else:
        return None
    target = "SPECIES_" + target
    assert target in available, (species, form, target)
    return target


def build():
    files = sources()
    constants = files["constants/pokemon_constants.asm"].split("DEF NUM_SPECIES", 1)[0]
    base = []
    for line in constants.splitlines():
        match = re.match(r"\s*const\s+(\w+)", line)
        if match:
            base.append(match[1])
        elif re.match(r"\s*const_skip\b", line):
            base.append(None)
    variants = files["data/pokemon/variant_forms.asm"]
    cosmetics, variants = variants.split("VariantSpeciesAndFormTable:", 1)
    cosmetics = re.findall(r"\bdp\s+(\w+),\s*(\w+)", cosmetics)
    variants = re.findall(r"\bdp\s+(\w+),\s*(\w+)", variants)
    pointers = re.findall(r"\bdw\s+(\w+)EvosAttacks", files["data/pokemon/evos_attacks_pointers.asm"])
    evolution_moves = [line.split()[1] for line in default_lines(files["data/pokemon/evolution_moves.asm"])
                       if line.startswith("db ")]
    assert len(pointers) == len(base) + len(variants) == len(evolution_moves)
    learnsets = {}
    current = None
    body = files["data/pokemon/evos_attacks.asm"].split("EvosAttacks::", 1)[1]
    for line in default_lines(body):
        if line.startswith("evos_attacks "):
            current = line.split()[1]
            assert current not in learnsets
            learnsets[current] = []
        elif line.startswith("learnset "):
            match = re.fullmatch(r"learnset (\d+), (\w+)", line)
            assert match and current, line
            level, move = int(match[1]), match[2]
            assert 1 <= level <= 100
            assert not learnsets[current] or level >= learnsets[current][-1][0], current
            learnsets[current].append([level, move])
        elif line.endswith("EvosAttacks:"):
            # These source labels occur after the preceding species' evolution
            # entries, immediately before the shared learnset bytes.
            alias = line.removesuffix("EvosAttacks:")
            if alias != "Egg":
                assert (alias, current) in (("MrRime", "MrMimeGalarian"), ("Farigiraf", "Girafarig")), alias
                assert not learnsets[current]
                learnsets[alias] = learnsets[current]
    assert all(v for k, v in learnsets.items()), "Empty or unresolved learnset"
    move_header = (ROOT / "include/constants/moves.h").read_text()
    move_ids = set(re.findall(r"\bMOVE_\w+", move_header))
    species_ids = set(re.findall(r"\bSPECIES_\w+", (ROOT / "include/constants/species.h").read_text()))

    def move_id(move):
        if move == "NO_MOVE":
            return "MOVE_NONE"
        target = "MOVE_" + MOVE_ALIASES.get(move, move)
        assert target in move_ids, (move, target)
        return target

    records = []
    for index, ((species, form), label, evo) in enumerate(zip(
            [(s, "PLAIN_FORM") for s in base] + variants, pointers, evolution_moves), 1):
        if species in (None, "EGG"):
            assert label == "Egg" and evo == "NO_MOVE"
            continue
        assert label in learnsets, (species, form, label)
        records.append({"source_table_index": index, "source_species": species,
            "source_form": form, "source_learnset": label,
            "target_species": target_species(species, form, species_ids),
            "evolution_move": move_id(evo),
            "moves": [{"level": lv, "source_move": mv, "target_move": move_id(mv)}
                      for lv, mv in learnsets[label]]})
    for species, form in cosmetics:
        original = next(r for r in records if r["source_species"] == species and r["source_form"] == "PLAIN_FORM")
        records.append({**original, "source_table_index": None, "source_form": form,
                        "target_species": target_species(species, form, species_ids),
                        "cosmetic_alias": True})
    targets = [r["target_species"] for r in records if r["target_species"]]
    assert len(targets) == len(set(targets))
    arrays = {}
    lines = ["/* Generated by build_pokemon_learnsets.py; Polished Crystal v3.2.3, default build. */", ""]
    for record in records:
        key = (record["source_learnset"], record["evolution_move"])
        if record["target_species"] is None or key in arrays:
            continue
        name = "sPolishedLearnset_" + key[0] + "_" + key[1].removeprefix("MOVE_")
        arrays[key] = name
        lines.append("static const struct LevelUpMove " + name + "[] = {")
        if record["evolution_move"] != "MOVE_NONE":
            lines.append(f"    {{.move = {record['evolution_move']}, .level = 0}},")
        lines.extend(f"    {{.move = {m['target_move']}, .level = {m['level']}}}," for m in record["moves"])
        lines.extend(["    {.move = LEVEL_UP_MOVE_END, .level = 0},", "};", ""])
    lines.append("const struct LevelUpMove *const gPolishedLevelUpLearnsets[NUM_SPECIES] = {")
    for record in records:
        if record["target_species"]:
            name = arrays[record["source_learnset"], record["evolution_move"]]
            lines.append(f"    [{record['target_species']}] = {name},")
    lines.extend(["};", ""])
    custom_header, custom_metadata = custom_moves(files)
    manifest = {"source_commit": PIN, "source_variant": "default; FAITHFUL undefined",
                "scope": "Level-up and evolution moves only; stats, evolution conditions, egg/TM/tutor data and battle effects are separate gaps.",
                "mapped_target_count": len(targets),
                "unmapped_forms": [{"species": r["source_species"], "form": r["source_form"],
                    "reason": "No exact source form identity is registered in the target yet; do not substitute its base species."}
                    for r in records if not r["target_species"]], "custom_moves": custom_metadata, "records": records}
    return {OUTPUT: "\n".join(lines), CUSTOM_MOVES: custom_header, MANIFEST: json.dumps(manifest, indent=2) + "\n"}


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
    print("PASS: pinned default source learnsets, evolution moves and explicit species/form mappings")


if __name__ == "__main__":
    main()
