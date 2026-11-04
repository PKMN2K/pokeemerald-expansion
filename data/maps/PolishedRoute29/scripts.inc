@ Generated from pinned Route29.asm: geography, static text and ambient residents.
PolishedRoute29_MapScripts::
	map_script MAP_SCRIPT_ON_LOAD, PolishedRoute29_Setup
	map_script MAP_SCRIPT_ON_RETURN_TO_FIELD, PolishedDeparture_Restore
	.byte 0

PolishedRoute29_Route29Sign1Text::
	msgbox PolishedRoute29_Route29Sign1Text_Text, MSGBOX_SIGN
	end

PolishedRoute29_Route29Sign1Text_Text:
	.string "Route 29\pNew Bark Town -\nCherrygrove City$"

PolishedRoute29_Route29Sign2Text::
	msgbox PolishedRoute29_Route29Sign2Text_Text, MSGBOX_SIGN
	end

PolishedRoute29_Route29Sign2Text_Text:
	.string "Route 29\pCherrygrove City -\nNew Bark Town$"

PolishedRoute29_Route29AdvancedTipsSignText::
	msgbox PolishedRoute29_Route29AdvancedTipsSignText_Text, MSGBOX_SIGN
	end

PolishedRoute29_Route29AdvancedTipsSignText_Text:
	.string "Advanced Tips!\pSome POKéMON appear\nonly at certain times.\pLook during both the\nday and the night!$"

PolishedRoute29_Route29YoungsterText::
	msgbox PolishedRoute29_Route29YoungsterText_Text, MSGBOX_NPC
	end

PolishedRoute29_Route29YoungsterText_Text:
	.string "Yo. How are your\nPOKéMON?\pIf they're weak\nand not ready for\pbattle, keep out\nof the grass.$"

PolishedRoute29_Route29TeacherText::
	msgbox PolishedRoute29_Route29TeacherText_Text, MSGBOX_NPC
	end

PolishedRoute29_Route29TeacherText_Text:
	.string "See those ledges?\nIt's scary to jump\loff them.\pBut you can go to\nNew Bark without\pwalking through\nthe grass.$"

PolishedRoute29_Route29FisherText::
	msgbox PolishedRoute29_Route29FisherText_Text, MSGBOX_NPC
	end

PolishedRoute29_Route29FisherText_Text:
	.string "I wanted to take a\nbreak, so I saved\pto record my\nprogress.$"

PolishedRoute29_Setup::
	call PolishedDeparture_RouteSetup
	special PolishedRoute29RefreshFruit
	end

PolishedRoute29_CoolDude::
	lock
	faceplayer
	gettime
	goto_if_lt VAR_0x8000, 5, PolishedRoute29_CoolDudeNight
	goto_if_ge VAR_0x8000, 17, PolishedRoute29_CoolDudeNight
	msgbox PolishedRoute29_Text_WaitingForNight, MSGBOX_DEFAULT
	release
	end
PolishedRoute29_CoolDudeNight::
	msgbox PolishedRoute29_Text_WaitingForMorning, MSGBOX_DEFAULT
	release
	end

PolishedRoute29_Potion::
	finditem ITEM_POTION
	end

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

PolishedRoute29_Text_WaitingForNight:
	.string "I'm waiting for\nPOKéMON that\pappear only in\nthe evening or\lat night.$"

PolishedRoute29_Text_WaitingForMorning:
	.string "I'm waiting for\nPOKéMON that\pappear only in the\nmorning.$"

PolishedRoute29_FruitBearing:
	.string "It's a fruit-\nbearing tree.$"

PolishedRoute29_FruitHey:
	.string "Hey! It's\n{STR_VAR_3}!$"

PolishedRoute29_FruitObtainedOne:
	.string "Obtained a\n{STR_VAR_3}!$"

PolishedRoute29_FruitObtainedTwo:
	.string "Obtained two\n{STR_VAR_3}!$"

PolishedRoute29_FruitObtainedThree:
	.string "Obtained three\n{STR_VAR_3}!$"

PolishedRoute29_FruitPackFull:
	.string "But the Bag is\nfull…$"

PolishedRoute29_FruitNothing:
	.string "There's nothing\nhere…$"

PolishedFruit_WantMulch:
	.string "Want to use\nMulch?$"

PolishedFruit_UsedMulch:
	.string "{PLAYER} used\nMulch!\pThe tree was\nfertilized!$"

PolishedDeparture_RouteSetup::
    setvar VAR_TEMP_2, 0
    setflag FLAG_TEMP_2
    goto_if_unset FLAG_POLISHED_GAVE_MYSTERY_EGG, PolishedDeparture_Tuesday
    goto_if_set FLAG_POLISHED_CAUGHT_TUTORIAL, PolishedDeparture_Tuesday
    clearflag FLAG_TEMP_2
    setvar VAR_TEMP_2, 1
PolishedDeparture_Tuesday::
    setflag FLAG_TEMP_3
    goto_if_unset FLAG_POLISHED_MOM_BANK_OPEN, PolishedDeparture_MaskDone
    specialvar VAR_TEMP_A, PolishedJohtoGetWeekday
    goto_if_ne VAR_TEMP_A, 2, PolishedDeparture_MaskDone
    clearflag FLAG_TEMP_3
PolishedDeparture_MaskDone::
    return
PolishedDeparture_Silent::
    end
PolishedDeparture_Approach1::
    lockall
    checkitemspace ITEM_POKE_BALL, 5
    goto_if_eq VAR_RESULT, FALSE, PolishedDeparture_Full
    turnobject LOCALID_ROUTE29_LYRA, DIR_NORTH
    applymovement LOCALID_ROUTE29_LYRA, PolishedIntro_Exclamation
    waitmovement 0
    delay 15
    applymovement LOCALID_ROUTE29_LYRA, PolishedDeparture_Move1a
    waitmovement 0
    turnobject OBJ_EVENT_ID_PLAYER, DIR_WEST
    msgbox PolishedDeparture_CatchingTutorialIntroText, MSGBOX_YESNO
    goto_if_eq VAR_RESULT, NO, PolishedDeparture_Decline
    closemessage
    applymovement OBJ_EVENT_ID_PLAYER, PolishedDeparture_Follow1
    applymovement LOCALID_ROUTE29_LYRA, PolishedDeparture_Move1b
    waitmovement OBJ_EVENT_ID_PLAYER
    waitmovement LOCALID_ROUTE29_LYRA
    goto PolishedDeparture_Demonstrate
PolishedDeparture_Approach2::
    lockall
    checkitemspace ITEM_POKE_BALL, 5
    goto_if_eq VAR_RESULT, FALSE, PolishedDeparture_Full
    turnobject LOCALID_ROUTE29_LYRA, DIR_NORTH
    applymovement LOCALID_ROUTE29_LYRA, PolishedIntro_Exclamation
    waitmovement 0
    delay 15
    applymovement LOCALID_ROUTE29_LYRA, PolishedDeparture_Move2a
    waitmovement 0
    turnobject OBJ_EVENT_ID_PLAYER, DIR_WEST
    msgbox PolishedDeparture_CatchingTutorialIntroText, MSGBOX_YESNO
    goto_if_eq VAR_RESULT, NO, PolishedDeparture_Decline
    closemessage
    applymovement OBJ_EVENT_ID_PLAYER, PolishedDeparture_Follow2
    applymovement LOCALID_ROUTE29_LYRA, PolishedDeparture_Move2b
    waitmovement OBJ_EVENT_ID_PLAYER
    waitmovement LOCALID_ROUTE29_LYRA
    goto PolishedDeparture_Demonstrate
PolishedDeparture_Demonstrate::
    special SavePlayerParty
    special StartPolishedLyraTutorial
    waitstate
    special LoadPlayerParty
    turnobject LOCALID_ROUTE29_LYRA, DIR_NORTH
    msgbox PolishedDeparture_CatchingTutorialDebriefText, MSGBOX_DEFAULT
    goto PolishedDeparture_Gift
PolishedDeparture_Decline::
    setflag FLAG_POLISHED_DECLINED_TUTORIAL
    msgbox PolishedDeparture_CatchingTutorialRefusedText, MSGBOX_DEFAULT
PolishedDeparture_Gift::
    giveitem ITEM_POKE_BALL, 5
    msgbox PolishedDeparture_CatchingTutorialGoodbyeText, MSGBOX_DEFAULT
    closemessage
    applymovement LOCALID_ROUTE29_LYRA, PolishedDeparture_Move3
    waitmovement 0
    removeobject LOCALID_ROUTE29_LYRA
    setflag FLAG_TEMP_2
    setflag FLAG_POLISHED_CAUGHT_TUTORIAL
    setvar VAR_TEMP_2, 0
    releaseall
    end
PolishedDeparture_Full::
    msgbox PolishedDeparture_MakeRoom, MSGBOX_DEFAULT
    releaseall
    end
PolishedDeparture_Tuscany::
    lock
    faceplayer
    specialvar VAR_RESULT, PolishedJohtoGetWeekday
    goto_if_ne VAR_RESULT, 2, PolishedDeparture_WrongDay
    goto_if_set FLAG_POLISHED_TUSCANY_SILK_SCARF, PolishedDeparture_TuesdayAgain
    goto_if_set FLAG_POLISHED_MET_TUSCANY, PolishedDeparture_Scarf
    msgbox PolishedDeparture_MeetTuscanyText, MSGBOX_DEFAULT
    setflag FLAG_POLISHED_MET_TUSCANY
PolishedDeparture_Scarf::
    msgbox PolishedDeparture_TuscanyGivesGiftText, MSGBOX_DEFAULT
    giveitem ITEM_SILK_SCARF
    goto_if_eq VAR_RESULT, FALSE, PolishedDeparture_TuscanyDone
    setflag FLAG_POLISHED_TUSCANY_SILK_SCARF
    msgbox PolishedDeparture_TuscanyGaveGiftText, MSGBOX_DEFAULT
    goto PolishedDeparture_TuscanyDone
PolishedDeparture_TuesdayAgain::
    msgbox PolishedDeparture_TuscanyTuesdayText, MSGBOX_DEFAULT
    goto PolishedDeparture_TuscanyDone
PolishedDeparture_WrongDay::
    msgbox PolishedDeparture_TuscanyNotTuesdayText, MSGBOX_DEFAULT
PolishedDeparture_TuscanyDone::
    release
    end
PolishedDeparture_Move1a:
    walk_up
    walk_up
    walk_up
    walk_up
    walk_right
    walk_right
    step_end
PolishedDeparture_Move2a:
    walk_up
    walk_up
    walk_up
    walk_right
    walk_right
    step_end
PolishedDeparture_Move1b:
    walk_left
    walk_left
    walk_down
    walk_down
    walk_down
    walk_down
    step_end
PolishedDeparture_Follow1:
    walk_left
    walk_left
    walk_left
    walk_down
    walk_down
    walk_down
    step_end
PolishedDeparture_Move2b:
    walk_left
    walk_left
    walk_down
    walk_down
    walk_down
    step_end
PolishedDeparture_Follow2:
    walk_left
    walk_left
    walk_left
    walk_down
    walk_down
    step_end
PolishedDeparture_Move3:
    walk_left
    walk_left
    walk_left
    walk_left
    walk_left
    step_end
PolishedDeparture_CatchingTutorialIntroText:
	.string "Lyra: {PLAYER}!\pYou can catch wild\nPOKéMON with POKé\pBalls. Follow me\nand I'll show you!$"

PolishedDeparture_CatchingTutorialDebriefText:
	.string "Lyra: See? Just\nlike that!\pIf you weaken them\nfirst, POKéMON are\leasier to catch.\pI'll give you\nthese. Good luck!$"

PolishedDeparture_CatchingTutorialRefusedText:
	.string "Aww… I wanted to\nshow off my cap-\lturing skills a\llittle…\pAnyway, I'll give\nyou these.\lGood luck!$"

PolishedDeparture_CatchingTutorialGoodbyeText:
	.string "Lyra: See you!$"

PolishedDeparture_MeetTuscanyText:
	.string "Tuscany: I do be-\nlieve that this is\pthe first time\nwe've met?\pPlease allow me to\nintroduce myself.\pI am Tuscany of\nTuesday.$"

PolishedDeparture_TuscanyGivesGiftText:
	.string "By way of intro-\nduction, please\paccept this gift,\na Silk Scarf.$"

PolishedDeparture_TuscanyGaveGiftText:
	.string "Tuscany: Wouldn't\nyou agree that it\lis most elegant?\pIt strengthens\nNormal-type moves.\pI am certain it\nwill be of use.$"

PolishedDeparture_TuscanyTuesdayText:
	.string "Tuscany: Have you\nmet Monica, my\lolder sister?\pOr my younger\nbrother, Wesley?\pI am the second of\nseven children.$"

PolishedDeparture_TuscanyNotTuesdayText:
	.string "Tuscany: Today is\nnot Tuesday. That\lis unfortunate…$"

PolishedDeparture_MakeRoom:
	.string "Make room for five\nPOKé BALLS first.$"

PolishedDeparture_Restore::
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
