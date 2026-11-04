@ Generated from pinned NewBarkTown.asm: geography and static text only.
PolishedNewBarkTown_MapScripts::
	.byte 0

PolishedNewBarkTown_NewBarkTownSignText::
	msgbox PolishedNewBarkTown_NewBarkTownSignText_Text, MSGBOX_SIGN
	end

PolishedNewBarkTown_NewBarkTownSignText_Text:
	.string "New Bark Town\pThe Town Where the\nWinds of a New\lBeginning Blow$"

PolishedNewBarkTown_PlayersHouseSignText::
	msgbox PolishedNewBarkTown_PlayersHouseSignText_Text, MSGBOX_SIGN
	end

PolishedNewBarkTown_PlayersHouseSignText_Text:
	.string "{PLAYER}'s House$"

PolishedNewBarkTown_ElmsLabSignText::
	msgbox PolishedNewBarkTown_ElmsLabSignText_Text, MSGBOX_SIGN
	end

PolishedNewBarkTown_ElmsLabSignText_Text:
	.string "Elm POKéMON Lab$"

PolishedNewBarkTown_LyrasHouseSignText::
	msgbox PolishedNewBarkTown_LyrasHouseSignText_Text, MSGBOX_SIGN
	end

PolishedNewBarkTown_LyrasHouseSignText_Text:
	.string "Lyra's House$"

PolishedNewBarkTown_Teacher::
	lock
	faceplayer
	msgbox PolishedNewBarkTown_Breeze, MSGBOX_DEFAULT
	msgbox PolishedNewBarkTown_Garden, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, PolishedNewBarkTown_Stay
	closemessage
	warp MAP_POLISHED_JOHTO_DEMO_OUTDOOR, 6, 8
	waitstate
PolishedNewBarkTown_Stay::
	release
	end

PolishedNewBarkTown_Breeze:
	.string "There's always\nsuch a refreshing\lbreeze here.$"

PolishedNewBarkTown_Garden:
	.string "Return to the demonstration\ngarden?$"
