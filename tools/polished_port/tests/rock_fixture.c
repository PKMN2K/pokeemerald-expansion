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

const u8 gPolishedRockFixtureMode = FIXTURE_MODE; // 0 knower; 1 compatible + TM; 2 ineligible; 3 full bag
const u8 gPolishedRockFixtureIndex = FIXTURE_INDEX; // each of the four real source rocks

static void FixtureLoaded(void)
{
    if (FIXTURE_MODE == 4)
    {
        // A separate reentry scenario starts by the exit with the state of
        // an already-smashed visit. Interaction scenarios create this state
        // through controller input; no unreachable traversal is implied.
        VarSet(VAR_TEMP_5, 1);
        for (u32 id = 5; id <= 8; id++)
        {
            RemoveObjectEventByLocalIdAndMap(id, MAP_NUM(MAP_POLISHED_DARK_CAVE_VIOLET_ENTRANCE), MAP_GROUP(MAP_POLISHED_DARK_CAVE_VIOLET_ENTRANCE));
            FlagSet(FLAG_TEMP_1 + id - 5);
        }
    }
    FieldCB_WarpExitFadeFromBlack();
}

void CB2_PolishedRockFixture(void)
{
    static const u8 positions[][2] = {{15,14},{27,7},{7,15},{36,32}};
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
    gSaveBlock2Ptr->playerName[0] = 0xcc; // encoded R
    ZeroPlayerPartyMons();
    CreateRandomMon(&gParties[B_TRAINER_PLAYER][0], SPECIES_CUBONE, 50);
    SetMonMoveSlot(&gParties[B_TRAINER_PLAYER][0], gPolishedRockFixtureMode == 1 || gPolishedRockFixtureMode == 2 ? MOVE_TACKLE : MOVE_BRICK_BREAK, 0);
    for (u32 i = 1; i < MAX_MON_MOVES; i++) SetMonMoveSlot(&gParties[B_TRAINER_PLAYER][0], MOVE_NONE, i);
    gPartiesCount[B_TRAINER_PLAYER] = 1;
    if (gPolishedRockFixtureMode == 1) AddBagItem(ITEM_TM_BRICK_BREAK, 1);
    if (gPolishedRockFixtureMode == 3)
        for (u32 item = 1; item < ITEMS_COUNT; item++)
            if (GetItemPocket(item) == POCKET_ITEMS) AddBagItem(item, MAX_BAG_ITEM_CAPACITY);
    FlagSet(FLAG_SYS_POKEMON_GET);
    FlagSet(FLAG_SYS_B_DASH);
    FlagSet(FLAG_POLISHED_CAVE_DONE);
    VarSet(VAR_REPEL_STEP_COUNT, 250); // Rock encounters must bypass Repel.
    SetWarpDestination(MAP_GROUP(MAP_POLISHED_DARK_CAVE_VIOLET_ENTRANCE), MAP_NUM(MAP_POLISHED_DARK_CAVE_VIOLET_ENTRANCE), WARP_ID_NONE,
        FIXTURE_MODE == 4 ? 3 : positions[gPolishedRockFixtureIndex][0], FIXTURE_MODE == 4 ? 14 : positions[gPolishedRockFixtureIndex][1]);
    WarpIntoMap();
    ResetInitialPlayerAvatarState();
    PlayTimeCounter_Start();
    gFieldCallback = FixtureLoaded;
    gFieldCallback2 = NULL;
    SetMainCallback2(CB2_LoadMap);
}
