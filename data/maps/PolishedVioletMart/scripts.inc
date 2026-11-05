@ Generated from pinned VioletMart.asm: geography, static text and ambient residents.
PolishedVioletMart_MapScripts::
	.byte 0

PolishedVioletMart_VioletMartGrannyText::
	msgbox PolishedVioletMart_VioletMartGrannyText_Text, MSGBOX_NPC
	end

PolishedVioletMart_VioletMartGrannyText_Text:
	.string "When you first\ncatch a POKéMON,\lit may be weak.\pBut it will even-\ntually grow to be\lstrong.\pIt's important to\ntreat POKéMON with\llove.$"

PolishedVioletMart_VioletMartCooltrainerMText::
	msgbox PolishedVioletMart_VioletMartCooltrainerMText_Text, MSGBOX_NPC
	end

PolishedVioletMart_VioletMartCooltrainerMText_Text:
	.string "POKéMON can hold\nitems like Potion\land Antidote.\pBut they don't\nappear to know how\pto use man-made\nitems.$"

PolishedVioletMart_Clerk::
    lock
    faceplayer
    message gText_HowMayIServeYou
    waitmessage
    pokemart PolishedVioletMart_Stock
    msgbox gText_PleaseComeAgain, MSGBOX_DEFAULT
    release
    end

    .align 2
PolishedVioletMart_Stock:
    .2byte ITEM_POKE_BALL
    .2byte ITEM_REPEAT_BALL
    .2byte ITEM_POTION
    .2byte ITEM_POLISHED_ESCAPE_ROPE
    .2byte ITEM_ANTIDOTE
    .2byte ITEM_PARALYZE_HEAL
    .2byte ITEM_AWAKENING
    .2byte ITEM_X_ATTACK
    .2byte ITEM_X_DEFENSE
    .2byte ITEM_X_SPEED
    .2byte ITEM_POLISHED_FLOWER_MAIL
    pokemartlistend
PolishedVioletMart_MerchandiseShelf::
	msgbox PolishedVioletMart_MerchandiseShelf_Text, MSGBOX_SIGN
	end

PolishedVioletMart_MerchandiseShelf_Text:
	.string "Lots of POKéMON\nmerchandise!$"
