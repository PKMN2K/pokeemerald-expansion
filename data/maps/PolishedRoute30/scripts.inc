@ Generated from pinned Route30.asm: geography, static text and ambient residents.
PolishedRoute30_MapScripts::
	map_script MAP_SCRIPT_ON_LOAD, PolishedRoute30_Setup
	map_script MAP_SCRIPT_ON_RETURN_TO_FIELD, PolishedRoute30_Setup
	map_script MAP_SCRIPT_ON_FRAME_TABLE, PolishedReturn_PhoneFrame
	.byte 0

PolishedRoute30_Route30SignText::
	msgbox PolishedRoute30_Route30SignText_Text, MSGBOX_SIGN
	end

PolishedRoute30_Route30SignText_Text:
	.string "Route 30\pCherrygrove City -\nViolet City$"

PolishedRoute30_MrPokemonsHouseDirectionsSignText::
	msgbox PolishedRoute30_MrPokemonsHouseDirectionsSignText_Text, MSGBOX_SIGN
	end

PolishedRoute30_MrPokemonsHouseDirectionsSignText_Text:
	.string "Mr.POKéMON's House\nStraight Ahead!$"

PolishedRoute30_MrPokemonsHouseSignText::
	msgbox PolishedRoute30_MrPokemonsHouseSignText_Text, MSGBOX_SIGN
	end

PolishedRoute30_MrPokemonsHouseSignText_Text:
	.string "Mr.POKéMON's House$"

PolishedRoute30_Route30TrainerTipsText::
	msgbox PolishedRoute30_Route30TrainerTipsText_Text, MSGBOX_SIGN
	end

PolishedRoute30_Route30TrainerTipsText_Text:
	.string "Trainer Tips\pNo stealing other\npeople's POKéMON!\pPOKé Balls are to\nbe thrown only at\lwild POKéMON!$"

PolishedRoute30_Route30AdvancedTipsText::
	msgbox PolishedRoute30_Route30AdvancedTipsText_Text, MSGBOX_SIGN
	end

PolishedRoute30_Route30AdvancedTipsText_Text:
	.string "Advanced Tips!\pDuring battle, choose\nPOKéMON to switch partners.\pChoose BAG to use an item,\nor RUN to leave a wild battle!$"

PolishedRoute30_BerryMastersHouseSignText::
	msgbox PolishedRoute30_BerryMastersHouseSignText_Text, MSGBOX_SIGN
	end

PolishedRoute30_BerryMastersHouseSignText_Text:
	.string "Berry Master's\nHouse$"

PolishedRoute30_Setup::
	setflag FLAG_TEMP_2
	call_if_set FLAG_POLISHED_GAVE_MYSTERY_EGG, PolishedReturn_ShowJoey
	special PolishedFruitRefreshAll
	end
PolishedRoute30_Silent::
	end
PolishedRoute30_Pidgey::
	end
PolishedRoute30_Antidote::
	finditem ITEM_ANTIDOTE
	end
PolishedRoute30_Oran::
	setvar VAR_0x8005, 1
	goto PolishedFruit_Interact
PolishedRoute30_Pecha::
	setvar VAR_0x8005, 2
	goto PolishedFruit_Interact
PolishedRoute30_Joey::
	lockall
	msgbox PolishedRoute30_UseTackleText, MSGBOX_DEFAULT
	closemessage
	playse SE_M_COMET_PUNCH
	applymovement LOCALID_ROUTE30_RATTATA, PolishedRoute30_RattataAttack
	waitmovement 0
	faceplayer
	msgbox PolishedRoute30_BigBattleText, MSGBOX_DEFAULT
	closemessage
	turnobject LOCALID_ROUTE30_JOEY, DIR_NORTH
	playse SE_M_COMET_PUNCH
	applymovement LOCALID_ROUTE30_PIDGEY, PolishedRoute30_PidgeyAttack
	waitmovement 0
	releaseall
	end
PolishedRoute30_RattataAttack:
	walk_fast_up
	walk_fast_down
	step_end
PolishedRoute30_PidgeyAttack:
	walk_fast_down
	walk_fast_up
	step_end
PolishedRoute30_UseTackleText:
	.string "Go, Rattata!\pTackle!$"

PolishedRoute30_BigBattleText:
	.string "What? This is a\nbig battle!\lLeave me alone!$"

PolishedRoute30_Youngster::
	goto_if_set FLAG_POLISHED_GAVE_MYSTERY_EGG, PolishedReturn_RouteYoungster
	msgbox PolishedRoute30_YoungsterText, MSGBOX_NPC
	end
PolishedRoute30_YoungsterText:
	.string "Mr.POKéMON's\nhouse? It's a bit\lfarther ahead.$"

PolishedRoute30_Lass::
	msgbox PolishedRoute30_LassText, MSGBOX_NPC
	end
PolishedRoute30_LassText:
	.string "I'm not a trainer.\pBut if you look\none in the eyes,\lprepare to battle.$"

PolishedRoute30_Mikey::
	trainerbattle_single TRAINER_POLISHED_MIKEY, PolishedRoute30_MikeySeen, PolishedRoute30_MikeyBeaten
	msgbox PolishedRoute30_MikeyAfter, MSGBOX_AUTOCLOSE
	end
PolishedRoute30_MikeySeen:
	.string "You're a POKéMON\ntrainer, right?\pThen you have to\nbattle!$"

PolishedRoute30_MikeyBeaten:
	.string "That's strange.\nI won before.$"

PolishedRoute30_MikeyAfter:
	.string "Becoming a good\ntrainer is really\ltough.\pI'm going to bat-\ntle other people\lto get better.$"

PolishedRoute30_Don::
	trainerbattle_single TRAINER_POLISHED_DON, PolishedRoute30_DonSeen, PolishedRoute30_DonBeaten
	msgbox PolishedRoute30_DonAfter, MSGBOX_AUTOCLOSE
	end
PolishedRoute30_DonSeen:
	.string "Instead of a bug\nPOKéMON, I found\la trainer!$"

PolishedRoute30_DonBeaten:
	.string "Argh! You're too\nstrong!$"

PolishedRoute30_DonAfter:
	.string "I ran out of POKé\nBalls while I was\lcatching POKéMON.\pI should've bought\nsome more…$"

PolishedReturn_RouteYoungster::
	msgbox PolishedReturn_RouteYoungsterText, MSGBOX_NPC
	end
PolishedReturn_RouteYoungsterText:
	.string "Everyone's having\nfun battling!\lYou should too!$"

PolishedReturn_ShowJoey::
	clearflag FLAG_TEMP_2
	return
PolishedReturn_PhoneFrame::
	map_script_2 VAR_POLISHED_ERRAND_STATE, 1, PolishedReturn_ElmCall
	.2byte 0
PolishedReturn_ElmCall::
	lockall
	playse SE_PC_LOGIN
	msgbox PolishedReturn_PhoneCaller, MSGBOX_DEFAULT
	msgbox PolishedReturn_ElmPhoneDisasterText, MSGBOX_DEFAULT
	setvar VAR_POLISHED_ERRAND_STATE, 2
	releaseall
	end
PolishedReturn_PhoneCaller:
	.string "Incoming call: PROF. ELM$"
PolishedReturn_Joey::
	trainerbattle_single TRAINER_POLISHED_JOEY, PolishedReturn_JoeySeen, PolishedReturn_JoeyBeaten
	msgbox PolishedReturn_JoeyAfter, MSGBOX_AUTOCLOSE
	end
PolishedReturn_ElmPhoneDisasterText:
	.string "H-hello? {PLAYER}?\nIt's a disaster!\pUh, um, it's just\nterrible!\pWhat should I do?\nIt… Oh, no…\pPlease get back\nhere now!$"

PolishedReturn_JoeySeen:
	.string "I just lost, so\nI'm trying to find\lmore POKéMON.\pWait! You look\nweak! Come on,\llet's battle!$"

PolishedReturn_JoeyBeaten:
	.string "Ack! I lost again!\nDoggone it!$"

PolishedReturn_JoeyAfter:
	.string "Do I have to have\nmore POKéMON in\porder to battle\nbetter?\pNo! I'm sticking\nwith this one no\lmatter what!$"
