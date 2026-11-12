#!/usr/bin/env python3
"""Audit the pinned learning-data import and its non-native source edge cases."""
import json
import re

from build_pokemon_learning import MANIFEST, build
from build_pokemon_learnsets import sources


def main():
    files = sources()
    for path, expected in build().items():
        assert path.read_text() == expected, path
    manifest = json.loads(MANIFEST.read_text())
    rows = {r["target_species"]: r for r in manifest["records"]}
    assert len(rows) == 391
    assert rows["SPECIES_BULBASAUR"]["egg_moves"] == [
        "MOVE_CHARM", "MOVE_PETAL_DANCE", "MOVE_LIGHT_SCREEN", "MOVE_SAFEGUARD", "MOVE_POWER_WHIP"]
    assert rows["SPECIES_EEVEE"]["egg_moves"] == ["MOVE_CHARM"]
    assert rows["SPECIES_MAGIKARP"]["egg_moves"] == []
    assert rows["SPECIES_MAGIKARP_RAINDROP"]["teaching_moves"] == []
    assert rows["SPECIES_ARBOK_ARIANA"]["egg_moves"] == rows["SPECIES_EKANS"]["egg_moves"]
    assert rows["SPECIES_UNOWN_Q"]["teaching_moves"] == ["MOVE_HIDDEN_POWER"]
    assert rows["SPECIES_SLOWPOKE"]["egg_moves"][:2] == ["MOVE_SLEEP_POWDER", "MOVE_DOUBLE_SLAP"]
    assert "MOVE_SLEEP_POWDER" not in rows["SPECIES_SLOWPOKE_GALAR"]["egg_moves"]
    assert "MOVE_FLARE_BLITZ" in rows["SPECIES_VULPIX"]["egg_moves"]
    assert "MOVE_FLARE_BLITZ" not in rows["SPECIES_VULPIX_ALOLA"]["egg_moves"]
    assert "MOVE_ICE_BEAM" in rows["SPECIES_VULPIX_ALOLA"]["egg_moves"]
    assert "MOVE_POLISHED_FURY_STRIKES" in rows["SPECIES_RATTATA"]["egg_moves"]
    catalog = {(r["kind"], r["number"]): r["target_move"] for r in manifest["teaching_catalog"]}
    assert len(catalog) == 112
    assert catalog["TM", 1] == "MOVE_DYNAMICPUNCH"
    assert catalog["TM", 29] == "MOVE_PSYCHIC"
    assert catalog["TM", 31] == "MOVE_BRICK_BREAK"
    assert catalog["TM", 51] == "MOVE_ROOST"
    assert catalog["TM", 70] == "MOVE_FLASH"
    assert catalog["HM", 5] == "MOVE_WHIRLPOOL"
    assert catalog["MT", 31] == "MOVE_ZEN_HEADBUTT"
    assert sum("MOVE_BRICK_BREAK" in r["teaching_moves"] for r in rows.values()) == 207
    for row in rows.values():
        assert set(row["teaching_moves"]) <= set(catalog.values())
        assert len(row["egg_moves"]) == len(set(row["egg_moves"]))
        assert "MOVE_TERA_BLAST" not in row["teaching_moves"]
        assert "MOVE_ROCK_SMASH" not in row["teaching_moves"]
    # Retain the actual engine contracts behind the native behavioral tests.
    inherit = files["engine/pokemon/breeding.asm"].split("InitEggMoves:", 1)[1].split("InheritLevelMove:", 1)[0]
    assert inherit.index("wBreedMon1Moves") < inherit.index("call InheritLevelMove")
    assert inherit.index("call GetFatherAddr") < inherit.index("call GetMotherAddr")
    assert "LIGHT_BALL" not in inherit and "TMHMMoves" not in inherit
    skip = files["engine/pokemon/breeding.asm"].split("InheritEggMove:", 1)[1].split(".loop", 1)[0]
    assert re.search(r"call GetFarWord\s+inc hl\s+inc hl", skip)
    assert r"db LOW(\1), HIGH(\1) << MON_EXTSPECIES_F | \2" in files["macros/data.asm"]
    assert "predef CanLearnTMHMMove" in files["engine/events/move_tutor.asm"]
    print("PASS: source learning identities, empty lists, regional differences, Slowpoke byte quirk and inheritance contracts")


if __name__ == "__main__":
    main()
