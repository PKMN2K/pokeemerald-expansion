@ Generated from pinned NewBarkTown.asm: geography, static text and ambient residents.
PolishedNewBarkTown_MapScripts::
	map_script MAP_SCRIPT_ON_LOAD, PolishedNewBarkTown_ResidentSchedule
	map_script MAP_SCRIPT_ON_RETURN_TO_FIELD, PolishedNewBarkTown_ResidentSchedule_Restore
	.byte 0

@ Map-local visibility, recomputed on entry and save loading.
PolishedNewBarkTown_ResidentSchedule::
	gettime
	goto_if_lt VAR_0x8000, 5, PolishedNewBarkTown_ResidentSchedule_Evening
	goto_if_ge VAR_0x8000, 17, PolishedNewBarkTown_ResidentSchedule_Evening
	clearflag FLAG_TEMP_1
	end
PolishedNewBarkTown_ResidentSchedule_Evening::
	setflag FLAG_TEMP_1
	end

@ Continue restores saved actors; reconcile only this scheduled resident with
@ the freshly computed mask. Ordinary menu returns retain the existing mask.
PolishedNewBarkTown_ResidentSchedule_Restore::
	goto_if_set FLAG_TEMP_1, PolishedNewBarkTown_ResidentSchedule_Hide
	addobject LOCALID_NEW_BARK_SCHOOLBOY
	end
PolishedNewBarkTown_ResidentSchedule_Hide::
	removeobject LOCALID_NEW_BARK_SCHOOLBOY
	end

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

PolishedNewBarkTown_Text_ElmDiscoveredNewMon::
	msgbox PolishedNewBarkTown_Text_ElmDiscoveredNewMon_Text, MSGBOX_NPC
	end

PolishedNewBarkTown_Text_ElmDiscoveredNewMon_Text:
	.string "Yo, {PLAYER}!\pI hear Prof.Elm\ndiscovered some\lnew POKéMON.$"

PolishedNewBarkTown_Text_GearIsImpressive::
	msgbox PolishedNewBarkTown_Text_GearIsImpressive_Text, MSGBOX_NPC
	end

PolishedNewBarkTown_Text_GearIsImpressive_Text:
	.string "Wow, your POKégear\nis impressive!\pDid your mom get\nit for you?$"
