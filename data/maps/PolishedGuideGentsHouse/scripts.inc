@ Generated from pinned GuideGentsHouse.asm: geography, static text and ambient residents.
PolishedGuideGentsHouse_MapScripts::
	map_script MAP_SCRIPT_ON_LOAD, PolishedGuideGentsHouse_Setup
	.byte 0

PolishedGuideGentsHouse_Setup::
	setflag FLAG_TEMP_2
	goto_if_unset FLAG_POLISHED_GUIDE_HOME, PolishedGuideGentsHouse_SetupDone
	clearflag FLAG_TEMP_2
PolishedGuideGentsHouse_SetupDone::
	end

PolishedGuideGentsHouse_GuideGentsHouseGuideGentText::
	lock
	faceplayer
	call_if_unset FLAG_POLISHED_MAP_CARD, PolishedCherrygrove_GiveMap
	msgbox PolishedGuideGentsHouse_GuideText, MSGBOX_DEFAULT
	release
	end

PolishedGuideGentsHouse_GuideText:
	.string "When I was a wee\nlad, I was a hot-\lshot trainer!\pHere's a word of\nadvice: Catch lots\lof POKéMON!\pTreat them all\nwith kindness!$"
PolishedGuideGentsHouse_Window::
	msgbox PolishedGuideGentsHouse_Window_Text, MSGBOX_SIGN
	end

PolishedGuideGentsHouse_Window_Text:
	.string "My reflection!\nLookin' good!$"

PolishedGuideGentsHouse_Refrigerator::
	msgbox PolishedGuideGentsHouse_Refrigerator_Text, MSGBOX_SIGN
	end

PolishedGuideGentsHouse_Refrigerator_Text:
	.string "It's a fridge.$"

PolishedGuideGentsHouse_Sink::
	msgbox PolishedGuideGentsHouse_Sink_Text, MSGBOX_SIGN
	end

PolishedGuideGentsHouse_Sink_Text:
	.string "It's a sink.$"

PolishedGuideGentsHouse_Stove::
	msgbox PolishedGuideGentsHouse_Stove_Text, MSGBOX_SIGN
	end

PolishedGuideGentsHouse_Stove_Text:
	.string "It's a stove.$"

PolishedGuideGentsHouse_TV::
	getobjectfacingdirection OBJ_EVENT_ID_PLAYER, VAR_0x8004
	goto_if_ne VAR_0x8004, DIR_NORTH, PolishedGuideGentsHouse_TV_WrongSide
	msgbox PolishedGuideGentsHouse_TV_Text, MSGBOX_SIGN
	end

PolishedGuideGentsHouse_TV_Text:
	.string "It's a TV.$"

PolishedGuideGentsHouse_TV_WrongSide::
	msgbox PolishedGuideGentsHouse_TV_WrongText, MSGBOX_SIGN
	end

PolishedGuideGentsHouse_TV_WrongText:
	.string "Oops, wrong side.$"
