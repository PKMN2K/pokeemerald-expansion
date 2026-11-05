@ Generated from pinned VioletGym.asm: geography, static text and ambient residents.
PolishedVioletGym_MapScripts::
	map_script MAP_SCRIPT_ON_LOAD, PolishedVioletGym_Setup
	map_script MAP_SCRIPT_ON_FRAME_TABLE, PolishedVioletGym_Frame
	.byte 0

PolishedVioletGym_Setup::
    special PolishedFalknerPrepareBadges
    setflag FLAG_TEMP_1
    setvar VAR_TEMP_4, 0
    goto_if_unset FLAG_POLISHED_CAVE_DONE, PolishedVioletGym_Nothing
    clearflag FLAG_TEMP_1
    setvar VAR_TEMP_4, 1
PolishedVioletGym_Nothing::
    end
PolishedVioletGym_Frame::
    map_script_2 VAR_TEMP_4, 0, PolishedVioletGym_Away
    .2byte 0
PolishedVioletGym_Away::
    lockall
    applymovement LOCALID_VIOLET_GYM_1, PolishedVioletGym_StepDown
    waitmovement 0
    msgbox PolishedVioletGym_AwayText, MSGBOX_DEFAULT
    closemessage
    warp MAP_POLISHED_VIOLET_CITY, 18, 18
    waitstate
    releaseall
    end
PolishedVioletGym_Falkner::
    lock
    faceplayer
    goto_if_set FLAG_POLISHED_ROOST, PolishedVioletGym_After
    goto_if_defeated TRAINER_POLISHED_FALKNER, PolishedVioletGym_Badge
    msgbox PolishedVioletGym_Intro, MSGBOX_DEFAULT
    trainerbattle_no_intro TRAINER_POLISHED_FALKNER, PolishedVioletGym_Beaten
PolishedVioletGym_Badge::
    goto_if_set FLAG_POLISHED_ZEPHYR_BADGE, PolishedVioletGym_TM
    special PolishedFalknerAwardBadge
    settrainerflag TRAINER_POLISHED_ROD
    settrainerflag TRAINER_POLISHED_ABE
    msgbox PolishedVioletGym_Received, MSGBOX_DEFAULT
    call Common_EventScript_PlayGymBadgeFanfare
PolishedVioletGym_TM::
    msgbox PolishedVioletGym_BadgeText, MSGBOX_DEFAULT
    giveitem ITEM_TM_ROOST
    goto_if_eq VAR_RESULT, FALSE, PolishedVioletGym_Done
    setflag FLAG_POLISHED_ROOST
    msgbox PolishedVioletGym_Explain, MSGBOX_DEFAULT
    goto PolishedVioletGym_Done
PolishedVioletGym_After::
    msgbox PolishedVioletGym_AfterText, MSGBOX_DEFAULT
PolishedVioletGym_Done::
    releaseall
    end
PolishedVioletGym_Guide::
    goto_if_set FLAG_POLISHED_ZEPHYR_BADGE, PolishedVioletGym_GuideWon
    msgbox PolishedVioletGym_GuideText, MSGBOX_NPC
    end
PolishedVioletGym_GuideWon::
    msgbox PolishedVioletGym_GuideWonText, MSGBOX_NPC
    end
PolishedVioletGym_Statue::
    goto_if_set FLAG_POLISHED_ZEPHYR_BADGE, PolishedVioletGym_StatueWon
    msgbox PolishedVioletGym_StatueText, MSGBOX_SIGN
    end
PolishedVioletGym_StatueWon::
    msgbox PolishedVioletGym_StatueWonText, MSGBOX_SIGN
    end
PolishedVioletGym_StepDown:
    walk_down
    step_end
PolishedVioletGym_Rod::
    trainerbattle_single TRAINER_POLISHED_ROD, PolishedVioletGym_RodSeen, PolishedVioletGym_RodBeaten
    msgbox PolishedVioletGym_RodAfter, MSGBOX_AUTOCLOSE
    end
PolishedVioletGym_RodSeen:
	.string "The keyword is\nguts!\pThose here are\ntraining night and\pday to become bird\nPOKéMON masters.\pCome on!$"

PolishedVioletGym_RodBeaten:
	.string "Gaaah!$"

PolishedVioletGym_RodAfter:
	.string "Falkner's skills\nare for real!\pDon't get cocky\njust because you\lbeat me!$"

PolishedVioletGym_Abe::
    trainerbattle_single TRAINER_POLISHED_ABE, PolishedVioletGym_AbeSeen, PolishedVioletGym_AbeBeaten
    msgbox PolishedVioletGym_AbeAfter, MSGBOX_AUTOCLOSE
    end
PolishedVioletGym_AbeSeen:
	.string "Let me see if you\nare good enough to\lface Falkner!$"

PolishedVioletGym_AbeBeaten:
	.string "This can't be\ntrue!$"

PolishedVioletGym_AbeAfter:
	.string "This is pathetic,\nlosing to some\lrookie trainer…$"

PolishedVioletGym_AwayText:
	.string "Hey! You can't\nchallenge Falkner\lright now.\pHe's in Dark Cave\ntaming an Ursa-\pring that attacked\na young trainer.\pCome back later.$"

PolishedVioletGym_Intro:
	.string "I'm Falkner, the\nViolet POKéMON Gym\lLeader!\pPeople say you can\nclip Flying-type\pPOKéMON's wings\nwith a jolt of\lelectricity…\pI won't allow such\ninsults to bird\lPOKéMON!\pI'll show you the\nreal power of the\pmagnificent bird\nPOKéMON!$"

PolishedVioletGym_Beaten:
	.string "…Darn! My dad's\ncherished bird\lPOKéMON…\pAll right.\nTake this.\pIt's the official\nPOKéMON League\lZephyr Badge.$"

PolishedVioletGym_BadgeText:
	.string "The Zephyr Badge\nenables you to\pcommand POKéMON,\neven traded ones,\lup to Lv20.\pHere--take this\ntoo.$"

PolishedVioletGym_Explain:
	.string "By using a TM, a\nPOKéMON will\pinstantly learn a\nnew move.\pA TM can be used\nas many times as\lyou want.\pTM51 contains\nRoost.\pIt lands on the\nground to restore\lhalf the max HP.$"

PolishedVioletGym_AfterText:
	.string "There are POKéMON\nGyms in cities and\ltowns ahead.\pYou should test\nyour skills at\lthese Gyms.\pI'm going to train\nharder to become\pthe greatest bird\nmaster!$"

PolishedVioletGym_GuideText:
	.string "Hey! I'm no train-\ner but I can give\lsome advice!\pBelieve me!\nIf you believe, a\pchampionship dream\ncan come true.\pYou believe?\nThen listen.\pThe Grass-type is\nweak against the\pFlying-type. Keep\nthis in mind.$"

PolishedVioletGym_GuideWonText:
	.string "Nice battle! Keep\nit up, and you'll\pbe the Champ in no\ntime at all!$"

PolishedVioletGym_Received:
	.string "{PLAYER} received the\nZephyr Badge!$"

PolishedVioletGym_StatueText:
	.string "Violet POKéMON GYM\nLeader: Falkner$"

PolishedVioletGym_StatueWonText:
	.string "Violet POKéMON GYM\nLeader: Falkner\pWinning Trainers:\n{PLAYER}$"
