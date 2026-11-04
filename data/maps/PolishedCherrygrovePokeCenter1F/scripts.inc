@ Generated from pinned CherrygrovePokeCenter1F.asm: geography, static text and ambient residents.
PolishedCherrygrovePokeCenter1F_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, PolishedCherrygrovePokeCenter1F_Respawn
	.byte 0

PolishedCherrygrovePokeCenter1F_CherrygrovePokeCenter1FTeacherScript::
	msgbox PolishedCherrygrovePokeCenter1F_CherrygrovePokeCenter1FTeacherScript_Text, MSGBOX_NPC
	end

PolishedCherrygrovePokeCenter1F_CherrygrovePokeCenter1FTeacherScript_Text:
	.string "The Communication\nCenter upstairs\lwas just built.\pBut they're still\nfinishing it up.$"

PolishedCherrygrovePokeCenter1F_CherrygrovePokeCenter1FFisherText::
	msgbox PolishedCherrygrovePokeCenter1F_CherrygrovePokeCenter1FFisherText_Text, MSGBOX_NPC
	end

PolishedCherrygrovePokeCenter1F_CherrygrovePokeCenter1FFisherText_Text:
	.string "It's great. I can\nstore any number\pof POKéMON, and\nit's all free.$"

PolishedCherrygrovePokeCenter1F_CherrygrovePokeCenter1FGentlemanText::
	msgbox PolishedCherrygrovePokeCenter1F_CherrygrovePokeCenter1FGentlemanText_Text, MSGBOX_NPC
	end

PolishedCherrygrovePokeCenter1F_CherrygrovePokeCenter1FGentlemanText_Text:
	.string "That PC is free\nfor any trainer\lto use.$"

PolishedCherrygrovePokeCenter1F_CherrygrovePokeCenter1FLadyText::
	msgbox PolishedCherrygrovePokeCenter1F_CherrygrovePokeCenter1FLadyText_Text, MSGBOX_NPC
	end

PolishedCherrygrovePokeCenter1F_CherrygrovePokeCenter1FLadyText_Text:
	.string "POKéMON Journal\nhas such fascin-\lating stories!\pI come to POKéMON\nCenters for the\platest issues, but\nthey're not the\ponly place with\na subscription.$"

PolishedCherrygrovePokeCenter1F_Respawn::
	setrespawn HEAL_LOCATION_POLISHED_CHERRYGROVE
	end

PolishedCherrygrovePokeCenter1F_Nurse::
	lockall
	gettime
	goto_if_lt VAR_0x8000, 5, PolishedCherrygrovePokeCenter1F_Nite
	goto_if_lt VAR_0x8000, 9, PolishedCherrygrovePokeCenter1F_Morn
	goto_if_lt VAR_0x8000, 17, PolishedCherrygrovePokeCenter1F_Day
	goto_if_lt VAR_0x8000, 21, PolishedCherrygrovePokeCenter1F_Eve
PolishedCherrygrovePokeCenter1F_Nite::
	msgbox PolishedCherrygrovePokeCenter1F_NurseNiteText, MSGBOX_DEFAULT
	goto PolishedCherrygrovePokeCenter1F_Ask
PolishedCherrygrovePokeCenter1F_Morn::
	msgbox PolishedCherrygrovePokeCenter1F_NurseMornText, MSGBOX_DEFAULT
	goto PolishedCherrygrovePokeCenter1F_Ask
PolishedCherrygrovePokeCenter1F_Day::
	msgbox PolishedCherrygrovePokeCenter1F_NurseDayText, MSGBOX_DEFAULT
	goto PolishedCherrygrovePokeCenter1F_Ask
PolishedCherrygrovePokeCenter1F_Eve::
	msgbox PolishedCherrygrovePokeCenter1F_NurseEveText, MSGBOX_DEFAULT
PolishedCherrygrovePokeCenter1F_Ask::
	msgbox PolishedCherrygrovePokeCenter1F_NurseAskHealText, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, PolishedCherrygrovePokeCenter1F_Goodbye
	msgbox PolishedCherrygrovePokeCenter1F_NurseTakePokemonText, MSGBOX_DEFAULT
	closemessage
	turnobject LOCALID_CHERRYGROVE_NURSE, DIR_WEST
	playfanfare MUS_HEAL
	waitfanfare
	special HealPlayerParty
	turnobject LOCALID_CHERRYGROVE_NURSE, DIR_SOUTH
	msgbox PolishedCherrygrovePokeCenter1F_NurseReturnPokemonText, MSGBOX_DEFAULT
PolishedCherrygrovePokeCenter1F_Goodbye::
	turnobject LOCALID_CHERRYGROVE_NURSE, DIR_NORTH
	delay 10
	turnobject LOCALID_CHERRYGROVE_NURSE, DIR_SOUTH
	msgbox PolishedCherrygrovePokeCenter1F_NurseGoodbyeText, MSGBOX_DEFAULT
	releaseall
	end

@ Native storage/Bag PC menus, without Emerald metatile substitutions.
PolishedCherrygrovePokeCenter1F_PC::
	lockall
	playse SE_PC_ON
	msgbox Text_BootUpPC, MSGBOX_DEFAULT
	goto EventScript_PCMainMenu

PolishedCherrygrovePokeCenter1F_Journal::
	setflag FLAG_POLISHED_READ_RED_JOURNAL
	msgbox PolishedCherrygrovePokeCenter1F_JournalText, MSGBOX_SIGN
	end

PolishedCherrygrovePokeCenter1F_NurseMornText:
	.string "Good morning!\nWelcome to our\lPOKéMON Center.$"

PolishedCherrygrovePokeCenter1F_NurseDayText:
	.string "Hello!\nWelcome to our\lPOKéMON Center.$"

PolishedCherrygrovePokeCenter1F_NurseEveText:
	.string "Good evening!\nWelcome to our\lPOKéMON Center.$"

PolishedCherrygrovePokeCenter1F_NurseNiteText:
	.string "Good evening!\nYou're out late.\pWelcome to our\nPOKéMON Center.$"

PolishedCherrygrovePokeCenter1F_NurseAskHealText:
	.string "We can heal your\nPOKéMON to perfect\lhealth.\pShall we heal your\nPOKéMON?$"

PolishedCherrygrovePokeCenter1F_NurseTakePokemonText:
	.string "OK, may I see your\nPOKéMON?$"

PolishedCherrygrovePokeCenter1F_NurseReturnPokemonText:
	.string "Thank you for\nwaiting.\pYour POKéMON are\nfully healed.$"

PolishedCherrygrovePokeCenter1F_NurseGoodbyeText:
	.string "We hope to see you\nagain.$"

PolishedCherrygrovePokeCenter1F_JournalText:
	.string "POKéMON Journal\pSpecial Feature:\nPOKéMON Trainer Red!\pRed is said to\nhave defeated his\prival Blue for the\ntitle of POKéMON\pLeague Champion in\nrecord time.$"
