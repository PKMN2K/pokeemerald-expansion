// Separate controller fixture linked with production objects; never shipped.
#include "global.h"
#include "load_save.h"
#include "main.h"
#include "new_game.h"
#include "pokemon.h"
#include "pokemon_summary_screen.h"
#include "save.h"

static void CB2_FixedDamageDisplayDone(void)
{
}

void CB2_PolishedFixedDamageDisplayFixture(void)
{
    const enum Move moves[] = {MOVE_SONIC_BOOM, MOVE_DRAGON_RAGE, MOVE_ABSORB, MOVE_WATER_GUN};
    SetSaveBlocksPointers(0);
    ResetMenuAndMonGlobals();
    Save_ResetSaveCounters();
    Sav2_ClearSetDefault();
    MoveSaveBlocks_ResetHeap();
    NewGameInitData();
    gSaveBlock2Ptr->optionsTextSpeed = OPTIONS_TEXT_SPEED_FAST;
    ZeroPlayerPartyMons();
    CreateRandomMon(&gParties[B_TRAINER_PLAYER][0], SPECIES_WOBBUFFET, 20);
    for (u32 slot = 0; slot < ARRAY_COUNT(moves); slot++)
        SetMonMoveSlot(&gParties[B_TRAINER_PLAYER][0], moves[slot], slot);
    gPartiesCount[B_TRAINER_PLAYER] = 1;
    ShowPokemonSummaryScreen(SUMMARY_MODE_NORMAL, gParties[B_TRAINER_PLAYER], 0, 0, CB2_FixedDamageDisplayDone);
}
