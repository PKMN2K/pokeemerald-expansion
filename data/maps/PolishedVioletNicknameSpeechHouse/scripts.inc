@ Generated from pinned VioletNicknameSpeechHouse.asm: geography, static text and ambient residents.
PolishedVioletNicknameSpeechHouse_MapScripts::
	map_script MAP_SCRIPT_ON_LOAD, PolishedVioletNicknameSpeechHouse_ResidentSchedule
	map_script MAP_SCRIPT_ON_RETURN_TO_FIELD, PolishedVioletNicknameSpeechHouse_ResidentSchedule_Restore
	.byte 0

@ Map-local visibility, recomputed on entry and save loading.
PolishedVioletNicknameSpeechHouse_ResidentSchedule::
	gettime
	goto_if_lt VAR_0x8000, 5, PolishedVioletNicknameSpeechHouse_ResidentSchedule_Evening
	goto_if_ge VAR_0x8000, 17, PolishedVioletNicknameSpeechHouse_ResidentSchedule_Evening
	setflag FLAG_TEMP_1
	end
PolishedVioletNicknameSpeechHouse_ResidentSchedule_Evening::
	clearflag FLAG_TEMP_1
	end

@ Continue restores saved actors; reconcile only this scheduled resident with
@ the freshly computed mask. Ordinary menu returns retain the existing mask.
PolishedVioletNicknameSpeechHouse_ResidentSchedule_Restore::
	goto_if_set FLAG_TEMP_1, PolishedVioletNicknameSpeechHouse_ResidentSchedule_Hide
	addobject LOCALID_VIOLET_NICKNAME_GRAMPS
	end
PolishedVioletNicknameSpeechHouse_ResidentSchedule_Hide::
	removeobject LOCALID_VIOLET_NICKNAME_GRAMPS
	end

PolishedVioletNicknameSpeechHouse_VioletNicknameSpeechHouseTeacherText::
	msgbox PolishedVioletNicknameSpeechHouse_VioletNicknameSpeechHouseTeacherText_Text, MSGBOX_NPC
	end

PolishedVioletNicknameSpeechHouse_VioletNicknameSpeechHouseTeacherText_Text:
	.string "She uses the names\nof her favorite\lthings to eat.\pFor the nicknames\nshe gives to her\lPOKéMON, I mean.$"

PolishedVioletNicknameSpeechHouse_VioletNicknameSpeechHouseLassText::
	msgbox PolishedVioletNicknameSpeechHouse_VioletNicknameSpeechHouseLassText_Text, MSGBOX_NPC
	end

PolishedVioletNicknameSpeechHouse_VioletNicknameSpeechHouseLassText_Text:
	.string "I call my Eevee\nStrawberry!$"

PolishedVioletNicknameSpeechHouse_VioletNicknameSpeechHouseGrampsText::
	msgbox PolishedVioletNicknameSpeechHouse_VioletNicknameSpeechHouseGrampsText_Text, MSGBOX_NPC
	end

PolishedVioletNicknameSpeechHouse_VioletNicknameSpeechHouseGrampsText_Text:
	.string "The Zephyr Badge\nis named after an\pancient god of the\nwest wind.\pVery fitting for a\nFlying-type Gym.$"

PolishedVioletNicknameSpeechHouse_Eevee::
    lock
    faceplayer
    playmoncry SPECIES_EEVEE, CRY_MODE_NORMAL
    msgbox PolishedVioletNicknameSpeechHouse_EeveeText, MSGBOX_DEFAULT
    waitmoncry
    release
    end

PolishedVioletNicknameSpeechHouse_EeveeText:
	.string "Strawberry: Vui!$"
PolishedVioletNicknameSpeechHouse_IncenseBurner::
	msgbox PolishedVioletNicknameSpeechHouse_IncenseBurner_Text, MSGBOX_SIGN
	end

PolishedVioletNicknameSpeechHouse_IncenseBurner_Text:
	.string "It's an incense\nburner!$"

PolishedVioletNicknameSpeechHouse_MagazineBookshelf::
	msgbox PolishedVioletNicknameSpeechHouse_MagazineBookshelf_Text, MSGBOX_SIGN
	end

PolishedVioletNicknameSpeechHouse_MagazineBookshelf_Text:
	.string "POKéMON magazines…\nPOKéMON Pal,\pPOKéMON Handbook,\nPOKéMON Graph…$"
