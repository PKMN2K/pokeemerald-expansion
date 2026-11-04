@ Generated from pinned PlayersNeighborsHouse.asm: geography, static text and ambient residents.
PolishedPlayersNeighborsHouse_MapScripts::
	map_script MAP_SCRIPT_ON_LOAD, PolishedPlayersNeighborsHouse_ResidentSchedule
	map_script MAP_SCRIPT_ON_RETURN_TO_FIELD, PolishedPlayersNeighborsHouse_ResidentSchedule_Restore
	.byte 0

@ Map-local visibility, recomputed on entry and save loading.
PolishedPlayersNeighborsHouse_ResidentSchedule::
	gettime
	goto_if_lt VAR_0x8000, 5, PolishedPlayersNeighborsHouse_ResidentSchedule_Evening
	goto_if_ge VAR_0x8000, 17, PolishedPlayersNeighborsHouse_ResidentSchedule_Evening
	setflag FLAG_TEMP_1
	end
PolishedPlayersNeighborsHouse_ResidentSchedule_Evening::
	clearflag FLAG_TEMP_1
	end

@ Continue restores saved actors; reconcile only this scheduled resident with
@ the freshly computed mask. Ordinary menu returns retain the existing mask.
PolishedPlayersNeighborsHouse_ResidentSchedule_Restore::
	goto_if_set FLAG_TEMP_1, PolishedPlayersNeighborsHouse_ResidentSchedule_Hide
	addobject LOCALID_NEIGHBORS_HUSBAND
	end
PolishedPlayersNeighborsHouse_ResidentSchedule_Hide::
	removeobject LOCALID_NEIGHBORS_HUSBAND
	end

PolishedPlayersNeighborsHouse_PlayersNeighborsDaughterText::
	msgbox PolishedPlayersNeighborsHouse_PlayersNeighborsDaughterText_Text, MSGBOX_NPC
	end

PolishedPlayersNeighborsHouse_PlayersNeighborsDaughterText_Text:
	.string "Pikachu is an\nevolved POKéMON.\pI was amazed by\nProf.Elm's find-\lings.\pHe's so famous for\nhis research on\lPOKéMON evolution.\p…sigh…\pI wish I could be\na researcher like\lhim…$"

PolishedPlayersNeighborsHouse_PlayersNeighborsHusbandText::
	msgbox PolishedPlayersNeighborsHouse_PlayersNeighborsHusbandText_Text, MSGBOX_NPC
	end

PolishedPlayersNeighborsHouse_PlayersNeighborsHusbandText_Text:
	.string "This town is the\nfarthest south-\peast one in all\nof Johto.$"
