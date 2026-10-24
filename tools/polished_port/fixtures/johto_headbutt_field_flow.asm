; Pinned Polished Crystal source extracts for offline Headbutt flow
; Rangi42/polishedcrystal@3fa43192379df5c3e7b09a08e4d5d79af4f02f42
; engine/overworld/events.asm
TryTileCollisionEvent:
    cp COLL_HEADBUTT_TREE
    jr z, .headbutt
.headbutt
    farcall TryHeadbuttOW
; engine/events/overworld.asm
TryHeadbuttFromMenu:
    call GetFacingTileCoord
    cp COLL_HEADBUTT_TREE
TryHeadbuttOW::
    lb de, HEADBUTT, -1 ; you need the tutor for Headbutt
    call CheckPartyMove
AskHeadbuttScript:
    checkflag ENGINE_HEADBUTT_ACTIVE
AutoHeadbuttScript:
    callasm ShakeHeadbuttTree
    callasm TreeMonEncounter
    ifequalfwd TREEMON_NO_ENCOUNTER_SET, .no_item
    callasm TreeItemEncounter
; engine/events/treemons.asm
TreeMonEncounter:
    ld hl, TreeMonMaps
    call GetTreeMonSet
