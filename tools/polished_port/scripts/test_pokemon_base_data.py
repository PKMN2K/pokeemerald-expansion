#!/usr/bin/env python3
"""Source-oracle and integration checks for the first species-data phase."""
import json
import re
from pathlib import Path

from build_pokemon_base_data import ROOT, MANIFEST, build, parse_base
from build_pokemon_learnsets import sources


def main():
    files = sources()
    for path, expected in build().items():
        assert path.read_text() == expected, path
    manifest = json.loads(MANIFEST.read_text())
    records = {r["target_species"]: r for r in manifest["records"]}
    assert len(records) == 391 and len(manifest["registered_forms"]) == 30
    chikorita = records["SPECIES_CHIKORITA"]
    assert chikorita["abilities"] == ["ABILITY_OVERGROW", "ABILITY_NATURAL_CURE", "ABILITY_LEAF_GUARD"]
    assert chikorita["stats"] == [45, 49, 65, 45, 49, 65]
    assert chikorita["gender_ratio"] == 32
    # All source eighths must map exactly, including ratios unused by this roster.
    for source, threshold in [("F0", 0), ("F12_5", 32), ("F25", 64), ("F37_5", 96),
                              ("F50", 128), ("F62_5", 160), ("F75", 192),
                              ("F87_5", 224), ("F100", 254), ("UNKNOWN", 255)]:
        fixture = files[chikorita["source_file"]].replace("GENDER_F12_5", "GENDER_" + source)
        assert parse_base(fixture)["gender_ratio"] == threshold
    butterfree = records["SPECIES_BUTTERFREE"]
    assert butterfree["types"] == ["TYPE_BUG", "TYPE_PSYCHIC"]
    assert butterfree["stats"] == [60, 45, 50, 80, 110, 110]
    assert butterfree["base_exp"] == 185
    assert records["SPECIES_NOCTOWL"]["types"] == ["TYPE_GHOST", "TYPE_FLYING"]
    assert records["SPECIES_LEDIAN"]["stats"] == [55, 95, 50, 85, 35, 110]
    armored = records["SPECIES_MEWTWO_ARMORED"]
    assert armored["stats"] == [106, 110, 100, 130, 134, 100]
    assert armored["types"] == ["TYPE_PSYCHIC", "TYPE_STEEL"]
    assert armored["ev_yield"] == dict(HP=0, Attack=0, Defense=1, Speed=0, SpAttack=2, SpDefense=1)
    assert (armored["gender_ratio"], armored["egg_cycles"]) == (255, 80)
    assert records["SPECIES_NIDORAN_F"]["gender_ratio"] == 254
    assert records["SPECIES_NIDORAN_M"]["gender_ratio"] == 0
    assert records["SPECIES_MR_MIME_GALAR"]["egg_cycles"] == 25  # source literal nibble 4
    assert records["SPECIES_MAGIKARP_SKELLY"]["source_file"] == records["SPECIES_MAGIKARP"]["source_file"]
    assert records["SPECIES_ARBOK_KOGA"]["source_file"] == records["SPECIES_ARBOK"]["source_file"]
    assert records["SPECIES_GYARADOS_RED"]["egg_cycles"] == 5
    assert records["SPECIES_TAUROS_PALDEA_BLAZE"]["types"] == ["TYPE_FIGHTING", "TYPE_FIRE"]
    assert records["SPECIES_TAUROS_PALDEA_AQUA"]["types"] == ["TYPE_FIGHTING", "TYPE_WATER"]
    assert "SPECIES_MUDKIP" not in records
    # Extra variants or source rows must not be silently accepted by the converter.
    for corrupt in [files[chikorita["source_file"]] + "\nunknown_macro 1\n",
                    files[chikorita["source_file"]].replace("DEF(FAITHFUL)", "DEF(UNREVIEWED)")]:
        try:
            parse_base(corrupt)
        except AssertionError:
            pass
        else:
            raise AssertionError("Unknown source syntax was accepted")

    # Catch direct gameplay reads reintroduced outside the shared source accessor.
    migrated = set("baseHP baseAttack baseDefense baseSpeed baseSpAttack baseSpDefense types catchRate expYield genderRatio eggCycles friendship growthRate eggGroups abilities evYield_HP evYield_Attack evYield_Defense evYield_Speed evYield_SpAttack evYield_SpDefense".split())
    migrated.add("base")  # Battle Dome's CALC_STAT macro parameter is a base-stat field.
    exceptions = []
    for path in (ROOT / "src").glob("*.c"):
        text = path.read_text()
        for match in re.finditer(r"gSpeciesInfo\[", text):
            end = match.end()
            depth = 1
            while depth:
                if text[end] == "[":
                    depth += 1
                elif text[end] == "]":
                    depth -= 1
                end += 1
            field = re.match(r"\.(\w+)", text[end:])
            if field and field[1] in migrated:
                raw = text[match.start():end + len(field[0])]
                assert path.name == "pokemon.c" and raw == "gSpeciesInfo[species].baseHP", (path, raw)
                exceptions.append(raw)  # IsSpeciesEnabled must avoid recursive sanitation.
    assert len(exceptions) == 1
    assert "GetSpeciesBaseData(species)" in (ROOT / "src/trainer_util.c").read_text()
    print("PASS: source rebalances, gender/hatch adapters, all form mappings and gameplay accessor coverage")


if __name__ == "__main__":
    main()
