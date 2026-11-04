@ Generated from pinned ElmsHouse.asm: geography, static text and ambient residents.
PolishedElmsHouse_MapScripts::
	.byte 0

PolishedElmsHouse_ElmsHouseFridgeText::
	msgbox PolishedElmsHouse_ElmsHouseFridgeText_Text, MSGBOX_SIGN
	end

PolishedElmsHouse_ElmsHouseFridgeText_Text:
	.string "There's some food\nhere. This must be\lfor POKéMON.$"

PolishedElmsHouse_ElmsHousePCText::
	msgbox PolishedElmsHouse_ElmsHousePCText_Text, MSGBOX_SIGN
	end

PolishedElmsHouse_ElmsHousePCText_Text:
	.string "POKéMON. Where do\nthey come from?\pWhere are they\ngoing?\pWhy has no one\never witnessed a\lPOKéMON's birth?\pI want to know! I\nwill dedicate my\plife to the study\nof POKéMON!\p…\pIt's a part of\nProf.Elm's re-\lsearch papers.$"

PolishedElmsHouse_ElmsWifeText::
	msgbox PolishedElmsHouse_ElmsWifeText_Text, MSGBOX_NPC
	end

PolishedElmsHouse_ElmsWifeText_Text:
	.string "Hi, {PLAYER}! My\nhusband's always\pso busy--I hope\nhe's OK.\pWhen he's caught\nup in his POKéMON\presearch, he even\nforgets to eat.$"

PolishedElmsHouse_ElmsSonText::
	msgbox PolishedElmsHouse_ElmsSonText_Text, MSGBOX_NPC
	end

PolishedElmsHouse_ElmsSonText_Text:
	.string "When I grow up,\nI'm going to help\lmy dad!\pI'm going to be a\ngreat POKéMON\lprofessor!$"
