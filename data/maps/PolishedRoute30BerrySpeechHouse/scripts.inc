@ Generated from pinned Route30BerrySpeechHouse.asm: geography, static text and ambient residents.
PolishedRoute30BerrySpeechHouse_MapScripts::
	.byte 0

PolishedRoute30BerrySpeechHouse_Talk::
	lock
	faceplayer
	goto_if_set FLAG_POLISHED_ROUTE30_MULCH, PolishedRoute30BerrySpeechHouse_After
	msgbox PolishedRoute30BerrySpeechHouse_BeforeText, MSGBOX_DEFAULT
	giveitem ITEM_POLISHED_MULCH
	goto_if_eq VAR_RESULT, FALSE, PolishedRoute30BerrySpeechHouse_Done
	setflag FLAG_POLISHED_ROUTE30_MULCH
PolishedRoute30BerrySpeechHouse_After::
	msgbox PolishedRoute30BerrySpeechHouse_AfterText, MSGBOX_DEFAULT
PolishedRoute30BerrySpeechHouse_Done::
	release
	end

PolishedRoute30BerrySpeechHouse_BeforeText:
	.string "You know, POKéMON\neat Berries.\pThey take time to\ngrow back when\lthey're picked,\pbut using Mulch\non the soil really\lspeeds that up!\pHere. I'll share\nsome with you!$"

PolishedRoute30BerrySpeechHouse_AfterText:
	.string "I buy top-quality\nMulch in Goldenrod\lCity.$"
PolishedRoute30BerrySpeechHouse_Window::
	msgbox PolishedRoute30BerrySpeechHouse_Window_Text, MSGBOX_SIGN
	end

PolishedRoute30BerrySpeechHouse_Window_Text:
	.string "My reflection!\nLookin' good!$"

PolishedRoute30BerrySpeechHouse_Refrigerator::
	msgbox PolishedRoute30BerrySpeechHouse_Refrigerator_Text, MSGBOX_SIGN
	end

PolishedRoute30BerrySpeechHouse_Refrigerator_Text:
	.string "It's a fridge.$"

PolishedRoute30BerrySpeechHouse_Sink::
	msgbox PolishedRoute30BerrySpeechHouse_Sink_Text, MSGBOX_SIGN
	end

PolishedRoute30BerrySpeechHouse_Sink_Text:
	.string "It's a sink.$"

PolishedRoute30BerrySpeechHouse_Stove::
	msgbox PolishedRoute30BerrySpeechHouse_Stove_Text, MSGBOX_SIGN
	end

PolishedRoute30BerrySpeechHouse_Stove_Text:
	.string "It's a stove.$"

PolishedRoute30BerrySpeechHouse_MagazineBookshelf::
	msgbox PolishedRoute30BerrySpeechHouse_MagazineBookshelf_Text, MSGBOX_SIGN
	end

PolishedRoute30BerrySpeechHouse_MagazineBookshelf_Text:
	.string "POKéMON magazines…\nPOKéMON Pal,\pPOKéMON Handbook,\nPOKéMON Graph…$"
