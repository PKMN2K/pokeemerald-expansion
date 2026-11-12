#!/usr/bin/env python3
"""Check source-variant selection, identity coverage and important source cases."""
import json
import re

from build_pokemon_learnsets import ROOT, MANIFEST, MOVE_IDENTITIES, build, default_lines, sources, move_targets


def main():
    files = sources()  # Hash-check every pinned input before interpreting it.
    for path, expected in build().items():
        assert path.read_text() == expected, path
    manifest = json.loads(MANIFEST.read_text())
    assert manifest["source_variant"] == "default; FAITHFUL undefined"
    assert len(manifest["records"]) == 391
    assert manifest["mapped_target_count"] == 391
    assert len(manifest["unmapped_forms"]) == 0
    assert len({r["target_species"] for r in manifest["records"] if r["target_species"]}) == 391
    records = {(r["source_species"], r["source_form"]): r for r in manifest["records"]}

    def levels(species, form="PLAIN_FORM"):
        return [(m["level"], m["source_move"]) for m in records[species, form]["moves"]]

    assert levels("CYNDAQUIL")[:4] == [(1, "TACKLE"), (1, "LEER"), (6, "SMOKESCREEN"), (10, "EMBER")]
    assert (12, "HEALINGLIGHT") in levels("CHIKORITA")
    assert [(lv, mv) for lv, mv in levels("CHIKORITA") if lv == 17] == [(17, "REFLECT"), (17, "LIGHT_SCREEN")]
    assert (29, "FRESH_SNACK") in levels("TOGEPI")
    assert (75, "IRON_HEAD") in levels("BLASTOISE")  # Non-FAITHFUL release branch.
    assert records["BLASTOISE", "PLAIN_FORM"]["evolution_move"] == "MOVE_FLASH_CANNON"
    assert levels("VOLTORB", "HISUIAN_FORM") != levels("VOLTORB")
    assert levels("MR__RIME") == levels("MR__MIME", "GALARIAN_FORM")
    assert levels("FARIGIRAF") == levels("GIRAFARIG")
    assert records["MEWTWO", "MEWTWO_ARMORED_FORM"]["target_species"] == "SPECIES_MEWTWO_ARMORED"
    assert records["GYARADOS", "GYARADOS_RED_FORM"]["target_species"] == "SPECIES_GYARADOS_RED"
    assert records["TAUROS", "TAUROS_PALDEAN_FIRE_FORM"]["target_species"] == "SPECIES_TAUROS_PALDEA_BLAZE"
    assert records["TAUROS", "TAUROS_PALDEAN_WATER_FORM"]["target_species"] == "SPECIES_TAUROS_PALDEA_AQUA"

    identities = json.loads(MOVE_IDENTITIES.read_text())["records"]
    assert len(identities) == 256
    assert identities[249] == dict(source_id=249, source_constant="ROCK_SMASH", source_move="BRICK_BREAK",
                                   source_aliases=["BRICK_BREAK"], target_move="MOVE_BRICK_BREAK")
    targets = move_targets(files)
    assert targets["ROCK_SMASH"] == targets["BRICK_BREAK"] == "MOVE_BRICK_BREAK"
    assert targets["NO_MOVE"] == "MOVE_NONE"
    assert "TERA_BLAST" not in targets  # Optional native moves are not source identities.
    affected = {(r["target_species"], m["level"]) for r in records.values() for m in r["moves"]
                if m["target_move"] == "MOVE_BRICK_BREAK"}
    assert affected == {("SPECIES_CUBONE", 41), ("SPECIES_MAROWAK", 49), ("SPECIES_HITMONLEE", 21),
                        ("SPECIES_PINSIR", 26), ("SPECIES_SIRFETCHD", 40), ("SPECIES_MAROWAK_ALOLA", 49),
                        ("SPECIES_FARFETCHD_GALAR", 40), ("SPECIES_ZAPDOS_GALAR", 43)}
    assert all(m["target_move"] != "MOVE_ROCK_SMASH" for r in records.values() for m in r["moves"])

    assert list(default_lines("if DEF(FAITHFUL)\nwrong\nelse\nright\nendc")) == ["right"]
    try:
        list(default_lines("if DEF(UNREVIEWED_VARIANT)\nwrong\nendc"))
    except AssertionError:
        pass
    else:
        raise AssertionError("Unknown source conditional was silently accepted")

    custom = {m["source_move"]: m for m in manifest["custom_moves"]}
    assert (custom["FURY_STRIKES"]["power"], custom["FURY_STRIKES"]["accuracy"], custom["FURY_STRIKES"]["pp"]) == (20, 85, 15)
    assert custom["HEALINGLIGHT"]["type"] == "NORMAL" and custom["HEALINGLIGHT"]["pp"] == 5
    assert custom["FRESH_SNACK"]["pp"] == 10
    # Retain the source mechanics used by the native battle regression cases.
    loop = files["engine/battle/effect_commands.asm"].split("BattleCommand_startloop:", 1)[1].split("BattleCommand_supereffectivetext:", 1)[0]
    assert re.search(r"ld a, 6\s+call BattleRandomRange\s+cp 4", loop)
    heal = files["engine/battle/move_effects/healinglight.asm"]
    assert re.search(r"call GetThirdMaxHP\s+sla c\s+rl b", heal)
    umbrella = files["home/battle.asm"].split("GetWeatherAfterUserUmbrella::", 1)[1].split("GetWeatherAfterCloudNine::", 1)[0]
    assert re.search(r"cp WEATHER_HAIL\s+ret z\s+cp WEATHER_SANDSTORM\s+ret z", umbrella)
    print("PASS: 391 source identities audited; 391 live learnsets, all source forms mapped, default source branch and 3 custom move definitions")


if __name__ == "__main__":
    main()
