PolishedJohtoDemoOutdoor_MapScripts::
	.byte 0

PolishedJohtoDemoOutdoor_EventScript_Npc::
	msgbox PolishedJohtoDemoOutdoor_Text_Breeze, MSGBOX_NPC
	end

PolishedJohtoDemoOutdoor_EventScript_Sign::
	msgbox PolishedJohtoDemoOutdoor_Text_Sign, MSGBOX_SIGN
	end

@ Demo-only travel to the original town; no source scene flags are consumed.
PolishedJohtoDemoOutdoor_EventScript_Travel::
	lock
	faceplayer
	msgbox PolishedJohtoDemoOutdoor_Text_Travel, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, PolishedJohtoDemoOutdoor_EventScript_Stay
	closemessage
	warp MAP_POLISHED_NEW_BARK_TOWN, 8, 10
	waitstate
PolishedJohtoDemoOutdoor_EventScript_Stay::
	release
	end

PolishedJohtoDemoOutdoor_Text_Travel:
	.string "Visit NEW BARK TOWN?$"

@ Pinned NewBarkTown.asm, Text_RefreshingBreeze; reflowed for Emerald.
PolishedJohtoDemoOutdoor_Text_Breeze:
	.string "There's always such a refreshing\n"
	.string "breeze here.$"

PolishedJohtoDemoOutdoor_Text_Sign:
	.string "Polished Emerald\n"
	.string "Johto demonstration garden\p"
	.string "The laboratory is open.\n"
	.string "Enjoy a stroll by the pond!$"
