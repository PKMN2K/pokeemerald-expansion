@ Generated from pinned Route31VioletGate.asm: geography, static text and ambient residents.
PolishedRoute31VioletGate_MapScripts::
	map_script MAP_SCRIPT_ON_LOAD, PolishedRoute31VioletGate_OpenWest
	.byte 0

PolishedRoute31VioletGate_Route31VioletGateOfficerText::
	msgbox PolishedRoute31VioletGate_Route31VioletGateOfficerText_Text, MSGBOX_NPC
	end

PolishedRoute31VioletGate_Route31VioletGateOfficerText_Text:
	.string "Hi there!\nDid you visit\lSprout Tower?$"

PolishedRoute31VioletGate_Route31VioletGateCooltrainerFText::
	msgbox PolishedRoute31VioletGate_Route31VioletGateCooltrainerFText_Text, MSGBOX_NPC
	end

PolishedRoute31VioletGate_Route31VioletGateCooltrainerFText_Text:
	.string "I came too far\nout. I'd better\lphone home!$"

PolishedRoute31VioletGate_OpenWest::
	setmetatile 0, 4, 140, FALSE
	setmetatile 0, 5, 142, FALSE
	end
