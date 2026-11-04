@ Generated from pinned CherrygroveCity.asm: geography, static text and ambient residents.
PolishedCherrygroveCity_MapScripts::
	.byte 0

PolishedCherrygroveCity_CherrygroveCitySignText::
	msgbox PolishedCherrygroveCity_CherrygroveCitySignText_Text, MSGBOX_SIGN
	end

PolishedCherrygroveCity_CherrygroveCitySignText_Text:
	.string "Cherrygrove City\pThe City of Cute,\nFragrant Flowers$"

PolishedCherrygroveCity_GuideGentsHouseSignText::
	msgbox PolishedCherrygroveCity_GuideGentsHouseSignText_Text, MSGBOX_SIGN
	end

PolishedCherrygroveCity_GuideGentsHouseSignText_Text:
	.string "Guide Gent's House$"

PolishedCherrygroveCity_CherrygroveCityAdvancedTipsSignText::
	msgbox PolishedCherrygroveCity_CherrygroveCityAdvancedTipsSignText_Text, MSGBOX_SIGN
	end

PolishedCherrygroveCity_CherrygroveCityAdvancedTipsSignText_Text:
	.string "Advanced Tips!\pPOKé Marts will\ngive you a free\pPremier Ball with\nevery purchase of\lten POKé Balls!$"

PolishedCherrygroveCity_CherrygroveTeacherText_HaveMapCard::
	msgbox PolishedCherrygroveCity_CherrygroveTeacherText_HaveMapCard_Text, MSGBOX_NPC
	end

PolishedCherrygroveCity_CherrygroveTeacherText_HaveMapCard_Text:
	.string "When you're with\nPOKéMON, going\lanywhere is fun.$"

PolishedCherrygroveCity_CherrygroveYoungsterScript::
	msgbox PolishedCherrygroveCity_CherrygroveYoungsterScript_Text, MSGBOX_NPC
	end

PolishedCherrygroveCity_CherrygroveYoungsterScript_Text:
	.string "Mr.POKéMON's house\nis still farther\lup ahead.$"

PolishedCherrygroveCity_Pidgey::
	lock
	playmoncry SPECIES_PIDGEY, CRY_MODE_NORMAL
	msgbox PolishedCherrygroveCity_Pidgey_Text, MSGBOX_DEFAULT
	waitmoncry
	release
	end

PolishedCherrygroveCity_Pidgey_Text:
	.string "Pidgey: Pijji!$"

PolishedCherrygroveCity_MysticWaterGuy::
	lock
	faceplayer
	goto_if_set FLAG_POLISHED_CHERRYGROVE_MYSTIC_WATER, PolishedCherrygroveCity_MysticAfter
	msgbox PolishedCherrygroveCity_MysticBefore_Text, MSGBOX_DEFAULT
	giveitem ITEM_MYSTIC_WATER
	goto_if_eq VAR_RESULT, FALSE, PolishedCherrygroveCity_MysticDone
	setflag FLAG_POLISHED_CHERRYGROVE_MYSTIC_WATER
PolishedCherrygroveCity_MysticAfter::
	msgbox PolishedCherrygroveCity_MysticAfter_Text, MSGBOX_DEFAULT
PolishedCherrygroveCity_MysticDone::
	release
	end

PolishedCherrygroveCity_MysticBefore_Text:
	.string "A POKéMON I caught\nhad an item.\pI think it's\nMystic Water.\pI don't need it,\nso do you want it?$"

PolishedCherrygroveCity_MysticAfter_Text:
	.string "Back to fishing\nfor me, then.$"

PolishedCherrygrove_Trigger::
	lockall
	applymovement OBJ_EVENT_ID_PLAYER, PolishedCherrygrove_Enter
	waitmovement 0
	goto PolishedCherrygrove_Tour

PolishedCherrygroveCity_CherrygroveCityGuideGent::
	lockall
	getplayerxy VAR_0x8004, VAR_0x8005
	goto_if_eq VAR_0x8004, 31, PolishedCherrygrove_FromWest
	goto_if_eq VAR_0x8004, 33, PolishedCherrygrove_FromEast
	goto PolishedCherrygrove_Tour
PolishedCherrygrove_FromWest::
	applymovement OBJ_EVENT_ID_PLAYER, PolishedCherrygrove_WestApproach
	waitmovement 0
	goto PolishedCherrygrove_Tour
PolishedCherrygrove_FromEast::
	applymovement OBJ_EVENT_ID_PLAYER, PolishedCherrygrove_EastApproach
	waitmovement 0
PolishedCherrygrove_Tour::
	turnobject OBJ_EVENT_ID_PLAYER, DIR_NORTH
	msgbox PolishedCherrygrove_GuideGentIntroText, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_CHERRYGROVE_GUIDE, PolishedCherrygrove_Guide1
	applymovement OBJ_EVENT_ID_PLAYER, PolishedCherrygrove_Follow1
	waitmovement OBJ_EVENT_ID_PLAYER
	waitmovement LOCALID_CHERRYGROVE_GUIDE
	turnobject OBJ_EVENT_ID_PLAYER, DIR_NORTH
	msgbox PolishedCherrygrove_GuideGentPokeCenterText, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_CHERRYGROVE_GUIDE, PolishedCherrygrove_Guide2
	applymovement OBJ_EVENT_ID_PLAYER, PolishedCherrygrove_Follow2
	waitmovement OBJ_EVENT_ID_PLAYER
	waitmovement LOCALID_CHERRYGROVE_GUIDE
	turnobject OBJ_EVENT_ID_PLAYER, DIR_NORTH
	msgbox PolishedCherrygrove_GuideGentMartText, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_CHERRYGROVE_GUIDE, PolishedCherrygrove_Guide3
	applymovement OBJ_EVENT_ID_PLAYER, PolishedCherrygrove_Follow3
	waitmovement OBJ_EVENT_ID_PLAYER
	waitmovement LOCALID_CHERRYGROVE_GUIDE
	turnobject OBJ_EVENT_ID_PLAYER, DIR_NORTH
	msgbox PolishedCherrygrove_GuideGentRoute30Text, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_CHERRYGROVE_GUIDE, PolishedCherrygrove_Guide3_5
	applymovement OBJ_EVENT_ID_PLAYER, PolishedCherrygrove_Follow3_5
	waitmovement OBJ_EVENT_ID_PLAYER
	waitmovement LOCALID_CHERRYGROVE_GUIDE
	turnobject OBJ_EVENT_ID_PLAYER, DIR_WEST
	msgbox PolishedCherrygrove_GuideGentAdvancedTipsText, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_CHERRYGROVE_GUIDE, PolishedCherrygrove_Guide4
	applymovement OBJ_EVENT_ID_PLAYER, PolishedCherrygrove_Follow4
	waitmovement OBJ_EVENT_ID_PLAYER
	waitmovement LOCALID_CHERRYGROVE_GUIDE
	turnobject OBJ_EVENT_ID_PLAYER, DIR_WEST
	msgbox PolishedCherrygrove_GuideGentSeaText, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_CHERRYGROVE_GUIDE, PolishedCherrygrove_Guide5
	applymovement OBJ_EVENT_ID_PLAYER, PolishedCherrygrove_Follow5
	waitmovement OBJ_EVENT_ID_PLAYER
	waitmovement LOCALID_CHERRYGROVE_GUIDE
	turnobject OBJ_EVENT_ID_PLAYER, DIR_EAST
	turnobject LOCALID_CHERRYGROVE_GUIDE, DIR_WEST
	msgbox PolishedCherrygrove_GuideGentGiftText, MSGBOX_DEFAULT
	call PolishedCherrygrove_GiveMap
	closemessage
	turnobject OBJ_EVENT_ID_PLAYER, DIR_NORTH
	applymovement LOCALID_CHERRYGROVE_GUIDE, PolishedCherrygrove_Leave
	waitmovement 0
	playse SE_DOOR
	removeobject LOCALID_CHERRYGROVE_GUIDE
	setflag FLAG_POLISHED_GUIDE_HOME
	setvar VAR_POLISHED_CHERRYGROVE_SCENE, 2
	releaseall
	end

PolishedCherrygrove_GiveMap::
	goto_if_set FLAG_POLISHED_MAP_CARD, PolishedCherrygrove_MapDone
	giveitem ITEM_TOWN_MAP
	goto_if_eq VAR_RESULT, FALSE, PolishedCherrygrove_MapFull
	setflag FLAG_POLISHED_MAP_CARD
	msgbox PolishedCherrygrove_MapAccess, MSGBOX_DEFAULT
PolishedCherrygrove_MapDone::
	return
PolishedCherrygrove_MapFull::
	msgbox PolishedCherrygrove_MapFullText, MSGBOX_DEFAULT
	return

PolishedCherrygrove_WestApproach:
	walk_down
	walk_right
	step_end
PolishedCherrygrove_EastApproach:
	walk_down
	walk_left
	step_end

PolishedCherrygrove_Enter:
	walk_left
	face_up
	step_end

PolishedCherrygrove_Leave:
	walk_up
	walk_up
	step_end

PolishedCherrygrove_Guide1:
	walk_left
	walk_left
	walk_up
	walk_left
	face_up
	step_end

PolishedCherrygrove_Follow1:
	walk_up
	walk_left
	walk_left
	walk_up
	step_end

PolishedCherrygrove_Guide2:
	walk_left
	walk_left
	walk_left
	walk_left
	walk_left
	walk_left
	face_up
	step_end

PolishedCherrygrove_Follow2:
	walk_left
	walk_left
	walk_left
	walk_left
	walk_left
	walk_left
	step_end

PolishedCherrygrove_Guide3:
	walk_left
	walk_left
	walk_left
	walk_left
	walk_left
	walk_left
	walk_left
	face_up
	step_end

PolishedCherrygrove_Follow3:
	walk_left
	walk_left
	walk_left
	walk_left
	walk_left
	walk_left
	walk_left
	step_end

PolishedCherrygrove_Guide3_5:
	walk_left
	walk_left
	walk_down
	walk_left
	face_up
	step_end

PolishedCherrygrove_Follow3_5:
	walk_left
	walk_left
	walk_left
	walk_down
	step_end

PolishedCherrygrove_Guide4:
	walk_left
	walk_left
	walk_left
	walk_down
	walk_down
	face_left
	step_end

PolishedCherrygrove_Follow4:
	walk_left
	walk_left
	walk_left
	walk_left
	walk_down
	step_end

PolishedCherrygrove_Guide5:
	walk_down
	walk_right
	walk_right
	walk_right
	walk_right
	walk_right
	walk_right
	walk_right
	walk_right
	walk_right
	walk_right
	walk_down
	walk_down
	walk_right
	walk_right
	walk_right
	walk_right
	walk_right
	face_up
	step_end

PolishedCherrygrove_Follow5:
	walk_down
	walk_down
	walk_right
	walk_right
	walk_right
	walk_right
	walk_right
	walk_right
	walk_right
	walk_right
	walk_right
	walk_right
	walk_down
	walk_down
	walk_right
	walk_right
	walk_right
	walk_right
	step_end

PolishedCherrygrove_GuideGentIntroText:
	.string "You're a rookie\ntrainer, aren't\lyou? I can tell!\pThat's OK! Every-\none is a rookie\lat some point!\pI can teach you\na few things.\lFollow me!$"

PolishedCherrygrove_GuideGentPokeCenterText:
	.string "This is a POKéMON\nCenter. They heal\pyour POKéMON in no\ntime at all.\pYou'll be relying\non them a lot, so\pyou better learn\nabout them.$"

PolishedCherrygrove_GuideGentMartText:
	.string "This is a POKéMON\nMart, or just\lPOKé Mart.\pThey sell Balls\nfor catching wild\pPOKéMON and other\nuseful items.$"

PolishedCherrygrove_GuideGentRoute30Text:
	.string "Route 30 is out\nthis way.\pTrainers will be\nbattling their\pprized POKéMON\nthere.$"

PolishedCherrygrove_GuideGentAdvancedTipsText:
	.string "Advanced Tips\nsigns have this\lunusual look.\pThey're full of\nhelpful advice.$"

PolishedCherrygrove_GuideGentSeaText:
	.string "This is the sea,\nas you can see.\pRoute 32 is just\nacross the bay.$"

PolishedCherrygrove_GuideGentGiftText:
	.string "Here…\pIt's my house!\nThanks for your\lcompany.\pLet me give you a\nsmall gift.$"

PolishedCherrygrove_MapAccess:
	.string "Open the TOWN MAP from\nyour BAG's KEY ITEMS.\pI wish you luck on\nyour journey!$"

PolishedCherrygrove_MapFullText:
	.string "Your BAG is full.\pCome to my house when\nyou have room for the map.$"
PolishedCherrygroveCity_MartSign::
	msgbox PolishedCherrygroveCity_MartSign_Text, MSGBOX_SIGN
	end

PolishedCherrygroveCity_MartSign_Text:
	.string "For All Your\nPOKéMON Needs\pPOKéMON Mart$"

PolishedCherrygroveCity_PokeCenterSign::
	msgbox PolishedCherrygroveCity_PokeCenterSign_Text, MSGBOX_SIGN
	end

PolishedCherrygroveCity_PokeCenterSign_Text:
	.string "Heal Your POKéMON!\nPOKéMON Center$"
