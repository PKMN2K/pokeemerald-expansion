#include "global.h"
#include "battle.h"
#include "battle_frontier.h"
#include "data.h"
#include "event_data.h"
#include "load_save.h"
#include "main.h"
#include "move.h"
#include "new_game.h"
#include "pokemon.h"
#include "random.h"
#include "save.h"
#include "constants/battle_frontier.h"

static const struct ExpectedTowerMon {
    u16 species, item, moves[4];
    u32 iv;
    u8 gender, nature, slot;
} sExpectedSets[] = {
/* SOURCE SETS */
};
static const struct {u16 wins, sets[3];} sExpectedParties[] = {
/* EXPECTED PARTIES */
};
static const u16 sFinalHistory[] = {/* FINAL HISTORY */};
EWRAM_DATA volatile u32 gTowerAudit[6] = {0};

static void CheckValue(u32 actual, u32 expected, u32 field)
{
    if (actual != expected && !gTowerAudit[2])
    {
        gTowerAudit[2] = field;
        gTowerAudit[3] = actual;
        gTowerAudit[4] = expected;
    }
}

void CB2_MoveMachineAudit(void)
{
    if (gTowerAudit[2] || gTowerAudit[0] >= 2)
        return;
    if (!gTowerAudit[0])
    {
        SetSaveBlocksPointers(0);
        ResetMenuAndMonGlobals();
        Save_ResetSaveCounters();
        Sav2_ClearSetDefault();
        MoveSaveBlocks_ResetHeap();
        if (LoadGameSave(SAVE_NORMAL) == SAVE_STATUS_OK)
        {
            for (u32 i = 0; i < ARRAY_COUNT(sFinalHistory); i++)
                CheckValue(gSaveBlock3Ptr->polishedTowerHistory[i], sFinalHistory[i], 25);
            gTowerAudit[0] = 3;
            return;
        }
        NewGameInitData();
        gBattleTypeFlags = BATTLE_TYPE_TRAINER | BATTLE_TYPE_BATTLE_TOWER;
        gBattleScripting.specialTrainerBattleType = FACILITY_BATTLE_TOWER;
        VarSet(VAR_FRONTIER_FACILITY, FRONTIER_FACILITY_TOWER);
        VarSet(VAR_FRONTIER_BATTLE_MODE, FRONTIER_MODE_SINGLES);
        gSaveBlock2Ptr->frontier.lvlMode = FRONTIER_LVL_50;
        gTowerAudit[0] = 1;
    }
    u32 index = gTowerAudit[1];
    if (index == ARRAY_COUNT(sExpectedParties))
    {
        CheckValue(TrySavingData(SAVE_NORMAL), SAVE_STATUS_OK, 26);
        gTowerAudit[0] = 2;
        return;
    }
    u32 wins = sExpectedParties[index].wins;
    gSaveBlock2Ptr->frontier.towerWinStreaks[FRONTIER_MODE_SINGLES][FRONTIER_LVL_50] = wins;
    SeedRng(index + 1);
    FillFrontierTrainerParty(FRONTIER_PARTY_SIZE);
    for (u32 i = 0; i < FRONTIER_PARTY_SIZE; i++)
    {
        u32 set = sExpectedParties[index].sets[i];
        gTowerAudit[5] = set;
        const struct ExpectedTowerMon *expected = &sExpectedSets[set];
        struct Pokemon *mon = &gParties[B_TRAINER_OPPONENT_A][i];
        CheckValue(gSaveBlock3Ptr->polishedTowerHistory[6 + i], set + 1, 1);
        CheckValue(GetMonData(mon, MON_DATA_SPECIES), expected->species, 2);
        CheckValue(GetMonData(mon, MON_DATA_LEVEL), 50, 3);
        CheckValue(GetMonData(mon, MON_DATA_IVS), expected->iv, 4);
        CheckValue(GetMonData(mon, MON_DATA_FRIENDSHIP), 160, 5);
        CheckValue(GetMonData(mon, MON_DATA_HELD_ITEM), expected->item, 6);
        CheckValue(GetMonData(mon, MON_DATA_ABILITY_NUM), expected->slot, 7);
        CheckValue(GetMonGender(mon), expected->gender, 8);
        CheckValue(GetNature(mon), expected->nature, 9);
        for (u32 j = 0; j < MAX_MON_MOVES; j++)
        {
            CheckValue(GetMonData(mon, MON_DATA_MOVE1 + j), expected->moves[j], 10 + j);
            CheckValue(GetMonData(mon, MON_DATA_PP1 + j), GetMovePP(expected->moves[j]), 14 + j);
        }
        u32 ev = min(wins * 16, 252u) / 3;
        for (u32 j = 0; j < NUM_STATS; j++)
            CheckValue(GetMonData(mon, MON_DATA_HP_EV + j), j == STAT_SPEED && !((expected->iv >> 15) & 31) ? 0 : ev, 18 + j);
    }
    gTowerAudit[1]++;
}
