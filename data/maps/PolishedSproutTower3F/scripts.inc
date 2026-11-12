@ Generated from pinned SproutTower3F.asm: geography, static text and ambient residents.
PolishedSproutTower3F_MapScripts::
	map_script MAP_SCRIPT_ON_LOAD, PolishedSproutTower3F_Setup
	.byte 0

PolishedSproutTower3F_SproutTower3FStatueText::
	msgbox PolishedSproutTower3F_SproutTower3FStatueText_Text, MSGBOX_SIGN
	end

PolishedSproutTower3F_SproutTower3FStatueText_Text:
	.string "A POKéMON statue…\pIt looks very\ndistinguished.$"

PolishedSproutTower3F_SproutTower3FPaintingText::
	msgbox PolishedSproutTower3F_SproutTower3FPaintingText_Text, MSGBOX_SIGN
	end

PolishedSproutTower3F_SproutTower3FPaintingText_Text:
	.string "It's a powerful\npainting of a\lBellsprout.$"

PolishedSproutTower3F_ObjectEvent::
	end
PolishedSproutTower3F_GenericTrainerSageJin::
	trainerbattle_single TRAINER_POLISHED_SPROUT_JIN, PolishedSproutTower3F_GenericTrainerSageJin_Seen, PolishedSproutTower3F_GenericTrainerSageJin_Beaten
	msgbox PolishedSproutTower3F_GenericTrainerSageJin_After, MSGBOX_AUTOCLOSE
	end
PolishedSproutTower3F_GenericTrainerSageJin_Seen:
	.string "I train to find\nenlightenment in\lPOKéMON!$"

PolishedSproutTower3F_GenericTrainerSageJin_Beaten:
	.string "My training is\nincomplete…$"

PolishedSproutTower3F_GenericTrainerSageJin_After:
	.string "As POKéMON grow\nstronger, so does\lthe trainer.\pNo, wait. As the\ntrainer grows\pstronger, so do\nthe POKéMON.$"

PolishedSproutTower3F_GenericTrainerSageTroy::
	trainerbattle_single TRAINER_POLISHED_SPROUT_TROY, PolishedSproutTower3F_GenericTrainerSageTroy_Seen, PolishedSproutTower3F_GenericTrainerSageTroy_Beaten
	msgbox PolishedSproutTower3F_GenericTrainerSageTroy_After, MSGBOX_AUTOCLOSE
	end
PolishedSproutTower3F_GenericTrainerSageTroy_Seen:
	.string "Let me see how\nmuch you trust\lyour POKéMON.$"

PolishedSproutTower3F_GenericTrainerSageTroy_Beaten:
	.string "Yes, your trust is\nreal!$"

PolishedSproutTower3F_GenericTrainerSageTroy_After:
	.string "It is not far to\nthe Elder.$"

PolishedSproutTower3F_GenericTrainerSageNeal::
	trainerbattle_single TRAINER_POLISHED_SPROUT_NEAL, PolishedSproutTower3F_GenericTrainerSageNeal_Seen, PolishedSproutTower3F_GenericTrainerSageNeal_Beaten
	msgbox PolishedSproutTower3F_GenericTrainerSageNeal_After, MSGBOX_AUTOCLOSE
	end
PolishedSproutTower3F_GenericTrainerSageNeal_Seen:
	.string "The Elder's TM\nlights even pitch-\lblack darkness.$"

PolishedSproutTower3F_GenericTrainerSageNeal_Beaten:
	.string "It is my head that\nis bright!$"

PolishedSproutTower3F_GenericTrainerSageNeal_After:
	.string "Let there be light\non your journey.$"

PolishedSproutTower3F_ElderLiScript::
    lock
    faceplayer
    goto_if_set FLAG_POLISHED_SPROUT_FLASH, PolishedSproutTower3F_ElderLiScript_After
    goto_if_defeated TRAINER_POLISHED_SPROUT_LI, PolishedSproutTower3F_ElderLiScript_Gift
    msgbox PolishedSproutTower3F_ElderLiScript_Seen, MSGBOX_DEFAULT
    trainerbattle_no_intro TRAINER_POLISHED_SPROUT_LI, PolishedSproutTower3F_ElderLiScript_Beaten
PolishedSproutTower3F_ElderLiScript_Gift::
    msgbox PolishedSproutTower3F_ElderLiScript_Take, MSGBOX_DEFAULT
    giveitem ITEM_TM70
    goto_if_eq VAR_RESULT, FALSE, PolishedSproutTower3F_ElderLiScript_Done
    setflag FLAG_POLISHED_SPROUT_FLASH
    msgbox PolishedSproutTower3F_ElderLiScript_Explain, MSGBOX_DEFAULT
    goto PolishedSproutTower3F_ElderLiScript_Done
PolishedSproutTower3F_ElderLiScript_After::
    msgbox PolishedSproutTower3F_ElderLiScript_AfterText, MSGBOX_DEFAULT
PolishedSproutTower3F_ElderLiScript_Done::
    releaseall
    end
PolishedSproutTower3F_ElderLiScript_Seen:
	.string "So good of you to\ncome here!\pSprout Tower is a\nplace of training.\pPeople and POKéMON\ntest their bonds\pto build a bright\nfuture together.\pI am the final\ntest.\pAllow me to check\nthe ties between\pyour POKéMON and\nyou!$"

PolishedSproutTower3F_ElderLiScript_Beaten:
	.string "Ah, excellent!$"

PolishedSproutTower3F_ElderLiScript_Take:
	.string "You and your POKé-\nmon should have\pno problem using\nthis move.\pTake this Flash\nTM.$"

PolishedSproutTower3F_ElderLiScript_AfterText:
	.string "I hope you learn\nand grow from your\ljourney.$"

PolishedSproutTower3F_ElderLiScript_Explain:
	.string "Flash illuminates\neven the darkest\lof all places.\pYou need not teach\nit to your POKéMON\punless you wish\nto use Flash in\lbattle.\pSimply press A in\nthe darkness, and\pif any of your\nPOKéMON can use\lFlash, they will.$"

PolishedSproutTower3F_POTION::
	finditem ITEM_POTION, 1
	end
PolishedSproutTower3F_ESCAPE_ROPE::
	finditem ITEM_POLISHED_ESCAPE_ROPE, 1
	end
PolishedSproutTower3F_Setup::
    setvar VAR_TEMP_4, 0
    goto_if_unset FLAG_POLISHED_SPROUT_RIVAL_LEFT, PolishedSproutTower3F_End
    setvar VAR_TEMP_4, 1
PolishedSproutTower3F_End::
    end
PolishedSproutTower3F_RivalScene::
    lockall
    turnobject OBJ_EVENT_ID_PLAYER, DIR_NORTH
    playse SE_M_COMET_PUNCH
    setvar VAR_0x8004, 2
    setvar VAR_0x8005, 2
    setvar VAR_0x8006, 40
    setvar VAR_0x8007, 2
    special ShakeCamera
    waitstate
    delay 15
    playse SE_M_COMET_PUNCH
    special ShakeCamera
    waitstate
    applymovement OBJ_EVENT_ID_PLAYER, PolishedSproutTower3F_Approach
    waitmovement OBJ_EVENT_ID_PLAYER
    applymovement LOCALID_SPROUT_3F_1, PolishedSproutTower3F_Up
    waitmovement LOCALID_SPROUT_3F_1
    msgbox PolishedSproutTower3F_Lecture, MSGBOX_DEFAULT
    closemessage
    turnobject LOCALID_SPROUT_3F_1, DIR_SOUTH
    delay 15
    applymovement LOCALID_SPROUT_3F_1, PolishedSproutTower3F_Leaves
    waitmovement LOCALID_SPROUT_3F_1
    msgbox PolishedSproutTower3F_RivalText, MSGBOX_DEFAULT
    closemessage
    turnobject LOCALID_SPROUT_3F_1, DIR_NORTH
    special PolishedErrandBufferRivalName
    msgbox PolishedSproutTower3F_Escape, MSGBOX_DEFAULT
    closemessage
    playse SE_WARP_IN
    fadescreen FADE_TO_BLACK
    setflag FLAG_POLISHED_SPROUT_RIVAL_LEFT
    removeobject LOCALID_SPROUT_3F_1
    setvar VAR_TEMP_4, 1
    fadescreen FADE_FROM_BLACK
    releaseall
    end
PolishedSproutTower3F_Approach:
    walk_up
    walk_up
    walk_up
    walk_up
    step_end
PolishedSproutTower3F_Up:
    walk_up
    step_end
PolishedSproutTower3F_Leaves:
    walk_right
    walk_down
    step_end
PolishedSproutTower3F_Lecture:
	.string "Elder: You are in-\ndeed skilled as a\ltrainer.\pAs promised, here\nis your TM.\pBut let me say\nthis: You should\ptreat your\nPOKéMON better.\pThe way you battle\nis far too harsh.\pPOKéMON are not\ntools of war…$"

PolishedSproutTower3F_RivalText:
	.string "…\n…Humph!\pHe claims to be\nthe Elder but\lhe's weak.\pIt stands to\nreason.\pI'd never lose to\nfools who babble\pabout being nice\nto POKéMON.\pI only care about\nstrong POKéMON\lthat can win.\pI really couldn't\ncare less about\lweak POKéMON.$"

PolishedSproutTower3F_Escape:
	.string "{STR_VAR_1} used an\nEscape Rope!$"
