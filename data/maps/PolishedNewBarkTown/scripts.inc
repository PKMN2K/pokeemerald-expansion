@ Generated from pinned NewBarkTown.asm: geography, static text and ambient residents.
PolishedNewBarkTown_MapScripts::
	map_script MAP_SCRIPT_ON_LOAD, PolishedNewBarkTown_ResidentSchedule
	map_script MAP_SCRIPT_ON_RETURN_TO_FIELD, PolishedNewBarkTown_ResidentSchedule_Restore
	map_script MAP_SCRIPT_ON_TRANSITION, PolishedIntro_TownSetup
	map_script MAP_SCRIPT_ON_TRANSITION, PolishedRoute29_SetRespawn
	.byte 0

@ Map-local visibility, recomputed on entry and save loading.
PolishedNewBarkTown_ResidentSchedule::
	gettime
	goto_if_lt VAR_0x8000, 5, PolishedNewBarkTown_ResidentSchedule_Evening
	goto_if_ge VAR_0x8000, 17, PolishedNewBarkTown_ResidentSchedule_Evening
	clearflag FLAG_TEMP_1
	end
PolishedNewBarkTown_ResidentSchedule_Evening::
	setflag FLAG_TEMP_1
	end

@ Continue restores saved actors; reconcile only this scheduled resident with
@ the freshly computed mask. Ordinary menu returns retain the existing mask.
PolishedNewBarkTown_ResidentSchedule_Restore::
	goto_if_set FLAG_TEMP_1, PolishedNewBarkTown_ResidentSchedule_Hide
	addobject LOCALID_NEW_BARK_SCHOOLBOY
	end
PolishedNewBarkTown_ResidentSchedule_Hide::
	removeobject LOCALID_NEW_BARK_SCHOOLBOY
	end

PolishedNewBarkTown_NewBarkTownSignText::
	msgbox PolishedNewBarkTown_NewBarkTownSignText_Text, MSGBOX_SIGN
	end

PolishedNewBarkTown_NewBarkTownSignText_Text:
	.string "New Bark Town\pThe Town Where the\nWinds of a New\lBeginning Blow$"

PolishedNewBarkTown_PlayersHouseSignText::
	msgbox PolishedNewBarkTown_PlayersHouseSignText_Text, MSGBOX_SIGN
	end

PolishedNewBarkTown_PlayersHouseSignText_Text:
	.string "{PLAYER}'s House$"

PolishedNewBarkTown_ElmsLabSignText::
	msgbox PolishedNewBarkTown_ElmsLabSignText_Text, MSGBOX_SIGN
	end

PolishedNewBarkTown_ElmsLabSignText_Text:
	.string "Elm POKéMON Lab$"

PolishedNewBarkTown_LyrasHouseSignText::
	msgbox PolishedNewBarkTown_LyrasHouseSignText_Text, MSGBOX_SIGN
	end

PolishedNewBarkTown_LyrasHouseSignText_Text:
	.string "Lyra's House$"

PolishedNewBarkTown_Teacher::
	lock
	faceplayer
	msgbox PolishedNewBarkTown_Breeze, MSGBOX_DEFAULT
	msgbox PolishedNewBarkTown_Garden, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, PolishedNewBarkTown_Stay
	closemessage
	warp MAP_POLISHED_JOHTO_DEMO_OUTDOOR, 6, 8
	waitstate
PolishedNewBarkTown_Stay::
	release
	end

PolishedNewBarkTown_Breeze:
	.string "There's always\nsuch a refreshing\lbreeze here.$"

PolishedNewBarkTown_Garden:
	.string "Return to the demonstration\ngarden?$"

PolishedNewBarkTown_Text_ElmDiscoveredNewMon::
	msgbox PolishedNewBarkTown_Text_ElmDiscoveredNewMon_Text, MSGBOX_NPC
	end

PolishedNewBarkTown_Text_ElmDiscoveredNewMon_Text:
	.string "Yo, {PLAYER}!\pI hear Prof.Elm\ndiscovered some\lnew POKéMON.$"

PolishedNewBarkTown_Text_GearIsImpressive::
	msgbox PolishedNewBarkTown_Text_GearIsImpressive_Text, MSGBOX_NPC
	end

PolishedNewBarkTown_Text_GearIsImpressive_Text:
	.string "Wow, your POKégear\nis impressive!\pDid your mom get\nit for you?$"

@ The west approach (2,2) is the only walkable adjacent tile.
@ Source follow queues one left step into the player's vacated cell; express
@ those two simultaneous moves directly without replacing any global follower.
@ Crystal jump_step_down travels TWO cells (Jump and Land), hence jump_2_down.
PolishedNewBarkTown_Rival::
	lockall
	msgbox PolishedNewBarkTown_Rival_Text1, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_NEW_BARK_RIVAL, PolishedNewBarkTown_Rival_FaceLeft
	waitmovement LOCALID_NEW_BARK_RIVAL
	msgbox PolishedNewBarkTown_Rival_Text2, MSGBOX_DEFAULT
	closemessage
	applymovement OBJ_EVENT_ID_PLAYER, PolishedNewBarkTown_Rival_BackStep
	applymovement LOCALID_NEW_BARK_RIVAL, PolishedNewBarkTown_Rival_StepLeft
	waitmovement OBJ_EVENT_ID_PLAYER
	waitmovement LOCALID_NEW_BARK_RIVAL
	delay 5
	applymovement LOCALID_NEW_BARK_RIVAL, PolishedNewBarkTown_Rival_FaceDown
	waitmovement LOCALID_NEW_BARK_RIVAL
	delay 5
@ Expansion's native Tackle impact (gBattleAnimMove_Tackle).
	playse SE_M_COMET_PUNCH
	applymovement OBJ_EVENT_ID_PLAYER, PolishedNewBarkTown_Rival_Shove
	waitmovement OBJ_EVENT_ID_PLAYER
	applymovement LOCALID_NEW_BARK_RIVAL, PolishedNewBarkTown_Rival_Return
	waitmovement LOCALID_NEW_BARK_RIVAL
	releaseall
	end

PolishedNewBarkTown_Rival_FaceLeft:
	face_left
	step_end
PolishedNewBarkTown_Rival_FaceDown:
	face_down
	step_end
PolishedNewBarkTown_Rival_BackStep:
	face_up
	lock_facing_direction
	walk_down
	step_end
PolishedNewBarkTown_Rival_StepLeft:
	walk_left
	step_end
PolishedNewBarkTown_Rival_Shove:
	jump_2_down
	unlock_facing_direction
	step_end
PolishedNewBarkTown_Rival_Return:
	walk_right
	step_end

PolishedNewBarkTown_Rival_Text1:
	.string "……\pSo this is the\nfamous Elm POKéMON\lLab…$"

PolishedNewBarkTown_Rival_Text2:
	.string "…What are you\nstaring at?$"

PolishedIntro_TownSetup::
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

PolishedIntro_TownEnters:
	walk_right
	walk_right
	step_end

PolishedIntro_TownApproaches:
	walk_right
	walk_up
	walk_up
	walk_right
	step_end

PolishedIntro_Text_LyraIntro:
	.string "Lyra: Oh, hello,\n{PLAYER}!\pI came by your\nhouse earlier,\pbut you were\nstill sleeping.\pYou know how I\nassist Prof.Elm\lsometimes?\pHe's starting new\nPOKéMON research\pand would like us\nboth to help.\pLet's go and see\nwhat he wants!$"

PolishedRoute29_SetRespawn::
	setrespawn HEAL_LOCATION_POLISHED_NEW_BARK_TOWN
	end

PolishedRoute29_Stop1::
	lockall
	msgbox PolishedRoute29_Text_WaitPlayer, MSGBOX_DEFAULT
	closemessage
	turnobject OBJ_EVENT_ID_PLAYER, DIR_EAST
	applymovement LOCALID_NEW_BARK_TEACHER, PolishedRoute29_Stop1_Approach
	waitmovement 0
	msgbox PolishedRoute29_Text_WhatDoYouThinkYoureDoing, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_NEW_BARK_TEACHER, PolishedRoute29_Stop1_Return
	applymovement OBJ_EVENT_ID_PLAYER, PolishedRoute29_Stop1_Follow
	waitmovement OBJ_EVENT_ID_PLAYER
	waitmovement LOCALID_NEW_BARK_TEACHER
	msgbox PolishedRoute29_Text_ItsDangerousToGoAlone, MSGBOX_DEFAULT
	releaseall
	end

PolishedRoute29_Stop1_Approach:
	walk_left
	walk_left
	walk_left
	walk_left
	step_end

PolishedRoute29_Stop1_Return:
	walk_right
	walk_right
	walk_right
	walk_right
	face_left
	step_end

PolishedRoute29_Stop1_Follow:
	walk_right
	walk_right
	walk_right
	walk_right
	step_end

PolishedRoute29_Stop2::
	lockall
	msgbox PolishedRoute29_Text_WaitPlayer, MSGBOX_DEFAULT
	closemessage
	turnobject OBJ_EVENT_ID_PLAYER, DIR_EAST
	applymovement LOCALID_NEW_BARK_TEACHER, PolishedRoute29_Stop2_Approach
	waitmovement 0
	msgbox PolishedRoute29_Text_WhatDoYouThinkYoureDoing, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_NEW_BARK_TEACHER, PolishedRoute29_Stop2_Return
	applymovement OBJ_EVENT_ID_PLAYER, PolishedRoute29_Stop2_Follow
	waitmovement OBJ_EVENT_ID_PLAYER
	waitmovement LOCALID_NEW_BARK_TEACHER
	msgbox PolishedRoute29_Text_ItsDangerousToGoAlone, MSGBOX_DEFAULT
	releaseall
	end

PolishedRoute29_Stop2_Approach:
	walk_left
	walk_left
	walk_left
	walk_left
	walk_left
	face_down
	step_end

PolishedRoute29_Stop2_Return:
	walk_right
	walk_right
	walk_right
	walk_right
	walk_right
	face_left
	step_end

PolishedRoute29_Stop2_Follow:
	walk_up
	walk_right
	walk_right
	walk_right
	walk_right
	step_end

PolishedRoute29_Text_WaitPlayer:
	.string "Wait, {PLAYER}!$"

PolishedRoute29_Text_WhatDoYouThinkYoureDoing:
	.string "What do you think\nyou're doing?$"

PolishedRoute29_Text_ItsDangerousToGoAlone:
	.string "It's dangerous to\ngo out without a\lPOKéMON!\pWild POKéMON\njump out of the\pgrass on the way\nto the next town.$"
