
#include "global.h"
#include "battle.h"
#include "battle_main.h"
#include "battle_setup.h"
#include "battle_controllers.h"
#include "link.h"
#include "load_save.h"
#include "main.h"
#include "new_game.h"
#include "pokemon.h"
#include "random.h"
#include "constants/opponents.h"
#include "string_util.h"

const u32 gLinkProbeOffsets[] = {
    sizeof(struct BattlePokemon), offsetof(struct BattlePokemon, types), offsetof(struct BattlePokemon, pp),
    offsetof(struct BattleResources, bufferA), sizeof(struct LinkPlayer), offsetof(struct LinkPlayer, polishedBattleProtocol),
    TYPE_FIRE, TYPE_WATER, CONTROLLER_CHOOSEMOVE, LINK_POLISHED_CONVERSION,
};

static void BattleDone(void) {}

void CB2_MoveMachineAudit(void)
{
    SetSaveBlocksPointers(0);
    ResetMenuAndMonGlobals();
    Sav2_ClearSetDefault();
    MoveSaveBlocks_ResetHeap();
    NewGameInitData();
    SetTrainerId(PROBE_TRAINER_ID, gSaveBlock2Ptr->playerTrainerId);
    static const u8 name[] = {0xbb, 0xbc, 0xff};
    StringCopy(gSaveBlock2Ptr->playerName, name);
    SeedRng(PROBE_TRAINER_ID);
    ZeroPlayerPartyMons();
    ZeroEnemyPartyMons();
    struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][0];
    CreateRandomMon(mon, SPECIES_PORYGON, 30);
    SetMonMoveSlot(mon, MOVE_CONVERSION, 0);
    SetMonMoveSlot(mon, MOVE_EMBER, 1);
    SetMonMoveSlot(mon, MOVE_WATER_GUN, 2);
    SetMonMoveSlot(mon, MOVE_TACKLE, 3);
    gPartiesCount[B_TRAINER_PLAYER] = 1;
    gSaveBlock2Ptr->optionsTextSpeed = OPTIONS_TEXT_SPEED_FAST;
    gSaveBlock2Ptr->optionsBattleSceneOff = 1;
    gMain.savedCallback = BattleDone;
    // The normal copyright screen installs the serial IRQ callback. Cable Club
    // saves these two trainer IDs before closing and reopening the connection.
    SetSerialCallback(SerialCB);
    gLinkPlayers[0].trainerId = 100;
    gLinkPlayers[1].trainerId = 101;
    SaveLinkPlayers(2);
    gLinkType = LINKTYPE_BATTLE;
    gBattleTypeFlags = BATTLE_TYPE_LINK | BATTLE_TYPE_TRAINER;
    TRAINER_BATTLE_PARAM.opponentA = TRAINER_LINK_OPPONENT;
    SetMainCallback2(CB2_InitBattle);
}
