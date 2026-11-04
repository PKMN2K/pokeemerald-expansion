@ Generated from pinned ElmsLab.asm: geography, static text and ambient residents.
PolishedElmsLab_MapScripts::
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
	msgbox PolishedElmsLab_HealText, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, PolishedElmsLab_HealDone
	special HealPlayerParty
PolishedElmsLab_HealDone::
	releaseall
	end

PolishedElmsLab_HealText:
	.string "Would you like to\nheal your POKéMON?$"
