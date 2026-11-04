@ Generated from pinned PlayersHouse1F.asm: geography, static text and ambient residents.
PolishedPlayersHouse1F_MapScripts::
	map_script MAP_SCRIPT_ON_LOAD, PolishedMom_Visibility
	map_script MAP_SCRIPT_ON_RETURN_TO_FIELD, PolishedMom_Restore
	.byte 0

PolishedPlayersHouse1F_PlayersHouse1FFridgeText::
	msgbox PolishedPlayersHouse1F_PlayersHouse1FFridgeText_Text, MSGBOX_SIGN
	end

PolishedPlayersHouse1F_PlayersHouse1FFridgeText_Text:
	.string "Let's see what's\nin the fridge…\pFresh Water and\ntasty Lemonade!$"

PolishedPlayersHouse1F_PlayersHouse1FSinkText::
	msgbox PolishedPlayersHouse1F_PlayersHouse1FSinkText_Text, MSGBOX_SIGN
	end

PolishedPlayersHouse1F_PlayersHouse1FSinkText_Text:
	.string "The sink is spot-\nless. Mom likes it\lclean.$"

PolishedPlayersHouse1F_PlayersHouse1FStoveText::
	msgbox PolishedPlayersHouse1F_PlayersHouse1FStoveText_Text, MSGBOX_SIGN
	end

PolishedPlayersHouse1F_PlayersHouse1FStoveText_Text:
	.string "Mom's specialty!\pCinnabar Volcano\nBurger!$"

PolishedPlayersHouse1F_PlayersHouse1FTVScript::
	msgbox PolishedPlayersHouse1F_PlayersHouse1FTVScript_Text, MSGBOX_SIGN
	end

PolishedPlayersHouse1F_PlayersHouse1FTVScript_Text:
	.string "There's a movie on\nTV: Stars dot the\psky as two boys\nride on a train…\pI'd better get\nrolling too!$"

@ Source schedule is sampled at map load, never during conversation/menu return.
PolishedMom_Visibility::
	setflag FLAG_TEMP_2
	setflag FLAG_TEMP_3
	setflag FLAG_TEMP_4
	setflag FLAG_TEMP_5
	setflag FLAG_TEMP_6
	goto_if_eq VAR_POLISHED_HOME_SCENE, 0, PolishedMom_ShowInitial
	gettime
	goto_if_lt VAR_0x8000, 5, PolishedMom_ShowNite
	goto_if_lt VAR_0x8000, 9, PolishedMom_ShowMorn
	goto_if_lt VAR_0x8000, 17, PolishedMom_ShowDay
	goto_if_lt VAR_0x8000, 21, PolishedMom_ShowEve
	goto PolishedMom_ShowNite
PolishedMom_ShowInitial::
	clearflag FLAG_TEMP_2
	end
PolishedMom_ShowMorn::
	clearflag FLAG_TEMP_3
	end
PolishedMom_ShowDay::
	clearflag FLAG_TEMP_4
	end
PolishedMom_ShowEve::
	clearflag FLAG_TEMP_5
	end
PolishedMom_ShowNite::
	clearflag FLAG_TEMP_6
	end

PolishedMom_Restore::
	call PolishedMom_RestoreINITIAL
	call PolishedMom_RestoreMORN
	call PolishedMom_RestoreDAY
	call PolishedMom_RestoreEVE
	call PolishedMom_RestoreNITE
	end

PolishedMom_RestoreINITIAL::
	goto_if_set FLAG_TEMP_2, PolishedMom_HideINITIAL
	addobject LOCALID_HOME_INITIAL
	return
PolishedMom_HideINITIAL::
	removeobject LOCALID_HOME_INITIAL
	return

PolishedMom_RestoreMORN::
	goto_if_set FLAG_TEMP_3, PolishedMom_HideMORN
	addobject LOCALID_HOME_MORN
	return
PolishedMom_HideMORN::
	removeobject LOCALID_HOME_MORN
	return

PolishedMom_RestoreDAY::
	goto_if_set FLAG_TEMP_4, PolishedMom_HideDAY
	addobject LOCALID_HOME_DAY
	return
PolishedMom_HideDAY::
	removeobject LOCALID_HOME_DAY
	return

PolishedMom_RestoreEVE::
	goto_if_set FLAG_TEMP_5, PolishedMom_HideEVE
	addobject LOCALID_HOME_EVE
	return
PolishedMom_HideEVE::
	removeobject LOCALID_HOME_EVE
	return

PolishedMom_RestoreNITE::
	goto_if_set FLAG_TEMP_6, PolishedMom_HideNITE
	addobject LOCALID_HOME_NITE
	return
PolishedMom_HideNITE::
	removeobject LOCALID_HOME_NITE
	return

PolishedMom_Trigger1::
	lockall
	applymovement LOCALID_HOME_INITIAL, PolishedIntro_Exclamation
	waitmovement 0
	turnobject LOCALID_HOME_INITIAL, DIR_EAST
	turnobject LOCALID_PLAYER, DIR_WEST
	goto PolishedMom_Intro

PolishedMom_Trigger2::
	lockall
	applymovement LOCALID_HOME_INITIAL, PolishedIntro_Exclamation
	waitmovement 0
	turnobject LOCALID_HOME_INITIAL, DIR_EAST
	applymovement LOCALID_PLAYER, PolishedMom_Approach2
	waitmovement 0
	goto PolishedMom_Intro

PolishedMom_Trigger3::
	lockall
	applymovement LOCALID_HOME_INITIAL, PolishedIntro_Exclamation
	waitmovement 0
	turnobject LOCALID_HOME_INITIAL, DIR_NORTH
	applymovement LOCALID_PLAYER, PolishedMom_Approach3
	waitmovement 0
	goto PolishedMom_Intro

PolishedMom_Trigger4::
	lockall
	applymovement LOCALID_HOME_INITIAL, PolishedIntro_Exclamation
	waitmovement 0
	turnobject LOCALID_HOME_INITIAL, DIR_NORTH
	applymovement LOCALID_PLAYER, PolishedMom_Approach4
	waitmovement 0
	goto PolishedMom_Intro

PolishedMom_Approach2:
	walk_slow_left
	step_end
PolishedMom_Approach3:
	walk_down
	walk_down
	step_end
PolishedMom_Approach4:
	walk_slow_down
	step_end

PolishedMom_Talk::
	lock
	faceplayer
	goto_if_eq VAR_POLISHED_HOME_SCENE, 0, PolishedMom_Intro
	goto_if_set FLAG_POLISHED_MOM_FIRST_VISIT, PolishedMom_FirstVisitAgain
	goto_if_set FLAG_POLISHED_MOM_BANK_OPEN, PolishedMom_Bank
	goto_if_set FLAG_POLISHED_GAVE_MYSTERY_EGG, PolishedMom_FirstBank
	goto_if_ne VAR_POLISHED_STARTER, 0, PolishedMom_Errand
	msgbox PolishedMom_ElmWaitingText, MSGBOX_DEFAULT
	release
	end
PolishedMom_Errand::
	msgbox PolishedMom_MomErrandText, MSGBOX_DEFAULT
	release
	end

PolishedMom_Intro::
	msgbox PolishedMom_MomIntroText, MSGBOX_DEFAULT
	playfanfare MUS_OBTAIN_ITEM
	msgbox PolishedMom_ReceivedGearText, MSGBOX_DEFAULT
	waitfanfare
	setflag FLAG_POLISHED_POKEGEAR
	setflag FLAG_POLISHED_PHONE_CARD
	setflag FLAG_POLISHED_HAS_MOM_CONTACT
	setflag FLAG_SYS_B_DASH
	setvar VAR_POLISHED_HOME_SCENE, 1
	msgbox PolishedMom_MomPokegearText, MSGBOX_DEFAULT
PolishedMom_ChooseWeekday::
	msgbox PolishedMom_WhatDayText, MSGBOX_DEFAULT
	multichoice 18, 0, MULTI_POLISHED_WEEKDAYS, TRUE
	copyvar VAR_0x8004, VAR_RESULT
	special PolishedJohtoBufferChosenWeekday
	msgbox PolishedMom_ConfirmWeekdayText, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, PolishedMom_ChooseWeekday
	special PolishedJohtoCommitWeekday
PolishedMom_ChooseDST::
	msgbox PolishedMom_MomDSTText, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, PolishedMom_StandardTime
	setflag FLAG_POLISHED_DAYLIGHT_SAVING
	special PolishedJohtoBufferClock
	msgbox PolishedMom_ConfirmDSTText, MSGBOX_YESNO
	goto PolishedMom_CheckTime
PolishedMom_StandardTime::
	clearflag FLAG_POLISHED_DAYLIGHT_SAVING
	special PolishedJohtoBufferClock
	msgbox PolishedMom_ConfirmTimeText, MSGBOX_YESNO
PolishedMom_CheckTime::
	goto_if_eq VAR_RESULT, NO, PolishedMom_ChooseDST
	msgbox PolishedMom_MomRunningShoesText, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, YES, PolishedMom_ShoesKnown
	msgbox PolishedMom_MomInstructionsText, MSGBOX_DEFAULT
PolishedMom_ShoesKnown::
	msgbox PolishedMom_MomOutroText, MSGBOX_DEFAULT
	closemessage
	turnobject LOCALID_HOME_INITIAL, DIR_WEST
	releaseall
	end

PolishedMom_Neighbor::
	lock
	faceplayer
	gettime
	goto_if_lt VAR_0x8000, 5, PolishedMom_NeighborEvening
	goto_if_lt VAR_0x8000, 9, PolishedMom_NeighborMorning
	goto_if_ge VAR_0x8000, 17, PolishedMom_NeighborEvening
	msgbox PolishedMom_NeighborDayText, MSGBOX_DEFAULT
	goto PolishedMom_NeighborMain
PolishedMom_NeighborMorning::
	msgbox PolishedMom_NeighborMornText, MSGBOX_DEFAULT
	goto PolishedMom_NeighborMain
PolishedMom_NeighborEvening::
	msgbox PolishedMom_NeighborNiteText, MSGBOX_DEFAULT
PolishedMom_NeighborMain::
	msgbox PolishedMom_NeighborText, MSGBOX_DEFAULT
	closemessage
	turnobject LOCALID_HOME_NEIGHBOR, DIR_EAST
	release
	end

PolishedMom_MomIntroText:
	.string "Oh, {PLAYER}!\nYou're awake.\pYour friend Lyra\nwas just here.\pShe said that our\nneighbor, Prof.\pElm, was looking\nfor you.\pOh! I almost for-\ngot! Your POKéMON\pGear is back from\nthe repair shop.\pHere you go!$"

PolishedMom_MomPokegearText:
	.string "POKéMON Gear, or\njust POKégear.\pIt's essential if\nyou want to be a\lgood trainer.\pOh, the day of the\nweek isn't set.\pYou mustn't forget\nthat!$"

PolishedMom_MomDSTText:
	.string "Is it Daylight\nSaving Time now?$"

PolishedMom_MomRunningShoesText:
	.string "Come home to\nadjust your clock\pfor Daylight\nSaving Time.\pBy the way, do\nyou know how to\puse your new\nRunning Shoes?$"

PolishedMom_MomInstructionsText:
	.string "I'll read the\ninstructions.\pJust hold down the\nB Button to run,\lindoors or out.$"

PolishedMom_MomOutroText:
	.string "Gee, aren't they\nconvenient?$"

PolishedMom_MomErrandText:
	.string "So, what was Prof.\nElm's errand?\p…\pThat does sound\nchallenging.\pBut, you should be\nproud that people\lrely on you.$"

PolishedMom_ElmWaitingText:
	.string "Prof.Elm is wait-\ning for you.\pHurry up, baby!$"

PolishedMom_NeighborMornText:
	.string "Good morning,\n{PLAYER}!\pI'm visiting!$"

PolishedMom_NeighborDayText:
	.string "Hello, {PLAYER}!\nI'm visiting!$"

PolishedMom_NeighborNiteText:
	.string "Good evening,\n{PLAYER}!\pI'm visiting!$"

PolishedMom_NeighborText:
	.string "{PLAYER}, have you\nheard?\pMy daughter is\nadamant about\pbecoming Prof.\nElm's assistant.\pShe really loves\nPOKéMON!$"

PolishedMom_ReceivedGearText:
	.string "{PLAYER} received\nthe POKéGEAR!$"

PolishedMom_WhatDayText:
	.string "What day is it?$"

PolishedMom_ConfirmWeekdayText:
	.string "{STR_VAR_1}, is it?$"

PolishedMom_ConfirmDSTText:
	.string "{STR_VAR_1} DST,\nis that OK?$"

PolishedMom_ConfirmTimeText:
	.string "{STR_VAR_1},\nis that OK?$"

PolishedMom_FirstBank::
    setflag FLAG_POLISHED_MOM_FIRST_VISIT
    msgbox PolishedMom_LeavingText1, MSGBOX_YESNO
    clearflag FLAG_POLISHED_MOM_SAVES_MONEY
    goto_if_eq VAR_RESULT, NO, PolishedMom_Opened
    setflag FLAG_POLISHED_MOM_SAVES_MONEY
    msgbox PolishedMom_LeavingText2, MSGBOX_DEFAULT
PolishedMom_Opened::
    setflag FLAG_POLISHED_MOM_BANK_OPEN
    msgbox PolishedMom_LeavingText3, MSGBOX_DEFAULT
    goto PolishedMom_Done
PolishedMom_FirstVisitAgain::
    msgbox PolishedMom_DoItText, MSGBOX_DEFAULT
    goto PolishedMom_Done
PolishedMom_Bank::
    msgbox PolishedMom_IsThisAboutYourMoneyText, MSGBOX_YESNO
    goto_if_eq VAR_RESULT, NO, PolishedMom_DSTCheck
    msgbox PolishedMom_BankWhatDoYouWantToDoText, MSGBOX_DEFAULT
    multichoice 0, 0, MULTI_POLISHED_MOM_BANK, FALSE
    goto_if_eq VAR_RESULT, 0, PolishedMom_Withdraw
    goto_if_eq VAR_RESULT, 1, PolishedMom_Deposit
    goto_if_eq VAR_RESULT, 2, PolishedMom_Change
    goto PolishedMom_Cancel
PolishedMom_Withdraw::
    setvar VAR_0x8004, 0
    msgbox PolishedMom_TakeMoneyText, MSGBOX_DEFAULT
    goto PolishedMom_Transfer
PolishedMom_Deposit::
    setvar VAR_0x8004, 1
    msgbox PolishedMom_StoreMoneyText, MSGBOX_DEFAULT
PolishedMom_Transfer::
    closemessage
    specialvar VAR_RESULT, PolishedMomOpenTransfer
    goto_if_eq VAR_RESULT, FALSE, PolishedMom_Cancel
    waitstate
    goto_if_eq VAR_RESULT, 0, PolishedMom_Cancel
    goto_if_eq VAR_RESULT, 2, PolishedMom_Insufficient
    goto_if_eq VAR_RESULT, 3, PolishedMom_NoRoom
    playse SE_SHOP
    goto_if_eq VAR_0x8004, 0, PolishedMom_Taken
    msgbox PolishedMom_StoredMoneyText, MSGBOX_DEFAULT
    goto PolishedMom_Done
PolishedMom_Taken::
    msgbox PolishedMom_TakenMoneyText, MSGBOX_DEFAULT
    goto PolishedMom_Done
PolishedMom_Insufficient::
    goto_if_eq VAR_0x8004, 0, PolishedMom_InsufficientBank
    msgbox PolishedMom_InsufficientFundsInWalletText, MSGBOX_DEFAULT
    goto PolishedMom_Deposit
PolishedMom_InsufficientBank::
    msgbox PolishedMom_HaventSavedThatMuchText, MSGBOX_DEFAULT
    goto PolishedMom_Withdraw
PolishedMom_NoRoom::
    goto_if_eq VAR_0x8004, 0, PolishedMom_WalletFull
    msgbox PolishedMom_NotEnoughRoomInBankText, MSGBOX_DEFAULT
    goto PolishedMom_Deposit
PolishedMom_WalletFull::
    msgbox PolishedMom_NotEnoughRoomInWalletText, MSGBOX_DEFAULT
    goto PolishedMom_Withdraw
PolishedMom_Change::
    msgbox PolishedMom_SaveMoneyText, MSGBOX_YESNO
    goto_if_eq VAR_RESULT, NO, PolishedMom_Stop
    setflag FLAG_POLISHED_MOM_SAVES_MONEY
    msgbox PolishedMom_StartSavingMoneyText, MSGBOX_DEFAULT
    goto PolishedMom_Done
PolishedMom_Stop::
    clearflag FLAG_POLISHED_MOM_SAVES_MONEY
PolishedMom_Cancel::
    msgbox PolishedMom_JustDoWhatYouCanText, MSGBOX_DEFAULT
PolishedMom_Done::
    release
    end
PolishedMom_DSTCheck::
    gettime
    goto_if_set FLAG_POLISHED_DAYLIGHT_SAVING, PolishedMom_DSTBack
    goto_if_eq VAR_0x8000, 23, PolishedMom_DSTBoundary
    msgbox PolishedMom_AskDSTText, MSGBOX_YESNO
    goto_if_eq VAR_RESULT, NO, PolishedMom_Cancel
    specialvar VAR_RESULT, PolishedMomAdjustClock
    goto_if_eq VAR_RESULT, FALSE, PolishedMom_DSTLost
    msgbox PolishedMom_DSTText, MSGBOX_DEFAULT
    goto PolishedMom_Cancel
PolishedMom_DSTBack::
    goto_if_eq VAR_0x8000, 0, PolishedMom_DSTBoundary
    msgbox PolishedMom_AskNotDSTText, MSGBOX_YESNO
    goto_if_eq VAR_RESULT, NO, PolishedMom_Cancel
    specialvar VAR_RESULT, PolishedMomAdjustClock
    goto_if_eq VAR_RESULT, FALSE, PolishedMom_DSTLost
    msgbox PolishedMom_NotDSTText, MSGBOX_DEFAULT
    goto PolishedMom_Cancel
PolishedMom_DSTBoundary::
    msgbox PolishedMom_AskAdjustDSTText, MSGBOX_YESNO
    goto_if_eq VAR_RESULT, NO, PolishedMom_Cancel
PolishedMom_DSTLost::
    msgbox PolishedMom_LostGearBookletText, MSGBOX_DEFAULT
    goto PolishedMom_Cancel
PolishedMom_DoItText:
	.string "{PLAYER}, do it!\pI'm behind you all\nthe way!$"

PolishedMom_LeavingText1:
	.string "Wow, that's a cute\nPOKéMON.\pWhere did you get\nit?\p…\pSo, you're leaving\non an adventure…\pOK!\nI'll help too.\pBut what can I do\nfor you?\pI know! I'll save\nmoney for you.\pOn a long journey,\nmoney's important.\pDo you want me to\nsave your money?$"

PolishedMom_LeavingText2:
	.string "OK, I'll take care\nof your money.\p………………$"

PolishedMom_LeavingText3:
	.string "Be careful.\pPOKéMON are your\nfriends. You need\lto work as a team.\pNow, go on!$"

PolishedMom_IsThisAboutYourMoneyText:
	.string "Hi! Welcome home!\nYou're trying very\lhard, I see.\pI've kept your\nroom tidy.\pOr is this about\nyour money?$"

PolishedMom_BankWhatDoYouWantToDoText:
	.string "What do you want\nto do?$"

PolishedMom_TakeMoneyText:
	.string "How much do you\nwant to take?$"

PolishedMom_StoreMoneyText:
	.string "How much do you\nwant to save?$"

PolishedMom_SaveMoneyText:
	.string "Do you want to\nsave some money?$"

PolishedMom_HaventSavedThatMuchText:
	.string "You haven't saved\nthat much.$"

PolishedMom_NotEnoughRoomInWalletText:
	.string "You can't take\nthat much.$"

PolishedMom_InsufficientFundsInWalletText:
	.string "You don't have\nthat much.$"

PolishedMom_NotEnoughRoomInBankText:
	.string "You can't save\nthat much.$"

PolishedMom_StartSavingMoneyText:
	.string "OK, I'll save your\nmoney. Trust me!\p{PLAYER}, stick\nwith it!$"

PolishedMom_StoredMoneyText:
	.string "Your money's safe\nhere! Get going!$"

PolishedMom_TakenMoneyText:
	.string "{PLAYER}, don't\ngive up!$"

PolishedMom_JustDoWhatYouCanText:
	.string "Just do what\nyou can.$"

PolishedMom_LostGearBookletText:
	.string "I lost the in-\nstruction booklet\lfor the POKégear.\pCome back again in\na while.$"

PolishedMom_AskDSTText:
	.string "Do you want to\nswitch to Daylight\lSaving Time?$"

PolishedMom_DSTText:
	.string "I set the clock\nforward by one\lhour.$"

PolishedMom_AskNotDSTText:
	.string "Is Daylight Saving\nTime over?$"

PolishedMom_NotDSTText:
	.string "I put the clock\nback one hour.$"

PolishedMom_AskAdjustDSTText:
	.string "Do you want to\nadjust your clock\pfor Daylight\nSaving Time?$"
