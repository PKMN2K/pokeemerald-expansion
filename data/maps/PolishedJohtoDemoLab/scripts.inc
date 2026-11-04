PolishedJohtoDemoLab_MapScripts::
	.byte 0

PolishedJohtoDemoLab_EventScript_Npc::
	lock
	faceplayer
	msgbox PolishedJohtoDemoLab_Text_Welcome, MSGBOX_DEFAULT
	special HealPlayerParty
	release
	end

PolishedJohtoDemoLab_Text_Welcome:
	.string "Welcome! Take a moment to rest\n"
	.string "with your POKéMON.$"
