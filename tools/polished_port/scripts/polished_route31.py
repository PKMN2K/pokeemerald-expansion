"""Route 31, its western gate, and the first two trainers in the extended roster."""
import re
import tempfile
from pathlib import Path

from polished_route29 import object_at
from polished_route30 import palette
from new_bark_intro import coord
from convert_player_walking import convert, _read_png_2bit_grayscale, _write_indexed_4bit_png

MAPS = {"Route31": "ROUTE_31", "Route31VioletGate": "ROUTE_31_VIOLET_GATE"}
# Preserve the original art at doors whose destination has not been imported.
CLOSED = {}


def trainers(files):
    parties = files["data/trainers/parties.asm"].decode()
    attacks = files["data/pokemon/evos_attacks.asm"].decode()
    wade = re.search(r'def_trainer WADE1, "Wade"\n(.*?)end_trainer', parties, re.S)[1]
    result = "/* Generated from Polished Crystal v3.2.3. */\n"
    result += "\n=== TRAINER_POLISHED_WADE ===\nName: Wade\nClass: Bug Catcher\nPic: Polished Bug Catcher\nGender: Male\nMusic: Male\nDouble Battle: No\n"
    for level, species in re.findall(r"tr_mon (\d+), (\w+)", wade):
        learnset = attacks.split(f"evos_attacks {species.title()}\n", 1)[1].split("evos_attacks ", 1)[0]
        moves = [m for lv,m in re.findall(r"learnset (\d+), (\w+)", learnset) if int(lv) <= int(level)][-4:]
        result += f"\n{species.title()} (M)\nLevel: {level}\nIVs: 12 HP / 12 Atk / 12 Def / 12 SpA / 12 SpD / 12 Spe\n"
        result += "".join("- " + move.replace("_"," ").title()+"\n" for move in moves)
    result += "\n=== TRAINER_POLISHED_FINCH ===\nName: Finch\nClass: Cooltrainer\nPic: Polished Cooltrainer M\nGender: Male\nMusic: Male\nDouble Battle: No\n"
    finch = parties.split('def_trainer FINCH, "Finch"\n',1)[1].split("end_trainer")[0]
    for mon in finch.split("tr_mon ")[1:]:
        offset, species, item = re.match(r"LEVEL_FROM_BADGES \+ (\d+), (\w+) @ (\w+), MALE", mon).groups()
        ability, nature = re.search(r"tr_extra (\w+), (\w+)", mon).groups()
        dv = 2 * int(re.search(r"tr_dvs (\d+) All", mon)[1])
        hp = re.search(r"tr_evs (\d+) HP", mon)[1]
        moves = re.search(r"tr_moves ([\w, ]+)", mon)[1].split(", ")
        nature = {"DEF_UP_SPE_DOWN": "Relaxed", "SATK_UP_SDEF_DOWN": "Rash"}[nature]
        result += f"\n{species.title()} (M) @ {item.replace('_',' ').title()}\nLevel: {4 + int(offset)}\nAbility: {ability.title()}\nNature: {nature}\nEVs: {hp} HP\n"
        result += "IVs: " + " / ".join(f"{dv} {stat}" for stat in ("HP","Atk","Def","SpA","SpD","Spe")) + "\n"
        # Preserve Expansion's move spelling and the source move slots.
        for move in moves:
            move_id = {"THUNDERSHOCK":"THUNDER_SHOCK"}.get(move, move)
            result += f"- MOVE_{move_id}\n"
    return result


def assets(files):
    result = {"src/data/polished_route31.party":trainers(files).encode()}
    with tempfile.TemporaryDirectory() as tmp:
        src, out = Path(tmp)/"source.png", Path(tmp)/"out.png"
        src.write_bytes(files["gfx/sprites/ace_trainer_m.png"])
        convert(src,out)
        result["graphics/object_events/polished_ace_trainer_m/walking.png"] = out.read_bytes()
        src.write_bytes(files["gfx/trainers/cooltrainer_m.png"])
        w,h,pixels = _read_png_2bit_grayscale(src)
        assert (w,h) == (56,56)
        canvas = [[0]*64 for _ in range(64)]
        for y,row in enumerate(pixels):
            canvas[y+8][4:60] = [3-v for v in row]
        _write_indexed_4bit_png(out,canvas)
        result["graphics/trainers/front_pics/polished_cooltrainer_m.png"] = out.read_bytes()
    colors = [tuple(map(int,row)) for row in re.findall(r"RGB\s+(\d+),\s*(\d+),\s*(\d+)",files["gfx/trainers/cooltrainer_m.pal"].decode())]
    assert len(colors)==2
    result["graphics/trainers/palettes/polished_cooltrainer_m.pal"] = palette(colors)
    return result


def events(name, script, header, files, text, define):
    prefix = "Polished" + name
    if name == "Route31VioletGate":
        # Emerald restores a saved 15x14 map view before running this hook.
        # Saves made inside the old gate may still contain its two solid tiles.
        blocks = files["maps/EastWestGate.ablk"]
        code = "PolishedRoute31VioletGate_OpenWest::\n"
        for y in (4, 5):
            tile = blocks[(y // 2) * 5] * 4 + (y % 2) * 2
            code += f"\tsetmetatile 0, {y}, {tile}, FALSE\n"
        return "\tmap_script MAP_SCRIPT_ON_LOAD, PolishedRoute31VioletGate_OpenWest\n", code+"\tend\n", {
            "gate": "East doors return to Route 31; west doors enter Violet City, including old cartridge saves."}

    header["connections"] = [{"map":"MAP_POLISHED_ROUTE_30","offset":20,"direction":"down"},
                             {"map":"MAP_POLISHED_VIOLET_CITY","offset":-18,"direction":"left"}]
    actors = [
        ("FINCH", "POLISHED_ACE_TRAINER_M", 28,7, "Finch"),
        ("SLEEPY_MAN", "POLISHED_FAT_GUY_BLUE", 17,7, "Sleepy"),
        ("WADE", "POLISHED_BUG_CATCHER", 21,13, "Wade"),
        ("CUT_1", "POLISHED_CUT_TREE", 13,5, "EventScript_CutTree"),
        ("CUT_2", "POLISHED_CUT_TREE", 25,10, "EventScript_CutTree"),
        ("PERSIM", "VAR_C", 16,7, "Persim"),
        ("POTION", "POLISHED_BALL_RED", 29,5, "Potion"),
        ("POKE_BALL", "POLISHED_BALL_RED", 19,15, "PokeBall")]
    for label, gfx, x, y, action in actors:
        assert re.search(rf"(?:object|cuttree|fruittree|itemball)_event +{x},\s*{y},", script), label
        persistent = label in ("CUT_1","CUT_2","POTION","POKE_BALL")
        npc = object_at("LOCALID_ROUTE31_"+label, gfx, x, y, action if action.startswith("EventScript") else prefix+"_"+action, "FLAG_POLISHED_ROUTE31_"+label if persistent else "0")
        if label == "WADE":
            npc.update(trainer_type="TRAINER_TYPE_NORMAL", movement_type="MOVEMENT_TYPE_FACE_LEFT", trainer_sight_or_berry_tree_id="5")
        header["object_events"].append(npc)
    for y in (8,9):
        coord(header,28,y,"VAR_TEMP_4",0,prefix+"_MeetFinch")
    hooks = ("\tmap_script MAP_SCRIPT_ON_LOAD, PolishedRoute31_Setup\n"
             "\tmap_script MAP_SCRIPT_ON_RETURN_TO_FIELD, PolishedRoute31_Setup\n"
             "\tmap_script MAP_SCRIPT_ON_FRAME_TABLE, PolishedRoute31_CallTable\n")
    code = """PolishedRoute31_Setup::
    special PolishedFruitRefreshAll
    setvar VAR_TEMP_4, 0
    goto_if_unset FLAG_POLISHED_ROUTE31_MET_FINCH, PolishedRoute31_End
    setvar VAR_TEMP_4, 1
PolishedRoute31_End::
    end
PolishedRoute31_CallTable::
    map_script_2 VAR_TEMP_5, 0, PolishedRoute31_CheckCall
    .2byte 0
PolishedRoute31_CheckCall::
    setvar VAR_TEMP_5, 1
    goto_if_set FLAG_POLISHED_MOM_BANK_OPEN, PolishedRoute31_End
    goto_if_unset FLAG_POLISHED_GAVE_MYSTERY_EGG, PolishedRoute31_End
    lockall
    playse SE_PC_LOGIN
    msgbox PolishedRoute31_MomLecture, MSGBOX_YESNO
    setflag FLAG_POLISHED_MOM_BANK_OPEN
    clearflag FLAG_POLISHED_MOM_SAVES_MONEY
    goto_if_eq VAR_RESULT, NO, PolishedRoute31_NoSavings
    setflag FLAG_POLISHED_MOM_SAVES_MONEY
    msgbox PolishedRoute31_MomYes, MSGBOX_DEFAULT
    goto PolishedRoute31_HangUp
PolishedRoute31_NoSavings::
    msgbox PolishedRoute31_MomNo, MSGBOX_DEFAULT
PolishedRoute31_HangUp::
    msgbox PolishedRoute31_MomBye, MSGBOX_DEFAULT
    releaseall
    end
PolishedRoute31_MeetFinch::
    lockall
    turnobject OBJ_EVENT_ID_PLAYER, DIR_NORTH
    turnobject LOCALID_ROUTE31_FINCH, DIR_SOUTH
    goto PolishedRoute31_Introduction
PolishedRoute31_Finch::
    lock
    faceplayer
    goto_if_set FLAG_POLISHED_ROUTE31_AIR_BALLOON, PolishedRoute31_AfterGift
    goto_if_defeated TRAINER_POLISHED_FINCH, PolishedRoute31_Gift
    goto_if_not_defeated TRAINER_POLISHED_JOEY, PolishedRoute31_Introduction
    goto_if_not_defeated TRAINER_POLISHED_MIKEY, PolishedRoute31_Introduction
    goto_if_not_defeated TRAINER_POLISHED_DON, PolishedRoute31_Introduction
    goto_if_not_defeated TRAINER_POLISHED_WADE, PolishedRoute31_Introduction
    msgbox PolishedRoute31_FinchQuestionText, MSGBOX_YESNO
    goto_if_eq VAR_RESULT, NO, PolishedRoute31_Refused
    msgbox PolishedRoute31_FinchSeenText, MSGBOX_DEFAULT
    trainerbattle_no_intro TRAINER_POLISHED_FINCH, PolishedRoute31_FinchBeatenText
PolishedRoute31_Gift::
    msgbox PolishedRoute31_FinchAfterText1, MSGBOX_DEFAULT
    giveitem ITEM_AIR_BALLOON
    goto_if_eq VAR_RESULT, FALSE, PolishedRoute31_Release
    setflag FLAG_POLISHED_ROUTE31_AIR_BALLOON
PolishedRoute31_AfterGift::
    msgbox PolishedRoute31_FinchAfterText2, MSGBOX_DEFAULT
    goto PolishedRoute31_Release
PolishedRoute31_Introduction::
    setflag FLAG_POLISHED_ROUTE31_MET_FINCH
    setvar VAR_TEMP_4, 1
    msgbox PolishedRoute31_FinchIntroText, MSGBOX_DEFAULT
    goto PolishedRoute31_Release
PolishedRoute31_Refused::
    msgbox PolishedRoute31_FinchRefusedText, MSGBOX_DEFAULT
PolishedRoute31_Release::
    releaseall
    end
PolishedRoute31_Wade::
    trainerbattle_single TRAINER_POLISHED_WADE, PolishedRoute31_WadeSeen, PolishedRoute31_WadeBeaten
    msgbox PolishedRoute31_WadeAfter, MSGBOX_AUTOCLOSE
    end
PolishedRoute31_Sleepy::
    msgbox PolishedRoute31_SleepyText, MSGBOX_NPC
    end
PolishedRoute31_Persim::
    setvar VAR_0x8005, 3
    goto PolishedFruit_Interact
PolishedRoute31_Potion::
    finditem ITEM_POTION
    end
PolishedRoute31_PokeBall::
    finditem ITEM_POKE_BALL
    end
"""
    for label in ("IntroText","QuestionText","RefusedText","SeenText","BeatenText","AfterText1","AfterText2"):
        code += define(prefix+"_Finch"+label, text(script, "."+label))
    for suffix in ("Seen","Beaten","After"):
        code += define(prefix+"_Wade"+suffix, text(script,"Bug_catcherWade1"+suffix+"Text"))
    code += define(prefix+"_SleepyText",text(script,"Text_Route31SleepyMan"))
    phone = files["data/phone/text/mom.asm"].decode()
    for out,label in (("MomLecture","MomPhoneLectureText"),("MomYes","MomOKIllSaveText"),
                      ("MomNo","MomPhoneWontSaveMoneyText"),("MomBye","MomPhoneHangUpText")):
        code += define(prefix+"_"+out,text(phone,label))
    return hooks, code, {"trainers":["Wade","Finch"], "sight": "Finch explains his challenge once; four earlier victories unlock the optional battle.",
                         "phone": "Mom calls if the player has not opened the bank.", "deferred":["Wade phone/rematches","Kenya mail quest"]}
