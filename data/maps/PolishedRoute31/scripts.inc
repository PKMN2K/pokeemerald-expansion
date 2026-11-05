@ Generated from pinned Route31.asm: geography, static text and ambient residents.
PolishedRoute31_MapScripts::
	map_script MAP_SCRIPT_ON_LOAD, PolishedRoute31_Setup
	map_script MAP_SCRIPT_ON_RETURN_TO_FIELD, PolishedRoute31_Setup
	map_script MAP_SCRIPT_ON_FRAME_TABLE, PolishedRoute31_CallTable
	.byte 0

PolishedRoute31_Route31SignText::
	msgbox PolishedRoute31_Route31SignText_Text, MSGBOX_SIGN
	end

PolishedRoute31_Route31SignText_Text:
	.string "Route 31\pViolet City -\nCherrygrove City$"

PolishedRoute31_DarkCaveSignText::
	msgbox PolishedRoute31_DarkCaveSignText_Text, MSGBOX_SIGN
	end

PolishedRoute31_DarkCaveSignText_Text:
	.string "Dark Cave$"

PolishedRoute31_Route31AdvancedTipsSignText::
	msgbox PolishedRoute31_Route31AdvancedTipsSignText_Text, MSGBOX_SIGN
	end

PolishedRoute31_Route31AdvancedTipsSignText_Text:
	.string "Advanced Tips!\pPress Start in\nthe Bag to sort\lan item pocket!$"

PolishedRoute31_Route31YoungsterText::
	msgbox PolishedRoute31_Route31YoungsterText_Text, MSGBOX_NPC
	end

PolishedRoute31_Route31YoungsterText_Text:
	.string "I found a good\nPOKéMON in Dark\lCave.\pI'm going to raise\nit to take on\lFalkner.\pHe's the leader of\nViolet City's Gym.$"

PolishedRoute31_Setup::
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
PolishedRoute31_FinchIntroText:
	.string "I am a trainer who\nuses Air Balloons\lin battle.\pI want to be\nswayed by your\lstrength…\pIf you can defeat\nevery trainer on\lRoute 30 and 31,\lthen come face me.$"

PolishedRoute31_FinchQuestionText:
	.string "So you've defeated\nall of the train-\lers here!\pThen you're fit to\nchallenge me!\pShall we battle?$"

PolishedRoute31_FinchRefusedText:
	.string "You don't want to\ntest your strength\lwith mine?\pI'll give you my\nsignature item if\lyou win…$"

PolishedRoute31_FinchSeenText:
	.string "Well met! I will\nstand before you\pas an Air Balloon\nuser!$"

PolishedRoute31_FinchBeatenText:
	.string "You've brought me\nto the ground…$"

PolishedRoute31_FinchAfterText1:
	.string "A fine battle!\nYou are a rising\pstar among train-\ners!\pHere is proof of\nour battle.\lAn Air Balloon!\pTake it, my\nstrong friend!$"

PolishedRoute31_FinchAfterText2:
	.string "You saw the effect\nof an Air Balloon\lin our battle.\pYou may find other\ntrainers like me\lwandering Johto.\pSearching for\nstrength.\pYou would do well\nto challenge them!$"

PolishedRoute31_WadeSeen:
	.string "I caught a bunch\nof POKéMON. Let me\lbattle with you!$"

PolishedRoute31_WadeBeaten:
	.string "Awwwww…$"

PolishedRoute31_WadeAfter:
	.string "You can catch\nPOKéMON even if\pyou have six with\nyou.\pIf you catch one,\nit'll go to your\lBox automatically.$"

PolishedRoute31_SleepyText:
	.string "…Hnuurg… Huh?\pI walked too far\ntoday looking for\lPOKéMON.\pMy feet hurt and\nI'm sleepy…\pIf I were a wild\nPOKéMON, I'd be\leasy to catch…\p…Zzzz…$"

PolishedRoute31_MomLecture:
	.string "Hello?\p……{PLAYER}?\nI heard from Prof.\pElm that you went\non a long trip.\pI wish you would\nhave told me…\pWhat about money?\nShould I save it?$"

PolishedRoute31_MomYes:
	.string "OK. I'll save your\nmoney.$"

PolishedRoute31_MomNo:
	.string "OK. I won't save\nyour money.$"

PolishedRoute31_MomBye:
	.string "{PLAYER}, keep it\nup! I'm rooting\lfor you, baby!$"
