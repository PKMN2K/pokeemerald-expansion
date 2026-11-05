@ Generated from pinned DarkCaveVioletEntrance.asm: geography, static text and ambient residents.
PolishedDarkCaveVioletEntrance_MapScripts::
	map_script MAP_SCRIPT_ON_LOAD, PolishedDarkCave_Setup
	.byte 0

PolishedDarkCave_Setup::
    setvar VAR_TEMP_4, 0
    goto_if_unset FLAG_POLISHED_CAVE_DONE, PolishedDarkCave_Nothing
    setvar VAR_TEMP_4, 1
PolishedDarkCave_Nothing::
    end
PolishedDarkCave_Scene::
    lockall
    specialvar VAR_RESULT, PolishedFalknerFlashLevel
    goto_if_eq VAR_RESULT, 0, PolishedDarkCave_Lit
    goto_if_set FLAG_POLISHED_SPROUT_FLASH, PolishedDarkCave_Dark
    msgbox PolishedDarkCave_Darkness, MSGBOX_DEFAULT
    closemessage
    applymovement OBJ_EVENT_ID_PLAYER, PolishedDarkCave_Left
    waitmovement 0
    releaseall
    end
PolishedDarkCave_Dark::
    msgbox PolishedDarkCave_ProgressAnyway, MSGBOX_DEFAULT
    call PolishedDarkCave_Rescue
    msgbox PolishedDarkCave_DarkIntro, MSGBOX_DEFAULT
    goto PolishedDarkCave_Finish
PolishedDarkCave_Lit::
    call PolishedDarkCave_Rescue
    msgbox PolishedDarkCave_Intro, MSGBOX_DEFAULT
PolishedDarkCave_Finish::
    msgbox PolishedDarkCave_Explanation, MSGBOX_DEFAULT
    closemessage
    applymovement OBJ_EVENT_ID_PLAYER, PolishedDarkCave_Aside
    applymovement LOCALID_DARK_CAVE_3, PolishedDarkCave_Aside
    waitmovement 0
    turnobject OBJ_EVENT_ID_PLAYER, DIR_SOUTH
    applymovement LOCALID_DARK_CAVE_3, PolishedDarkCave_Leave
    waitmovement 0
    removeobject LOCALID_DARK_CAVE_3
    setflag FLAG_POLISHED_CAVE_DONE
    setvar VAR_TEMP_4, 1
    releaseall
    end
PolishedDarkCave_Rescue::
    msgbox PolishedDarkCave_Attack, MSGBOX_DEFAULT
    closemessage
    playmoncry SPECIES_PIDGEOTTO, CRY_MODE_NORMAL
    waitmoncry
    applymovement LOCALID_DARK_CAVE_2, PolishedDarkCave_Strike
    waitmovement 0
    playse SE_M_COMET_PUNCH
    playmoncry SPECIES_URSARING, CRY_MODE_NORMAL
    waitmoncry
    removeobject LOCALID_DARK_CAVE_1
    msgbox PolishedDarkCave_Return, MSGBOX_DEFAULT
    closemessage
    removeobject LOCALID_DARK_CAVE_2
    applymovement LOCALID_DARK_CAVE_3, PolishedDarkCave_Left
    waitmovement 0
    return
PolishedDarkCave_Left:
    walk_left
    step_end
PolishedDarkCave_Aside:
    walk_left
    walk_left
    walk_left
    step_end
PolishedDarkCave_Leave:
    walk_down
    walk_left
    walk_down
    walk_down
    walk_down
    walk_down
    step_end
PolishedDarkCave_Strike:
    walk_fast_right
    walk_fast_left
    step_end
PolishedDarkCave_Darkness:
	.string "Falkner: Is\nsomeone there?\pDon't come any\ncloser!\pIt's too dark, you\ncould get hurt.$"

PolishedDarkCave_ProgressAnyway:
	.string "Falkner: Is\nsomeone there?\pIt's reckless to\nexplore Dark Cave\lwithout any light!\pI'm in the middle\nof a battle, so\lstay back!$"

PolishedDarkCave_DarkIntro:
	.string "You must be brave\nto come here in\lpitch darkness.$"

PolishedDarkCave_Intro:
	.string "Oh! You must be\na trainer.$"

PolishedDarkCave_Explanation:
	.string "I'm Falkner, the\nViolet POKéMON\lGym Leader.\pI don't explore\ncaves for fun,\pbut that Ursaring\nwas attacking\ptrainers too weak\nto defend them-\lselves.\pAs a Gym Leader,\nit's my job to\phandle this sort\nof situation.\pSo you're here to\nearn a Badge?\pThen meet me at\nthe Violet Gym.$"

PolishedDarkCave_Attack:
	.string "Falkner: Use Wing\nAttack, Pidgeotto!$"

PolishedDarkCave_Return:
	.string "Good job,\nPidgeotto.$"

PolishedDarkCave_POTION::
    finditem ITEM_POTION
    end
PolishedDarkCave_DUSK_STONE::
    finditem ITEM_DUSK_STONE
    end
PolishedDarkCave_HYPER_POTION::
    finditem ITEM_HYPER_POTION
    end
PolishedDarkCave_DIRE_HIT::
    finditem ITEM_DIRE_HIT
    end
