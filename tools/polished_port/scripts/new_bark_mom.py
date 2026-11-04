"""Translate the release opening at home; banking follows the Mystery Egg quest.

The native weekday menu replaces Crystal's spinner. Initial DST selection only
records the flag in the source, so it must not add an hour to the running RTC.
"""
import hashlib
import json
import re
import polished_departure

from build_johto_demo import FIXTURES, PIN
from new_bark_intro import coord


def house(script, header, files, source_text, text_definition):
    prefix = "PolishedMom"
    rtc = files["engine/rtc/timeset.asm"].decode()
    assert re.search(r"Special_InitialSetDSTFlag:\n\tld a, \[wDST\]\n\tset 7, a\n\tld \[wDST\], a", rtc)
    assert re.search(r"Special_InitialClearDSTFlag:\n\tld a, \[wDST\]\n\tres 7, a\n\tld \[wDST\], a", rtc)
    source = json.loads((FIXTURES / "new_bark/mom_clock_excerpts.json").read_text())
    assert source["commit"] == PIN
    for entry in source["extracts"]:
        assert hashlib.sha256(entry["excerpt"].encode()).hexdigest() == entry["sha256"]
    clock_text = "\n".join(entry["excerpt"] for entry in source["extracts"])
    events = re.findall(r"^\s*object_event (.*)$", script, re.M)
    assert len(events) == 6
    positions = [(9, 4, "LEFT"), (3, 2, "UP"), (9, 4, "LEFT"),
                 (6, 3, "UP"), (1, 2, "UP"), (6, 4, "RIGHT")]
    names = ("INITIAL", "MORN", "DAY", "EVE", "NITE", "NEIGHBOR")
    for i, (raw, (x, y, facing), name) in enumerate(zip(events, positions, names)):
        fields = [x.strip() for x in raw.split(",")]
        assert fields[:4] == [str(x), str(y), "SPRITE_MATRON" if i == 5 else "SPRITE_MOM",
                              "SPRITEMOVEDATA_STANDING_" + facing]
        assert fields[10] == ("NeighborScript" if i == 5 else "MomScript")
        if 1 <= i <= 4:
            assert fields[6] == f"(1 << {name})"
        header["object_events"].append({"local_id": "LOCALID_HOME_" + name,
            "graphics_id": "OBJ_EVENT_GFX_POLISHED_" + ("MATRON" if i == 5 else "MOM"),
            "x": x, "y": y, "elevation": 3, "movement_type": "MOVEMENT_TYPE_FACE_" + facing,
            "movement_range_x": 0, "movement_range_y": 0, "trainer_type": "TRAINER_TYPE_NONE",
            "trainer_sight_or_berry_tree_id": "0", "script": prefix + ("_Neighbor" if i == 5 else "_Talk"),
            "flag": "FLAG_HIDE_POLISHED_NEW_BARK_RIVAL" if i == 5 else f"FLAG_TEMP_{i + 2}"})
    assert "MomSpriteGFX, WALKING_SPRITE, PAL_OW_RED" in files["data/sprites/sprites.asm"].decode()
    for i, (x, y) in enumerate(((10, 4), (11, 4), (9, 1), (9, 2)), 1):
        assert re.search(rf"coord_event\s+{x},\s+{y},\s+0, MomTrigger{i}", script)
        coord(header, x, y, "VAR_POLISHED_HOME_SCENE", 0, f"{prefix}_Trigger{i}")

    constants = files["constants/misc_constants.asm"].decode()
    hours = {name: int(re.search(rf"DEF {name}_HOUR\s+EQU\s+(\d+)", constants)[1])
             for name in ("MORN", "DAY", "EVE", "NITE")}
    assert hours == {"MORN": 5, "DAY": 9, "EVE": 17, "NITE": 21}
    hooks = """\tmap_script MAP_SCRIPT_ON_LOAD, PolishedMom_Visibility
\tmap_script MAP_SCRIPT_ON_RETURN_TO_FIELD, PolishedMom_Restore
"""
    code = """@ Source schedule is sampled at map load, never during conversation/menu return.
PolishedMom_Visibility::
\tsetflag FLAG_TEMP_2
\tsetflag FLAG_TEMP_3
\tsetflag FLAG_TEMP_4
\tsetflag FLAG_TEMP_5
\tsetflag FLAG_TEMP_6
\tgoto_if_eq VAR_POLISHED_HOME_SCENE, 0, PolishedMom_ShowInitial
\tgettime
\tgoto_if_lt VAR_0x8000, 5, PolishedMom_ShowNite
\tgoto_if_lt VAR_0x8000, 9, PolishedMom_ShowMorn
\tgoto_if_lt VAR_0x8000, 17, PolishedMom_ShowDay
\tgoto_if_lt VAR_0x8000, 21, PolishedMom_ShowEve
\tgoto PolishedMom_ShowNite
PolishedMom_ShowInitial::
\tclearflag FLAG_TEMP_2
\tend
PolishedMom_ShowMorn::
\tclearflag FLAG_TEMP_3
\tend
PolishedMom_ShowDay::
\tclearflag FLAG_TEMP_4
\tend
PolishedMom_ShowEve::
\tclearflag FLAG_TEMP_5
\tend
PolishedMom_ShowNite::
\tclearflag FLAG_TEMP_6
\tend

PolishedMom_Restore::
"""
    for name in names[:5]:
        code += f"\tcall PolishedMom_Restore{name}\n"
    code += "\tend\n\n"
    for i, name in enumerate(names[:5], 2):
        code += f"""PolishedMom_Restore{name}::
\tgoto_if_set FLAG_TEMP_{i}, PolishedMom_Hide{name}
\taddobject LOCALID_HOME_{name}
\treturn
PolishedMom_Hide{name}::
\tremoveobject LOCALID_HOME_{name}
\treturn

"""
    for i in range(1, 5):
        code += f"""PolishedMom_Trigger{i}::
\tlockall
\tapplymovement LOCALID_HOME_INITIAL, PolishedIntro_Exclamation
\twaitmovement 0
\tturnobject LOCALID_HOME_INITIAL, {'DIR_EAST' if i < 3 else 'DIR_NORTH'}
"""
        if i == 1:
            code += "\tturnobject LOCALID_PLAYER, DIR_WEST\n"
        else:
            code += f"\tapplymovement LOCALID_PLAYER, PolishedMom_Approach{i}\n\twaitmovement 0\n"
        code += "\tgoto PolishedMom_Intro\n\n"
    code += """PolishedMom_Approach2:
\twalk_slow_left
\tstep_end
PolishedMom_Approach3:
\twalk_down
\twalk_down
\tstep_end
PolishedMom_Approach4:
\twalk_slow_down
\tstep_end

PolishedMom_Talk::
\tlock
\tfaceplayer
\tgoto_if_eq VAR_POLISHED_HOME_SCENE, 0, PolishedMom_Intro
\tgoto_if_set FLAG_POLISHED_MOM_FIRST_VISIT, PolishedMom_FirstVisitAgain
\tgoto_if_set FLAG_POLISHED_MOM_BANK_OPEN, PolishedMom_Bank
\tgoto_if_set FLAG_POLISHED_GAVE_MYSTERY_EGG, PolishedMom_FirstBank
\tgoto_if_ne VAR_POLISHED_STARTER, 0, PolishedMom_Errand
\tmsgbox PolishedMom_ElmWaitingText, MSGBOX_DEFAULT
\trelease
\tend
PolishedMom_Errand::
\tmsgbox PolishedMom_MomErrandText, MSGBOX_DEFAULT
\trelease
\tend

PolishedMom_Intro::
\tmsgbox PolishedMom_MomIntroText, MSGBOX_DEFAULT
\tplayfanfare MUS_OBTAIN_ITEM
\tmsgbox PolishedMom_ReceivedGearText, MSGBOX_DEFAULT
\twaitfanfare
\tsetflag FLAG_POLISHED_POKEGEAR
\tsetflag FLAG_POLISHED_PHONE_CARD
\tsetflag FLAG_POLISHED_HAS_MOM_CONTACT
\tsetflag FLAG_SYS_B_DASH
\tsetvar VAR_POLISHED_HOME_SCENE, 1
\tmsgbox PolishedMom_MomPokegearText, MSGBOX_DEFAULT
PolishedMom_ChooseWeekday::
\tmsgbox PolishedMom_WhatDayText, MSGBOX_DEFAULT
\tmultichoice 18, 0, MULTI_POLISHED_WEEKDAYS, TRUE
\tcopyvar VAR_0x8004, VAR_RESULT
\tspecial PolishedJohtoBufferChosenWeekday
\tmsgbox PolishedMom_ConfirmWeekdayText, MSGBOX_YESNO
\tgoto_if_eq VAR_RESULT, NO, PolishedMom_ChooseWeekday
\tspecial PolishedJohtoCommitWeekday
PolishedMom_ChooseDST::
\tmsgbox PolishedMom_MomDSTText, MSGBOX_YESNO
\tgoto_if_eq VAR_RESULT, NO, PolishedMom_StandardTime
\tsetflag FLAG_POLISHED_DAYLIGHT_SAVING
\tspecial PolishedJohtoBufferClock
\tmsgbox PolishedMom_ConfirmDSTText, MSGBOX_YESNO
\tgoto PolishedMom_CheckTime
PolishedMom_StandardTime::
\tclearflag FLAG_POLISHED_DAYLIGHT_SAVING
\tspecial PolishedJohtoBufferClock
\tmsgbox PolishedMom_ConfirmTimeText, MSGBOX_YESNO
PolishedMom_CheckTime::
\tgoto_if_eq VAR_RESULT, NO, PolishedMom_ChooseDST
\tmsgbox PolishedMom_MomRunningShoesText, MSGBOX_YESNO
\tgoto_if_eq VAR_RESULT, YES, PolishedMom_ShoesKnown
\tmsgbox PolishedMom_MomInstructionsText, MSGBOX_DEFAULT
PolishedMom_ShoesKnown::
\tmsgbox PolishedMom_MomOutroText, MSGBOX_DEFAULT
\tclosemessage
\tturnobject LOCALID_HOME_INITIAL, DIR_WEST
\treleaseall
\tend

PolishedMom_Neighbor::
\tlock
\tfaceplayer
\tgettime
\tgoto_if_lt VAR_0x8000, 5, PolishedMom_NeighborEvening
\tgoto_if_lt VAR_0x8000, 9, PolishedMom_NeighborMorning
\tgoto_if_ge VAR_0x8000, 17, PolishedMom_NeighborEvening
\tmsgbox PolishedMom_NeighborDayText, MSGBOX_DEFAULT
\tgoto PolishedMom_NeighborMain
PolishedMom_NeighborMorning::
\tmsgbox PolishedMom_NeighborMornText, MSGBOX_DEFAULT
\tgoto PolishedMom_NeighborMain
PolishedMom_NeighborEvening::
\tmsgbox PolishedMom_NeighborNiteText, MSGBOX_DEFAULT
PolishedMom_NeighborMain::
\tmsgbox PolishedMom_NeighborText, MSGBOX_DEFAULT
\tclosemessage
\tturnobject LOCALID_HOME_NEIGHBOR, DIR_EAST
\trelease
\tend

"""
    texts = {name: source_text(script, name) for name in (
        "MomIntroText", "MomPokegearText", "MomDSTText", "MomRunningShoesText",
        "MomInstructionsText", "MomOutroText", "MomErrandText")}
    # The bank now supports returning to Mom to adjust the clock. The always-run
    # option still awaits its native Options entry.
    remove_clock = "Come home to\\nadjust your clock\\pfor Daylight\\nSaving Time.\\p"
    remove_option = "\\pOr use the Option\\nto always run, and\\lhold B to walk."
    assert texts["MomRunningShoesText"].startswith(remove_clock)
    assert texts["MomInstructionsText"].endswith(remove_option + "$")
    texts["MomInstructionsText"] = texts["MomInstructionsText"].removesuffix(remove_option + "$") + "$"
    waiting = script.split("MomScript:\n", 1)[1].split(".FirstTimeBanking:", 1)[0]
    waiting = "ElmWaitingText:\n" + waiting.split("jumpthisopenedtext\n", 1)[1]
    texts["ElmWaitingText"] = source_text(waiting, "ElmWaitingText")
    for target, label in (("NeighborMornText", ".MornIntroText"), ("NeighborDayText", ".DayIntroText"),
                          ("NeighborNiteText", ".NiteIntroText"), ("NeighborText", ".NeighborText")):
        texts[target] = source_text(script, label)
    texts.update(ReceivedGearText="{PLAYER} received\\nthe POKéGEAR!$",
        WhatDayText=source_text(clock_text, "_OakTimeWhatDayIsItText"),
        ConfirmWeekdayText="{STR_VAR_1}" + source_text(clock_text, "_OakTimeIsItText"),
        ConfirmDSTText="{STR_VAR_1}" + source_text(clock_text, "Text_DSTIsThatOK"),
        ConfirmTimeText="{STR_VAR_1}" + source_text(clock_text, "_TimeAskOkayText"))
    for name, text in texts.items():
        code += text_definition(prefix + "_" + name, text)
    code += polished_departure.bank(script, files, source_text, text_definition)
    return hooks, code, {
        "source_script": "MomEventScript", "source_scene": 0, "next_scene": 1,
        "source_triggers": [[10, 4], [11, 4], [9, 1], [9, 2]],
        "source_schedule_hours": hours, "objects": header["object_events"],
        "clock": "native RTC weekday bias; initial DST flag does not adjust hours",
        "bank": "Post-Egg offer, prize sharing, Get/Save/Change and DST",
        "deferred": ["Pokégear phone interface", "always-run option", "Mom's remote shopping"],
        "text_adaptations": ["omit unavailable always-run option instructions"],
    }
