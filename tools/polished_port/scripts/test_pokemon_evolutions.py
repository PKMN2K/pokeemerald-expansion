#!/usr/bin/env python3
"""Independent source-regression examples for the evolution converter."""
import json

from build_pokemon_evolutions import build, MANIFEST, OUTPUT, parse
from build_pokemon_learnsets import sources


def main():
    generated = build()
    for path, content in generated.items():
        assert path.read_text() == content, path
    data = json.loads(generated[MANIFEST])
    records = {r["target_species"]: r for r in data["records"]}
    assert data["mapped_target_count"] == len(records) == 391
    assert data["source_evolution_row_count"] == 190
    assert data["friendship_threshold"] == 160
    assert not records["SPECIES_PICHU_SPIKY_EARED"]["rules"]
    assert not records["SPECIES_MR_RIME"]["rules"]
    assert not records["SPECIES_FARIGIRAF"]["rules"]
    assert not records["SPECIES_FARFETCHD"]["rules"]
    assert records["SPECIES_FLAAFFY"]["rules"][0]["argument"] == "36"
    assert records["SPECIES_RATTATA_ALOLA"]["rules"][0]["method"] == "LEVEL"
    assert records["SPECIES_MEOWTH_ALOLA"]["rules"][0]["argument"] == "28"
    assert records["SPECIES_PRIMEAPE"]["rules"][0]["argument"] == "OUTRAGE"
    assert records["SPECIES_MIME_JR"]["rules"][0]["argument"] == "ICE_PATH"
    assert records["SPECIES_MIME_JR"]["rules"][1]["argument"] == "30"
    assert records["SPECIES_GIRAFARIG"]["rules"][0]["argument"] == "ZEN_HEADBUTT"
    assert records["SPECIES_KOFFING"]["rules"][0]["argument"] == "CHARCOAL"
    assert records["SPECIES_KOFFING"]["rules"][0]["form"] == "GALARIAN_FORM"
    assert records["SPECIES_MAGIKARP_SKELLY"]["rules"] == records["SPECIES_MAGIKARP"]["rules"]
    assert len(records["SPECIES_EEVEE"]["rules"]) == 12
    assert "SPECIES_MUDKIP" not in records
    assert len(data["locations"]) == 20
    assert not any(r["target_map"] for r in data["locations"]), "Update coverage when evolution landmarks are ported"
    pinned = sources()
    assert "(1 << EVOLVE_IN_BATTLE_OPT)" in pinned["data/options/default_options.asm"]
    timing = pinned["engine/battle/core.asm"].split("\n.defer_evolve\n", 1)[0].rsplit("call CheckEnemyTrainerDefeated", 1)[1]
    assert timing.index("bit EVOLVE_IN_BATTLE_OPT") < timing.index("call EvolveDuringBattle")
    scene = pinned["engine/battle/evolve_battle.asm"]
    assert scene.index("farcall TryToEvolve") < scene.index("call .load_mon_data")
    assert scene.index("ret c ; cancelled") < scene.index("call ResetPlayerAbility") < scene.index("farjp RunEntryAbilitiesInner")
    abilities = pinned["engine/battle/abilities.asm"]
    assert abilities.index("call HasOpponentFainted") < abilities.index("BattleEntryAbilitiesNonfainted:")
    # Unknown source syntax must not silently become a native evolution.
    text = sources()["data/pokemon/evos_attacks.asm"]
    for corrupt in [text.replace("EVOLVE_LEVEL, 16", "EVOLVE_UNKNOWN, 16", 1),
                    text.replace("DEF(FAITHFUL)", "DEF(UNREVIEWED)")]:
        try:
            parse(corrupt)
        except AssertionError:
            pass
        else:
            raise AssertionError("Unreviewed source evolution accepted")
    print("PASS: 190 source evolution rows, terminal aliases, regional rules and location isolation")


if __name__ == "__main__":
    main()
