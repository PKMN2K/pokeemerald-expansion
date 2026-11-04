#include "global.h"
#include "event_data.h"
#include "naming_screen.h"
#include "overworld.h"
#include "string_util.h"

STATIC_ASSERT(sizeof(((struct SaveBlock2 *)0)->polishedRivalName) == 8, PolishedRivalNameFitsReservedSaveBytes);
STATIC_ASSERT(offsetof(struct SaveBlock2, localTimeOffset) - offsetof(struct SaveBlock2, polishedRivalName) == 8,
              PolishedRivalNamePreservesClockOffset);

void PolishedErrandNameRival(void)
{
    // Reuse the eight reserved bytes in SaveBlock2; native save offsets stay intact.
    // A blank confirmation retains the source default; typed names use the
    // engine's ordinary length and character handling.
    StringCopy(gSaveBlock2Ptr->polishedRivalName, COMPOUND_STRING("Silver"));
    DoNamingScreen(NAMING_SCREEN_RIVAL, gSaveBlock2Ptr->polishedRivalName, 0, 0, 0,
                   CB2_ReturnToFieldContinueScriptPlayMapMusic);
}

void PolishedErrandBufferRivalName(void)
{
    StringCopy(gStringVar1, gSaveBlock2Ptr->polishedRivalName);
}
