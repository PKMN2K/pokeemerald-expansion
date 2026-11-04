PolishedJohtoDemoOutdoor_MapScripts::
	.byte 0

PolishedJohtoDemoOutdoor_EventScript_Npc::
	msgbox PolishedJohtoDemoOutdoor_Text_Breeze, MSGBOX_NPC
	end

PolishedJohtoDemoOutdoor_EventScript_Sign::
	msgbox PolishedJohtoDemoOutdoor_Text_Sign, MSGBOX_SIGN
	end

@ Pinned NewBarkTown.asm, Text_RefreshingBreeze; reflowed for Emerald.
PolishedJohtoDemoOutdoor_Text_Breeze:
	.string "There's always such a refreshing\n"
	.string "breeze here.$"

PolishedJohtoDemoOutdoor_Text_Sign:
	.string "Polished Emerald\n"
	.string "Johto demonstration garden\p"
	.string "The laboratory is open.\n"
	.string "Enjoy a stroll by the pond!$"
