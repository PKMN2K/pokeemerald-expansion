@ Generated from pinned VioletPokeCenter1F.asm: geography, static text and ambient residents.
PolishedVioletPokeCenter1F_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, PolishedVioletPokeCenter1F_Respawn
	.byte 0

PolishedVioletPokeCenter1F_VioletPokeCenter1FGameboyKidText::
	msgbox PolishedVioletPokeCenter1F_VioletPokeCenter1FGameboyKidText_Text, MSGBOX_NPC
	end

PolishedVioletPokeCenter1F_VioletPokeCenter1FGameboyKidText_Text:
	.string "A guy named Bill\nmade the POKéMON\lPC storage system.$"

PolishedVioletPokeCenter1F_VioletPokeCenter1FGentlemanText::
	msgbox PolishedVioletPokeCenter1F_VioletPokeCenter1FGentlemanText_Text, MSGBOX_NPC
	end

PolishedVioletPokeCenter1F_VioletPokeCenter1FGentlemanText_Text:
	.string "It was around\nthree years ago.\pTeam Rocket was up\nto no good with\lPOKéMON.\pBut justice pre-\nvailed--a young\lkid broke 'em up.$"

PolishedVioletPokeCenter1F_VioletPokeCenter1FSageText::
	msgbox PolishedVioletPokeCenter1F_VioletPokeCenter1FSageText_Text, MSGBOX_NPC
	end

PolishedVioletPokeCenter1F_VioletPokeCenter1FSageText_Text:
	.string "Some Bellsprout\nare found with\lGold Leaves.\pAnd some Oddish\nare found with\lSilver Leaves.\pThese are valued\nby a certain type\lof person.$"

PolishedVioletPokeCenter1F_VioletPokeCenter1FYoungsterText::
	msgbox PolishedVioletPokeCenter1F_VioletPokeCenter1FYoungsterText_Text, MSGBOX_NPC
	end

PolishedVioletPokeCenter1F_VioletPokeCenter1FYoungsterText_Text:
	.string "POKéMON are smart.\nThey won't obey a\ptrainer they don't\nrespect.\pWithout the right\nGym Badges, they\pwill just do as\nthey please.$"

PolishedVioletPokeCenter1F_Respawn::
	setrespawn HEAL_LOCATION_POLISHED_VIOLET
	end

PolishedVioletPokeCenter1F_Nurse::
	lockall
	gettime
	goto_if_lt VAR_0x8000, 5, PolishedVioletPokeCenter1F_Nite
	goto_if_lt VAR_0x8000, 9, PolishedVioletPokeCenter1F_Morn
	goto_if_lt VAR_0x8000, 17, PolishedVioletPokeCenter1F_Day
	goto_if_lt VAR_0x8000, 21, PolishedVioletPokeCenter1F_Eve
PolishedVioletPokeCenter1F_Nite::
	msgbox PolishedVioletPokeCenter1F_NurseNiteText, MSGBOX_DEFAULT
	goto PolishedVioletPokeCenter1F_Ask
PolishedVioletPokeCenter1F_Morn::
	msgbox PolishedVioletPokeCenter1F_NurseMornText, MSGBOX_DEFAULT
	goto PolishedVioletPokeCenter1F_Ask
PolishedVioletPokeCenter1F_Day::
	msgbox PolishedVioletPokeCenter1F_NurseDayText, MSGBOX_DEFAULT
	goto PolishedVioletPokeCenter1F_Ask
PolishedVioletPokeCenter1F_Eve::
	msgbox PolishedVioletPokeCenter1F_NurseEveText, MSGBOX_DEFAULT
PolishedVioletPokeCenter1F_Ask::
	msgbox PolishedVioletPokeCenter1F_NurseAskHealText, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, PolishedVioletPokeCenter1F_Goodbye
	msgbox PolishedVioletPokeCenter1F_NurseTakePokemonText, MSGBOX_DEFAULT
	closemessage
	turnobject LOCALID_VIOLET_NURSE, DIR_WEST
	playfanfare MUS_HEAL
	waitfanfare
	special HealPlayerParty
	turnobject LOCALID_VIOLET_NURSE, DIR_SOUTH
	msgbox PolishedVioletPokeCenter1F_NurseReturnPokemonText, MSGBOX_DEFAULT
PolishedVioletPokeCenter1F_Goodbye::
	turnobject LOCALID_VIOLET_NURSE, DIR_NORTH
	delay 10
	turnobject LOCALID_VIOLET_NURSE, DIR_SOUTH
	msgbox PolishedVioletPokeCenter1F_NurseGoodbyeText, MSGBOX_DEFAULT
	releaseall
	end

@ Native storage/Bag PC menus, without Emerald metatile substitutions.
PolishedVioletPokeCenter1F_PC::
	lockall
	playse SE_PC_ON
	msgbox Text_BootUpPC, MSGBOX_DEFAULT
	goto EventScript_PCMainMenu

PolishedVioletPokeCenter1F_Journal::
	setflag FLAG_POLISHED_READ_FALKNER_JOURNAL
	msgbox PolishedVioletPokeCenter1F_JournalText, MSGBOX_SIGN
	end

PolishedVioletPokeCenter1F_NurseMornText:
	.string "Good morning!\nWelcome to our\lPOKéMON Center.$"

PolishedVioletPokeCenter1F_NurseDayText:
	.string "Hello!\nWelcome to our\lPOKéMON Center.$"

PolishedVioletPokeCenter1F_NurseEveText:
	.string "Good evening!\nWelcome to our\lPOKéMON Center.$"

PolishedVioletPokeCenter1F_NurseNiteText:
	.string "Good evening!\nYou're out late.\pWelcome to our\nPOKéMON Center.$"

PolishedVioletPokeCenter1F_NurseAskHealText:
	.string "We can heal your\nPOKéMON to perfect\lhealth.\pShall we heal your\nPOKéMON?$"

PolishedVioletPokeCenter1F_NurseTakePokemonText:
	.string "OK, may I see your\nPOKéMON?$"

PolishedVioletPokeCenter1F_NurseReturnPokemonText:
	.string "Thank you for\nwaiting.\pYour POKéMON are\nfully healed.$"

PolishedVioletPokeCenter1F_NurseGoodbyeText:
	.string "We hope to see you\nagain.$"

PolishedVioletPokeCenter1F_JournalText:
	.string "POKéMON Journal\pSpecial Feature:\nLeader Falkner!\pPeople say that\nFalkner reveres\lhis father, who\pled the Violet Gym\nbefore him.$"
