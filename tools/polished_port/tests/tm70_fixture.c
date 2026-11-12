// Controller-only boot fixture linked separately against the production Johto
// objects. This file is absent from both shipping ROMs; gameplay code is unchanged.
#include "global.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "field_screen_effect.h"
#include "item.h"
#include "load_save.h"
#include "main.h"
#include "new_game.h"
#include "overworld.h"
#include "play_time.h"
#include "pokemon.h"
#include "save.h"
#include "constants/maps.h"
#include "constants/pokeball.h"
#include "constants/vars.h"

void CB2_PolishedTM70Fixture(void)
{
    SetSaveBlocksPointers(0);
    ResetMenuAndMonGlobals();
    Save_ResetSaveCounters();
    LoadGameSave(SAVE_NORMAL);
    if (gSaveFileStatus == SAVE_STATUS_OK)
    {
        SetMainCallback2(CB2_ContinueSavedGame);
        return;
    }
    Sav2_ClearSetDefault();
    MoveSaveBlocks_ResetHeap();
    NewGameInitData();
    gSaveBlock2Ptr->optionsTextSpeed = OPTIONS_TEXT_SPEED_FAST;
    memset(gSaveBlock2Ptr->playerName, 0xff, sizeof(gSaveBlock2Ptr->playerName));
    gSaveBlock2Ptr->playerName[0] = 0xc0; // F
    ZeroPlayerPartyMons();
    for (u32 slot = 0; slot < 2; slot++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][slot];
        CreateRandomMon(mon, FIXTURE_MODE == 2 && slot == 0 ? SPECIES_MAGIKARP : SPECIES_BELLSPROUT, 20);
        for (u32 move = 0; move < MAX_MON_MOVES; move++) SetMonMoveSlot(mon, MOVE_NONE, move);
        SetMonMoveSlot(mon, MOVE_VINE_WHIP, 0);
        if (FIXTURE_MODE == 1 || FIXTURE_MODE == 5)
        {
            SetMonMoveSlot(mon, MOVE_GROWTH, 1);
            SetMonMoveSlot(mon, MOVE_WRAP, 2);
            SetMonMoveSlot(mon, MOVE_POISON_POWDER, 3);
            u8 bonuses = 0x57, pp = 1;
            SetMonData(mon, MON_DATA_PP_BONUSES, &bonuses);
            SetMonData(mon, MON_DATA_PP1, &pp);
        }
        if (FIXTURE_MODE == 3) SetMonMoveSlot(mon, MOVE_FLASH, 1);
        if (FIXTURE_MODE == 4 && slot == 0)
        {
            u8 egg = TRUE;
            SetMonData(mon, MON_DATA_IS_EGG, &egg);
        }
        u8 friendship = slot == 0 ? 159 : 160;
        u16 held = slot == 0 ? ITEM_SOOTHE_BELL : ITEM_NONE;
        u8 ball = slot == 0 ? BALL_LUXURY : BALL_POKE;
        SetMonData(mon, MON_DATA_FRIENDSHIP, &friendship);
        SetMonData(mon, MON_DATA_HELD_ITEM, &held);
        SetMonData(mon, MON_DATA_POKEBALL, &ball);
    }
    gPartiesCount[B_TRAINER_PLAYER] = 2;
    AddBagItem(ITEM_TM70, 1);
    AddBagItem(ITEM_TM_TOXIC, 1);
    FlagSet(FLAG_SYS_POKEMON_GET);
    FlagSet(FLAG_SYS_B_DASH);
    FlagSet(FLAG_POLISHED_CAVE_DONE);
    VarSet(VAR_REPEL_STEP_COUNT, 250);
    SetWarpDestination(MAP_GROUP(MAP_POLISHED_DARK_CAVE_VIOLET_ENTRANCE), MAP_NUM(MAP_POLISHED_DARK_CAVE_VIOLET_ENTRANCE), WARP_ID_NONE,
        3, 13);
    WarpIntoMap();
    ResetInitialPlayerAvatarState();
    PlayTimeCounter_Start();
    gFieldCallback = FieldCB_WarpExitFadeFromBlack;
    gFieldCallback2 = NULL;
    SetMainCallback2(CB2_LoadMap);
}
