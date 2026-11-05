@ Generated from pinned SproutTower2F.asm: geography, static text and ambient residents.
PolishedSproutTower2F_MapScripts::
	.byte 0

PolishedSproutTower2F_SproutTower2FStatueText::
	msgbox PolishedSproutTower2F_SproutTower2FStatueText_Text, MSGBOX_SIGN
	end

PolishedSproutTower2F_SproutTower2FStatueText_Text:
	.string "A POKéMON statue…\pIt looks very\ndistinguished.$"

PolishedSproutTower2F_GenericTrainerSageNico::
	trainerbattle_single TRAINER_POLISHED_SPROUT_NICO, PolishedSproutTower2F_GenericTrainerSageNico_Seen, PolishedSproutTower2F_GenericTrainerSageNico_Beaten
	msgbox PolishedSproutTower2F_GenericTrainerSageNico_After, MSGBOX_AUTOCLOSE
	end
PolishedSproutTower2F_GenericTrainerSageNico_Seen:
	.string "However hard we\nbattle, the tower\lwill stand strong.$"

PolishedSproutTower2F_GenericTrainerSageNico_Beaten:
	.string "I fought hard but\nI'm too weak.$"

PolishedSproutTower2F_GenericTrainerSageNico_After:
	.string "The flexible pil-\nlar protects the\ptower, even from\nearthquakes.$"

PolishedSproutTower2F_GenericTrainerSageEdmond::
	trainerbattle_single TRAINER_POLISHED_SPROUT_EDMOND, PolishedSproutTower2F_GenericTrainerSageEdmond_Seen, PolishedSproutTower2F_GenericTrainerSageEdmond_Beaten
	msgbox PolishedSproutTower2F_GenericTrainerSageEdmond_After, MSGBOX_AUTOCLOSE
	end
PolishedSproutTower2F_GenericTrainerSageEdmond_Seen:
	.string "…Sway like leaves\nin the wind…$"

PolishedSproutTower2F_GenericTrainerSageEdmond_Beaten:
	.string "Oh, I'm weak!$"

PolishedSproutTower2F_GenericTrainerSageEdmond_After:
	.string "I tried to copy\nBellsprout's\pgentle movements\nfor battle…\pBut I didn't train\nwell enough.$"

PolishedSproutTower2F_X_ACCURACY::
	finditem ITEM_X_ACCURACY, 1
	end
