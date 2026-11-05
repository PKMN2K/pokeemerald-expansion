@ Generated from pinned SproutTower1F.asm: geography, static text and ambient residents.
PolishedSproutTower1F_MapScripts::
	.byte 0

PolishedSproutTower1F_SproutTower1FStatueText::
	msgbox PolishedSproutTower1F_SproutTower1FStatueText_Text, MSGBOX_SIGN
	end

PolishedSproutTower1F_SproutTower1FStatueText_Text:
	.string "A POKéMON statue…\pIt looks very\ndistinguished.$"

PolishedSproutTower1F_KimonoGirlMakoScript::
	msgbox PolishedSproutTower1F_KimonoGirlMakoScript_Text, MSGBOX_NPC
	end
PolishedSproutTower1F_KimonoGirlMakoScript_Text:
	.string "Hello, trainer.\pI am a Kimono\nGirl.\pI journeyed here\nfrom Ecruteak\pCity, through for-\nest and cavern,\pto train in this\ntower.\pHave you met my\nfive dancing cou-\lsins in Ecruteak?\pCome back and\ntell me if you do!$"

PolishedSproutTower1F_SproutTower1FSage1Text::
	msgbox PolishedSproutTower1F_SproutTower1FSage1Text_Text, MSGBOX_NPC
	end
PolishedSproutTower1F_SproutTower1FSage1Text_Text:
	.string "Only if you reach\nthe top will you\lobtain a TM.$"

PolishedSproutTower1F_SproutTower1FSage2Text::
	msgbox PolishedSproutTower1F_SproutTower1FSage2Text_Text, MSGBOX_NPC
	end
PolishedSproutTower1F_SproutTower1FSage2Text_Text:
	.string "Sprout Tower was\nbuilt long ago\pas a place for\nPOKéMON training.$"

PolishedSproutTower1F_SproutTower1FGrannyScript::
	msgbox PolishedSproutTower1F_SproutTower1FGrannyScript_Text, MSGBOX_NPC
	end
PolishedSproutTower1F_SproutTower1FGrannyScript_Text:
	.string "A Bellsprout over\n100 feet tall…\pPeople say that it\nbecame the center\lpillar here.$"

PolishedSproutTower1F_SproutTower1FTeacherText::
	msgbox PolishedSproutTower1F_SproutTower1FTeacherText_Text, MSGBOX_NPC
	end
PolishedSproutTower1F_SproutTower1FTeacherText_Text:
	.string "See the pillar\nshaking?\pPeople are train-\ning upstairs.$"

PolishedSproutTower1F_GenericTrainerSageChow::
	trainerbattle_single TRAINER_POLISHED_SPROUT_CHOW, PolishedSproutTower1F_GenericTrainerSageChow_Seen, PolishedSproutTower1F_GenericTrainerSageChow_Beaten
	msgbox PolishedSproutTower1F_GenericTrainerSageChow_After, MSGBOX_AUTOCLOSE
	end
PolishedSproutTower1F_GenericTrainerSageChow_Seen:
	.string "We stand guard in\nthis tower.\pHere, we express\nour gratitude to\lhonor all POKéMON.$"

PolishedSproutTower1F_GenericTrainerSageChow_Beaten:
	.string "Th-Thank you!$"

PolishedSproutTower1F_GenericTrainerSageChow_After:
	.string "All living beings\ncoexist through\lcooperation.\pWe must always be\nthankful for this.$"

PolishedSproutTower1F_PARALYZE_HEAL::
	finditem ITEM_PARALYZE_HEAL, 1
	end
