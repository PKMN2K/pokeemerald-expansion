#include "global.h"
#include "battle.h"
#include "battle_frontier.h"
#include "data.h"
#include "event_data.h"
#include "load_save.h"
#include "frontier_util.h"
#include "constants/frontier_util.h"
#include "polished_tower.h"
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

static void CheckTowerPoints(void)
{
    static const u8 expected[8][7] = {
        {1,2,2,3,3,4,5}, {2,3,3,4,4,5,6}, {3,4,4,5,5,6,7}, {4,5,5,6,6,7,8},
        {5,6,6,7,7,8,8}, {6,7,7,8,8,8,8}, {7,8,8,8,8,8,8}, {8,8,8,8,8,8,8}
    };
    struct BattleFrontier *frontier = &gSaveBlock2Ptr->frontier;
    for (u32 round = 0; round < 8; round++)
    {
        u32 points = 0;
        for (u32 count = 0; count <= 7; count++)
        {
            if (count)
                points += expected[round][count-1];
            frontier->challengeStatus = count == 7 ? CHALLENGE_STATUS_WON : CHALLENGE_STATUS_LOST;
            frontier->curChallengeBattleNum = count;
            frontier->towerWinStreaks[0][0] = round * 7 + count;
            frontier->battlePoints = 0;
            gSpecialVar_0x8004 = FRONTIER_UTIL_FUNC_GIVE_BATTLE_POINTS;
            CallFrontierUtilFunc();
            CheckValue(frontier->battlePoints, points, 27);
            CheckValue(frontier->towerWinStreaks[0][0], count == 7 ? round * 7 + count : 0, 28);
            CallFrontierUtilFunc();
            CheckValue(frontier->battlePoints, points, 29);
        }
    }
    frontier->battlePoints = 49998;
    frontier->towerWinStreaks[0][0] = 259;
    frontier->curChallengeBattleNum = 2;
    frontier->challengeStatus = CHALLENGE_STATUS_SAVING;
    PolishedSettleTowerPoints();
    CheckValue(frontier->battlePoints, 50000, 30);
    CheckValue(frontier->towerWinStreaks[0][0], 0, 31);
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
            CheckValue(gSaveBlock2Ptr->frontier.battlePoints, 50000, 32);
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
        CheckTowerPoints();
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
