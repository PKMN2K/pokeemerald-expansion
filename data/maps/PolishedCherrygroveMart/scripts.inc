@ Generated from pinned CherrygroveMart.asm: geography, static text and ambient residents.
PolishedCherrygroveMart_MapScripts::
	.byte 0

PolishedCherrygroveMart_CherrygroveMartCooltrainerMScript::
	msgbox PolishedCherrygroveMart_CherrygroveMartCooltrainerMScript_Text, MSGBOX_NPC
	end

PolishedCherrygroveMart_CherrygroveMartCooltrainerMScript_Text:
	.string "They're fresh out\nof POKé Balls!\pWhen will they get\nmore of them?$"

PolishedCherrygroveMart_CherrygroveMartYoungsterText::
	msgbox PolishedCherrygroveMart_CherrygroveMartYoungsterText_Text, MSGBOX_NPC
	end

PolishedCherrygroveMart_CherrygroveMartYoungsterText_Text:
	.string "When I was walking\nin the grass, a\pbug POKéMON poi-\nsoned my POKéMON!\pI just kept going,\nand my POKéMON\lbarely recovered.\pYou should keep an\nAntidote with you.$"

PolishedCherrygroveMart_CherrygroveMartClerkScript::
	lock
	faceplayer
	message gText_HowMayIServeYou
	waitmessage
	pokemart PolishedCherrygroveMart_Stock
	msgbox gText_PleaseComeAgain, MSGBOX_DEFAULT
	release
	end

	.align 2
PolishedCherrygroveMart_Stock:
	.2byte ITEM_POTION
	.2byte ITEM_ANTIDOTE
	.2byte ITEM_PARALYZE_HEAL
	.2byte ITEM_AWAKENING
	pokemartlistend
PolishedCherrygroveMart_MerchandiseShelf::
	msgbox PolishedCherrygroveMart_MerchandiseShelf_Text, MSGBOX_SIGN
	end

PolishedCherrygroveMart_MerchandiseShelf_Text:
	.string "Lots of POKéMON\nmerchandise!$"
