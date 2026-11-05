@ Generated from pinned VioletCity.asm: geography, static text and ambient residents.
PolishedVioletCity_MapScripts::
	map_script MAP_SCRIPT_ON_LOAD, PolishedVioletCity_Setup
	map_script MAP_SCRIPT_ON_RETURN_TO_FIELD, PolishedVioletCity_Restore
	.byte 0

PolishedVioletCity_VioletCitySignText::
	msgbox PolishedVioletCity_VioletCitySignText_Text, MSGBOX_SIGN
	end

PolishedVioletCity_VioletCitySignText_Text:
	.string "Violet City\pThe City of\nNostalgic Scents$"

PolishedVioletCity_VioletGymSignText::
	msgbox PolishedVioletCity_VioletGymSignText_Text, MSGBOX_SIGN
	end

PolishedVioletCity_VioletGymSignText_Text:
	.string "Violet City\nPOKéMON Gym\lLeader: Falkner\pThe Elegant Master\nof Flying POKéMON$"

PolishedVioletCity_SproutTowerSignText::
	msgbox PolishedVioletCity_SproutTowerSignText_Text, MSGBOX_SIGN
	end

PolishedVioletCity_SproutTowerSignText_Text:
	.string "Sprout Tower\pExperience the\nWay of POKéMON$"

PolishedVioletCity_EarlsPokemonAcademySignText::
	msgbox PolishedVioletCity_EarlsPokemonAcademySignText_Text, MSGBOX_SIGN
	end

PolishedVioletCity_EarlsPokemonAcademySignText_Text:
	.string "Earl's POKéMON\nAcademy$"

PolishedVioletCity_VioletCityLassText::
	msgbox PolishedVioletCity_VioletCityLassText_Text, MSGBOX_NPC
	end

PolishedVioletCity_VioletCityLassText_Text:
	.string "Ghosts are rumored\nto appear in\lSprout Tower.\pThey said Normal-\ntype POKéMON moves\phad no effect on\nghosts.$"

PolishedVioletCity_VioletCityCooltrainerM1Text::
	msgbox PolishedVioletCity_VioletCityCooltrainerM1Text_Text, MSGBOX_NPC
	end

PolishedVioletCity_VioletCityCooltrainerM1Text_Text:
	.string "Hey, you're a\nPOKéMON trainer?\pIf you beat the\nGym Leader here,\pyou'll be ready\nfor prime time!$"

PolishedVioletCity_VioletCityCooltrainerFText::
	msgbox PolishedVioletCity_VioletCityCooltrainerFText_Text, MSGBOX_NPC
	end

PolishedVioletCity_VioletCityCooltrainerFText_Text:
	.string "Bellsprout is a\npopular POKéMON\lin this town.\pIt doesn't do well\nagainst our Gym\lLeader, though…$"

PolishedVioletCity_VioletCityGrampsText::
	msgbox PolishedVioletCity_VioletCityGrampsText_Text, MSGBOX_NPC
	end

PolishedVioletCity_VioletCityGrampsText_Text:
	.string "Falkner, from the\nViolet POKéMON\pGym, is a fine\ntrainer!\pHe inherited his\nfather's gym and\phas done a great\njob with it.$"

PolishedVioletCity_VioletCityYoungsterText::
	msgbox PolishedVioletCity_VioletCityYoungsterText_Text, MSGBOX_NPC
	end

PolishedVioletCity_VioletCityYoungsterText_Text:
	.string "I saw a wiggly\ntree up ahead!\pIf you touch it,\nit squirms and\ldances! Cool!$"

PolishedVioletCity_VioletCityFisherText::
	msgbox PolishedVioletCity_VioletCityFisherText_Text, MSGBOX_NPC
	end

PolishedVioletCity_VioletCityFisherText_Text:
	.string "How does such a\nwobbly building\psurvive an earth-\nquake? I must be\lmissing something.$"

PolishedVioletCity_VioletCityCooltrainerM2Text::
	msgbox PolishedVioletCity_VioletCityCooltrainerM2Text_Text, MSGBOX_NPC
	end

PolishedVioletCity_VioletCityCooltrainerM2Text_Text:
	.string "We care about the\ntraditional build-\lings around here.\pEven the POKé Mart\nand POKéMON Center\pare painted som-\nberly to blend in.$"

PolishedVioletCity_PP_UP::
	finditem ITEM_PP_UP
	end

PolishedVioletCity_RARE_CANDY::
	finditem ITEM_RARE_CANDY
	end

PolishedEarl_Talk::
    lockall
    applymovement LOCALID_VIOLET_EARL, PolishedEarl_Spin
    waitmovement LOCALID_VIOLET_EARL
    faceplayer
    msgbox PolishedEarl_Text_EarlAsksIfYouBeatFalkner, MSGBOX_YESNO
    goto_if_eq VAR_RESULT, YES, PolishedEarl_AlreadyWon
    msgbox PolishedEarl_Text_FollowEarl, MSGBOX_DEFAULT
    closemessage
    getobjectfacingdirection OBJ_EVENT_ID_PLAYER, VAR_0x8004
    applymovement LOCALID_VIOLET_EARL, PolishedEarl_FirstStep
    switch VAR_0x8004
    case DIR_NORTH, PolishedEarl_From_up
    case DIR_SOUTH, PolishedEarl_From_down
    case DIR_WEST, PolishedEarl_From_left
    case DIR_EAST, PolishedEarl_From_right
PolishedEarl_From_up::
    applymovement OBJ_EVENT_ID_PLAYER, PolishedEarl_First_up
    goto PolishedEarl_Following
PolishedEarl_From_down::
    applymovement OBJ_EVENT_ID_PLAYER, PolishedEarl_First_down
    goto PolishedEarl_Following
PolishedEarl_From_left::
    applymovement OBJ_EVENT_ID_PLAYER, PolishedEarl_First_left
    goto PolishedEarl_Following
PolishedEarl_From_right::
    applymovement OBJ_EVENT_ID_PLAYER, PolishedEarl_First_right
    goto PolishedEarl_Following
PolishedEarl_Following::
    waitmovement OBJ_EVENT_ID_PLAYER
    waitmovement LOCALID_VIOLET_EARL
    applymovement LOCALID_VIOLET_EARL, PolishedEarl_Path0
    applymovement OBJ_EVENT_ID_PLAYER, PolishedEarl_Path0_Follow
    waitmovement OBJ_EVENT_ID_PLAYER
    waitmovement LOCALID_VIOLET_EARL
    applymovement LOCALID_VIOLET_EARL, PolishedEarl_Path1
    waitmovement LOCALID_VIOLET_EARL
    applymovement LOCALID_VIOLET_EARL, PolishedEarl_Path2
    applymovement OBJ_EVENT_ID_PLAYER, PolishedEarl_Path2_Follow
    waitmovement OBJ_EVENT_ID_PLAYER
    waitmovement LOCALID_VIOLET_EARL
    applymovement LOCALID_VIOLET_EARL, PolishedEarl_Path3
    waitmovement LOCALID_VIOLET_EARL
    applymovement LOCALID_VIOLET_EARL, PolishedEarl_Path4
    applymovement OBJ_EVENT_ID_PLAYER, PolishedEarl_Path4_Follow
    waitmovement OBJ_EVENT_ID_PLAYER
    waitmovement LOCALID_VIOLET_EARL
    applymovement LOCALID_VIOLET_EARL, PolishedEarl_Path5
    waitmovement LOCALID_VIOLET_EARL
    applymovement LOCALID_VIOLET_EARL, PolishedEarl_Path6
    applymovement OBJ_EVENT_ID_PLAYER, PolishedEarl_Path6_Follow
    waitmovement OBJ_EVENT_ID_PLAYER
    waitmovement LOCALID_VIOLET_EARL
    applymovement LOCALID_VIOLET_EARL, PolishedEarl_Path7
    waitmovement LOCALID_VIOLET_EARL
    applymovement LOCALID_VIOLET_EARL, PolishedEarl_Path8
    applymovement OBJ_EVENT_ID_PLAYER, PolishedEarl_Path8_Follow
    waitmovement OBJ_EVENT_ID_PLAYER
    waitmovement LOCALID_VIOLET_EARL
    applymovement LOCALID_VIOLET_EARL, PolishedEarl_Path9
    waitmovement LOCALID_VIOLET_EARL
    turnobject OBJ_EVENT_ID_PLAYER, DIR_NORTH
    applymovement LOCALID_VIOLET_EARL, PolishedEarl_Spin
    waitmovement LOCALID_VIOLET_EARL
    msgbox PolishedEarl_Text_HereTeacherIAm, MSGBOX_DEFAULT
    call PolishedEarl_GiveChart
    goto_if_eq VAR_RESULT, FALSE, PolishedEarl_EnterAcademy
    msgbox PolishedEarl_Text_EarlsTypeChart, MSGBOX_DEFAULT
PolishedEarl_EnterAcademy::
    closemessage
    applymovement LOCALID_VIOLET_EARL, PolishedEarl_Spin
    waitmovement LOCALID_VIOLET_EARL
    applymovement LOCALID_VIOLET_EARL, PolishedEarl_Door
    waitmovement LOCALID_VIOLET_EARL
    playse SE_DOOR
    removeobject LOCALID_VIOLET_EARL
    setflag FLAG_POLISHED_EARL_IN_ACADEMY
    waitse
    releaseall
    end

PolishedEarl_AlreadyWon::
    msgbox PolishedEarl_Text_VeryNiceIndeed, MSGBOX_DEFAULT
    releaseall
    end

PolishedEarl_GiveChart::
    checkitem ITEM_POLISHED_TYPE_CHART
    goto_if_eq VAR_RESULT, TRUE, PolishedEarl_ChartDone
    giveitem ITEM_POLISHED_TYPE_CHART
    goto_if_eq VAR_RESULT, TRUE, PolishedEarl_ChartDone
    msgbox PolishedEarl_FullText, MSGBOX_DEFAULT
    setvar VAR_RESULT, FALSE
PolishedEarl_ChartDone::
    return

PolishedEarl_FullText:
    .string "Make room in your Key Items pocket,\nthen speak to me at the Academy.$"

PolishedEarl_Door:
    walk_up
    step_end

PolishedEarl_Spin:
	face_down
	face_left
	face_up
	face_right
	face_down
	face_left
	face_up
	face_right
	face_down
	face_left
	face_up
	face_right
	face_down
	step_end

PolishedEarl_First_up:
    walk_fast_up
    step_end

PolishedEarl_First_down:
    walk_fast_down
    step_end

PolishedEarl_First_left:
    walk_fast_left
    step_end

PolishedEarl_First_right:
    walk_fast_right
    step_end

PolishedEarl_FirstStep:
    walk_fast_down
    step_end

PolishedEarl_Path0:
    walk_fast_down
    step_end

PolishedEarl_Path0_Follow:
    walk_fast_down
    step_end

PolishedEarl_Path1:
    face_down
    face_left
    face_up
    face_right
    face_down
    face_left
    face_up
    face_right
    step_end

PolishedEarl_Path2:
    walk_fast_right
    walk_fast_right
    walk_fast_right
    walk_fast_right
    walk_fast_right
    walk_fast_right
    walk_fast_right
    walk_fast_right
    walk_fast_right
    step_end

PolishedEarl_Path2_Follow:
    walk_fast_down
    walk_fast_right
    walk_fast_right
    walk_fast_right
    walk_fast_right
    walk_fast_right
    walk_fast_right
    walk_fast_right
    walk_fast_right
    step_end

PolishedEarl_Path3:
    face_right
    face_down
    face_left
    face_up
    face_right
    face_down
    face_left
    face_up
    face_right
    face_down
    step_end

PolishedEarl_Path4:
    walk_fast_down
    step_end

PolishedEarl_Path4_Follow:
    walk_fast_right
    step_end

PolishedEarl_Path5:
    face_down
    face_left
    face_up
    face_right
    face_down
    face_left
    face_up
    face_right
    step_end

PolishedEarl_Path6:
    walk_fast_right
    walk_fast_right
    walk_fast_right
    walk_fast_right
    walk_fast_right
    walk_fast_right
    walk_fast_right
    walk_fast_right
    step_end

PolishedEarl_Path6_Follow:
    walk_fast_down
    walk_fast_right
    walk_fast_right
    walk_fast_right
    walk_fast_right
    walk_fast_right
    walk_fast_right
    walk_fast_right
    step_end

PolishedEarl_Path7:
    face_right
    face_down
    face_left
    face_up
    face_right
    face_down
    face_left
    face_up
    step_end

PolishedEarl_Path8:
    walk_fast_up
    step_end

PolishedEarl_Path8_Follow:
    walk_fast_right
    step_end

PolishedEarl_Path9:
    face_down
    step_end
PolishedEarl_Text_EarlAsksIfYouBeatFalkner:
	.string "Hello!\nYou are trainer?\pBattle Gym Leader,\nwin you did?$"

PolishedEarl_Text_FollowEarl:
	.string "Is that so? Then\nstudy shall you!\lFollow me!$"

PolishedEarl_Text_HereTeacherIAm:
	.string "Here, teacher I\nam. Good it is\lyou study here!\pTake this and\nlearn you will!$"

PolishedEarl_Text_EarlsTypeChart:
	.string "In battle, type\nadvantages you\lmust know!\pAt any time, con-\nsult this chart\lyou may!$"

PolishedEarl_Text_VeryNiceIndeed:
	.string "Ooh, la la!\nVery indeed nice!$"

PolishedVioletCity_Cheri::
    setvar VAR_0x8005, 4
    goto PolishedFruit_Interact

PolishedVioletCity_Setup::
    setmetatile 30, 17, 158, FALSE
    setmetatile 9, 17, 503, FALSE
    setflag FLAG_POLISHED_VISITED_VIOLET
    special PolishedFruitRefreshAll
    gettime
    goto_if_lt VAR_0x8000, 5, PolishedVioletCity_Night
    goto_if_ge VAR_0x8000, 17, PolishedVioletCity_Night
    clearflag FLAG_TEMP_1
    setflag FLAG_TEMP_2
    end

PolishedVioletCity_Night::
    setflag FLAG_TEMP_1
    clearflag FLAG_TEMP_2
    end

PolishedVioletCity_Restore::
    goto_if_set FLAG_POLISHED_EARL_IN_ACADEMY, PolishedVioletCity_RestoreResidents
    addobject LOCALID_VIOLET_EARL
PolishedVioletCity_RestoreResidents::
    goto_if_set FLAG_TEMP_1, PolishedVioletCity_ShowNight
    removeobject LOCALID_VIOLET_CITY_ACE_TRAINER_F
    addobject LOCALID_VIOLET_CITY_ACE_TRAINER_M
    end

PolishedVioletCity_ShowNight::
    removeobject LOCALID_VIOLET_CITY_ACE_TRAINER_M
    addobject LOCALID_VIOLET_CITY_ACE_TRAINER_F
    end
PolishedVioletCity_MartSign::
	msgbox PolishedVioletCity_MartSign_Text, MSGBOX_SIGN
	end

PolishedVioletCity_MartSign_Text:
	.string "For All Your\nPOKéMON Needs\pPOKéMON Mart$"

PolishedVioletCity_PokeCenterSign::
	msgbox PolishedVioletCity_PokeCenterSign_Text, MSGBOX_SIGN
	end

PolishedVioletCity_PokeCenterSign_Text:
	.string "Heal Your POKéMON!\nPOKéMON Center$"
