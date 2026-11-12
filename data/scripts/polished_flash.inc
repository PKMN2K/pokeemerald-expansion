@ Pinned Polished Crystal Flash field flow; source TM70 uses the existing HM05 adapter.
PolishedFlash_Ask::
    lockall
    msgbox PolishedFlash_AskText, MSGBOX_YESNO
    goto_if_eq VAR_RESULT, NO, PolishedFlash_Done
    goto PolishedFlash_Use
PolishedFlash_Menu::
    lockall
PolishedFlash_Use:
    closemessage
    showmonpic VAR_0x8004, 10, 3
    playmoncry VAR_0x8004, CRY_MODE_NORMAL
    waitmoncry
    hidemonpic
    message PolishedFlash_UsedText
    waitmessage
    playse SE_M_REFLECT
    waitse
    waitbuttonpress
    fadescreen FADE_TO_WHITE
    special PolishedFlashIlluminate
    fadescreen FADE_FROM_WHITE
PolishedFlash_Done:
    releaseall
    end
    .include "data/text/polished_flash.inc"
