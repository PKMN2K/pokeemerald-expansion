#!/usr/bin/env python3
"""Source provenance, probabilities and register-flow boundaries for forms."""
import json
import re

from build_pokemon_forms import build, MANIFEST, wild_mon
from build_pokemon_learnsets import sources


def main():
    generated = build()
    for path, text in generated.items():
        assert path.read_text() == text, path
    data = json.loads(generated[MANIFEST])
    records = {r["target_species"]: r for r in data["records"]}
    assert len(records) == 31
    assert records["SPECIES_MAGIKARP_RAINDROP"]["form"] == 20
    assert records["SPECIES_GYARADOS_RED"]["form"] == 21
    assert records["SPECIES_ARBOK_ARIANA"]["form"] == 5
    assert records["SPECIES_DUDUNSPARCE_THREE_SEGMENT"]["form"] == 2
    pinned = sources()
    wild = pinned["engine/battle/random_wild_forms.asm"]
    dispatch = dict(re.findall(r"random_wild_form (\w+),\s*(\.\w+)", wild))
    assert dispatch == {"UNOWN": ".Unown", "MAGIKARP": ".Magikarp", "EKANS": ".EkansArbok",
                        "ARBOK": ".EkansArbok", "DUNSPARCE": ".Dudunsparce", "DUDUNSPARCE": ".Dudunsparce"}
    assert "ld a, 2\n\t; fallthrough\n.RandomForm:\n\tcall BattleRandomRange\n\tinc a" in wild
    assert "ld a, 25\n\tcall BattleRandomRange\n\tand a\n\tld a, DUDUNSPARCE_THREE_SEGMENT_FORM\n\tret z" in wild
    assert wild.index("jr nz, .done") < wild.index("ld hl, RandomWildSpeciesForms")
    egg = pinned["engine/events/daycare.asm"]
    inherit = egg.split("\n.inherit_mother_unless_samespecies\n", 1)[1]
    assert inherit.index("ld a, [wBreedMon1Species]") < inherit.index("call Random\n\tand 1")
    assert "ld a, [wBreedMotherOrNonDitto]\n\tjr nz, .use_mother" in inherit
    # The executable source, not the invalid-variant comment, defines the pin:
    # d holds the selected form; NONE of the intervening paths replace it.
    path = egg.split("\tld d, a\n\tpush bc\n\tfarcall CheckInvalidVariants", 1)[1].split("\n.form_ok\n", 1)[0]
    assert "ld a, d\n\tand FORM_MASK\n\tor b\n\tld [wCurForm], a" in path
    assert not re.search(r"\b(?:ld d,|pop de|inc d|dec d)", path)
    species_index = pinned["home/pokemon.asm"].split("GetSpeciesAndFormIndexFromHL::", 1)[1]
    assert species_index.lstrip().startswith("; input:") and "\tpush de" in species_index
    assert species_index.split(".helper:", 1)[0].count("\tpop de") == 2
    farword = pinned["home/copy.asm"].split("GetFarWord::", 1)[1].split("\tret", 1)[0]
    assert not re.search(r"\b(?:ld d,|pop de|inc d|dec d)", farword)
    fill = pinned["home/copy.asm"].split("_ByteFill::", 1)[1].split("GetFarWord::", 1)[0]
    assert not re.search(r"\b(?:ld d,|pop de|inc d|dec d)", fill)
    assert "\trst FarCall" in pinned["macros/rst.asm"]
    farcall = pinned["home/farcall.asm"].split("_RstFarCall::", 1)[1].split("_ReturnFarCall::", 1)[0]
    assert "\tpush de" in farcall and "\tpop de" in farcall
    trainer = pinned["engine/battle/read_trainer_party.asm"].split("SetDynamicForm:", 1)[1]
    assert trainer.index("and FORM_MASK\n\tret nz") < trainer.index("cp LOW(ARBOK)") < trainer.index("call RegionCheck")
    for name, label in (("EKANS", "Ekans"), ("MAGIKARP", "Magikarp"), ("DUNSPARCE", "Dunsparce")):
        assert f"{label}EggSpeciesMoves:\n\tdp {name}, NO_FORM" in pinned["data/pokemon/egg_moves.asm"]
    assert wild_mon(5, "EKANS", "ARBOK_JOHTO_FORM") == dict(min_level=5, max_level=5, species="SPECIES_EKANS", polished_form=1)
    assert wild_mon(5, "EKANS", "ARBOK_KANTO_FORM")["polished_form"] == 2
    assert wild_mon(20, "ARBOK", "ARBOK_KANTO_FORM")["species"] == "SPECIES_ARBOK_KANTO"
    assert "polished_form" not in wild_mon(5, "DUNSPARCE")
    assert wild_mon(5, "DUNSPARCE", "NO_FORM") == wild_mon(5, "DUNSPARCE")
    assert wild_mon(20, "MAGIKARP", "GYARADOS_RED_FORM")["species"] == "SPECIES_MAGIKARP"
    try:
        wild_mon(5, "EKANS", "UNKNOWN_FORM")
    except KeyError:
        pass
    else:
        raise AssertionError("Unknown source form was discarded")
    print("PASS: hidden-form source mapping, explicit encounters, 1/25 probability and pinned daycare quirk")


if __name__ == "__main__":
    main()
