@ Generated from pinned EarlsPokemonAcademy.asm: geography, static text and ambient residents.
PolishedEarlsPokemonAcademy_MapScripts::
	map_script MAP_SCRIPT_ON_LOAD, PolishedEarlsPokemonAcademy_Setup
	.byte 0

PolishedEarlsPokemonAcademy_Setup::
    setflag FLAG_TEMP_1
    goto_if_unset FLAG_POLISHED_EARL_IN_ACADEMY, PolishedEarlsPokemonAcademy_End
    clearflag FLAG_TEMP_1
PolishedEarlsPokemonAcademy_End::
    end

PolishedEarlsPokemonAcademy_AcademyEarl::
    lock
    call PolishedEarl_GiveChart
    applymovement LOCALID_ACADEMY_EARL, PolishedEarlsPokemonAcademy_Spin
    waitmovement LOCALID_ACADEMY_EARL
    faceplayer
    msgbox PolishedEarlsPokemonAcademy_AcademyEarlIntroText, MSGBOX_YESNO
    goto_if_eq VAR_RESULT, NO, PolishedEarlsPokemonAcademy_More
    msgbox PolishedEarlsPokemonAcademy_AcademyEarlTeachHowToWinText, MSGBOX_YESNO
    goto_if_eq VAR_RESULT, NO, PolishedEarlsPokemonAcademy_DoneTeaching
PolishedEarlsPokemonAcademy_More::
    msgbox PolishedEarlsPokemonAcademy_AcademyEarlTeachMoreText, MSGBOX_YESNO
    goto_if_eq VAR_RESULT, NO, PolishedEarlsPokemonAcademy_DoneTeaching
    msgbox PolishedEarlsPokemonAcademy_AcademyEarlTeachHowToRaiseWellText, MSGBOX_DEFAULT
    goto PolishedEarlsPokemonAcademy_Release
PolishedEarlsPokemonAcademy_DoneTeaching::
    msgbox PolishedEarlsPokemonAcademy_AcademyEarlNoMoreToTeachText, MSGBOX_DEFAULT
PolishedEarlsPokemonAcademy_Release::
    release
    end

PolishedEarlsPokemonAcademy_PokemonJournalWalkerScript::
    setflag FLAG_POLISHED_READ_WALKER_JOURNAL
    msgbox PolishedEarlsPokemonAcademy_JournalText, MSGBOX_SIGN
    end

PolishedEarlsPokemonAcademy_AcademyBlackboard::
    lockall
    msgbox PolishedEarlsPokemonAcademy_AcademyBlackboardText, MSGBOX_DEFAULT
PolishedEarlsPokemonAcademy_BoardMenu::
    multichoicegrid 0, 0, MULTI_POLISHED_BLACKBOARD, 2, FALSE
    switch VAR_RESULT
    case 0, PolishedEarlsPokemonAcademy_BoardPoison
    case 1, PolishedEarlsPokemonAcademy_BoardParalysis
    case 2, PolishedEarlsPokemonAcademy_BoardSleep
    case 3, PolishedEarlsPokemonAcademy_BoardBurn
    case 4, PolishedEarlsPokemonAcademy_BoardFreeze
    releaseall
    end

PolishedEarlsPokemonAcademy_BoardPoison::
    msgbox PolishedEarlsPokemonAcademy_AcademyPoisonText, MSGBOX_DEFAULT
    goto PolishedEarlsPokemonAcademy_BoardMenu
PolishedEarlsPokemonAcademy_BoardParalysis::
    msgbox PolishedEarlsPokemonAcademy_AcademyParalysisText, MSGBOX_DEFAULT
    goto PolishedEarlsPokemonAcademy_BoardMenu
PolishedEarlsPokemonAcademy_BoardSleep::
    msgbox PolishedEarlsPokemonAcademy_AcademySleepText, MSGBOX_DEFAULT
    goto PolishedEarlsPokemonAcademy_BoardMenu
PolishedEarlsPokemonAcademy_BoardBurn::
    msgbox PolishedEarlsPokemonAcademy_AcademyBurnText, MSGBOX_DEFAULT
    goto PolishedEarlsPokemonAcademy_BoardMenu
PolishedEarlsPokemonAcademy_BoardFreeze::
    msgbox PolishedEarlsPokemonAcademy_AcademyFreezeText, MSGBOX_DEFAULT
    goto PolishedEarlsPokemonAcademy_BoardMenu

PolishedEarlsPokemonAcademy_AcademyNotebook::
    lockall
    msgbox PolishedEarlsPokemonAcademy_AcademyNotebookText, MSGBOX_YESNO
    goto_if_eq VAR_RESULT, NO, PolishedEarlsPokemonAcademy_CloseBook
    msgbox PolishedEarlsPokemonAcademy_AcademyNotebookText1, MSGBOX_YESNO
    goto_if_eq VAR_RESULT, NO, PolishedEarlsPokemonAcademy_CloseBook
    msgbox PolishedEarlsPokemonAcademy_AcademyNotebookText2, MSGBOX_YESNO
    goto_if_eq VAR_RESULT, NO, PolishedEarlsPokemonAcademy_CloseBook
    msgbox PolishedEarlsPokemonAcademy_AcademyNotebookText3, MSGBOX_DEFAULT
PolishedEarlsPokemonAcademy_CloseBook::
    releaseall
    end

PolishedEarlsPokemonAcademy_EarlsPokemonAcademyYoungster1Text::
    msgbox PolishedEarlsPokemonAcademy_EarlsPokemonAcademyYoungster1Text_Text, MSGBOX_NPC
    end
PolishedEarlsPokemonAcademy_EarlsPokemonAcademyYoungster1Text_Text:
	.string "I'm taking notes\nof the teacher's\llecture.\pI'd better copy\nthe stuff on the\lblackboard too.$"


PolishedEarlsPokemonAcademy_EarlsPokemonAcademyGameboyKid1Script::
    lock
    faceplayer
    msgbox PolishedEarlsPokemonAcademy_EarlsPokemonAcademyGameboyKid1Text, MSGBOX_DEFAULT
    turnobject VAR_LAST_TALKED, DIR_SOUTH
    release
    end
PolishedEarlsPokemonAcademy_EarlsPokemonAcademyGameboyKid1Text:
	.string "I traded my best\nPOKéMON to the\lguy beside me.$"


PolishedEarlsPokemonAcademy_EarlsPokemonAcademyYoungster2Text::
    msgbox PolishedEarlsPokemonAcademy_EarlsPokemonAcademyYoungster2Text_Text, MSGBOX_NPC
    end
PolishedEarlsPokemonAcademy_EarlsPokemonAcademyYoungster2Text_Text:
	.string "A POKéMON holding\na Berry will heal\litself in battle.\pMany other items\ncan be held by\lPOKéMON…\pIt sure is tough\ntaking notes…$"


PolishedEarlsPokemonAcademy_EarlsPokemonAcademyGameboyKid2Script::
    lock
    faceplayer
    msgbox PolishedEarlsPokemonAcademy_EarlsPokemonAcademyGameboyKid2Text, MSGBOX_DEFAULT
    turnobject VAR_LAST_TALKED, DIR_WEST
    release
    end
PolishedEarlsPokemonAcademy_EarlsPokemonAcademyGameboyKid2Text:
	.string "Huh? The POKéMON I\njust got is hold-\ling something!$"

PolishedEarlsPokemonAcademy_Spin:
	face_down
	face_left
	face_up
	face_right
	face_down
	face_left
	face_up
	face_right
	face_down
	face_left
	face_up
	face_right
	face_down
	step_end

PolishedEarlsPokemonAcademy_AcademyEarlIntroText:
	.string "Earl, I am!\pWonderful are\nPOKéMON, yes!\pTeach you I will\nto be a better\ltrainer!\pWhat you want to\nknow? Want to be\la winner is you?$"

PolishedEarlsPokemonAcademy_AcademyEarlTeachHowToWinText:
	.string "Good! Teach you,\nI will!\pIn battle, POKéMON\ntop on list jump\lout first!\pChange order in\nlist, make battle\leasy, maybe!\pMore from me you\nwant to hear?$"

PolishedEarlsPokemonAcademy_AcademyEarlTeachMoreText:
	.string "So, want to know\nhow to raise\lPOKéMON well?$"

PolishedEarlsPokemonAcademy_AcademyEarlTeachHowToRaiseWellText:
	.string "Fine! Teach you,\nI will!\pIf POKéMON come\nout in battle even\pbriefly, some Exp.\nPoints it gets.\pAt top of list put\nweak POKéMON.\pSwitch in battle\nquick!\pThis way, weak\nPOKéMON strong\lbecome!$"

PolishedEarlsPokemonAcademy_AcademyEarlNoMoreToTeachText:
	.string "Oh! Smart student\nyou are! Nothing\lmore do I teach!\pGood to POKéMON\nyou must be!$"

PolishedEarlsPokemonAcademy_AcademyBlackboardText:
	.string "The blackboard\ndescribes POKéMON\pstatus changes in\nbattle.$"

PolishedEarlsPokemonAcademy_AcademyPoisonText:
	.string "If poisoned, a\nPOKéMON steadily\lloses HP.\pPoison lingers\nafter the battle,\pand HP is lost as\nyou walk.\pTo cure it, use an\nAntidote.$"

PolishedEarlsPokemonAcademy_AcademyParalysisText:
	.string "Paralysis reduces\nspeed and may\lprevent movement.\pIt remains after\nbattle, so use\la ParalyzeHeal.$"

PolishedEarlsPokemonAcademy_AcademySleepText:
	.string "If asleep, your\nPOKéMON can't make\la move.\pA sleeping POKéMON\ndoesn't wake up\lafter battle.\pWake it up with\nan Awakening.$"

PolishedEarlsPokemonAcademy_AcademyBurnText:
	.string "A burn steadily\nconsumes HP.\pIt also reduces\nattack power.\pA burn lingers\nafter battle.\pUse a Burn Heal as\nthe cure.$"

PolishedEarlsPokemonAcademy_AcademyFreezeText:
	.string "If your POKéMON is\nfrozen, it can't\ldo a thing.\pIt remains frozen\nafter battle.\pThaw it out with\nan Ice Heal.$"

PolishedEarlsPokemonAcademy_AcademyNotebookText:
	.string "It's this kid's\nnotebook…\pCatch POKéMON\nusing POKé Balls.\pUp to six can be\nin your party.\pKeep reading?$"

PolishedEarlsPokemonAcademy_AcademyNotebookText1:
	.string "Before throwing a\nPOKé Ball, weaken\lthe target first.\pA poisoned or\nburned POKéMON is\leasier to catch.\pKeep reading?$"

PolishedEarlsPokemonAcademy_AcademyNotebookText2:
	.string "Some moves may\ncause confusion.\pConfusion may make\na POKéMON attack\litself.\pLeaving battle\nclears up any\lconfusion.\pKeep reading?$"

PolishedEarlsPokemonAcademy_AcademyNotebookText3:
	.string "People who catch\nand use POKéMON\pin battle are\nPOKéMON trainers.\pThey are expected\nto visit POKéMON\pGyms and defeat\nother trainers.\pThe next page\nis… Blank!\pBoy: E-he-he…\pI haven't written\nany more…$"

PolishedEarlsPokemonAcademy_JournalText:
	.string "POKéMON Journal\pSpecial Feature:\nEx-Leader Walker!\pFalkner's father\nWalker is rumored\pto be wandering\naround Johto as\la vagabond.$"
