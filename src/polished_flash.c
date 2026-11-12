#include "global.h"
#include "polished_flash.h"
#include "polished_pokemon_data.h"
#include "event_data.h"
#include "gpu_regs.h"
#include "item.h"
#include "overworld.h"
#include "party_menu.h"
#include "scanline_effect.h"
#include "script.h"
#include "constants/maps.h"

bool32 PolishedFlashMap(u8 group, u8 num)
{
    // Register source maps explicitly. Ruins of Alph's chamber is not ported.
    return group == MAP_GROUP(MAP_POLISHED_DARK_CAVE_VIOLET_ENTRANCE)
        && num == MAP_NUM(MAP_POLISHED_DARK_CAVE_VIOLET_ENTRANCE);
}

bool32 PolishedFlashKnowsMove(struct Pokemon *mon)
{
    if (GetMonData(mon, MON_DATA_SPECIES) == SPECIES_NONE || GetMonData(mon, MON_DATA_IS_EGG))
        return FALSE;
    for (u32 i = 0; i < MAX_MON_MOVES; i++)
        if (GetMonData(mon, MON_DATA_MOVE1 + i) == MOVE_FLASH)
            return TRUE;
    return FALSE;
}

u8 PolishedFlashPartyUser(struct Pokemon *party, u8 count, bool32 ownsTM)
{
    count = min(count, PARTY_SIZE);
    // Source CheckPartyMove scans every knower before trying TM compatibility.
    // Neither HP, PP nor badges are requirements for this field move.
    for (u32 i = 0; i < count; i++)
        if (PolishedFlashKnowsMove(&party[i]))
            return i;
    if (ownsTM)
        for (u32 i = 0; i < count; i++)
        {
            if (GetMonData(&party[i], MON_DATA_IS_EGG))
                continue;
            const u16 *moves = PolishedGetTeachableLearnset(GetMonData(&party[i], MON_DATA_SPECIES));
            if (moves != NULL)
                for (; *moves != MOVE_UNAVAILABLE; moves++)
                    if (*moves == MOVE_FLASH)
                        return i;
        }
    return PARTY_SIZE;
}

static bool32 IsDark(void)
{
    return PolishedFlashMap(gSaveBlock1Ptr->location.mapGroup, gSaveBlock1Ptr->location.mapNum)
        && GetFlashLevel() > 0 && !FlagGet(FLAG_SYS_USE_FLASH);
}

static void PrepareMon(u8 slot)
{
    gSpecialVar_0x8004 = GetMonData(&gParties[B_TRAINER_PLAYER][slot], MON_DATA_SPECIES);
}

const u8 *PolishedFlashInteraction(void)
{
    if (!IsDark())
        return NULL;
    // Preserve the established save ID: source TM70 is currently represented
    // by HM05. Source TM numbering/inventory remains a separate migration.
    u8 slot = PolishedFlashPartyUser(gParties[B_TRAINER_PLAYER], gPartiesCount[B_TRAINER_PLAYER], CheckBagHasItem(ITEM_HM_FLASH, 1));
    if (slot == PARTY_SIZE)
        return NULL;
    PrepareMon(slot);
    return PolishedFlash_Ask;
}

static void MenuCallback(void)
{
    PrepareMon(GetCursorSelectionMonId());
    ScriptContext_SetupScript(PolishedFlash_Menu);
}

bool32 PolishedFlashFromMenu(void)
{
    u8 slot = GetCursorSelectionMonId();
    if (slot >= PARTY_SIZE || !IsDark() || !PolishedFlashKnowsMove(&gParties[B_TRAINER_PLAYER][slot]))
        return FALSE;
    gFieldCallback2 = FieldCallback_PrepareFadeInFromMenu;
    gPostMenuFieldCallback = MenuCallback;
    return TRUE;
}

void PolishedFlashIlluminate(void)
{
    FlagSet(FLAG_SYS_USE_FLASH);
    SetFlashLevel(0);
    // The source lights the entire map after fading to white. Remove the GBA
    // darkness window under that fade, rather than retaining Emerald's circle.
    ScanlineEffect_Stop();
    ScanlineEffect_Clear();
    SetGpuReg(REG_OFFSET_WIN0H, WIN_RANGE(0, 255));
    SetGpuReg(REG_OFFSET_WIN0V, WIN_RANGE(0, 255));
}
