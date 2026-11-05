@ Generated from pinned VioletCity.asm: geography, static text and ambient residents.
PolishedVioletCity_MapScripts::
	map_script MAP_SCRIPT_ON_LOAD, PolishedVioletCity_Setup
	map_script MAP_SCRIPT_ON_RETURN_TO_FIELD, PolishedVioletCity_Restore
	.byte 0

PolishedVioletCity_VioletCitySignText::
	msgbox PolishedVioletCity_VioletCitySignText_Text, MSGBOX_SIGN
	end

PolishedVioletCity_VioletCitySignText_Text:
	.string "Violet City\pThe City of\nNostalgic Scents$"

PolishedVioletCity_VioletGymSignText::
	msgbox PolishedVioletCity_VioletGymSignText_Text, MSGBOX_SIGN
	end

PolishedVioletCity_VioletGymSignText_Text:
	.string "Violet City\nPOKéMON Gym\lLeader: Falkner\pThe Elegant Master\nof Flying POKéMON$"

PolishedVioletCity_SproutTowerSignText::
	msgbox PolishedVioletCity_SproutTowerSignText_Text, MSGBOX_SIGN
	end

PolishedVioletCity_SproutTowerSignText_Text:
	.string "Sprout Tower\pExperience the\nWay of POKéMON$"

PolishedVioletCity_EarlsPokemonAcademySignText::
	msgbox PolishedVioletCity_EarlsPokemonAcademySignText_Text, MSGBOX_SIGN
	end

PolishedVioletCity_EarlsPokemonAcademySignText_Text:
	.string "Earl's POKéMON\nAcademy$"

PolishedVioletCity_VioletCityLassText::
	msgbox PolishedVioletCity_VioletCityLassText_Text, MSGBOX_NPC
	end

PolishedVioletCity_VioletCityLassText_Text:
	.string "Ghosts are rumored\nto appear in\lSprout Tower.\pThey said Normal-\ntype POKéMON moves\phad no effect on\nghosts.$"

PolishedVioletCity_VioletCityCooltrainerM1Text::
	msgbox PolishedVioletCity_VioletCityCooltrainerM1Text_Text, MSGBOX_NPC
	end

PolishedVioletCity_VioletCityCooltrainerM1Text_Text:
	.string "Hey, you're a\nPOKéMON trainer?\pIf you beat the\nGym Leader here,\pyou'll be ready\nfor prime time!$"

PolishedVioletCity_VioletCityCooltrainerFText::
	msgbox PolishedVioletCity_VioletCityCooltrainerFText_Text, MSGBOX_NPC
	end

PolishedVioletCity_VioletCityCooltrainerFText_Text:
	.string "Bellsprout is a\npopular POKéMON\lin this town.\pIt doesn't do well\nagainst our Gym\lLeader, though…$"

PolishedVioletCity_VioletCityGrampsText::
	msgbox PolishedVioletCity_VioletCityGrampsText_Text, MSGBOX_NPC
	end

PolishedVioletCity_VioletCityGrampsText_Text:
	.string "Falkner, from the\nViolet POKéMON\pGym, is a fine\ntrainer!\pHe inherited his\nfather's gym and\phas done a great\njob with it.$"

PolishedVioletCity_VioletCityYoungsterText::
	msgbox PolishedVioletCity_VioletCityYoungsterText_Text, MSGBOX_NPC
	end

PolishedVioletCity_VioletCityYoungsterText_Text:
	.string "I saw a wiggly\ntree up ahead!\pIf you touch it,\nit squirms and\ldances! Cool!$"

PolishedVioletCity_VioletCityFisherText::
	msgbox PolishedVioletCity_VioletCityFisherText_Text, MSGBOX_NPC
	end

PolishedVioletCity_VioletCityFisherText_Text:
	.string "How does such a\nwobbly building\psurvive an earth-\nquake? I must be\lmissing something.$"

PolishedVioletCity_VioletCityCooltrainerM2Text::
	msgbox PolishedVioletCity_VioletCityCooltrainerM2Text_Text, MSGBOX_NPC
	end

PolishedVioletCity_VioletCityCooltrainerM2Text_Text:
	.string "We care about the\ntraditional build-\lings around here.\pEven the POKé Mart\nand POKéMON Center\pare painted som-\nberly to blend in.$"

PolishedVioletCity_PP_UP::
	finditem ITEM_PP_UP
	end

PolishedVioletCity_RARE_CANDY::
	finditem ITEM_RARE_CANDY
	end

PolishedVioletCity_Cheri::
    setvar VAR_0x8005, 4
    goto PolishedFruit_Interact

PolishedVioletCity_Setup::
    setflag FLAG_POLISHED_VISITED_VIOLET
    special PolishedFruitRefreshAll
    gettime
    goto_if_lt VAR_0x8000, 5, PolishedVioletCity_Night
    goto_if_ge VAR_0x8000, 17, PolishedVioletCity_Night
    clearflag FLAG_TEMP_1
    setflag FLAG_TEMP_2
    end

PolishedVioletCity_Night::
    setflag FLAG_TEMP_1
    clearflag FLAG_TEMP_2
    end

PolishedVioletCity_Restore::
    goto_if_set FLAG_TEMP_1, PolishedVioletCity_ShowNight
    removeobject LOCALID_VIOLET_CITY_ACE_TRAINER_F
    addobject LOCALID_VIOLET_CITY_ACE_TRAINER_M
    end

PolishedVioletCity_ShowNight::
    removeobject LOCALID_VIOLET_CITY_ACE_TRAINER_M
    addobject LOCALID_VIOLET_CITY_ACE_TRAINER_F
    end
PolishedVioletCity_MartSign::
	msgbox PolishedVioletCity_MartSign_Text, MSGBOX_SIGN
	end

PolishedVioletCity_MartSign_Text:
	.string "For All Your\nPOKéMON Needs\pPOKéMON Mart$"

PolishedVioletCity_PokeCenterSign::
	msgbox PolishedVioletCity_PokeCenterSign_Text, MSGBOX_SIGN
	end

PolishedVioletCity_PokeCenterSign_Text:
	.string "Heal Your POKéMON!\nPOKéMON Center$"
