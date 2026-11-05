"""Pinned Dark Cave rescue, Violet Gym and Elm's first Togepi Egg delivery."""
import re
import tempfile
from pathlib import Path

from convert_player_walking import convert, _read_png_2bit_grayscale, _write_indexed_4bit_png
from polished_cherrygrove import standing_sheet
from polished_route29 import object_at
from polished_route30 import palette
from polished_sprout import SLOT_ASSIGNMENT, inline_text
from new_bark_intro import coord

MAPS = {"DarkCaveVioletEntrance": "DARK_CAVE_VIOLET_ENTRANCE", "VioletGym": "VIOLET_GYM"}
CLOSED = {"DarkCaveVioletEntrance": ((17, 1), (35, 33))}


def trainers(files):
    parties = files["data/trainers/parties.asm"].decode()
    attacks = files["data/pokemon/evos_attacks.asm"].decode()
    result = "/* Generated from Polished Crystal v3.2.3; source first Gym teams. */\n"
    for name in ("Rod", "Abe", "Falkner"):
        leader = name == "Falkner"
        group = parties.split(("Falkner" if leader else "BirdKeeper") + "Group:\n", 1)[1]
        body = re.search(r'def_trainer ' + ("1" if leader else name.upper()) + r', "' + name + r'"\n(.*?)end_trainer', group, re.S)[1]
        result += f"\n=== TRAINER_POLISHED_{name.upper()} ===\nName: {name}\nClass: {'Leader' if leader else 'Bird Keeper'}\nPic: Polished {'Falkner' if leader else 'Bird Keeper'}\nGender: Male\nMusic: Male\nDouble Battle: No\n"
        if leader:
            assert "; Falkner\n\tdb SUPER_POTION, 0" in files["data/trainers/attributes.asm"].decode()
            result += "Items: Super Potion\n"
        kind = "falkner" if leader else "bird keeper"
        values = re.search(r"db\s+(\d+), \$([0-9A-F]{2}), \$([0-9A-F]{2}), \$([0-9A-F]{2}), MALE\s*; " + kind + r"\b", files["data/trainers/dvs.asm"].decode()).groups()
        assert len(set("".join(values[1:]))) == 1
        source_iv, source_ev = int(values[1][0], 16) * 2, int(values[0]) // 3
        for mon in re.split(r"\btr_mon ", body)[1:]:
            lv, sp, item = re.match(r"(\d+), (\w+)(?: @ (\w+))?", mon).groups()
            if leader:
                assert "tr_evs 32 HP" in mon
                moves = re.search(r"tr_moves (.*)", mon)[1].split(", ")
                ivs, evs = source_iv, [32, 0, 0, 0, 0, 0]
            else:
                learn = attacks.split("evos_attacks " + sp.title() + "\n", 1)[1].split("evos_attacks ", 1)[0]
                moves = [m for l, m in re.findall(r"learnset (\d+), (\w+)", learn) if int(l) <= int(lv)][-4:]
                ivs, evs = source_iv, [source_ev] * 6
            result += f"\n{sp.title()} (M)" + (" @ " + item.replace("_", " ").title() if item else "") + f"\nLevel: {lv}\nNature: Hardy\n"
            for label, values in (("IVs", [ivs] * 6), ("EVs", evs)):
                result += label + ": " + " / ".join(f"{v} {s}" for v, s in zip(values, ("HP", "Atk", "Def", "SpA", "SpD", "Spe"))) + "\n"
            result += "".join(f"- MOVE_{m}\n" for m in moves)
    return result


def assets(files):
    result = {"src/data/polished_falkner.party": trainers(files).encode()}
    with tempfile.TemporaryDirectory() as tmp:
        src, out = Path(tmp) / "source.png", Path(tmp) / "out.png"
        for name in ("falkner", "bird_keeper", "gym_guy", "pidgeotto_side", "ursaring", "rock"):
            src.write_bytes(files[f"gfx/{'minis' if name == 'ursaring' else 'sprites'}/{'boulder_rock_fossil' if name == 'rock' else name}.png"])
            if name in ("pidgeotto_side", "ursaring", "rock"):
                w, h, _ = _read_png_2bit_grayscale(src)
                assert w == 16 and h in (16, 32, 48)
                if name == "rock":
                    pixels = _read_png_2bit_grayscale(src)[2]
                    _write_indexed_4bit_png(out, [[0] * 16 for _ in range(16)] + [[3 - v for v in row] for row in pixels[16:32]])
                else:
                    standing_sheet(src, out, h)
            else:
                convert(src, out)
            result[f"graphics/object_events/polished_{name}/walking.png"] = out.read_bytes()
        for name in ("falkner", "bird_keeper"):
            src.write_bytes(files[f"gfx/trainers/{name}.png"])
            w, h, pixels = _read_png_2bit_grayscale(src)
            assert (w, h) == (56, 56)
            canvas = [[0] * 64 for _ in range(64)]
            for y, row in enumerate(pixels):
                canvas[y + 8][4:60] = [3 - v for v in row]
            _write_indexed_4bit_png(out, canvas)
            result[f"graphics/trainers/front_pics/polished_{name}.png"] = out.read_bytes()
            colors = [tuple(map(int, row)) for row in re.findall(r"RGB\s+(\d+),\s*(\d+),\s*(\d+)", files[f"gfx/trainers/{name}.pal"].decode())]
            result[f"graphics/trainers/palettes/polished_{name}.pal"] = palette(colors)
    return result


def wild_entries(files):
    body = re.search(r"def_grass_wildmons DARK_CAVE_VIOLET_ENTRANCE\n(.*?)end_grass_wildmons", files["data/wild/johto_grass.asm"].decode(), re.S)[1]
    assert "4 percent, 4 percent, 4 percent" in body
    slots = [(int(lv), sp) for lv, sp in re.findall(r"wildmon (\d+), (\w+)", body)]
    water = re.search(r"def_water_wildmons DARK_CAVE_VIOLET_ENTRANCE\n(.*?)end_water_wildmons", files["data/wild/johto_water.asm"].decode(), re.S)[1]
    water_slots = [(int(lv), sp) for lv, sp in re.findall(r"wildmon (\d+), (\w+)", water)]
    assert "2 percent" in water and len(water_slots) == 3
    result = []
    for n, period in enumerate(("Morning", "Day", "Night")):
        group = slots[n * 7:n * 7 + 7]
        result.append({"map": "MAP_POLISHED_DARK_CAVE_VIOLET_ENTRANCE", "base_label": "gPolishedDarkCaveVioletEntrance_" + period,
            "always_time_of_day": True, "land_mons": {"encounter_rate": 7,
                "mons": [{"min_level": group[i][0], "max_level": group[i][0], "species": "SPECIES_" + group[i][1]} for i in SLOT_ASSIGNMENT]},
            "water_mons": {"encounter_rate": 4,
                "mons": [{"min_level": water_slots[i][0], "max_level": water_slots[i][0], "species": "SPECIES_" + water_slots[i][1]} for i in (0, 1, 2, 2, 2)]}})
    return result


def movement(label, directions):
    return label + ":\n" + "".join("    " + d + "\n" for d in directions) + "    step_end\n"


def cave(script, header, files, text, define):
    p = "PolishedDarkCave"
    for number, gfx, x, y in ((1, "URSARING", 10, 2), (2, "PIDGEOTTO_SIDE", 9, 2), (3, "FALKNER", 8, 2)):
        obj = object_at("LOCALID_DARK_CAVE_" + str(number), "POLISHED_" + gfx, x, y, p + "_Nothing", "FLAG_POLISHED_CAVE_DONE")
        obj["movement_type"] = "MOVEMENT_TYPE_FACE_RIGHT" if number == 3 else "MOVEMENT_TYPE_FACE_DOWN"
        header["object_events"].append(obj)
    for line in script.splitlines():
        m = re.match(r"\s*(itemball|smashrock)_event\s+(\d+),\s*(\d+)(?:, (\w+), 1, \w+)?", line)
        if not m:
            continue
        kind, x, y, item = m.groups()
        number = len(header["object_events"]) + 1
        action = p + "_" + item if item else "EventScript_RockSmash"
        obj = object_at("LOCALID_DARK_CAVE_" + str(number), "POLISHED_BALL_RED" if item else "POLISHED_ROCK", int(x), int(y), action,
                        "FLAG_POLISHED_CAVE_" + item if item else "FLAG_TEMP_" + str(number - 4))
        header["object_events"].append(obj)
    header["bg_events"].append({"type": "hidden_item", "x": 26, "y": 3, "elevation": 0, "item": "ITEM_ELIXIR", "flag": "FLAG_POLISHED_CAVE_ELIXIR"})
    coord(header, 6, 2, "VAR_TEMP_4", 0, p + "_Scene")
    code = f"""{p}_Setup::
    setvar VAR_TEMP_4, 0
    goto_if_unset FLAG_POLISHED_CAVE_DONE, {p}_Nothing
    setvar VAR_TEMP_4, 1
{p}_Nothing::
    end
{p}_Scene::
    lockall
    specialvar VAR_RESULT, PolishedFalknerFlashLevel
    goto_if_eq VAR_RESULT, 0, {p}_Lit
    goto_if_set FLAG_POLISHED_SPROUT_FLASH, {p}_Dark
    msgbox {p}_Darkness, MSGBOX_DEFAULT
    closemessage
    applymovement OBJ_EVENT_ID_PLAYER, {p}_Left
    waitmovement 0
    releaseall
    end
{p}_Dark::
    msgbox {p}_ProgressAnyway, MSGBOX_DEFAULT
    call {p}_Rescue
    msgbox {p}_DarkIntro, MSGBOX_DEFAULT
    goto {p}_Finish
{p}_Lit::
    call {p}_Rescue
    msgbox {p}_Intro, MSGBOX_DEFAULT
{p}_Finish::
    msgbox {p}_Explanation, MSGBOX_DEFAULT
    closemessage
    applymovement OBJ_EVENT_ID_PLAYER, {p}_Aside
    applymovement LOCALID_DARK_CAVE_3, {p}_Aside
    waitmovement 0
    turnobject OBJ_EVENT_ID_PLAYER, DIR_SOUTH
    applymovement LOCALID_DARK_CAVE_3, {p}_Leave
    waitmovement 0
    removeobject LOCALID_DARK_CAVE_3
    setflag FLAG_POLISHED_CAVE_DONE
    setvar VAR_TEMP_4, 1
    releaseall
    end
{p}_Rescue::
    msgbox {p}_Attack, MSGBOX_DEFAULT
    closemessage
    playmoncry SPECIES_PIDGEOTTO, CRY_MODE_NORMAL
    waitmoncry
    applymovement LOCALID_DARK_CAVE_2, {p}_Strike
    waitmovement 0
    playse SE_M_COMET_PUNCH
    playmoncry SPECIES_URSARING, CRY_MODE_NORMAL
    waitmoncry
    removeobject LOCALID_DARK_CAVE_1
    msgbox {p}_Return, MSGBOX_DEFAULT
    closemessage
    removeobject LOCALID_DARK_CAVE_2
    applymovement LOCALID_DARK_CAVE_3, {p}_Left
    waitmovement 0
    return
"""
    for suffix, dirs in (("Left", ["walk_left"]), ("Aside", ["walk_left"] * 3),
                         ("Leave", ["walk_down", "walk_left"] + ["walk_down"] * 4), ("Strike", ["walk_fast_right", "walk_fast_left"])):
        code += movement(p + "_" + suffix, dirs)
    for suffix in ("Darkness", "ProgressAnyway", "DarkIntro", "Intro", "Explanation", "Attack", "Return"):
        code += define(p + "_" + suffix, text(script.replace("\tprompt\n", "\tdone\n"), "DarkCaveVioletEntranceFalkner" + suffix + "Text"))
    for item in ("POTION", "DUSK_STONE", "HYPER_POTION", "DIRE_HIT"):
        code += f"{p}_{item}::\n    finditem ITEM_{item}\n    end\n"
    return f"\tmap_script MAP_SCRIPT_ON_LOAD, {p}_Setup\n", code, {
        "story": "Flash acquisition unlocks the source rescue even in darkness; lit caves also allow it.",
        "deferred": ["Blackthorn and Route 46 exits", "source cave music"],
        "rate_translation": "Land 7/180 (3.89%) and water 4/180 (2.22%); native movement/ability modifiers apply."}


def gym(script, header, files, text, define):
    p = "PolishedVioletGym"
    for i, (gfx, x, y, face, action) in enumerate((("GYM_GUY", 4, 13, "DOWN", "Nothing"), ("FALKNER", 5, 1, "DOWN", "Falkner"),
            ("BIRD_KEEPER", 7, 6, "LEFT", "Rod"), ("BIRD_KEEPER", 2, 10, "RIGHT", "Abe"), ("GYM_GUY", 7, 13, "DOWN", "Guide")), 1):
        obj = object_at("LOCALID_VIOLET_GYM_" + str(i), "POLISHED_" + gfx, x, y, p + "_" + action,
                        "FLAG_POLISHED_CAVE_DONE" if i == 1 else "FLAG_TEMP_1")
        obj["movement_type"] = "MOVEMENT_TYPE_FACE_" + face
        if i in (3, 4):
            obj.update(trainer_type="TRAINER_TYPE_NORMAL", trainer_sight_or_berry_tree_id="3")
        header["object_events"].append(obj)
    for x in (3, 6):
        header["bg_events"].append({"type": "sign", "x": x, "y": 13, "elevation": 0,
            "player_facing_dir": "BG_EVENT_PLAYER_FACING_ANY", "script": p + "_Statue"})
    code = f"""{p}_Setup::
    special PolishedFalknerPrepareBadges
    setflag FLAG_TEMP_1
    setvar VAR_TEMP_4, 0
    goto_if_unset FLAG_POLISHED_CAVE_DONE, {p}_Nothing
    clearflag FLAG_TEMP_1
    setvar VAR_TEMP_4, 1
{p}_Nothing::
    end
{p}_Frame::
    map_script_2 VAR_TEMP_4, 0, {p}_Away
    .2byte 0
{p}_Away::
    lockall
    applymovement LOCALID_VIOLET_GYM_1, {p}_StepDown
    waitmovement 0
    msgbox {p}_AwayText, MSGBOX_DEFAULT
    closemessage
    warp MAP_POLISHED_VIOLET_CITY, 18, 18
    waitstate
    releaseall
    end
{p}_Falkner::
    lock
    faceplayer
    goto_if_set FLAG_POLISHED_ROOST, {p}_After
    goto_if_defeated TRAINER_POLISHED_FALKNER, {p}_Badge
    msgbox {p}_Intro, MSGBOX_DEFAULT
    trainerbattle_no_intro TRAINER_POLISHED_FALKNER, {p}_Beaten
{p}_Badge::
    goto_if_set FLAG_POLISHED_ZEPHYR_BADGE, {p}_TM
    special PolishedFalknerAwardBadge
    settrainerflag TRAINER_POLISHED_ROD
    settrainerflag TRAINER_POLISHED_ABE
    msgbox {p}_Received, MSGBOX_DEFAULT
    call Common_EventScript_PlayGymBadgeFanfare
{p}_TM::
    msgbox {p}_BadgeText, MSGBOX_DEFAULT
    giveitem ITEM_TM_ROOST
    goto_if_eq VAR_RESULT, FALSE, {p}_Done
    setflag FLAG_POLISHED_ROOST
    msgbox {p}_Explain, MSGBOX_DEFAULT
    goto {p}_Done
{p}_After::
    msgbox {p}_AfterText, MSGBOX_DEFAULT
{p}_Done::
    releaseall
    end
{p}_Guide::
    goto_if_set FLAG_POLISHED_ZEPHYR_BADGE, {p}_GuideWon
    msgbox {p}_GuideText, MSGBOX_NPC
    end
{p}_GuideWon::
    msgbox {p}_GuideWonText, MSGBOX_NPC
    end
{p}_Statue::
    goto_if_set FLAG_POLISHED_ZEPHYR_BADGE, {p}_StatueWon
    msgbox {p}_StatueText, MSGBOX_SIGN
    end
{p}_StatueWon::
    msgbox {p}_StatueWonText, MSGBOX_SIGN
    end
"""
    code += movement(p + "_StepDown", ["walk_down"])
    for n in ("Rod", "Abe"):
        code += f"{p}_{n}::\n    trainerbattle_single TRAINER_POLISHED_{n.upper()}, {p}_{n}Seen, {p}_{n}Beaten\n    msgbox {p}_{n}After, MSGBOX_AUTOCLOSE\n    end\n"
        for suffix in ("Seen", "Beaten"):
            code += define(p + "_" + n + suffix, text(script, "Bird_keeper" + n + suffix + "Text"))
        code += define(p + "_" + n + "After", inline_text(script, "GenericTrainerBird_keeper" + n, text))
    for suffix, label in (("AwayText", "VioletGymGuyFalknerAwayText"), ("Intro", "FalknerIntroText"), ("Beaten", "FalknerWinLossText"),
            ("BadgeText", "FalknerZephyrBadgeText"), ("Explain", "FalknerTMMudSlapText"), ("AfterText", "FalknerFightDoneText"),
            ("GuideText", "VioletGymGuyText"), ("GuideWonText", "VioletGymGuyWinText")):
        code += define(p + "_" + suffix, text(script.replace("<LV>", "Lv"), label))
    code += define(p + "_Received", "{PLAYER} received the\\nZephyr Badge!$")
    code += define(p + "_StatueText", "Violet POKéMON GYM\\nLeader: Falkner$")
    code += define(p + "_StatueWonText", "Violet POKéMON GYM\\nLeader: Falkner\\pWinning Trainers:\\n{PLAYER}$")
    return f"\tmap_script MAP_SCRIPT_ON_LOAD, {p}_Setup\n\tmap_script MAP_SCRIPT_ON_FRAME_TABLE, {p}_Frame\n", code, {
        "trainers": ["Rod", "Abe", "Falkner"], "rewards": ["Zephyr Badge (native first badge slot)", "reusable TM51 Roost"],
        "deferred": ["late-game Lyra statue entry", "source music", "Johto badge artwork on Trainer Card"]}


def events(name, script, header, files, text, define):
    return (cave if name == "DarkCaveVioletEntrance" else gym)(script, header, files, text, define)


def adapt(name, script, header, files, code, text, define):
    if name in ("VioletCity", "Route31"):
        x, y, width = (18, 17, 20) if name == "VioletCity" else (34, 5, 22)
        tile = files[f"maps/{name}.ablk"][y // 2 * width + x // 2] * 4 + y % 2 * 2 + x % 2
        code = code.replace("Polished" + name + "_Setup::\n", f"Polished{name}_Setup::\n    setmetatile {x}, {y}, {tile}, FALSE\n")
    if name == "VioletCity":
        code = code.replace("\t.byte 0\n", "\tmap_script MAP_SCRIPT_ON_FRAME_TABLE, PolishedEgg_Frame\n\t.byte 0\n", 1)
        code = code.replace("PolishedVioletCity_Setup::\n", "PolishedVioletCity_Setup::\n    call PolishedEgg_Ready\n")
        code += """PolishedEgg_Ready::
    special PolishedFalknerPrepareBadges
    setvar VAR_TEMP_4, 0
    goto_if_unset FLAG_POLISHED_ZEPHYR_BADGE, PolishedEgg_ReadyDone
    goto_if_set FLAG_POLISHED_EGG_CALL, PolishedEgg_ReadyDone
    setvar VAR_TEMP_4, 1
PolishedEgg_ReadyDone::
    return
PolishedEgg_Frame::
    map_script_2 VAR_TEMP_4, 1, PolishedEgg_Call
    .2byte 0
PolishedEgg_Call::
    lockall
    playse SE_PIN
    msgbox PolishedEgg_CallText, MSGBOX_DEFAULT
    setflag FLAG_POLISHED_EGG_CALL
    setflag FLAG_POLISHED_AIDE_IN_VIOLET
    setvar VAR_TEMP_4, 0
    releaseall
    end
"""
        code += define("PolishedEgg_CallText", text(files["data/phone/text/elm.asm"].decode(), "ElmPhoneEggAssistantText"))
    if name == "ElmsLab":
        next(o for o in header["object_events"] if o["local_id"] == "LOCALID_ELMS_LAB_AIDE")["flag"] = "FLAG_POLISHED_AIDE_IN_VIOLET"
        code = code.replace("PolishedReturn_ElmStudying::\n", "PolishedReturn_ElmStudying::\n    goto_if_set FLAG_POLISHED_EGG_GIVEN, PolishedEgg_ElmWaiting\n    goto_if_set FLAG_POLISHED_ZEPHYR_BADGE, PolishedEgg_ElmAide\n")
        for suffix, label in (("ElmWaiting", "ElmWaitingEggHatchText"), ("ElmAide", "ElmAideHasEggText")):
            code += f"PolishedEgg_{suffix}::\n    msgbox PolishedEgg_{suffix}Text, MSGBOX_DEFAULT\n    releaseall\n    end\n"
            code += define("PolishedEgg_" + suffix + "Text", text(script, label))
    if name == "VioletPokeCenter1F":
        p = "PolishedEgg"
        obj = object_at("LOCALID_VIOLET_EGG_AIDE", "POLISHED_SCIENTIST", 10, 2, p + "_Aide", "FLAG_TEMP_3")
        obj["movement_type"] = "MOVEMENT_TYPE_FACE_UP"
        header["object_events"].append(obj)
        code = code.replace("\t.byte 0\n", "\tmap_script MAP_SCRIPT_ON_LOAD, PolishedEgg_Visibility\n\t.byte 0\n", 1)
        code += """PolishedEgg_Visibility::
    setflag FLAG_TEMP_3
    goto_if_unset FLAG_POLISHED_EGG_CALL, PolishedEgg_End
    goto_if_set FLAG_POLISHED_EGG_GIVEN, PolishedEgg_End
    clearflag FLAG_TEMP_3
PolishedEgg_End::
    end
PolishedEgg_Aide::
    lockall
    faceplayer
    goto_if_set FLAG_POLISHED_EGG_GIVEN, PolishedEgg_Done
    goto_if_set FLAG_POLISHED_EGG_REFUSED, PolishedEgg_AskAgain
    msgbox PolishedEgg_Intro, MSGBOX_YESNO
    goto PolishedEgg_Answer
PolishedEgg_AskAgain::
    msgbox PolishedEgg_Question, MSGBOX_YESNO
PolishedEgg_Answer::
    goto_if_eq VAR_RESULT, NO, PolishedEgg_Refuse
    specialvar VAR_RESULT, PolishedFalknerGiveEgg
    goto_if_eq VAR_RESULT, MON_CANT_GIVE, PolishedEgg_Full
    goto_if_eq VAR_RESULT, MON_GIVEN_TO_PC, PolishedEgg_Box
    msgbox PolishedEgg_Received, MSGBOX_DEFAULT
    goto PolishedEgg_Goodbye
PolishedEgg_Box::
    msgbox PolishedEgg_BoxText, MSGBOX_DEFAULT
PolishedEgg_Goodbye::
    msgbox PolishedEgg_GoodbyeText, MSGBOX_DEFAULT
    closemessage
    getplayerxy VAR_0x8004, VAR_0x8005
    goto_if_eq VAR_0x8005, 3, PolishedEgg_Around
    applymovement LOCALID_VIOLET_EGG_AIDE, PolishedEgg_Straight
    goto PolishedEgg_Leave
PolishedEgg_Around::
    applymovement LOCALID_VIOLET_EGG_AIDE, PolishedEgg_WalkAround
PolishedEgg_Leave::
    waitmovement 0
    removeobject LOCALID_VIOLET_EGG_AIDE
    setflag FLAG_TEMP_3
    goto PolishedEgg_Done
PolishedEgg_Refuse::
    setflag FLAG_POLISHED_EGG_REFUSED
    msgbox PolishedEgg_RefusalText, MSGBOX_DEFAULT
    goto PolishedEgg_Done
PolishedEgg_Full::
    msgbox PolishedEgg_FullText, MSGBOX_DEFAULT
PolishedEgg_Done::
    releaseall
    end
"""
        code += movement(p + "_Straight", ["walk_down"] + ["walk_left"] * 4 + ["walk_down"] * 4)
        code += movement(p + "_WalkAround", ["walk_left"] * 2 + ["walk_down"] + ["walk_left"] * 2 + ["walk_down"] * 4)
        aide = script.split("VioletPokeCenter1FElmsAideScript:\n", 1)[1]
        for suffix, label in (("Intro", ".IntroText"), ("Question", ".QuestionText"), ("FullText", ".PartyAndBoxFull"), ("GoodbyeText", ".GoodbyeText")):
            code += define(p + "_" + suffix, text(aide, label))
        code += define(p + "_RefusalText", inline_text(aide, ".RefusedEgg", text))
        code += define(p + "_Received", "{PLAYER} received the\\nPOKéMON Egg!$")
        code += define(p + "_BoxText", "{PLAYER} received the Egg!\\pYour party is full, so the\\nEgg was sent to your PC.$")
    return code
