@ Generated from pinned ElmsLab.asm: geography, static text and ambient residents.
PolishedElmsLab_MapScripts::
	map_script MAP_SCRIPT_ON_LOAD, PolishedIntro_LabVisibility
	map_script MAP_SCRIPT_ON_TRANSITION, PolishedIntro_LabPositions
	map_script MAP_SCRIPT_ON_RETURN_TO_FIELD, PolishedIntro_LabRestore
	map_script MAP_SCRIPT_ON_FRAME_TABLE, PolishedIntro_LabFrame
	.byte 0

PolishedElmsLab_ElmsLabTravelTip1Text::
	msgbox PolishedElmsLab_ElmsLabTravelTip1Text_Text, MSGBOX_SIGN
	end

PolishedElmsLab_ElmsLabTravelTip1Text_Text:
	.string "{PLAYER} opened a\nbook.\pTravel Tip 1:\pPress Start to\nopen the Menu.$"

PolishedElmsLab_ElmsLabTravelTip2Text::
	msgbox PolishedElmsLab_ElmsLabTravelTip2Text_Text, MSGBOX_SIGN
	end

PolishedElmsLab_ElmsLabTravelTip2Text_Text:
	.string "{PLAYER} opened a\nbook.\pTravel Tip 2:\pRecord your trip\nwith Save!$"

PolishedElmsLab_ElmsLabTravelTip3Text::
	msgbox PolishedElmsLab_ElmsLabTravelTip3Text_Text, MSGBOX_SIGN
	end

PolishedElmsLab_ElmsLabTravelTip3Text_Text:
	.string "{PLAYER} opened a\nbook.\pTravel Tip 3:\pOpen your Bag and\npress Select to\lmove items.$"

PolishedElmsLab_ElmsLabTravelTip4Text::
	msgbox PolishedElmsLab_ElmsLabTravelTip4Text_Text, MSGBOX_SIGN
	end

PolishedElmsLab_ElmsLabTravelTip4Text_Text:
	.string "{PLAYER} opened a\nbook.\pTravel Tip 4:\pCheck your POKéMON\nmoves. Press the\pA Button to switch\nmoves.$"

PolishedElmsLab_ElmsLabTrashcanText::
	msgbox PolishedElmsLab_ElmsLabTrashcanText_Text, MSGBOX_SIGN
	end

PolishedElmsLab_ElmsLabTrashcanText_Text:
	.string "The wrapper from\nthe snack Prof.Elm\late is in there…$"

PolishedElmsLab_Heal::
	lockall
	goto_if_eq VAR_POLISHED_STARTER, 0, PolishedElmsLab_CannotHeal
	msgbox PolishedElmsLab_HealText, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, PolishedElmsLab_HealDone
	special HealPlayerParty
PolishedElmsLab_HealDone::
	releaseall
	end

PolishedElmsLab_CannotHeal::
	msgbox PolishedElmsLab_CannotHealText, MSGBOX_DEFAULT
	releaseall
	end

PolishedElmsLab_HealText:
	.string "Would you like to\nheal your POKéMON?$"

PolishedElmsLab_CannotHealText:
	.string "I wonder what this\ndoes?$"

@ Ball visibility derives from the saved choice; no extra permanent flags.
PolishedIntro_LabVisibility::
	call PolishedReturn_LabVisibility
	clearflag FLAG_TEMP_5
	call_if_eq VAR_POLISHED_ELMS_LAB_SCENE, 5, PolishedBattle_HideLyra
	call_if_eq VAR_POLISHED_ELMS_LAB_SCENE, 3, PolishedBattle_HideLyra
	call_if_eq VAR_POLISHED_ELMS_LAB_SCENE, 2, PolishedBattle_HideLyra
	clearflag FLAG_TEMP_2
	clearflag FLAG_TEMP_3
	clearflag FLAG_TEMP_4
	goto_if_eq VAR_POLISHED_MR_POKEMON_SCENE, 1, PolishedRoute30_AllBallsTaken
	goto_if_eq VAR_POLISHED_STARTER, 1, PolishedIntro_HideCyndaquilChikorita
	goto_if_eq VAR_POLISHED_STARTER, 2, PolishedIntro_HideTotodileCyndaquil
	goto_if_eq VAR_POLISHED_STARTER, 3, PolishedIntro_HideChikoritaTotodile
	end
PolishedRoute30_AllBallsTaken::
	setflag FLAG_TEMP_2
	setflag FLAG_TEMP_3
	setflag FLAG_TEMP_4
	end
PolishedIntro_HideCyndaquilChikorita::
	setflag FLAG_TEMP_2
	setflag FLAG_TEMP_4
	end
PolishedIntro_HideTotodileCyndaquil::
	setflag FLAG_TEMP_3
	setflag FLAG_TEMP_2
	end
PolishedIntro_HideChikoritaTotodile::
	setflag FLAG_TEMP_4
	setflag FLAG_TEMP_3
	end

@ Continue restores saved actors; reconcile them with the map-load masks.
@ Nickname/menu returns retain the current masks and choreography.
PolishedIntro_LabRestore::
	call_if_set FLAG_TEMP_6, PolishedReturn_RemoveOfficer
	call_if_set FLAG_TEMP_5, PolishedBattle_RemoveLyra
	call_if_set FLAG_TEMP_2, PolishedIntro_RemoveCyndaquil
	call_if_set FLAG_TEMP_3, PolishedIntro_RemoveTotodile
	call_if_set FLAG_TEMP_4, PolishedIntro_RemoveChikorita
	end
PolishedIntro_RemoveCyndaquil::
	removeobject LOCALID_ELMS_LAB_CYNDAQUIL
	return
PolishedIntro_RemoveTotodile::
	removeobject LOCALID_ELMS_LAB_TOTODILE
	return
PolishedIntro_RemoveChikorita::
	removeobject LOCALID_ELMS_LAB_CHIKORITA
	return

PolishedIntro_LabPositions::
	goto_if_eq VAR_POLISHED_ELMS_LAB_SCENE, 0, PolishedIntro_LabInitialPositions
	setobjectxyperm LOCALID_ELMS_LAB_ELM, 5, 2
	goto_if_eq VAR_POLISHED_ELMS_LAB_SCENE, 1, PolishedIntro_LabChoosingPositions
	setobjectxyperm LOCALID_ELMS_LAB_LYRA, 5, 5
	end
PolishedIntro_LabChoosingPositions::
	setobjectxyperm LOCALID_ELMS_LAB_LYRA, 4, 5
	end
PolishedIntro_LabInitialPositions::
	setobjectxyperm LOCALID_ELMS_LAB_ELM, 3, 4
	setobjectxyperm LOCALID_ELMS_LAB_LYRA, 5, 11
	end

PolishedIntro_LabFrame:
	map_script_2 VAR_POLISHED_ELMS_LAB_SCENE, 0, PolishedIntro_WalkToElm
	.2byte 0

PolishedIntro_WalkToElm::
	lockall
	showplayer
	applymovement LOCALID_PLAYER, PolishedIntro_WalkUpToElm
	applymovement LOCALID_ELMS_LAB_LYRA, PolishedIntro_LyraFollowElm
	waitmovement 0
	applymovement LOCALID_ELMS_LAB_ELM, PolishedIntro_Exclamation
	waitmovement 0
	turnobject LOCALID_ELMS_LAB_ELM, DIR_EAST
	msgbox PolishedIntro_ElmText_Intro, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, YES, PolishedIntro_ElmAccepted
PolishedIntro_ElmRefused::
	msgbox PolishedIntro_ElmText_Refused, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, PolishedIntro_ElmRefused
PolishedIntro_ElmAccepted::
	msgbox PolishedIntro_ElmText_Accepted, MSGBOX_DEFAULT
	msgbox PolishedIntro_ElmText_ResearchAmbitions, MSGBOX_DEFAULT
	closemessage
	playse SE_PC_LOGIN
	delay 30
	applymovement LOCALID_ELMS_LAB_ELM, PolishedIntro_Exclamation
	waitmovement 0
	turnobject LOCALID_ELMS_LAB_ELM, DIR_SOUTH
	msgbox PolishedIntro_ElmText_GotAnEmail, MSGBOX_DEFAULT
	turnobject LOCALID_ELMS_LAB_ELM, DIR_EAST
	msgbox PolishedIntro_ElmText_MissionFromMrPokemon, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_ELMS_LAB_ELM, PolishedIntro_StepUp
	waitmovement 0
	turnobject LOCALID_PLAYER, DIR_NORTH
	applymovement LOCALID_ELMS_LAB_ELM, PolishedIntro_ElmDefaultPosition
	waitmovement 0
	turnobject LOCALID_PLAYER, DIR_EAST
	msgbox PolishedIntro_ElmText_ChooseAPokemon, MSGBOX_DEFAULT
	setvar VAR_POLISHED_ELMS_LAB_SCENE, 1
	releaseall
	end

PolishedIntro_ProfElmScript::
	goto_if_eq VAR_POLISHED_MR_POKEMON_SCENE, 1, PolishedReturn_ElmTalk
	lock
	faceplayer
	goto_if_ne VAR_POLISHED_STARTER, 0, PolishedIntro_ElmAfterStarter
	msgbox PolishedIntro_ElmText_LetYourMonBattleIt, MSGBOX_DEFAULT
	release
	end
PolishedIntro_ElmAfterStarter::
	msgbox PolishedIntro_ElmDescribesMrPokemonText, MSGBOX_DEFAULT
	release
	end
PolishedIntro_ElmsLabLyraScript::
	lock
	faceplayer
	goto_if_ne VAR_POLISHED_STARTER, 0, PolishedIntro_LyraAfterStarter
	msgbox PolishedIntro_ElmsLabLyraWhichPokemonText, MSGBOX_DEFAULT
	release
	end
PolishedIntro_LyraAfterStarter::
	msgbox PolishedIntro_ElmsLabLyraGoodChoiceText, MSGBOX_DEFAULT
	release
	end
PolishedIntro_ElmsAideScript::
	goto_if_set FLAG_POLISHED_GAVE_MYSTERY_EGG, PolishedReturn_AideBusy
	goto_if_eq VAR_POLISHED_MR_POKEMON_SCENE, 1, PolishedReturn_Aide
PolishedReturn_AideBusy::
	msgbox PolishedIntro_AideText_AlwaysBusy, MSGBOX_NPC
	end

PolishedIntro_TryToLeave::
	lockall
	turnobject LOCALID_ELMS_LAB_ELM, DIR_SOUTH
	msgbox PolishedIntro_LabWhereGoingText, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_PLAYER, PolishedIntro_StepUp
	waitmovement 0
	releaseall
	end

PolishedIntro_DidNotChoose::
	msgbox PolishedIntro_DidntChooseStarterText, MSGBOX_DEFAULT
	releaseall
	end
PolishedIntro_RemainingBall::
	msgbox PolishedIntro_ElmPokeBallText, MSGBOX_DEFAULT
	releaseall
	end
PolishedIntro_NoRoom::
	msgbox gText_NoMoreRoomForPokemon, MSGBOX_DEFAULT
	releaseall
	end

PolishedIntro_NicknameParty::
	msgbox gText_NicknameThisPokemon, MSGBOX_YESNO
	call_if_eq VAR_RESULT, YES, PolishedIntro_NameParty
	return
PolishedIntro_NameParty::
	call Common_EventScript_GetGiftMonPartySlot
	call Common_EventScript_NameReceivedPartyMon
	lockall
	return
PolishedIntro_NicknameBox::
	msgbox gText_NicknameThisPokemon, MSGBOX_YESNO
	call_if_eq VAR_RESULT, YES, PolishedIntro_NameBox
	return
PolishedIntro_NameBox::
	call Common_EventScript_NameReceivedBoxMon
	lockall
	return

PolishedIntro_Directions::
	turnobject LOCALID_PLAYER, DIR_NORTH
	msgbox PolishedIntro_ElmDirectionsText1, MSGBOX_DEFAULT
	setflag FLAG_POLISHED_HAS_ELM_CONTACT
	playfanfare MUS_REGISTER_MATCH_CALL
	msgbox PolishedIntro_GotElmsNumberText, MSGBOX_DEFAULT
	waitfanfare
	turnobject LOCALID_ELMS_LAB_ELM, DIR_WEST
	msgbox PolishedIntro_ElmDirectionsText2, MSGBOX_DEFAULT
	turnobject LOCALID_ELMS_LAB_ELM, DIR_SOUTH
	msgbox PolishedIntro_ElmDirectionsText3, MSGBOX_DEFAULT
	setvar VAR_POLISHED_ELMS_LAB_SCENE, 6
	releaseall
	end

PolishedIntro_LyraFollowElm:
	walk_left
	walk_up
	walk_up
	walk_up
	walk_up
	walk_up
	walk_up
	face_up
	step_end

PolishedIntro_CyndaquilPokeBallScript::
	lockall
	goto_if_ne VAR_POLISHED_STARTER, 0, PolishedIntro_RemainingBall
	getobjectfacingdirection LOCALID_PLAYER, VAR_TEMP_6
	turnobject LOCALID_ELMS_LAB_ELM, DIR_SOUTH
	showmonpic SPECIES_CYNDAQUIL, 10, 3
	playmoncry SPECIES_CYNDAQUIL, CRY_MODE_NORMAL
	waitmoncry
	waitbuttonpress
	hidemonpic
	msgbox PolishedIntro_TakeCyndaquilText, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, PolishedIntro_DidNotChoose
	msgbox PolishedIntro_ChoseStarterText, MSGBOX_DEFAULT
	setvar VAR_TEMP_TRANSFERRED_SPECIES, SPECIES_CYNDAQUIL
	givemon SPECIES_CYNDAQUIL, 5, ITEM_ORAN_BERRY
	goto_if_eq VAR_RESULT, MON_CANT_GIVE, PolishedIntro_NoRoom
	copyvar VAR_TEMP_5, VAR_RESULT
	setvar VAR_POLISHED_STARTER, 1
	removeobject LOCALID_ELMS_LAB_CYNDAQUIL
	bufferspeciesname STR_VAR_1, SPECIES_CYNDAQUIL
	playfanfare MUS_OBTAIN_ITEM
	msgbox PolishedIntro_ReceivedGift, MSGBOX_DEFAULT
	waitfanfare
	call_if_eq VAR_TEMP_5, MON_GIVEN_TO_PARTY, PolishedIntro_NicknameParty
	call_if_eq VAR_TEMP_5, MON_GIVEN_TO_PC, PolishedIntro_NicknameBox
	call_if_eq VAR_TEMP_5, MON_GIVEN_TO_PC, Common_EventScript_TransferredToPC
	msgbox PolishedIntro_LyraChoosesStarterText, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_ELMS_LAB_LYRA, PolishedIntro_Cyndaquil_LyraPick
	waitmovement 0
	delay 15
	removeobject LOCALID_ELMS_LAB_CHIKORITA
	bufferspeciesname STR_VAR_3, SPECIES_CHIKORITA
	playfanfare MUS_OBTAIN_ITEM
	msgbox PolishedIntro_LyraReceivedStarterText, MSGBOX_DEFAULT
	waitfanfare
	msgbox PolishedIntro_LyraNicknamedChikoritaText, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_ELMS_LAB_LYRA, PolishedIntro_Cyndaquil_LyraAfter
	waitmovement 0
	goto_if_eq VAR_TEMP_6, DIR_EAST, PolishedIntro_Directions
	applymovement LOCALID_PLAYER, PolishedIntro_Cyndaquil_PlayerAfter
	waitmovement 0
	goto PolishedIntro_Directions

PolishedIntro_Cyndaquil_LyraPick:
	walk_right
	walk_right
	walk_right
	walk_right
	walk_up
	step_end

PolishedIntro_Cyndaquil_LyraAfter:
	walk_down
	walk_left
	walk_left
	walk_left
	face_up
	step_end

PolishedIntro_Cyndaquil_PlayerAfter:
	walk_left
	walk_up
	face_up
	step_end

PolishedIntro_TotodilePokeBallScript::
	lockall
	goto_if_ne VAR_POLISHED_STARTER, 0, PolishedIntro_RemainingBall
	getobjectfacingdirection LOCALID_PLAYER, VAR_TEMP_6
	turnobject LOCALID_ELMS_LAB_ELM, DIR_SOUTH
	showmonpic SPECIES_TOTODILE, 10, 3
	playmoncry SPECIES_TOTODILE, CRY_MODE_NORMAL
	waitmoncry
	waitbuttonpress
	hidemonpic
	msgbox PolishedIntro_TakeTotodileText, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, PolishedIntro_DidNotChoose
	msgbox PolishedIntro_ChoseStarterText, MSGBOX_DEFAULT
	setvar VAR_TEMP_TRANSFERRED_SPECIES, SPECIES_TOTODILE
	givemon SPECIES_TOTODILE, 5, ITEM_ORAN_BERRY
	goto_if_eq VAR_RESULT, MON_CANT_GIVE, PolishedIntro_NoRoom
	copyvar VAR_TEMP_5, VAR_RESULT
	setvar VAR_POLISHED_STARTER, 2
	removeobject LOCALID_ELMS_LAB_TOTODILE
	bufferspeciesname STR_VAR_1, SPECIES_TOTODILE
	playfanfare MUS_OBTAIN_ITEM
	msgbox PolishedIntro_ReceivedGift, MSGBOX_DEFAULT
	waitfanfare
	call_if_eq VAR_TEMP_5, MON_GIVEN_TO_PARTY, PolishedIntro_NicknameParty
	call_if_eq VAR_TEMP_5, MON_GIVEN_TO_PC, PolishedIntro_NicknameBox
	call_if_eq VAR_TEMP_5, MON_GIVEN_TO_PC, Common_EventScript_TransferredToPC
	msgbox PolishedIntro_LyraChoosesStarterText, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_ELMS_LAB_LYRA, PolishedIntro_Totodile_LyraPick
	waitmovement 0
	delay 15
	removeobject LOCALID_ELMS_LAB_CYNDAQUIL
	bufferspeciesname STR_VAR_3, SPECIES_CYNDAQUIL
	playfanfare MUS_OBTAIN_ITEM
	msgbox PolishedIntro_LyraReceivedStarterText, MSGBOX_DEFAULT
	waitfanfare
	msgbox PolishedIntro_LyraNicknamedCyndaquilText, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_ELMS_LAB_LYRA, PolishedIntro_Totodile_LyraAfter
	waitmovement 0
	applymovement LOCALID_PLAYER, PolishedIntro_Totodile_PlayerAfter
	waitmovement 0
	goto PolishedIntro_Directions

PolishedIntro_Totodile_LyraPick:
	walk_right
	walk_right
	walk_up
	step_end

PolishedIntro_Totodile_LyraAfter:
	walk_down
	walk_left
	face_up
	step_end

PolishedIntro_Totodile_PlayerAfter:
	walk_left
	walk_left
	walk_up
	face_up
	step_end

PolishedIntro_ChikoritaPokeBallScript::
	lockall
	goto_if_ne VAR_POLISHED_STARTER, 0, PolishedIntro_RemainingBall
	getobjectfacingdirection LOCALID_PLAYER, VAR_TEMP_6
	turnobject LOCALID_ELMS_LAB_ELM, DIR_SOUTH
	showmonpic SPECIES_CHIKORITA, 10, 3
	playmoncry SPECIES_CHIKORITA, CRY_MODE_NORMAL
	waitmoncry
	waitbuttonpress
	hidemonpic
	msgbox PolishedIntro_TakeChikoritaText, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, PolishedIntro_DidNotChoose
	msgbox PolishedIntro_ChoseStarterText, MSGBOX_DEFAULT
	setvar VAR_TEMP_TRANSFERRED_SPECIES, SPECIES_CHIKORITA
	givemon SPECIES_CHIKORITA, 5, ITEM_ORAN_BERRY
	goto_if_eq VAR_RESULT, MON_CANT_GIVE, PolishedIntro_NoRoom
	copyvar VAR_TEMP_5, VAR_RESULT
	setvar VAR_POLISHED_STARTER, 3
	removeobject LOCALID_ELMS_LAB_CHIKORITA
	bufferspeciesname STR_VAR_1, SPECIES_CHIKORITA
	playfanfare MUS_OBTAIN_ITEM
	msgbox PolishedIntro_ReceivedGift, MSGBOX_DEFAULT
	waitfanfare
	call_if_eq VAR_TEMP_5, MON_GIVEN_TO_PARTY, PolishedIntro_NicknameParty
	call_if_eq VAR_TEMP_5, MON_GIVEN_TO_PC, PolishedIntro_NicknameBox
	call_if_eq VAR_TEMP_5, MON_GIVEN_TO_PC, Common_EventScript_TransferredToPC
	msgbox PolishedIntro_LyraChoosesStarterText, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_ELMS_LAB_LYRA, PolishedIntro_Chikorita_LyraPick
	waitmovement 0
	delay 15
	removeobject LOCALID_ELMS_LAB_TOTODILE
	bufferspeciesname STR_VAR_3, SPECIES_TOTODILE
	playfanfare MUS_OBTAIN_ITEM
	msgbox PolishedIntro_LyraReceivedStarterText, MSGBOX_DEFAULT
	waitfanfare
	msgbox PolishedIntro_LyraNicknamedTotodileText, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_ELMS_LAB_LYRA, PolishedIntro_Chikorita_LyraAfter
	waitmovement 0
	applymovement LOCALID_PLAYER, PolishedIntro_Chikorita_PlayerAfter
	waitmovement 0
	goto PolishedIntro_Directions

PolishedIntro_Chikorita_LyraPick:
	walk_right
	walk_right
	walk_right
	walk_up
	step_end

PolishedIntro_Chikorita_LyraAfter:
	walk_down
	walk_left
	walk_left
	face_up
	step_end

PolishedIntro_Chikorita_PlayerAfter:
	walk_left
	walk_left
	walk_left
	walk_up
	face_up
	step_end

PolishedIntro_WalkUpToElm:
	walk_up
	walk_up
	walk_up
	walk_up
	walk_up
	walk_up
	walk_up
	face_left
	step_end

PolishedIntro_ElmDefaultPosition:
	walk_right
	walk_right
	walk_up
	face_down
	step_end

PolishedIntro_ElmText_Intro:
	.string "Elm: {PLAYER}!\nThere you are!\pHello to you too,\nLyra.\pI needed to ask\nyou both a favor.\pYou see…\pI'm writing a\npaper that I want\pto present at a\nconference.\pBut there are some\nthings I don't\pquite understand\nyet.\pSo!\pI'd like you both\nto raise POKéMON\pthat I recently\ncaught.$"

PolishedIntro_ElmText_Refused:
	.string "But… Please, I\nneed your help!$"

PolishedIntro_ElmText_Accepted:
	.string "Thanks, {PLAYER}!\pYou're a great\nhelp!$"

PolishedIntro_ElmText_ResearchAmbitions:
	.string "When I announce my\nfindings, I'm sure\pwe'll delve a bit\ndeeper into the\pmany mysteries of\nPOKéMON.\pYou can count on\nit!$"

PolishedIntro_ElmText_GotAnEmail:
	.string "Oh, hey! I got an\ne-mail!\p………………\nHm… Uh-huh…\pOK…$"

PolishedIntro_ElmText_MissionFromMrPokemon:
	.string "Hey, listen.\pI have an acquain-\ntance called Mr.\lPOKéMON.\pHe keeps finding\nweird things and\praving about his\ndiscoveries.\pAnyway, I just got\nan e-mail from him\psaying that this\ntime it's real.\pIt is intriguing,\nbut we're busy\pwith our POKéMON\nresearch…\pWait!\pI know!\p{PLAYER}, can you\ngo in our place?$"

PolishedIntro_ElmText_ChooseAPokemon:
	.string "I want you to\nraise one of the\pPOKéMON contained\nin these Balls.\pYou'll be that\nPOKéMON's first\lpartner, {PLAYER}!\pGo on. Pick one!$"

PolishedIntro_ElmText_LetYourMonBattleIt:
	.string "If a wild POKéMON\nappears, let your\lPOKéMON battle it!$"

PolishedIntro_ElmDescribesMrPokemonText:
	.string "Mr.POKéMON goes\neverywhere and\lfinds rarities.\pToo bad they're\njust rare and\lnot very useful…$"

PolishedIntro_ElmsLabLyraWhichPokemonText:
	.string "Which POKéMON are\nyou going to pick,\l{PLAYER}?$"

PolishedIntro_ElmsLabLyraGoodChoiceText:
	.string "Your POKéMON\nlooks cute too!$"

PolishedIntro_AideText_AlwaysBusy:
	.string "There are only two\nof us, so we're\lalways busy.$"

PolishedIntro_LabWhereGoingText:
	.string "Elm: Wait! Where\nare you going?$"

PolishedIntro_DidntChooseStarterText:
	.string "Elm: Think it over\ncarefully.\pYour partner is\nimportant.$"

PolishedIntro_ElmPokeBallText:
	.string "It contains a\nPOKéMON caught by\lProf.Elm.$"

PolishedIntro_ElmDirectionsText1:
	.string "Mr.POKéMON lives a\nlittle bit beyond\pCherrygrove, the\nnext city over.\pIt's almost a\ndirect route\pthere, so you\ncan't miss it.\pBut just in case,\nhere's my phone\pnumber. Call me if\nanything comes up!$"

PolishedIntro_ElmDirectionsText2:
	.string "If your POKéMON is\nhurt, you should\pheal it with this\nmachine.\pFeel free to use\nit anytime.$"

PolishedIntro_ElmDirectionsText3:
	.string "{PLAYER}, I'm\ncounting on you!$"

PolishedIntro_GotElmsNumberText:
	.string "{PLAYER} got Elm's\nphone number.$"

PolishedIntro_ChoseStarterText:
	.string "Elm: I think\nthat's a great\lPOKéMON too!$"

PolishedIntro_LyraChoosesStarterText:
	.string "Lyra: Then I'll\npick this one!$"

PolishedIntro_LyraReceivedStarterText:
	.string "Lyra received\n{STR_VAR_3}!$"

PolishedIntro_TakeCyndaquilText:
	.string "Elm: You'll take\nCyndaquil, the\lfire POKéMON?$"

PolishedIntro_TakeTotodileText:
	.string "Elm: Do you want\nTotodile, the\lwater POKéMON?$"

PolishedIntro_TakeChikoritaText:
	.string "Elm: So, you like\nChikorita, the\lgrass POKéMON?$"

PolishedIntro_LyraNicknamedCyndaquilText:
	.string "Lyra: It's so\ncute! I'll nick-\lname it Cinder!$"

PolishedIntro_LyraNicknamedTotodileText:
	.string "Lyra: It's so\ncute! I'll nick-\lname it Toto!$"

PolishedIntro_LyraNicknamedChikoritaText:
	.string "Lyra: It's so\ncute! I'll nick-\lname it Chicory!$"

PolishedIntro_ReceivedGift:
	.string "{PLAYER} received\n{STR_VAR_1}!$"

@ Scene 6: both battle outcomes continue in the lab, never white out.
PolishedBattle_Lyra::
	lockall
	turnobject LOCALID_ELMS_LAB_LYRA, DIR_SOUTH
	msgbox PolishedBattle_ElmsLabLyraChallengeText, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_ELMS_LAB_LYRA, PolishedBattle_Approach
	waitmovement 0
	turnobject LOCALID_PLAYER, DIR_EAST
	setvar VAR_LAST_TALKED, LOCALID_ELMS_LAB_LYRA
	goto_if_eq VAR_POLISHED_STARTER, 2, PolishedBattle_Cyndaquil
	goto_if_eq VAR_POLISHED_STARTER, 3, PolishedBattle_Totodile
	trainerbattle_earlyrival TRAINER_POLISHED_LYRA_CHIKORITA, RIVAL_BATTLE_HEAL_AFTER, PolishedBattle_ElmsLabLyraWinText, PolishedBattle_ElmsLabLyraLossText
	goto PolishedBattle_After
PolishedBattle_Cyndaquil::
	trainerbattle_earlyrival TRAINER_POLISHED_LYRA_CYNDAQUIL, RIVAL_BATTLE_HEAL_AFTER, PolishedBattle_ElmsLabLyraWinText, PolishedBattle_ElmsLabLyraLossText
	goto PolishedBattle_After
PolishedBattle_Totodile::
	trainerbattle_earlyrival TRAINER_POLISHED_LYRA_TOTODILE, RIVAL_BATTLE_HEAL_AFTER, PolishedBattle_ElmsLabLyraWinText, PolishedBattle_ElmsLabLyraLossText
PolishedBattle_After::
	lockall
	msgbox PolishedBattle_ElmsLabLyraText_YouLost, MSGBOX_DEFAULT
	turnobject LOCALID_ELMS_LAB_LYRA, DIR_NORTH
	msgbox PolishedBattle_ElmsLabLyraThankYouText, MSGBOX_DEFAULT
	turnobject LOCALID_ELMS_LAB_LYRA, DIR_WEST
	msgbox PolishedBattle_ElmsLabLyraSeeYouText, MSGBOX_DEFAULT
	closemessage
	turnobject LOCALID_PLAYER, DIR_SOUTH
	applymovement LOCALID_ELMS_LAB_LYRA, PolishedBattle_Leaves
	waitmovement 0
	removeobject LOCALID_ELMS_LAB_LYRA
	special HealPlayerParty
	setvar VAR_POLISHED_ELMS_LAB_SCENE, 5
	releaseall
	end

@ Scene 5: a single Potion, from either source approach. Do not commit the
@ gift if native Bag insertion fails; walking across the trigger can retry.
PolishedBattle_AideLeft::
	lockall
	applymovement LOCALID_ELMS_LAB_AIDE, PolishedBattle_AideRight1
	waitmovement 0
	call PolishedBattle_Potion
	applymovement LOCALID_ELMS_LAB_AIDE, PolishedBattle_AideLeft1
	waitmovement 0
	releaseall
	end
PolishedBattle_AideRight::
	lockall
	applymovement LOCALID_ELMS_LAB_AIDE, PolishedBattle_AideRight2
	waitmovement 0
	call PolishedBattle_Potion
	applymovement LOCALID_ELMS_LAB_AIDE, PolishedBattle_AideLeft2
	waitmovement 0
	releaseall
	end
PolishedBattle_Potion::
	turnobject LOCALID_PLAYER, DIR_SOUTH
	msgbox PolishedBattle_AideText_GiveYouPotions, MSGBOX_DEFAULT
	giveitem ITEM_POTION
	goto_if_eq VAR_RESULT, FALSE, PolishedBattle_PotionDone
	setvar VAR_POLISHED_ELMS_LAB_SCENE, 2
PolishedBattle_PotionDone::
	msgbox PolishedIntro_AideText_AlwaysBusy, MSGBOX_DEFAULT
	closemessage
	return

PolishedBattle_RemoveLyra::
	removeobject LOCALID_ELMS_LAB_LYRA
	return
PolishedBattle_HideLyra::
	setflag FLAG_TEMP_5
	return

PolishedBattle_Approach:
	walk_down
	face_left
	step_end

PolishedBattle_Leaves:
	walk_down
	walk_down
	walk_down
	walk_down
	walk_down
	step_end

PolishedBattle_AideRight1:
	walk_right
	walk_right
	face_up
	step_end

PolishedBattle_AideRight2:
	walk_right
	walk_right
	walk_right
	face_up
	step_end

PolishedBattle_AideLeft1:
	walk_left
	walk_left
	face_down
	step_end

PolishedBattle_AideLeft2:
	walk_left
	walk_left
	walk_left
	face_down
	step_end

PolishedBattle_ElmsLabLyraChallengeText:
	.string "Lyra: {PLAYER}!\nLet's get to know\pour POKéMON with\na battle!$"

PolishedBattle_ElmsLabLyraWinText:
	.string "Good job,\n{PLAYER}!$"

PolishedBattle_ElmsLabLyraLossText:
	.string "Yay! I won!$"

PolishedBattle_ElmsLabLyraText_YouLost:
	.string "That was an\nexciting battle!$"

PolishedBattle_ElmsLabLyraThankYouText:
	.string "Thank you for the\nPOKéMON, Prof.\lElm.$"

PolishedBattle_ElmsLabLyraSeeYouText:
	.string "{PLAYER}, I'll\nsee you later!\pHave fun on your\nerrand!$"

PolishedBattle_AideText_GiveYouPotions:
	.string "{PLAYER}, I want\nyou to have this\lfor your errand.$"

PolishedReturn_LabVisibility::
	setflag FLAG_TEMP_6
	goto_if_ne VAR_POLISHED_MR_POKEMON_SCENE, 1, PolishedReturn_LabVisibilityDone
	goto_if_set FLAG_POLISHED_ELM_THEFT_REPORTED, PolishedReturn_LabVisibilityDone
	clearflag FLAG_TEMP_6
PolishedReturn_LabVisibilityDone::
	return
PolishedReturn_RemoveOfficer::
	removeobject LOCALID_ELMS_LAB_OFFICER
	return
PolishedReturn_LabWindow::
	goto_if_eq VAR_POLISHED_ERRAND_STATE, 2, PolishedReturn_BrokenWindow
	msgbox PolishedReturn_ElmsLabWindowText1, MSGBOX_SIGN
	end
PolishedReturn_BrokenWindow::
	msgbox PolishedReturn_ElmsLabWindowText2, MSGBOX_SIGN
	end
PolishedReturn_CopRight::
	lockall
	applymovement OBJ_EVENT_ID_PLAYER, PolishedReturn_PlayerLeft
	waitmovement 0
	goto PolishedReturn_CopApproach
PolishedReturn_CopLeft::
	lockall
PolishedReturn_CopApproach::
	applymovement OBJ_EVENT_ID_PLAYER, PolishedReturn_WalkToCop
	waitmovement 0
PolishedReturn_Cop::
	lockall
	turnobject LOCALID_ELMS_LAB_OFFICER, DIR_WEST
	msgbox PolishedReturn_ElmsLabOfficerText1, MSGBOX_DEFAULT
	closemessage
	removeobject LOCALID_ELMS_LAB_LYRA
	setobjectxyperm LOCALID_ELMS_LAB_LYRA, 5, 8
	clearflag FLAG_TEMP_5
	addobject LOCALID_ELMS_LAB_LYRA
	applymovement LOCALID_ELMS_LAB_LYRA, PolishedReturn_LyraRunsIn
	waitmovement 0
	turnobject LOCALID_ELMS_LAB_OFFICER, DIR_SOUTH
	msgbox PolishedReturn_ElmsLabLyraTheftInnocentText, MSGBOX_DEFAULT
	delay 10
	turnobject LOCALID_ELMS_LAB_OFFICER, DIR_WEST
	msgbox PolishedReturn_ElmsLabOfficerText2, MSGBOX_DEFAULT
	closemessage
	fadescreen FADE_TO_BLACK
	special PolishedErrandNameRival
	waitstate
	special PolishedErrandBufferRivalName
	msgbox PolishedReturn_ElmsLabOfficerText3, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_ELMS_LAB_LYRA, PolishedReturn_LyraAside
	waitmovement 0
	applymovement LOCALID_ELMS_LAB_OFFICER, PolishedReturn_OfficerLeaves
	waitmovement 0
	removeobject LOCALID_ELMS_LAB_OFFICER
	setflag FLAG_TEMP_6
	delay 10
	turnobject LOCALID_ELMS_LAB_LYRA, DIR_NORTH
	turnobject OBJ_EVENT_ID_PLAYER, DIR_SOUTH
	msgbox PolishedReturn_ElmsLabLyraTheftGoodbyeText, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_ELMS_LAB_LYRA, PolishedReturn_LyraLeaves
	waitmovement 0
	removeobject LOCALID_ELMS_LAB_LYRA
	setflag FLAG_TEMP_5
	setflag FLAG_POLISHED_ELM_THEFT_REPORTED
	setvar VAR_POLISHED_ELMS_LAB_SCENE, 2
	delay 10
	applymovement OBJ_EVENT_ID_PLAYER, PolishedReturn_GiveEggApproach
	waitmovement 0
	goto PolishedReturn_GiveEgg

PolishedReturn_ElmTalk::
	lockall
	faceplayer
	goto_if_set FLAG_POLISHED_GAVE_MYSTERY_EGG, PolishedReturn_ElmStudying
PolishedReturn_GiveEgg::
	msgbox PolishedReturn_ElmAfterTheftText1, MSGBOX_DEFAULT
	checkitem ITEM_POLISHED_MYSTERY_EGG
	goto_if_eq VAR_RESULT, FALSE, PolishedReturn_ElmDone
	msgbox PolishedReturn_ElmAfterTheftText2, MSGBOX_DEFAULT
	removeitem ITEM_POLISHED_MYSTERY_EGG, 1
	closemessage
	getplayerxy VAR_0x8004, VAR_0x8005
	call PolishedReturn_ElmRecoil
	msgbox PolishedReturn_ElmAfterTheftText3, MSGBOX_DEFAULT
	closemessage
	call PolishedReturn_ElmRecover
	msgbox PolishedReturn_ElmAfterTheftText4, MSGBOX_DEFAULT
	msgbox PolishedReturn_ElmAfterTheftText5, MSGBOX_DEFAULT
	setflag FLAG_POLISHED_GAVE_MYSTERY_EGG
	msgbox PolishedReturn_ElmAfterTheftText6, MSGBOX_DEFAULT
PolishedReturn_ElmDone::
	releaseall
	end
PolishedReturn_ElmStudying::
    goto_if_set FLAG_POLISHED_EGG_GIVEN, PolishedEgg_ElmWaiting
    goto_if_set FLAG_POLISHED_ZEPHYR_BADGE, PolishedEgg_ElmAide
	msgbox PolishedReturn_ElmStudyingEggText, MSGBOX_DEFAULT
	releaseall
	end
PolishedReturn_Aide::
	msgbox PolishedReturn_AideText_TheftTestimony, MSGBOX_NPC
	end

PolishedReturn_ElmRecoil::
	goto_if_eq VAR_0x8004, 4, PolishedReturn_ElmRight
	goto_if_eq VAR_0x8004, 6, PolishedReturn_ElmLeft
	goto_if_eq VAR_0x8005, 1, PolishedReturn_ElmDown
	goto PolishedReturn_ElmUp
PolishedReturn_ElmRecover::
	goto_if_eq VAR_0x8004, 4, PolishedReturn_ElmLeft
	goto_if_eq VAR_0x8004, 6, PolishedReturn_ElmRight
	goto_if_eq VAR_0x8005, 1, PolishedReturn_ElmUp
	goto PolishedReturn_ElmDown
PolishedReturn_ElmUp::
	applymovement LOCALID_ELMS_LAB_ELM, PolishedReturn_ElmStepUp
	waitmovement 0
	return
PolishedReturn_ElmStepUp:
	lock_facing_direction
	walk_fast_up
	unlock_facing_direction
	step_end

PolishedReturn_ElmDown::
	applymovement LOCALID_ELMS_LAB_ELM, PolishedReturn_ElmStepDown
	waitmovement 0
	return
PolishedReturn_ElmStepDown:
	lock_facing_direction
	walk_fast_down
	unlock_facing_direction
	step_end

PolishedReturn_ElmLeft::
	applymovement LOCALID_ELMS_LAB_ELM, PolishedReturn_ElmStepLeft
	waitmovement 0
	return
PolishedReturn_ElmStepLeft:
	lock_facing_direction
	walk_fast_left
	unlock_facing_direction
	step_end

PolishedReturn_ElmRight::
	applymovement LOCALID_ELMS_LAB_ELM, PolishedReturn_ElmStepRight
	waitmovement 0
	return
PolishedReturn_ElmStepRight:
	lock_facing_direction
	walk_fast_right
	unlock_facing_direction
	step_end

PolishedReturn_PlayerLeft:
	walk_left
	step_end

PolishedReturn_WalkToCop:
	walk_up
	walk_up
	face_right
	step_end

PolishedReturn_LyraRunsIn:
	walk_up
	walk_up
	walk_up
	walk_up
	step_end

PolishedReturn_LyraAside:
	walk_left
	face_right
	step_end

PolishedReturn_OfficerLeaves:
	walk_down
	walk_down
	walk_down
	walk_down
	walk_down
	step_end

PolishedReturn_LyraLeaves:
	walk_down
	walk_down
	walk_down
	walk_down
	walk_down
	step_end

PolishedReturn_GiveEggApproach:
	walk_right
	face_up
	step_end

PolishedReturn_ElmsLabOfficerText1:
	.string "I heard a POKéMON\nwas stolen here.\pRule number one!\n“The criminal will\palways return to\nthe scene of the\lcrime…”\pOh my… So you're\nthe thief?$"

PolishedReturn_ElmsLabOfficerText2:
	.string "What?\pYou battled a\ntrainer like that?\pDid you happen to\nget his name?$"

PolishedReturn_ElmsLabOfficerText3:
	.string "OK! So {STR_VAR_1}\nwas his name.\pThanks for helping\nmy investigation!$"

PolishedReturn_ElmsLabLyraTheftInnocentText:
	.string "Lyra: Hold on!\n{PLAYER} has noth-\ling to do with it!\pI saw a red-haired\nboy spying on the\lbuilding!$"

PolishedReturn_ElmsLabLyraTheftGoodbyeText:
	.string "Lyra: {PLAYER},\nI'm glad he under-\pstood that you're\ninnocent.\pI hope he makes\nthe thief return\lthat POKéMON…\pWell then, see\nyou later!$"

PolishedReturn_ElmAfterTheftText1:
	.string "Elm: {PLAYER}, this\nis terrible…\pOh, yes, what was\nMr.POKéMON's big\ldiscovery?$"

PolishedReturn_ElmAfterTheftText2:
	.string "{PLAYER} handed\nthe Mystery Egg to\lProf.Elm.$"

PolishedReturn_ElmAfterTheftText3:
	.string "Elm: This?$"

PolishedReturn_ElmAfterTheftText4:
	.string "But… Is it a\nPOKéMON Egg?\pIf it is, it is a\ngreat discovery!$"

PolishedReturn_ElmAfterTheftText5:
	.string "Elm: What?!\pProf.Oak gave you\na POKédex?\p{PLAYER}, is that\ntrue? Th-that's\lincredible!\pHe is superb at\nseeing the poten-\ltial of people as\ltrainers.\pWow, {PLAYER}. You\nmay have what it\ptakes to become\nthe Champion.\pYou seem to be\ngetting on great\lwith POKéMON too.\pYou should take\nthe POKéMON Gym\lchallenge.\pThe closest Gym\nwould be the one\lin Violet City.$"

PolishedReturn_ElmAfterTheftText6:
	.string "…{PLAYER}. The\nroad to the\pchampionship will\nbe a long one.\pBefore you leave,\nmake sure that you\ltalk to your mom.\pAnd give me a call\nsometimes too.\pI can tell you all\nabout my research\pon how POKéMON\nevolve!$"

PolishedReturn_ElmStudyingEggText:
	.string "Elm: Don't give\nup! I'll call if\pI learn anything\nabout that Egg!$"

PolishedReturn_AideText_TheftTestimony:
	.string "There was a loud\nnoise outside…\pWhen we went to\nlook, someone\lstole a POKéMON.\pIt's unbelievable\nthat anyone would\ldo that!\p…sigh… That\nstolen POKéMON.\pI wonder how it's\ndoing.\pThey say a POKéMON\nraised by a bad\pperson turns bad\nitself.$"

PolishedReturn_ElmsLabWindowText1:
	.string "The window's open.\pA pleasant breeze\nis blowing in.$"

PolishedReturn_ElmsLabWindowText2:
	.string "He broke in\nthrough here!$"

PolishedEgg_ElmWaiting::
    msgbox PolishedEgg_ElmWaitingText, MSGBOX_DEFAULT
    releaseall
    end
PolishedEgg_ElmWaitingText:
	.string "Elm: Hey, has that\nEgg changed any?$"

PolishedEgg_ElmAide::
    msgbox PolishedEgg_ElmAideText, MSGBOX_DEFAULT
    releaseall
    end
PolishedEgg_ElmAideText:
	.string "Elm: {PLAYER}?\nDidn't you meet my\lassistant?\pHe should have met\nyou with the Egg\pat Violet City's\nPOKéMON Center.\pYou must have just\nmissed him. Try to\lcatch him there.$"
