"""Exercise Elm/Lyra story branches with real input and read-only ROM/RAM probes.

Called by test_new_bark_emulator.py. Snapshots originate in this freshly booted
ROM, including cartridge save data. No injected party, flags or script context.
"""
from functools import lru_cache
import json
import re
import struct
import subprocess

from build_johto_demo import ROOT
from build_new_bark import source_files
from test_johto_demo_emulator import A, B, START, UP, RIGHT, DOWN
from test_new_bark_emulator import (walk, travel, actors, resident_id, save_game,
    continue_game, close_resident_dialogue, check_resident_palette, player_state)


@lru_cache
def layout():
    """Ask the ARM compiler for this revision's layouts; never patch the ROM."""
    fields = {
        "vars": "offsetof(struct SaveBlock1, vars)",
        "flags": "offsetof(struct SaveBlock1, flags)",
        "mon_size": "sizeof(struct Pokemon)",
        "level": "offsetof(struct Pokemon, level)",
        "secure": "offsetof(struct BoxPokemon, secure)",
        "checksum": "offsetof(struct BoxPokemon, checksum)",
        "substruct_size": "sizeof(union PokemonSubstruct)",
        "Cyndaquil": "SPECIES_CYNDAQUIL", "Totodile": "SPECIES_TOTODILE",
        "Chikorita": "SPECIES_CHIKORITA", "berry": "ITEM_ORAN_BERRY",
        "town_scene": "VAR_POLISHED_NEW_BARK_SCENE", "lab_scene": "VAR_POLISHED_ELMS_LAB_SCENE",
        "starter": "VAR_POLISHED_STARTER", "contact": "FLAG_POLISHED_HAS_ELM_CONTACT",
    }
    source = ('#include "global.h"\n#include "constants/items.h"\n#include "constants/species.h"\n'
              '#include "constants/vars.h"\n#include "constants/flags.h"\n'
              'const u32 layout[] = {' + ','.join(fields.values()) + '};\n')
    result = subprocess.run(["arm-none-eabi-gcc", "-mcpu=arm7tdmi", "-mthumb", "-mabi=apcs-gnu",
        "-DMODERN=1", "-DEMERALD", "-std=gnu17", "-iquote", "include", "-iquote", ".", "-S", "-x", "c", "-o", "-", "-"],
        cwd=ROOT, input=source, text=True, capture_output=True, check=True)
    values = [int(x) for x in re.findall(r"\.word\s+(\d+)", result.stdout)]
    assert len(values) == len(fields)
    return dict(zip(fields, values))


def var(game, name):
    data = layout()
    addr = game.u32(game.sym["gSaveBlock1Ptr"]) + data["vars"] + 2 * (data[name] - 0x4000)
    return struct.unpack("<H", game.read(addr, 2))[0]


def party(game):
    data = layout()
    count = game.read(game.u32(game.sym["gPlayerPartyCountPtr"]), 1)[0]
    base = game.u32(game.sym["gPlayerPartyPtr"])
    result = []
    for index in range(count):
        raw = game.read(base + index * data["mon_size"], data["mon_size"])
        personality, trainer = struct.unpack_from("<II", raw)
        secure = raw[data["secure"]:data["secure"] + 4 * data["substruct_size"]]
        decoded = b"".join(struct.pack("<I", word ^ personality ^ trainer)
            for word in struct.unpack(f"<{len(secure) // 4}I", secure))
        assert sum(struct.unpack(f"<{len(decoded) // 2}H", decoded)) & 0xFFFF == struct.unpack_from("<H", raw, data["checksum"])[0]
        offset = game.read(game.sym["sSubstructOffsets"] + personality % 24, 1)[0] * data["substruct_size"]
        species, item = struct.unpack_from("<HH", decoded, offset)
        result.append({"species": species & 0x7FF, "item": item & 0x3FF, "level": raw[data["level"]],
            "personality": personality, "trainer": trainer, "nickname": raw[8:18].hex()})
    return result


def script_active(game):
    return game.read(game.sym["sGlobalScriptContext"] + 1, 1) != b"\0"


def task_active(game, name):
    raw = game.read(game.sym["gTasks"], 16 * 40)
    return any(raw[i + 4] and struct.unpack_from("<I", raw, i)[0] == game.sym[name] | 1
               for i in range(0, len(raw), 40))


def text_matches(game, label):
    text = game.read(game.sym[label], 1024).split(b"\xff", 1)[0].split(b"\xfd", 1)[0]
    assert text, label
    return game.read(game.sym["gStringVar4"], len(text)) == text


def advance(game, condition, limit=200):
    for _ in range(limit):
        if condition():
            return
        game.press(A, 45)
    raise AssertionError(f"Dialogue stalled at {game.position()}: {game.read(game.sym['gStringVar4'], 80).hex()}")


def choice_prompt(game, label):
    advance(game, lambda: task_active(game, "Task_HandleYesNoInput") and text_matches(game, label))
    game.frames(30)  # The native yes/no task has an input delay.


def remaining(game, lab):
    live = actors(game)
    return [s for s in ("Cyndaquil", "Totodile", "Chikorita")
            if resident_id(lab, "LOCALID_ELMS_LAB_" + s.upper()) in live]


def check_intro(game, maps, rows, checks):
    town, lab = maps["PolishedNewBarkTown"], maps["PolishedElmsLab"]
    elm = resident_id(lab, "LOCALID_ELMS_LAB_ELM")
    lyra = resident_id(lab, "LOCALID_ELMS_LAB_LYRA")
    old_party = party(game)
    assert len(old_party) == 1 and old_party[0]["species"] == layout()["Totodile"] and old_party[0]["level"] == 10
    assert (var(game, "town_scene"), var(game, "lab_scene"), var(game, "starter")) == (0, 0, 0)
    walk(game, town, (6, 5))
    game.step(UP)
    advance(game, lambda: text_matches(game, "PolishedIntro_Text_LyraIntro"))
    assert game.position() == (6, 4)
    assert actors(game)[resident_id(town, "LOCALID_NEW_BARK_LYRA")]["position"] == (5, 4)
    game.screenshot("intro-lyra-town")
    choice_prompt(game, "PolishedIntro_ElmText_Intro")
    assert game.position() == (4, 4) and actors(game)[elm]["position"] == (3, 4)
    assert actors(game)[lyra]["position"] == (4, 5)
    assert var(game, "town_scene") == 2 and var(game, "lab_scene") == 0
    game.screenshot("intro-elm-request")
    game.press(B, 90)
    choice_prompt(game, "PolishedIntro_ElmText_Refused")
    assert var(game, "lab_scene") == 0 and party(game) == old_party
    game.screenshot("intro-elm-refusal")
    game.press(A, 60)
    advance(game, lambda: var(game, "lab_scene") == 1 and not script_active(game))
    assert game.position() == (4, 4) and actors(game)[elm]["position"] == (5, 2)
    assert actors(game)[lyra]["position"] == (4, 5)
    assert len(remaining(game, lab)) == 3 and party(game) == old_party
    for local_id, color in ((elm, 3), (lyra, 0), (resident_id(lab, "LOCALID_ELMS_LAB_AIDE"), 3)):
        check_resident_palette(game, lab, local_id, color, "day", rows)
    colors = re.findall(r"^\s*ow_npc_pal_const (\w+)",
        source_files()["constants/sprite_data_constants.asm"].decode(), re.M)
    for species, color in (("CYNDAQUIL", "POKE_BALL"), ("TOTODILE", "DECO_ITEM"), ("CHIKORITA", "KEY_ITEM")):
        check_resident_palette(game, lab, resident_id(lab, "LOCALID_ELMS_LAB_" + species), colors.index(color), "day", rows)
    game.screenshot("intro-choose-starter")
    checks.append("Lyra leads the player into Elm's lab; source introduction/refusal loop and all six source actor palettes work")
    walk(game, lab, (2, 2))
    game.face(UP)
    game.press(A, 100)
    assert text_matches(game, "PolishedElmsLab_CannotHealText")
    close_resident_dialogue(game)
    walk(game, lab, (5, 5))
    game.step(DOWN)
    advance(game, lambda: text_matches(game, "PolishedIntro_LabWhereGoingText"))
    close_resident_dialogue(game)
    assert game.position() == (5, 5) and var(game, "lab_scene") == 1
    save_game(game)
    continue_game(game)
    assert game.position() == (5, 5) and var(game, "lab_scene") == 1 and not script_active(game)
    assert len(remaining(game, lab)) == 3 and actors(game)[lyra]["position"] == (4, 5)
    game.snapshot("intro-choosing")
    checks.append("Elm stops an early exit; Save/Continue resumes starter selection without replaying the introduction")
    report = []
    for index, species, lyra_species, left in ((1, "Cyndaquil", "Chikorita", "Totodile"),
            (2, "Totodile", "Cyndaquil", "Chikorita"), (3, "Chikorita", "Totodile", "Cyndaquil")):
        game.restore("intro-choosing")
        walk(game, lab, (index + 5, 4))
        game.face(UP)
        game.press(A, 120)
        game.screenshot("starter-" + species.lower() + "-preview")
        choice_prompt(game, "PolishedIntro_Take" + species + "Text")
        if index == 1:
            game.press(B, 90)
            close_resident_dialogue(game)
            assert var(game, "starter") == 0 and len(remaining(game, lab)) == 3 and party(game) == old_party
            game.press(A, 120)
            choice_prompt(game, "PolishedIntro_Take" + species + "Text")
        game.press(A, 60)
        choice_prompt(game, "gText_NicknameThisPokemon")
        if index == 1:
            game.press(A, 180)
            assert game.u32(game.sym["gMain"] + 4) == game.sym["CB2_NamingScreen"] | 1
            game.screenshot("starter-nickname")
            # The first capital triggers Expansion's animated lowercase page
            # swap, during which input is disabled. Let it finish before START.
            game.press(A, 120)
            game.press(START, 30)
            game.press(A, 180)
        else:
            game.press(B, 60)
        advance(game, lambda: text_matches(game, "PolishedIntro_LyraNicknamed" + lyra_species + "Text"))
        assert actors(game)[lyra]["position"] == ({"Chikorita": 8, "Cyndaquil": 6, "Totodile": 7}[lyra_species], 4)
        assert remaining(game, lab) == [left]
        game.screenshot("starter-" + species.lower() + "-lyra")
        advance(game, lambda: var(game, "lab_scene") == 6 and not script_active(game))
        assert game.position() == (5, 3) and actors(game)[lyra]["position"] == (5, 5), (species, player_state(game), actors(game))
        assert not player_state(game)["frozen"]
        assert var(game, "starter") == index
        actual = party(game)
        assert len(actual) == 2 and actual[0] == old_party[0]
        assert (actual[1]["species"], actual[1]["level"], actual[1]["item"]) == (layout()[species], 5, layout()["berry"])
        if index == 1:
            assert bytes.fromhex(actual[1]["nickname"]).split(b"\xff", 1)[0] == b"\xbb", actual[1]  # A
        addr = game.u32(game.sym["gSaveBlock1Ptr"]) + layout()["flags"] + layout()["contact"] // 8
        assert game.read(addr, 1)[0] & (1 << (layout()["contact"] % 8))
        game.screenshot("starter-" + species.lower() + "-complete")
        # The remaining ball is descriptive, never a second gift.
        left_index = ("Cyndaquil", "Totodile", "Chikorita").index(left)
        walk(game, lab, (left_index + 6, 4))
        game.face(UP)
        game.press(A, 100)
        assert text_matches(game, "PolishedIntro_ElmPokeBallText")
        close_resident_dialogue(game)
        assert party(game) == actual
        save_game(game)
        continue_game(game)
        assert var(game, "starter") == index and var(game, "lab_scene") == 6
        assert remaining(game, lab) == [left] and party(game) == actual
        travel(game, lab, town)
        travel(game, town, lab)
        # Actors outside the camera are culled, so approach the table first.
        walk(game, lab, (4, 4))
        assert not script_active(game) and remaining(game, lab) == [left]
        assert actors(game)[lyra]["position"] == (5, 5) and actors(game)[elm]["position"] == (5, 2)
        assert party(game) == actual
        report.append({"player_starter": species, "lyra_starter": lyra_species, "remaining_ball": left,
            "party": actual, "scene": var(game, "lab_scene"), "saved_and_reentered": True})
        print("PASS: starter", species, "-> Lyra", lyra_species, "with save, reentry and duplicate-gift guard", flush=True)
    checks.append("all three Lv5 Oran Berry gifts preserve the demo Totodile, support native nicknames and select Lyra's corresponding partner")
    checks.append("all three choices retain Elm's contact, remaining ball and actor positions across Save/Continue and reentry; no duplicate gifts")
    (game.output / "starter-branches.json").write_text(json.dumps(report, indent=2) + "\n")
    # Cyndaquil can also be selected from the west, where the player is already
    # in front of Elm. That source branch must skip the post-choice walk.
    game.restore("intro-choosing")
    walk(game, lab, (5, 3))
    game.face(RIGHT)
    game.press(A, 120)
    choice_prompt(game, "PolishedIntro_TakeCyndaquilText")
    game.press(A, 60)
    choice_prompt(game, "gText_NicknameThisPokemon")
    game.press(B, 60)
    advance(game, lambda: var(game, "lab_scene") == 6 and not script_active(game))
    assert game.position() == (5, 3) and actors(game)[lyra]["position"] == (5, 5)
    assert var(game, "starter") == 1 and remaining(game, lab) == ["Totodile"]
    assert len(party(game)) == 2
    game.screenshot("starter-cyndaquil-west-approach")
    checks.append("Cyndaquil's west approach keeps the player in front of Elm; healing is unavailable until a starter is chosen")
    travel(game, lab, town)
