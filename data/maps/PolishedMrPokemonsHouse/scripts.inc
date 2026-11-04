@ Generated from pinned MrPokemonsHouse.asm: geography, static text and ambient residents.
PolishedMrPokemonsHouse_MapScripts::
	map_script MAP_SCRIPT_ON_LOAD, PolishedMrPokemonsHouse_Setup
	map_script MAP_SCRIPT_ON_RETURN_TO_FIELD, PolishedMrPokemonsHouse_Restore
	map_script MAP_SCRIPT_ON_FRAME_TABLE, PolishedMrPokemonsHouse_Frame
	.byte 0

PolishedMrPokemonsHouse_MrPokemonsHouse_ForeignMagazinesText::
	msgbox PolishedMrPokemonsHouse_MrPokemonsHouse_ForeignMagazinesText_Text, MSGBOX_SIGN
	end

PolishedMrPokemonsHouse_MrPokemonsHouse_ForeignMagazinesText_Text:
	.string "It's packed with\nforeign magazines.\pCan't even read\ntheir titles…$"

PolishedMrPokemonsHouse_MrPokemonsHouse_BrokenComputerText::
	msgbox PolishedMrPokemonsHouse_MrPokemonsHouse_BrokenComputerText_Text, MSGBOX_SIGN
	end

PolishedMrPokemonsHouse_MrPokemonsHouse_BrokenComputerText_Text:
	.string "It's a big com-\nputer. Hmm. It's\lbroken.$"

PolishedMrPokemonsHouse_MrPokemonsHouse_StrangeCoinsText::
	msgbox PolishedMrPokemonsHouse_MrPokemonsHouse_StrangeCoinsText_Text, MSGBOX_SIGN
	end

PolishedMrPokemonsHouse_MrPokemonsHouse_StrangeCoinsText_Text:
	.string "A whole pile of\nstrange coins!\pMaybe they're from\nanother country…$"

PolishedMrPokemonsHouse_Setup::
	clearflag FLAG_TEMP_2
	clearflag FLAG_TEMP_3
	goto_if_ne VAR_POLISHED_MR_POKEMON_SCENE, 1, PolishedMrPokemonsHouse_End
	setflag FLAG_TEMP_2
	setflag FLAG_TEMP_3
PolishedMrPokemonsHouse_End::
	end
PolishedMrPokemonsHouse_Restore::
	goto_if_ne VAR_POLISHED_MR_POKEMON_SCENE, 1, PolishedMrPokemonsHouse_End
	removeobject LOCALID_OAK
	removeobject LOCALID_POKEDEX
	end
PolishedMrPokemonsHouse_Frame::
	map_script_2 VAR_TEMP_1, 0, PolishedMrPokemonsHouse_CheckIntro
	.2byte 0
PolishedMrPokemonsHouse_CheckIntro::
	setvar VAR_TEMP_1, 1
	goto_if_ne VAR_POLISHED_MR_POKEMON_SCENE, 0, PolishedMrPokemonsHouse_End
	lockall
	applymovement LOCALID_MR_POKEMON, PolishedIntro_Exclamation
	waitmovement 0
	turnobject LOCALID_MR_POKEMON, DIR_SOUTH
	msgbox PolishedMrPokemonsHouse_MrPokemonIntroText1, MSGBOX_DEFAULT
	closemessage
	applymovement OBJ_EVENT_ID_PLAYER, PolishedMrPokemonsHouse_PlayerApproach
	waitmovement 0
	turnobject OBJ_EVENT_ID_PLAYER, DIR_NORTH
	msgbox PolishedMrPokemonsHouse_MrPokemonIntroText2, MSGBOX_DEFAULT
	giveitem ITEM_POLISHED_MYSTERY_EGG
	goto_if_eq VAR_RESULT, FALSE, PolishedMrPokemonsHouse_Full
	setrespawn HEAL_LOCATION_POLISHED_CHERRYGROVE
	msgbox PolishedMrPokemonsHouse_MrPokemonIntroText3, MSGBOX_DEFAULT
	turnobject LOCALID_MR_POKEMON, DIR_EAST
	msgbox PolishedMrPokemonsHouse_MrPokemonIntroText4, MSGBOX_DEFAULT
	turnobject LOCALID_MR_POKEMON, DIR_SOUTH
	turnobject LOCALID_OAK, DIR_WEST
	msgbox PolishedMrPokemonsHouse_MrPokemonIntroText5, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_OAK, PolishedMrPokemonsHouse_OakApproach
	waitmovement 0
	turnobject OBJ_EVENT_ID_PLAYER, DIR_EAST
	msgbox PolishedMrPokemonsHouse_MrPokemonsHouse_OakText1, MSGBOX_DEFAULT
	closemessage
	turnobject LOCALID_OAK, DIR_NORTH
	delay 10
	applymovement LOCALID_POKEDEX, PolishedMrPokemonsHouse_TakeDex
	waitmovement 0
	removeobject LOCALID_POKEDEX
	setflag FLAG_TEMP_3
	delay 10
	turnobject LOCALID_OAK, DIR_WEST
	delay 10
	setflag FLAG_SYS_POKEDEX_GET
	special EnableNationalPokedex
	playfanfare MUS_OBTAIN_ITEM
	msgbox PolishedMrPokemonsHouse_ReceivedDex, MSGBOX_DEFAULT
	waitfanfare
	msgbox PolishedMrPokemonsHouse_MrPokemonsHouse_OakText2, MSGBOX_DEFAULT
	closemessage
	turnobject OBJ_EVENT_ID_PLAYER, DIR_SOUTH
	applymovement LOCALID_OAK, PolishedMrPokemonsHouse_OakExit
	waitmovement 0
	playse SE_DOOR
	removeobject LOCALID_OAK
	setflag FLAG_TEMP_2
	delay 15
	turnobject OBJ_EVENT_ID_PLAYER, DIR_NORTH
	msgbox PolishedMrPokemonsHouse_MrPokemonsHouse_MrPokemonHealText, MSGBOX_DEFAULT
	closemessage
	fadescreen FADE_TO_BLACK
	playfanfare MUS_HEAL
	special HealPlayerParty
	waitfanfare
	fadescreen FADE_FROM_BLACK
	msgbox PolishedMrPokemonsHouse_MrPokemonText_ImDependingOnYou, MSGBOX_DEFAULT
	setflag FLAG_HIDE_POLISHED_NEW_BARK_RIVAL
	setvar VAR_POLISHED_MR_POKEMON_SCENE, 1
@ Stage 1 records the stolen-starter/Elm-call handoff for the return phase.
	setvar VAR_POLISHED_ERRAND_STATE, 1
	setvar VAR_POLISHED_CHERRYGROVE_SCENE, 1
	setvar VAR_POLISHED_ELMS_LAB_SCENE, 3
	releaseall
	end
PolishedMrPokemonsHouse_Full::
	msgbox PolishedMrPokemonsHouse_MakeRoom, MSGBOX_DEFAULT
	releaseall
	end
PolishedMrPokemonsHouse_Talk::
	goto_if_set FLAG_POLISHED_GAVE_MYSTERY_EGG, PolishedReturn_MrPokemonAfter
	lock
	faceplayer
	goto_if_eq VAR_POLISHED_MR_POKEMON_SCENE, 0, PolishedMrPokemonsHouse_FullTalk
	msgbox PolishedMrPokemonsHouse_MrPokemonText_ImDependingOnYou, MSGBOX_DEFAULT
	release
	end
PolishedMrPokemonsHouse_FullTalk::
	msgbox PolishedMrPokemonsHouse_MakeRoom, MSGBOX_DEFAULT
	release
	end
PolishedMrPokemonsHouse_Silent::
	end
PolishedMrPokemonsHouse_Cabinet::
	msgbox PolishedMrPokemonsHouse_MrPokemonsHouse_CabinetText, MSGBOX_SIGN
	end
PolishedMrPokemonsHouse_PlayerApproach:
@ The first-frame scene runs before the native doorway step.
	walk_right
	walk_up
	step_end
PolishedMrPokemonsHouse_TakeDex:
	lock_facing_direction
	walk_down
	unlock_facing_direction
	step_end
PolishedMrPokemonsHouse_OakExit:
	walk_down
	walk_left
	face_down
	delay_2
	step_end
PolishedMrPokemonsHouse_OakApproach:
	walk_down
	walk_left
	walk_left
	step_end

PolishedMrPokemonsHouse_MrPokemonIntroText1:
	.string "Hello, hello! You\nmust be {PLAYER}.\pProf.Elm said that\nyou would visit.$"

PolishedMrPokemonsHouse_MrPokemonIntroText2:
	.string "This is what I\nwant Prof.Elm to\lexamine.$"

PolishedMrPokemonsHouse_MrPokemonIntroText3:
	.string "I know a couple\nwho run a POKéMON\lDay-Care service.\pThey gave me that\nEgg.\pI was intrigued,\nso I sent mail to\lProf.Elm.\pFor POKéMON evolu-\ntion, Prof.Elm is\lthe authority.$"

PolishedMrPokemonsHouse_MrPokemonIntroText4:
	.string "Even Prof.Oak here\nrecognizes that.$"

PolishedMrPokemonsHouse_MrPokemonIntroText5:
	.string "If my assumption\nis correct, Prof.\lElm will know it.$"

PolishedMrPokemonsHouse_MrPokemonsHouse_OakText1:
	.string "Oak: Aha! So\nyou're {PLAYER}!\pI'm Oak! A POKéMON\nresearcher.\pI was just visit-\ning my old friend\lMr.POKéMON.\pI heard you were\nrunning an errand\pfor Prof.Elm, so I\nwaited here.\pOh! What's this?\nA rare POKéMON!\pLet's see…\pHm, I see!\pI understand why\nProf.Elm gave you\pa POKéMON for this\nerrand.\pTo researchers\nlike Prof.Elm and\pI, POKéMON are our\nfriends.\pHe saw that you\nwould treat your\pPOKéMON with love\nand care.\p…Ah!\pYou seem to be\ndependable.\pHow would you like\nto help me out?\pSee? This is the\nlatest version of\lPOKédex.\pIt automatically\nrecords data on\pPOKéMON you've\nseen or caught.\pIt's a hi-tech\nencyclopedia!$"

PolishedMrPokemonsHouse_MrPokemonsHouse_OakText2:
	.string "Go meet many kinds\nof POKéMON and\pcomplete that\nPOKédex!\pBut I've stayed\ntoo long.\pI have to get to\nGoldenrod for my\lusual radio show.\p{PLAYER}, I'm\ncounting on you!$"

PolishedMrPokemonsHouse_MrPokemonsHouse_MrPokemonHealText:
	.string "You are returning\nto Prof.Elm?\pHere. Your POKéMON\nshould have some\lrest.$"

PolishedMrPokemonsHouse_MrPokemonText_ImDependingOnYou:
	.string "I'm depending on\nyou!$"

PolishedMrPokemonsHouse_MrPokemonsHouse_CabinetText:
	.string "A collection of\nrare curiosities\lfrom all over!$"

PolishedMrPokemonsHouse_ReceivedDex:
	.string "{PLAYER} received the POKéDEX!$"

PolishedMrPokemonsHouse_MakeRoom:
	.string "Please make room for the Egg\nand visit me again.$"

PolishedReturn_MrPokemonAfter::
	msgbox PolishedReturn_MrPokemonAfterText, MSGBOX_NPC
	end
PolishedReturn_MrPokemonAfterText:
	.string "Life is delight-\nful! Always, new\pdiscoveries to be\nmade!$"
PolishedMrPokemonsHouse_TrashCan::
	msgbox PolishedMrPokemonsHouse_TrashCan_Text, MSGBOX_SIGN
	end

PolishedMrPokemonsHouse_TrashCan_Text:
	.string "There's nothing in\nhere…$"
