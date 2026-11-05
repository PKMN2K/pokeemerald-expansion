"""Route 29 first-visit events from Polished Crystal v3.2.3.

Tuscany and the catching tutorial are activated by their original quest conditions.
"""
import re
import polished_departure
from new_bark_intro import coord, movement, obj


def object_at(local, graphics, x, y, script, flag="0"):
    return {"local_id": local, "graphics_id": "OBJ_EVENT_GFX_" + graphics,
        "x": x, "y": y, "elevation": 3, "movement_type": "MOVEMENT_TYPE_FACE_DOWN",
        "movement_range_x": 0, "movement_range_y": 0, "trainer_type": "TRAINER_TYPE_NONE",
        "trainer_sight_or_berry_tree_id": "0", "script": script, "flag": flag}


def events(name, script, header, files, text, define):
    if name == "Route2946Gate":
        return "", "", {"source_warp_to_native": {"3": 0, "4": 1},
            "deferred": "Route 46 north warps 1/2; cells 4,0 and 5,0 remain blocked"}
    if name == "NewBarkTown":
        header["connections"] = [{"map": "MAP_POLISHED_ROUTE_29", "offset": 0, "direction": "left"}]
        code = "PolishedRoute29_SetRespawn::\n\tsetrespawn HEAL_LOCATION_POLISHED_NEW_BARK_TOWN\n\tend\n\n"
        for n, y in ((1, 8), (2, 9)):
            p = f"PolishedRoute29_Stop{n}"
            coord(header, 1, y, "VAR_POLISHED_STARTER", 0, p)
            code += f"""{p}::
\tlockall
\tmsgbox PolishedRoute29_Text_WaitPlayer, MSGBOX_DEFAULT
\tclosemessage
\tturnobject OBJ_EVENT_ID_PLAYER, DIR_EAST
\tapplymovement LOCALID_NEW_BARK_TEACHER, {p}_Approach
\twaitmovement 0
\tmsgbox PolishedRoute29_Text_WhatDoYouThinkYoureDoing, MSGBOX_DEFAULT
\tclosemessage
\tapplymovement LOCALID_NEW_BARK_TEACHER, {p}_Return
\tapplymovement OBJ_EVENT_ID_PLAYER, {p}_Follow
\twaitmovement OBJ_EVENT_ID_PLAYER
\twaitmovement LOCALID_NEW_BARK_TEACHER
\tmsgbox PolishedRoute29_Text_ItsDangerousToGoAlone, MSGBOX_DEFAULT
\treleaseall
\tend

"""
            code += movement(script, f"Movement_TeacherRunsToYou{n}_NBT", p + "_Approach")
            code += movement(script, f"Movement_TeacherBringsYouBack{n}_NBT", p + "_Return")
            code += p + "_Follow:\n" + ("\twalk_up\n" if n == 2 else "") + "\twalk_right\n" * 4 + "\tstep_end\n\n"
        for label in ("Text_WaitPlayer", "Text_WhatDoYouThinkYoureDoing", "Text_ItsDangerousToGoAlone"):
            code += define("PolishedRoute29_" + label, text(script, label))
        return "\tmap_script MAP_SCRIPT_ON_TRANSITION, PolishedRoute29_SetRespawn\n", code, {"west_connection": "Route29", "exit_guard": "Both source approach rows; requires Elm starter", "blackout": "New Bark Town 6,5, native full-party recovery"}
    header["connections"] = [{"map": "MAP_POLISHED_NEW_BARK_TOWN", "offset": 0, "direction": "right"},
        {"map": "MAP_POLISHED_CHERRYGROVE_CITY", "offset": 0, "direction": "left"}]
    npc = obj(script, "Route29CooltrainerMScript", "LOCALID_ROUTE29_COOL_DUDE", "COOL_DUDE")
    npc["script"] = "PolishedRoute29_CoolDude"
    header["object_events"].append(npc)
    for n, (x, y) in enumerate(((30, 9), (21, 11)), 1):
        assert re.search(rf"cuttree_event\s+{x},\s*{y}, EVENT_ROUTE_29_CUT_TREE_{n}", script)
        header["object_events"].append(object_at(f"LOCALID_ROUTE29_CUT_{n}", "POLISHED_CUT_TREE", x, y,
            "EventScript_CutTree", f"FLAG_POLISHED_ROUTE29_CUT_{n}"))
    header["object_events"].append(object_at("LOCALID_ROUTE29_FRUIT", "VAR_F", 12, 2, "PolishedRoute29_Fruit"))
    header["object_events"].append(object_at("LOCALID_ROUTE29_POTION", "POLISHED_BALL_RED", 48, 2,
        "PolishedRoute29_Potion", "FLAG_POLISHED_ROUTE29_POTION"))
    code = """PolishedRoute29_Setup::
\tspecial PolishedRoute29RefreshFruit
\tend

PolishedRoute29_CoolDude::
\tlock
\tfaceplayer
\tgettime
\tgoto_if_lt VAR_0x8000, 5, PolishedRoute29_CoolDudeNight
\tgoto_if_ge VAR_0x8000, 17, PolishedRoute29_CoolDudeNight
\tmsgbox PolishedRoute29_Text_WaitingForNight, MSGBOX_DEFAULT
\trelease
\tend
PolishedRoute29_CoolDudeNight::
\tmsgbox PolishedRoute29_Text_WaitingForMorning, MSGBOX_DEFAULT
\trelease
\tend

PolishedRoute29_Potion::
\tfinditem ITEM_POTION
\tend

PolishedRoute29_Fruit::
	setvar VAR_0x8005, 0
	goto PolishedFruit_Interact

PolishedFruit_Interact::
	lockall
	special PolishedFruitPrepare
	msgbox PolishedRoute29_FruitBearing, MSGBOX_DEFAULT
	specialvar VAR_RESULT, PolishedFruitAvailable
	goto_if_eq VAR_RESULT, FALSE, PolishedFruit_None
	bufferitemname STR_VAR_3, VAR_0x8006
	msgbox PolishedRoute29_FruitHey, MSGBOX_DEFAULT
	special PolishedFruitPick
	goto_if_eq VAR_RESULT, 0, PolishedFruit_Full
	copyvar VAR_0x8004, VAR_RESULT
	goto_if_eq VAR_RESULT, 1, PolishedFruit_One
	goto_if_eq VAR_RESULT, 2, PolishedFruit_Two
	msgbox PolishedRoute29_FruitObtainedThree, MSGBOX_DEFAULT
	goto PolishedFruit_Done
PolishedFruit_One::
	msgbox PolishedRoute29_FruitObtainedOne, MSGBOX_DEFAULT
	goto PolishedFruit_Done
PolishedFruit_Two::
	msgbox PolishedRoute29_FruitObtainedTwo, MSGBOX_DEFAULT
PolishedFruit_Done::
	playfanfare MUS_OBTAIN_ITEM
	waitfanfare
	putitemaway VAR_0x8006, VAR_0x8004
	releaseall
	end
PolishedFruit_Full::
	msgbox PolishedRoute29_FruitPackFull, MSGBOX_DEFAULT
	releaseall
	end
PolishedFruit_None::
	msgbox PolishedRoute29_FruitNothing, MSGBOX_DEFAULT
	checkitem ITEM_POLISHED_MULCH
	goto_if_eq VAR_RESULT, FALSE, PolishedFruit_End
	msgbox PolishedFruit_WantMulch, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, PolishedFruit_End
	special PolishedFruitUseMulch
	goto_if_eq VAR_RESULT, FALSE, PolishedFruit_End
	msgbox PolishedFruit_UsedMulch, MSGBOX_DEFAULT
PolishedFruit_End::
	releaseall
	end

"""
    for label in ("Text_WaitingForNight", "Text_WaitingForMorning"):
        code += define("PolishedRoute29_" + label, text(script, label))
    common = files["data/text/common.asm"].decode()
    # SECTION lines separate the globally exported common text labels.
    common = re.sub(r'^SECTION .*\n', '', common, flags=re.M)
    for label, source in {"Bearing": "_FruitBearingTreeText", "Hey": "_HeyItsFruitText",
        "ObtainedOne": "_ObtainedOneFruitText", "ObtainedTwo": "_ObtainedTwoFruitText",
        "ObtainedThree": "_ObtainedThreeFruitText", "PackFull": "_FruitPackIsFullText",
        "Nothing": "_NothingHereText"}.items():
        code += define("PolishedRoute29_Fruit" + label, text(common, source))
    for key, label in (("WantMulch", "_WantToUseMulchText"), ("UsedMulch", "_UsedMulchText")):
        code += define("PolishedFruit_" + key, text(common, label))
    code = code.replace("PolishedRoute29_Setup::\n", "PolishedRoute29_Setup::\n\tcall PolishedDeparture_RouteSetup\n")
    code += polished_departure.route(script, header, text, define, object_at)
    code += """PolishedDeparture_Restore::
    call PolishedDeparture_RouteSetup
    special PolishedRoute29RefreshFruit
    goto_if_unset FLAG_TEMP_2, PolishedDeparture_ShowLyra
    removeobject LOCALID_ROUTE29_LYRA
    goto PolishedDeparture_RestoreTuscany
PolishedDeparture_ShowLyra::
    addobject LOCALID_ROUTE29_LYRA
PolishedDeparture_RestoreTuscany::
    goto_if_unset FLAG_TEMP_3, PolishedDeparture_ShowTuscany
    removeobject LOCALID_ROUTE29_TUSCANY
    end
PolishedDeparture_ShowTuscany::
    addobject LOCALID_ROUTE29_TUSCANY
    end
"""
    return ("\tmap_script MAP_SCRIPT_ON_LOAD, PolishedRoute29_Setup\n"
            "\tmap_script MAP_SCRIPT_ON_RETURN_TO_FIELD, PolishedDeparture_Restore\n"), code, {
        "first_visit": "4 residents, 3 signs, Potion, 2 persistent Cut trees, daily Oran Berry tree",
        "berry_yield": "Random 1-3, falls back to fitting quantity; commit daily stamp only after addition",
        "tips_translation": "Unavailable GBC title shortcuts replaced by encounter-time advice",
        "post_egg": "Two Lyra approaches, optional Pidgey tutorial, five Balls, Tuesday Tuscany",
        "deferred": ["Route46"]}


def wild_entries(files, route=29):
    body = re.search(rf"def_grass_wildmons ROUTE_{route}\n(.*?)end_grass_wildmons",
        files["data/wild/johto_grass.asm"].decode(), re.S)[1]
    slots = [(int(level), species) for level, species in re.findall(r"wildmon (\d+), (\w+)", body)]
    assert len(slots) == 21 and "10 percent, 10 percent, 10 percent" in body
    # Native 12-slot weights reproduce the source species/level distribution.
    # Source slots 5/6 are identical, so their 3%+2% can use native 4%+1%.
    assignment = [0, 1, 0, 1, 2, 2, 3, 3, 4, 5, 4, 6]
    result = []
    for n, period in enumerate(("Morning", "Day", "Night")):
        group = slots[n*7:n*7+7]
        assert group[5] == group[6]
        result.append({"map": f"MAP_POLISHED_ROUTE_{route}", "base_label": f"gPolishedRoute{route}_" + period,
            "always_time_of_day": True, "land_mons": {"encounter_rate": 18,
                "mons": [{"min_level": group[i][0], "max_level": group[i][0], "species": "SPECIES_" + group[i][1]} for i in assignment]}})
    return result
