@ Generated from pinned VioletOnixTradeHouse.asm: geography, static text and ambient residents.
PolishedVioletOnixTradeHouse_MapScripts::
	.byte 0

PolishedVioletOnixTradeHouse_VioletKylesHousePokefanMText::
	msgbox PolishedVioletOnixTradeHouse_VioletKylesHousePokefanMText_Text, MSGBOX_NPC
	end

PolishedVioletOnixTradeHouse_VioletKylesHousePokefanMText_Text:
	.string "A POKéMON you get\nin a trade grows\lquickly.\pBut if you don't\nhave the right Gym\pBadge, they may\ndisobey you.$"

PolishedVioletOnixTradeHouse_Kyle::
    lock
    faceplayer
    goto_if_set FLAG_POLISHED_KYLE_TRADE, PolishedVioletOnixTradeHouse_After
    ingame_trade INGAME_TRADE_POLISHED_KYLE, PolishedVioletOnixTradeHouse_IntroText, PolishedVioletOnixTradeHouse_Cancel, PolishedVioletOnixTradeHouse_Wrong, PolishedVioletOnixTradeHouse_CompleteText
    setflag FLAG_POLISHED_KYLE_TRADE
    release
    end

PolishedVioletOnixTradeHouse_Cancel::
    msgbox PolishedVioletOnixTradeHouse_CancelText, MSGBOX_DEFAULT
    release
    end

PolishedVioletOnixTradeHouse_Wrong::
    bufferspeciesname STR_VAR_1, VAR_0x8009
    msgbox PolishedVioletOnixTradeHouse_WrongText, MSGBOX_DEFAULT
    release
    end

PolishedVioletOnixTradeHouse_After::
    bufferspeciesname STR_VAR_2, SPECIES_VOLTORB
    msgbox PolishedVioletOnixTradeHouse_AfterText, MSGBOX_DEFAULT
    release
    end

PolishedVioletOnixTradeHouse_IntroText:
	.string "I collect POKéMON.\nDo you have\l{STR_VAR_1}?\pWant to trade it\nfor my {STR_VAR_2}?$"

PolishedVioletOnixTradeHouse_CancelText:
	.string "You don't want to\ntrade? Aww…$"

PolishedVioletOnixTradeHouse_WrongText:
	.string "Huh? That's not\n{STR_VAR_1}.\lWhat a letdown…$"

PolishedVioletOnixTradeHouse_CompleteText:
	.string "Yay! I got myself\n{STR_VAR_1}!\lThanks!$"

PolishedVioletOnixTradeHouse_AfterText:
	.string "Hi, how's my old\n{STR_VAR_2} doing?$"
PolishedVioletOnixTradeHouse_IncenseBurner::
	msgbox PolishedVioletOnixTradeHouse_IncenseBurner_Text, MSGBOX_SIGN
	end

PolishedVioletOnixTradeHouse_IncenseBurner_Text:
	.string "It's an incense\nburner!$"

PolishedVioletOnixTradeHouse_MagazineBookshelf::
	msgbox PolishedVioletOnixTradeHouse_MagazineBookshelf_Text, MSGBOX_SIGN
	end

PolishedVioletOnixTradeHouse_MagazineBookshelf_Text:
	.string "POKéMON magazines…\nPOKéMON Pal,\pPOKéMON Handbook,\nPOKéMON Graph…$"
