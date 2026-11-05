@ Generated from pinned VioletPokeCenter1F.asm: geography, static text and ambient residents.
PolishedVioletPokeCenter1F_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, PolishedVioletPokeCenter1F_Respawn
	map_script MAP_SCRIPT_ON_LOAD, PolishedEgg_Visibility
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

PolishedEgg_Visibility::
    setflag FLAG_TEMP_3
    goto_if_unset FLAG_POLISHED_EGG_CALL, PolishedEgg_End
    goto_if_set FLAG_POLISHED_EGG_GIVEN, PolishedEgg_End
    clearflag FLAG_TEMP_3
PolishedEgg_End::
    end
PolishedEgg_Aide::
    lockall
    faceplayer
    goto_if_set FLAG_POLISHED_EGG_GIVEN, PolishedEgg_Done
    goto_if_set FLAG_POLISHED_EGG_REFUSED, PolishedEgg_AskAgain
    msgbox PolishedEgg_Intro, MSGBOX_YESNO
    goto PolishedEgg_Answer
PolishedEgg_AskAgain::
    msgbox PolishedEgg_Question, MSGBOX_YESNO
PolishedEgg_Answer::
    goto_if_eq VAR_RESULT, NO, PolishedEgg_Refuse
    specialvar VAR_RESULT, PolishedFalknerGiveEgg
    goto_if_eq VAR_RESULT, MON_CANT_GIVE, PolishedEgg_Full
    goto_if_eq VAR_RESULT, MON_GIVEN_TO_PC, PolishedEgg_Box
    msgbox PolishedEgg_Received, MSGBOX_DEFAULT
    goto PolishedEgg_Goodbye
PolishedEgg_Box::
    msgbox PolishedEgg_BoxText, MSGBOX_DEFAULT
PolishedEgg_Goodbye::
    msgbox PolishedEgg_GoodbyeText, MSGBOX_DEFAULT
    closemessage
    getplayerxy VAR_0x8004, VAR_0x8005
    goto_if_eq VAR_0x8005, 3, PolishedEgg_Around
    applymovement LOCALID_VIOLET_EGG_AIDE, PolishedEgg_Straight
    goto PolishedEgg_Leave
PolishedEgg_Around::
    applymovement LOCALID_VIOLET_EGG_AIDE, PolishedEgg_WalkAround
PolishedEgg_Leave::
    waitmovement 0
    removeobject LOCALID_VIOLET_EGG_AIDE
    setflag FLAG_TEMP_3
    goto PolishedEgg_Done
PolishedEgg_Refuse::
    setflag FLAG_POLISHED_EGG_REFUSED
    msgbox PolishedEgg_RefusalText, MSGBOX_DEFAULT
    goto PolishedEgg_Done
PolishedEgg_Full::
    msgbox PolishedEgg_FullText, MSGBOX_DEFAULT
PolishedEgg_Done::
    releaseall
    end
PolishedEgg_Straight:
    walk_down
    walk_left
    walk_left
    walk_left
    walk_left
    walk_down
    walk_down
    walk_down
    walk_down
    step_end
PolishedEgg_WalkAround:
    walk_left
    walk_left
    walk_down
    walk_left
    walk_left
    walk_down
    walk_down
    walk_down
    walk_down
    step_end
PolishedEgg_Intro:
	.string "{PLAYER}, long\ntime, no see.\pI was browsing the\nPOKéMON Journal\lwhile I waited.\pIts gossip is a\ngood pastime…\pAnyway, Prof.Elm\nhas another favor\lto ask you.\pWould you take the\nPOKéMON Egg?$"

PolishedEgg_Question:
	.string "{PLAYER}, will you\ntake the Egg?$"

PolishedEgg_FullText:
	.string "Oh, no. You can't\ncarry any more\lPOKéMON with you.\pYou have no space\nin your Box, too.\pI'll wait here\nwhile you make\lroom for the Egg.$"

PolishedEgg_GoodbyeText:
	.string "We discovered that\na POKéMON will not\phatch until it\ngrows in the Egg.\pIt also has to be\nwith other active\lPOKéMON to hatch.\p{PLAYER}, you're\nthe only person\lwe can rely on.\pPlease call Prof.\nElm when that Egg\lhatches!$"

PolishedEgg_RefusalText:
	.string "B-but… Prof.Elm\nasked for you…$"

PolishedEgg_Received:
	.string "{PLAYER} received the\nPOKéMON Egg!$"

PolishedEgg_BoxText:
	.string "{PLAYER} received the Egg!\pYour party is full, so the\nEgg was sent to your PC.$"
