@ Pinned default Polished Crystal; these four rocks use Brick Break (source TM31).
PolishedRockSmash_Interact::
    lock
    specialvar VAR_RESULT, PolishedRockSmashPrepare
    goto_if_eq VAR_RESULT, FALSE, PolishedRockSmash_Unavailable
    goto_if_eq VAR_TEMP_5, 1, PolishedRockSmash_Auto
    msgbox PolishedRockSmash_AskText, MSGBOX_YESNO
    goto_if_eq VAR_RESULT, NO, PolishedRockSmash_Done
    goto PolishedRockSmash_Use
PolishedRockSmash_Menu::
    lock
PolishedRockSmash_Use:
    msgbox PolishedRockSmash_UsedText, MSGBOX_DEFAULT
    closemessage
    showmonpic VAR_0x8004, 10, 3
    playmoncry VAR_0x8004, CRY_MODE_NORMAL
    waitmoncry
    hidemonpic
    setvar VAR_TEMP_5, 1
PolishedRockSmash_Auto:
    closemessage
    special PolishedRockSmashAnimate
    waitstate
    removeobject VAR_LAST_TALKED
    special PolishedRockSmashEncounter
    goto_if_eq VAR_RESULT, TRUE, PolishedRockSmash_Battle
    goto_if_eq VAR_0x8004, ITEM_NONE, PolishedRockSmash_Done
    copyvar VAR_0x8000, VAR_0x8004
    additem VAR_0x8000, 1
    copyvar VAR_0x8007, VAR_RESULT
    bufferitemname STR_VAR_2, VAR_0x8000
    message PolishedRockSmash_GainedText
    waitmessage
    special PolishedRockSmashShowItem
    goto_if_eq VAR_0x8007, FALSE, PolishedRockSmash_Full
    playfanfare MUS_OBTAIN_ITEM
    waitfanfare
    waitbuttonpress
    msgbox PolishedRockSmash_PutAwayText, MSGBOX_DEFAULT
    special PolishedRockSmashHideItem
    goto PolishedRockSmash_Done
PolishedRockSmash_Full:
    waitbuttonpress
    msgbox PolishedRockSmash_FullText, MSGBOX_DEFAULT
    special PolishedRockSmashHideItem
    goto PolishedRockSmash_Done
PolishedRockSmash_Battle:
    waitstate
    release
    end
PolishedRockSmash_Unavailable:
    msgbox PolishedRockSmash_UnavailableText, MSGBOX_DEFAULT
PolishedRockSmash_Done:
    release
    end
    .include "data/text/polished_rock_smash.inc"
