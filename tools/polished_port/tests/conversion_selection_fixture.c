// Production-linked battle fixture. Controller input is supplied by mGBA;
// the fixture only constructs the starting party and opens a normal battle.
#include "global.h"
#include "battle.h"
#include "battle_main.h"
#include "load_save.h"
#include "main.h"
#include "new_game.h"
#include "pokemon.h"
#include "save.h"

const volatile u32 gConversionProbeLayout[] = {
    offsetof(struct BattleStruct, polishedConversionSelecting),
    sizeof(struct BattlePokemon), offsetof(struct BattlePokemon, types),
    offsetof(struct BattlePokemon, pp), TYPE_WATER,
};

static void CB2_ConversionDone(void) {}

void CB2_MoveMachineAudit(void)
{
    (void)gConversionProbeLayout[0]; // Keep the read-only layout available to the observer.
    SetSaveBlocksPointers(0);
    ResetMenuAndMonGlobals();
    Save_ResetSaveCounters();
    Sav2_ClearSetDefault();
    MoveSaveBlocks_ResetHeap();
    NewGameInitData();
    gSaveBlock2Ptr->optionsTextSpeed = OPTIONS_TEXT_SPEED_FAST;
    ZeroPlayerPartyMons();
    ZeroEnemyPartyMons();
    CreateRandomMon(&gParties[B_TRAINER_PLAYER][0], SPECIES_PORYGON, 5);
    SetMonMoveSlot(&gParties[B_TRAINER_PLAYER][0], MOVE_CONVERSION, 0);
    SetMonMoveSlot(&gParties[B_TRAINER_PLAYER][0], MOVE_TACKLE, 1);
    SetMonMoveSlot(&gParties[B_TRAINER_PLAYER][0], MOVE_WATER_GUN, 2);
    SetMonMoveSlot(&gParties[B_TRAINER_PLAYER][0], MOVE_SPLASH, 3);
    u32 zero = 0;
    SetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_PP3, &zero);
    CreateRandomMon(&gParties[B_TRAINER_OPPONENT_A][0], SPECIES_MAGIKARP, 5);
    SetMonMoveSlot(&gParties[B_TRAINER_OPPONENT_A][0], MOVE_SPLASH, 0);
    for (u32 slot = 1; slot < MAX_MON_MOVES; slot++)
        SetMonMoveSlot(&gParties[B_TRAINER_OPPONENT_A][0], MOVE_NONE, slot);
    gPartiesCount[B_TRAINER_PLAYER] = gPartiesCount[B_TRAINER_OPPONENT_A] = 1;
    gBattleTypeFlags = 0;
    gMain.savedCallback = CB2_ConversionDone;
    SetMainCallback2(CB2_InitBattle);
}
