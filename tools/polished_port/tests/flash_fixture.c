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
#include "constants/vars.h"

static void FixtureLoaded(void)
{
    // Only the legacy-save fixture supplies an already-used native Flash state.
    // Save/reset then exercises the production Continue loader unchanged.
    if (FIXTURE_MODE == 6)
    {
        FlagSet(FLAG_SYS_USE_FLASH);
        SetFlashLevel(1);
    }
    FieldCB_WarpExitFadeFromBlack();
}

void CB2_PolishedFlashFixture(void)
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
    CreateRandomMon(&gParties[B_TRAINER_PLAYER][0], FIXTURE_MODE == 2 ? SPECIES_MAGIKARP : SPECIES_BELLSPROUT, 50);
    CreateRandomMon(&gParties[B_TRAINER_PLAYER][1], SPECIES_MAREEP, 50);
    gPartiesCount[B_TRAINER_PLAYER] = 2;
    for (u32 slot = 0; slot < 2; slot++)
        for (u32 move = 0; move < MAX_MON_MOVES; move++)
            SetMonMoveSlot(&gParties[B_TRAINER_PLAYER][slot], MOVE_NONE, move);
    SetMonMoveSlot(&gParties[B_TRAINER_PLAYER][0], MOVE_BRICK_BREAK, 0);
    SetMonMoveSlot(&gParties[B_TRAINER_PLAYER][1], MOVE_TACKLE, 0);
    if (FIXTURE_MODE == 0) SetMonMoveSlot(&gParties[B_TRAINER_PLAYER][1], MOVE_FLASH, 3);
    if (FIXTURE_MODE == 4) SetMonMoveSlot(&gParties[B_TRAINER_PLAYER][0], MOVE_FLASH, 3);
    if (FIXTURE_MODE == 1 || FIXTURE_MODE == 2 || FIXTURE_MODE == 3 || FIXTURE_MODE == 5) AddBagItem(ITEM_HM_FLASH, 1);
    if (FIXTURE_MODE == 2) gPartiesCount[B_TRAINER_PLAYER] = 1;
    if (FIXTURE_MODE == 3)
    {
        u8 egg = TRUE;
        SetMonMoveSlot(&gParties[B_TRAINER_PLAYER][0], MOVE_FLASH, 3);
        SetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_IS_EGG, &egg);
        CreateRandomMon(&gParties[B_TRAINER_PLAYER][1], SPECIES_MAGIKARP, 50);
    }
    if (FIXTURE_MODE == 1)
    {
        u16 hp = 0;
        SetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_HP, &hp);
    }
    FlagSet(FLAG_SYS_POKEMON_GET);
    FlagSet(FLAG_SYS_B_DASH);
    FlagSet(FLAG_POLISHED_CAVE_DONE);
    VarSet(VAR_REPEL_STEP_COUNT, 250);
    SetWarpDestination(MAP_GROUP(MAP_POLISHED_DARK_CAVE_VIOLET_ENTRANCE), MAP_NUM(MAP_POLISHED_DARK_CAVE_VIOLET_ENTRANCE), WARP_ID_NONE,
        FIXTURE_MODE == 5 ? 7 : 3, FIXTURE_MODE == 5 ? 15 : 13);
    WarpIntoMap();
    ResetInitialPlayerAvatarState();
    PlayTimeCounter_Start();
    gFieldCallback = FixtureLoaded;
    gFieldCallback2 = NULL;
    SetMainCallback2(CB2_LoadMap);
}
