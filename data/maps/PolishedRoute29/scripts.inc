@ Generated from pinned Route29.asm: geography, static text and ambient residents.
PolishedRoute29_MapScripts::
	map_script MAP_SCRIPT_ON_LOAD, PolishedRoute29_Setup
	map_script MAP_SCRIPT_ON_RETURN_TO_FIELD, PolishedRoute29_Setup
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
	lockall
	special PolishedRoute29RefreshFruit
	msgbox PolishedRoute29_FruitBearing, MSGBOX_DEFAULT
	specialvar VAR_RESULT, PolishedRoute29FruitAvailable
	goto_if_eq VAR_RESULT, FALSE, PolishedRoute29_NoFruit
	bufferitemname STR_VAR_3, ITEM_ORAN_BERRY
	msgbox PolishedRoute29_FruitHey, MSGBOX_DEFAULT
	special PolishedRoute29PickFruit
	goto_if_eq VAR_RESULT, 0, PolishedRoute29_FruitFull
	copyvar VAR_0x8004, VAR_RESULT
	goto_if_eq VAR_RESULT, 1, PolishedRoute29_FruitOne
	goto_if_eq VAR_RESULT, 2, PolishedRoute29_FruitTwo
	msgbox PolishedRoute29_FruitObtainedThree, MSGBOX_DEFAULT
	goto PolishedRoute29_FruitDone
PolishedRoute29_FruitOne::
	msgbox PolishedRoute29_FruitObtainedOne, MSGBOX_DEFAULT
	goto PolishedRoute29_FruitDone
PolishedRoute29_FruitTwo::
	msgbox PolishedRoute29_FruitObtainedTwo, MSGBOX_DEFAULT
PolishedRoute29_FruitDone::
	playfanfare MUS_OBTAIN_ITEM
	waitfanfare
	putitemaway ITEM_ORAN_BERRY, VAR_0x8004
	releaseall
	end
PolishedRoute29_FruitFull::
	msgbox PolishedRoute29_FruitPackFull, MSGBOX_DEFAULT
	releaseall
	end
PolishedRoute29_NoFruit::
	msgbox PolishedRoute29_FruitNothing, MSGBOX_DEFAULT
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
