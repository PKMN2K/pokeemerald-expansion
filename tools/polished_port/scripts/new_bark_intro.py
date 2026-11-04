"""Native event translation of the pinned Lyra/Elm introduction and starters.

Source scene numbers are retained. Scene 6 hands off to new_bark_battle.
"""
import re


def movement(script, label, target):
    # Source movement labels deliberately fall through (Lyra's three choices).
    match = re.search(r"^" + re.escape(label) + r":\n(.*?)^\s*step_end\s*$", script, re.M | re.S)
    assert match, label
    body = match[1]
    commands = []
    for line in body.splitlines():
        if not line.strip() or line.endswith(":"):
            continue
        command = line.strip()
        assert re.fullmatch(r"(step|turn_head)_(up|down|left|right)", command), (label, command)
        commands.append(command.replace("turn_head_", "face_").replace("step_", "walk_"))
    return target + ":\n" + "".join("\t" + cmd + "\n" for cmd in commands) + "\tstep_end\n\n"


def obj(script, source_label, local, graphics, flag="0", facing="DOWN"):
    fields = next([v.strip() for v in line.split(",")] for line in
        re.findall(r"^\s*object_event (.*)$", script, re.M) if f", {source_label}," in line)
    return {"local_id": local, "graphics_id": "OBJ_EVENT_GFX_POLISHED_" + graphics,
        "x": int(fields[0]), "y": int(fields[1]), "elevation": 3,
        "movement_type": "MOVEMENT_TYPE_FACE_" + facing,
        "movement_range_x": 0, "movement_range_y": 0, "trainer_type": "TRAINER_TYPE_NONE",
        "trainer_sight_or_berry_tree_id": "0", "script": "PolishedIntro_" + source_label,
        "flag": flag}


def coord(header, x, y, variable, value, event):
    header["coord_events"].append({"type": "trigger", "x": x, "y": y, "elevation": 3,
        "var": variable, "var_value": str(value), "script": event})


def town(script, header, source_text, text_definition):
    lyra = obj(script, "ObjectEvent", "LOCALID_NEW_BARK_LYRA", "LYRA", "FLAG_TEMP_2", "RIGHT")
    lyra["script"] = "PolishedIntro_NewBarkTownLyraScript"
    header["object_events"].append(lyra)
    assert re.search(r"coord_event\s+6,\s+4,\s+0, NewBarkTown_LyraIntroTrigger", script)
    coord(header, 6, 4, "VAR_POLISHED_NEW_BARK_SCENE", 0, "PolishedIntro_TownApproach")
    code = """PolishedIntro_TownSetup::
	setflag FLAG_TEMP_2
	end

PolishedIntro_NewBarkTownLyraScript::
	end

PolishedIntro_TownApproach::
	lockall
	addobject LOCALID_NEW_BARK_LYRA
	applymovement LOCALID_NEW_BARK_LYRA, PolishedIntro_TownEnters
	waitmovement 0
	applymovement LOCALID_NEW_BARK_LYRA, PolishedIntro_Exclamation
	waitmovement 0
	applymovement LOCALID_NEW_BARK_LYRA, PolishedIntro_TownApproaches
	waitmovement 0
	turnobject LOCALID_PLAYER, DIR_WEST
	msgbox PolishedIntro_Text_LyraIntro, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_PLAYER, PolishedIntro_StepUp
	applymovement LOCALID_NEW_BARK_LYRA, PolishedIntro_StepRight
	waitmovement 0
	playse SE_DOOR
	hideplayer
	applymovement LOCALID_NEW_BARK_LYRA, PolishedIntro_StepUp
	waitmovement 0
	playse SE_DOOR
	removeobject LOCALID_NEW_BARK_LYRA
	setvar VAR_POLISHED_NEW_BARK_SCENE, 2
	warp MAP_POLISHED_ELMS_LAB, 4, 11
	waitstate
	releaseall
	end

PolishedIntro_StepUp:
	walk_up
	step_end
PolishedIntro_StepRight:
	walk_right
	step_end
PolishedIntro_Exclamation:
	emote_exclamation_mark
	step_end

"""
    code += movement(script, "Movement_LyraEnters_NBT", "PolishedIntro_TownEnters")
    code += movement(script, "Movement_LyraApproaches_NBT", "PolishedIntro_TownApproaches")
    code += text_definition("PolishedIntro_Text_LyraIntro", source_text(script, "Text_LyraIntro"))
    return "\tmap_script MAP_SCRIPT_ON_TRANSITION, PolishedIntro_TownSetup\n", code, {
        "source_script": "NewBarkTown_LyraIntroTrigger", "trigger": [6, 4],
        "source_scene": 0, "next_scene": 2, "arrival": [4, 11], "event": lyra}


def lab(script, header, source_text, text_definition):
    objects = [obj(script, "ProfElmScript", "LOCALID_ELMS_LAB_ELM", "ELM"),
        obj(script, "ElmsAideScript", "LOCALID_ELMS_LAB_AIDE", "SCIENTIST"),
        obj(script, "CyndaquilPokeBallScript", "LOCALID_ELMS_LAB_CYNDAQUIL", "BALL_RED", "FLAG_TEMP_2"),
        obj(script, "TotodilePokeBallScript", "LOCALID_ELMS_LAB_TOTODILE", "BALL_BLUE", "FLAG_TEMP_3"),
        obj(script, "ChikoritaPokeBallScript", "LOCALID_ELMS_LAB_CHIKORITA", "BALL_GREEN", "FLAG_TEMP_4"),
        obj(script, "ElmsLabLyraScript", "LOCALID_ELMS_LAB_LYRA", "LYRA", facing="UP")]
    objects[1]["movement_type"] = "MOVEMENT_TYPE_LOOK_AROUND"
    header["object_events"].extend(objects)
    for x in (4, 5):
        assert re.search(rf"coord_event\s+{x},\s+6,\s+1, LabTryToLeaveScript", script)
        coord(header, x, 6, "VAR_POLISHED_ELMS_LAB_SCENE", 1, "PolishedIntro_TryToLeave")
    hooks = """	map_script MAP_SCRIPT_ON_LOAD, PolishedIntro_LabVisibility
	map_script MAP_SCRIPT_ON_TRANSITION, PolishedIntro_LabPositions
	map_script MAP_SCRIPT_ON_RETURN_TO_FIELD, PolishedIntro_LabRestore
	map_script MAP_SCRIPT_ON_FRAME_TABLE, PolishedIntro_LabFrame
"""
    code = """@ Ball visibility derives from the saved choice; no extra permanent flags.
PolishedIntro_LabVisibility::
	clearflag FLAG_TEMP_5
	call_if_eq VAR_POLISHED_ELMS_LAB_SCENE, 5, PolishedBattle_HideLyra
	call_if_eq VAR_POLISHED_ELMS_LAB_SCENE, 2, PolishedBattle_HideLyra
	clearflag FLAG_TEMP_2
	clearflag FLAG_TEMP_3
	clearflag FLAG_TEMP_4
	goto_if_eq VAR_POLISHED_MR_POKEMON_SCENE, 1, PolishedRoute30_AllBallsTaken
	goto_if_eq VAR_POLISHED_STARTER, 1, PolishedIntro_HideCyndaquilChikorita
	goto_if_eq VAR_POLISHED_STARTER, 2, PolishedIntro_HideTotodileCyndaquil
	goto_if_eq VAR_POLISHED_STARTER, 3, PolishedIntro_HideChikoritaTotodile
	end
PolishedRoute30_AllBallsTaken::
	setflag FLAG_TEMP_2
	setflag FLAG_TEMP_3
	setflag FLAG_TEMP_4
	end
PolishedIntro_HideCyndaquilChikorita::
	setflag FLAG_TEMP_2
	setflag FLAG_TEMP_4
	end
PolishedIntro_HideTotodileCyndaquil::
	setflag FLAG_TEMP_3
	setflag FLAG_TEMP_2
	end
PolishedIntro_HideChikoritaTotodile::
	setflag FLAG_TEMP_4
	setflag FLAG_TEMP_3
	end

@ Continue restores saved actors; reconcile them with the map-load masks.
@ Nickname/menu returns retain the current masks and choreography.
PolishedIntro_LabRestore::
	call_if_set FLAG_TEMP_5, PolishedBattle_RemoveLyra
	call_if_set FLAG_TEMP_2, PolishedIntro_RemoveCyndaquil
	call_if_set FLAG_TEMP_3, PolishedIntro_RemoveTotodile
	call_if_set FLAG_TEMP_4, PolishedIntro_RemoveChikorita
	end
PolishedIntro_RemoveCyndaquil::
	removeobject LOCALID_ELMS_LAB_CYNDAQUIL
	return
PolishedIntro_RemoveTotodile::
	removeobject LOCALID_ELMS_LAB_TOTODILE
	return
PolishedIntro_RemoveChikorita::
	removeobject LOCALID_ELMS_LAB_CHIKORITA
	return

PolishedIntro_LabPositions::
	goto_if_eq VAR_POLISHED_ELMS_LAB_SCENE, 0, PolishedIntro_LabInitialPositions
	setobjectxyperm LOCALID_ELMS_LAB_ELM, 5, 2
	goto_if_eq VAR_POLISHED_ELMS_LAB_SCENE, 1, PolishedIntro_LabChoosingPositions
	setobjectxyperm LOCALID_ELMS_LAB_LYRA, 5, 5
	end
PolishedIntro_LabChoosingPositions::
	setobjectxyperm LOCALID_ELMS_LAB_LYRA, 4, 5
	end
PolishedIntro_LabInitialPositions::
	setobjectxyperm LOCALID_ELMS_LAB_ELM, 3, 4
	setobjectxyperm LOCALID_ELMS_LAB_LYRA, 5, 11
	end

PolishedIntro_LabFrame:
	map_script_2 VAR_POLISHED_ELMS_LAB_SCENE, 0, PolishedIntro_WalkToElm
	.2byte 0

PolishedIntro_WalkToElm::
	lockall
	showplayer
	applymovement LOCALID_PLAYER, PolishedIntro_WalkUpToElm
	applymovement LOCALID_ELMS_LAB_LYRA, PolishedIntro_LyraFollowElm
	waitmovement 0
	applymovement LOCALID_ELMS_LAB_ELM, PolishedIntro_Exclamation
	waitmovement 0
	turnobject LOCALID_ELMS_LAB_ELM, DIR_EAST
	msgbox PolishedIntro_ElmText_Intro, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, YES, PolishedIntro_ElmAccepted
PolishedIntro_ElmRefused::
	msgbox PolishedIntro_ElmText_Refused, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, PolishedIntro_ElmRefused
PolishedIntro_ElmAccepted::
	msgbox PolishedIntro_ElmText_Accepted, MSGBOX_DEFAULT
	msgbox PolishedIntro_ElmText_ResearchAmbitions, MSGBOX_DEFAULT
	closemessage
	playse SE_PC_LOGIN
	delay 30
	applymovement LOCALID_ELMS_LAB_ELM, PolishedIntro_Exclamation
	waitmovement 0
	turnobject LOCALID_ELMS_LAB_ELM, DIR_SOUTH
	msgbox PolishedIntro_ElmText_GotAnEmail, MSGBOX_DEFAULT
	turnobject LOCALID_ELMS_LAB_ELM, DIR_EAST
	msgbox PolishedIntro_ElmText_MissionFromMrPokemon, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_ELMS_LAB_ELM, PolishedIntro_StepUp
	waitmovement 0
	turnobject LOCALID_PLAYER, DIR_NORTH
	applymovement LOCALID_ELMS_LAB_ELM, PolishedIntro_ElmDefaultPosition
	waitmovement 0
	turnobject LOCALID_PLAYER, DIR_EAST
	msgbox PolishedIntro_ElmText_ChooseAPokemon, MSGBOX_DEFAULT
	setvar VAR_POLISHED_ELMS_LAB_SCENE, 1
	releaseall
	end

PolishedIntro_ProfElmScript::
	lock
	faceplayer
	goto_if_ne VAR_POLISHED_STARTER, 0, PolishedIntro_ElmAfterStarter
	msgbox PolishedIntro_ElmText_LetYourMonBattleIt, MSGBOX_DEFAULT
	release
	end
PolishedIntro_ElmAfterStarter::
	msgbox PolishedIntro_ElmDescribesMrPokemonText, MSGBOX_DEFAULT
	release
	end
PolishedIntro_ElmsLabLyraScript::
	lock
	faceplayer
	goto_if_ne VAR_POLISHED_STARTER, 0, PolishedIntro_LyraAfterStarter
	msgbox PolishedIntro_ElmsLabLyraWhichPokemonText, MSGBOX_DEFAULT
	release
	end
PolishedIntro_LyraAfterStarter::
	msgbox PolishedIntro_ElmsLabLyraGoodChoiceText, MSGBOX_DEFAULT
	release
	end
PolishedIntro_ElmsAideScript::
	msgbox PolishedIntro_AideText_AlwaysBusy, MSGBOX_NPC
	end

PolishedIntro_TryToLeave::
	lockall
	turnobject LOCALID_ELMS_LAB_ELM, DIR_SOUTH
	msgbox PolishedIntro_LabWhereGoingText, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_PLAYER, PolishedIntro_StepUp
	waitmovement 0
	releaseall
	end

PolishedIntro_DidNotChoose::
	msgbox PolishedIntro_DidntChooseStarterText, MSGBOX_DEFAULT
	releaseall
	end
PolishedIntro_RemainingBall::
	msgbox PolishedIntro_ElmPokeBallText, MSGBOX_DEFAULT
	releaseall
	end
PolishedIntro_NoRoom::
	msgbox gText_NoMoreRoomForPokemon, MSGBOX_DEFAULT
	releaseall
	end

PolishedIntro_NicknameParty::
	msgbox gText_NicknameThisPokemon, MSGBOX_YESNO
	call_if_eq VAR_RESULT, YES, PolishedIntro_NameParty
	return
PolishedIntro_NameParty::
	call Common_EventScript_GetGiftMonPartySlot
	call Common_EventScript_NameReceivedPartyMon
	lockall
	return
PolishedIntro_NicknameBox::
	msgbox gText_NicknameThisPokemon, MSGBOX_YESNO
	call_if_eq VAR_RESULT, YES, PolishedIntro_NameBox
	return
PolishedIntro_NameBox::
	call Common_EventScript_NameReceivedBoxMon
	lockall
	return

PolishedIntro_Directions::
	turnobject LOCALID_PLAYER, DIR_NORTH
	msgbox PolishedIntro_ElmDirectionsText1, MSGBOX_DEFAULT
	setflag FLAG_POLISHED_HAS_ELM_CONTACT
	playfanfare MUS_REGISTER_MATCH_CALL
	msgbox PolishedIntro_GotElmsNumberText, MSGBOX_DEFAULT
	waitfanfare
	turnobject LOCALID_ELMS_LAB_ELM, DIR_WEST
	msgbox PolishedIntro_ElmDirectionsText2, MSGBOX_DEFAULT
	turnobject LOCALID_ELMS_LAB_ELM, DIR_SOUTH
	msgbox PolishedIntro_ElmDirectionsText3, MSGBOX_DEFAULT
	setvar VAR_POLISHED_ELMS_LAB_SCENE, 6
	releaseall
	end

PolishedIntro_LyraFollowElm:
	walk_left
	walk_up
	walk_up
	walk_up
	walk_up
	walk_up
	walk_up
	face_up
	step_end

"""
    starters = ("Cyndaquil", "Totodile", "Chikorita")
    choices = []
    for index, species in enumerate(starters, 1):
        lyra_species = starters[(index + 1) % 3]
        event = "PolishedIntro_" + species
        # Capture facing before the nickname screen, which calls faceplayer.
        code += f"""{event}PokeBallScript::
	lockall
	goto_if_ne VAR_POLISHED_STARTER, 0, PolishedIntro_RemainingBall
	getobjectfacingdirection LOCALID_PLAYER, VAR_TEMP_6
	turnobject LOCALID_ELMS_LAB_ELM, DIR_SOUTH
	showmonpic SPECIES_{species.upper()}, 10, 3
	playmoncry SPECIES_{species.upper()}, CRY_MODE_NORMAL
	waitmoncry
	waitbuttonpress
	hidemonpic
	msgbox PolishedIntro_Take{species}Text, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, PolishedIntro_DidNotChoose
	msgbox PolishedIntro_ChoseStarterText, MSGBOX_DEFAULT
	setvar VAR_TEMP_TRANSFERRED_SPECIES, SPECIES_{species.upper()}
	givemon SPECIES_{species.upper()}, 5, ITEM_ORAN_BERRY
	goto_if_eq VAR_RESULT, MON_CANT_GIVE, PolishedIntro_NoRoom
	copyvar VAR_TEMP_5, VAR_RESULT
	setvar VAR_POLISHED_STARTER, {index}
	removeobject LOCALID_ELMS_LAB_{species.upper()}
	bufferspeciesname STR_VAR_1, SPECIES_{species.upper()}
	playfanfare MUS_OBTAIN_ITEM
	msgbox PolishedIntro_ReceivedGift, MSGBOX_DEFAULT
	waitfanfare
	call_if_eq VAR_TEMP_5, MON_GIVEN_TO_PARTY, PolishedIntro_NicknameParty
	call_if_eq VAR_TEMP_5, MON_GIVEN_TO_PC, PolishedIntro_NicknameBox
	call_if_eq VAR_TEMP_5, MON_GIVEN_TO_PC, Common_EventScript_TransferredToPC
	msgbox PolishedIntro_LyraChoosesStarterText, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_ELMS_LAB_LYRA, {event}_LyraPick
	waitmovement 0
	delay 15
	removeobject LOCALID_ELMS_LAB_{lyra_species.upper()}
	bufferspeciesname STR_VAR_3, SPECIES_{lyra_species.upper()}
	playfanfare MUS_OBTAIN_ITEM
	msgbox PolishedIntro_LyraReceivedStarterText, MSGBOX_DEFAULT
	waitfanfare
	msgbox PolishedIntro_LyraNicknamed{lyra_species}Text, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_ELMS_LAB_LYRA, {event}_LyraAfter
	waitmovement 0
"""
        if index == 1:
            code += "\tgoto_if_eq VAR_TEMP_6, DIR_EAST, PolishedIntro_Directions\n"
        code += f"\tapplymovement LOCALID_PLAYER, {event}_PlayerAfter\n\twaitmovement 0\n\tgoto PolishedIntro_Directions\n\n"
        for source, target in ((f"LyraPicks{lyra_species}Movement", event + "_LyraPick"),
                               (f"LyraAfter{lyra_species}Movement", event + "_LyraAfter"),
                               (f"After{species}Movement", event + "_PlayerAfter")):
            code += movement(script, source, target)
        choices.append({"value": index, "species": species, "level": 5, "held_item": "ORAN_BERRY",
            "lyra_species": lyra_species, "lyra_nickname": {"Chikorita": "Chicory", "Cyndaquil": "Cinder", "Totodile": "Toto"}[lyra_species]})
    for source, target in (("ElmsLab_WalkUpToElmMovement", "PolishedIntro_WalkUpToElm"),
                           ("ElmsLab_ElmToDefaultPositionMovement", "PolishedIntro_ElmDefaultPosition")):
        code += movement(script, source, target)
    labels = ("ElmText_Intro ElmText_Refused ElmText_Accepted ElmText_ResearchAmbitions "
        "ElmText_GotAnEmail ElmText_MissionFromMrPokemon ElmText_ChooseAPokemon ElmText_LetYourMonBattleIt "
        "ElmDescribesMrPokemonText ElmsLabLyraWhichPokemonText ElmsLabLyraGoodChoiceText AideText_AlwaysBusy "
        "LabWhereGoingText DidntChooseStarterText ElmPokeBallText ElmDirectionsText1 ElmDirectionsText2 "
        "ElmDirectionsText3 GotElmsNumberText ChoseStarterText LyraChoosesStarterText LyraReceivedStarterText").split()
    labels += [f"Take{s}Text" for s in starters] + [f"LyraNicknamed{s}Text" for s in starters]
    for label in labels:
        code += text_definition("PolishedIntro_" + label, source_text(script, label))
    # The source givepoke command supplies this receipt before its nickname UI.
    code += text_definition("PolishedIntro_ReceivedGift", "{PLAYER} received\\n{STR_VAR_1}!$")
    return hooks, code, {"source_script": "ElmsLab_AutowalkUpToElm", "scenes": [0, 1, 6],
        "choices": choices, "events": objects, "next_phase": "LyraBattleScript (new_bark_battle.py)"}
