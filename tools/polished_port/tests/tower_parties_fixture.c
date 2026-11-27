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
#include "polished_growth.h"
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

static void MakePartySlot(u32 slot, enum Species species, u8 level, enum Item item)
{
    struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][slot];
    CreateMon(mon, species, level, 0x12345678 + slot, OTID_STRUCT_PRESET(0));
    SetMonData(mon, MON_DATA_HELD_ITEM, &item);
}

static void CheckLobbyEntry(bool32 expected)
{
    gSpecialVar_Result = FRONTIER_LVL_50;
    gSpecialVar_0x8004 = FRONTIER_UTIL_FUNC_CHECK_INELIGIBLE;
    CallFrontierUtilFunc();
    CheckValue(gSpecialVar_0x8004 == FALSE, expected, 33);
}

static void SelectThree(void)
{
    const u16 chosen[] = {5, 4, 1};
    memcpy(gSaveBlock2Ptr->frontier.selectedPartyMons, chosen, sizeof(chosen));
    gSpecialVar_0x8004 = FRONTIER_UTIL_FUNC_SET_PARTY_ORDER;
    gSpecialVar_0x8005 = FRONTIER_PARTY_SIZE;
    CallFrontierUtilFunc();
}

static void CheckOriginalRoster(void)
{
    static const u8 levels[] = {1, 33, 44, 100, 55, 77};
    static const u16 species[] = {SPECIES_GYARADOS, SPECIES_GYARADOS_RED, SPECIES_MARILL,
                                  SPECIES_RAIKOU, SPECIES_ARTICUNO, SPECIES_MEW};
    LoadPlayerParty();
    CheckValue(gPartiesCount[B_TRAINER_PLAYER], PARTY_SIZE, 40);
    for (u32 i = 0; i < PARTY_SIZE; i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];
        CheckValue(GetMonData(mon, MON_DATA_LEVEL), levels[i], 41);
        CheckValue(GetMonData(mon, MON_DATA_HELD_ITEM), (i == 2 || i == 3) ? ITEM_LEFTOVERS : ITEM_NONE, 46);
        CheckValue(GetMonData(mon, MON_DATA_SPECIES), species[i], 42);
        CheckValue(GetMonData(mon, MON_DATA_EXP), GetSpeciesExperienceAtLevel(species[i], levels[i]), 43);
    }
}

static void CheckPlayerRegistration(void)
{
    static const u16 banned[] = {SPECIES_MEWTWO, SPECIES_MEWTWO_ARMORED, SPECIES_MEW,
                                 SPECIES_LUGIA, SPECIES_HO_OH, SPECIES_CELEBI};
    const u32 zero = 0, one = 1;
    ZeroPlayerPartyMons();
    MakePartySlot(0, SPECIES_GYARADOS, 1, ITEM_NONE);
    MakePartySlot(1, SPECIES_GYARADOS_RED, 33, ITEM_NONE);
    MakePartySlot(2, SPECIES_MARILL, 44, ITEM_NONE);
    CheckLobbyEntry(FALSE); // Only two distinct base species.
    for (u32 i = 0; i < ARRAY_COUNT(banned); i++)
    {
        MakePartySlot(3, banned[i], 12, ITEM_NONE);
        CheckLobbyEntry(FALSE);
    }
    MakePartySlot(3, SPECIES_RAIKOU, 100, ITEM_NONE);
    SetMonData(&gParties[B_TRAINER_PLAYER][3], MON_DATA_IS_EGG, &one);
    CheckLobbyEntry(FALSE);
    SetMonData(&gParties[B_TRAINER_PLAYER][3], MON_DATA_IS_EGG, &zero);
    SetMonData(&gParties[B_TRAINER_PLAYER][3], MON_DATA_HP, &zero);
    CheckLobbyEntry(TRUE); // Fainted, above level 50, and no held items.
    enum Item item = ITEM_LEFTOVERS;
    SetMonData(&gParties[B_TRAINER_PLAYER][2], MON_DATA_HELD_ITEM, &item);
    SetMonData(&gParties[B_TRAINER_PLAYER][3], MON_DATA_HELD_ITEM, &item);
    CheckLobbyEntry(FALSE);
    MakePartySlot(4, SPECIES_ARTICUNO, 55, ITEM_NONE);
    MakePartySlot(5, SPECIES_MEW, 77, ITEM_NONE);
    CheckLobbyEntry(TRUE);
    gPartiesCount[B_TRAINER_PLAYER] = PARTY_SIZE;
    SavePlayerParty();
    SelectThree();
    FillFrontierTrainerParty(FRONTIER_PARTY_SIZE);
    static const u16 expected[] = {SPECIES_ARTICUNO, SPECIES_RAIKOU, SPECIES_GYARADOS};
    CheckValue(gPartiesCount[B_TRAINER_PLAYER], FRONTIER_PARTY_SIZE, 34);
    for (u32 i = 0; i < FRONTIER_PARTY_SIZE; i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];
        CheckValue(GetMonData(mon, MON_DATA_SPECIES), expected[i], 35);
        CheckValue(GetMonData(mon, MON_DATA_LEVEL), 50, 36);
        CheckValue(GetMonData(mon, MON_DATA_EXP), GetSpeciesExperienceAtLevel(expected[i], 50), 37);
        CheckValue(GetMonData(mon, MON_DATA_HP), GetMonData(mon, MON_DATA_MAX_HP), 38);
        CheckValue(GetMonData(mon, MON_DATA_MAX_HP) > 1, TRUE, 39);
    }
    SetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_HELD_ITEM, &zero);
    SetMonMoveSlot(&gParties[B_TRAINER_PLAYER][1], MOVE_SPLASH, 0);
    FillFrontierTrainerParty(FRONTIER_PARTY_SIZE);
    CheckValue(GetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_HELD_ITEM), ITEM_LEFTOVERS, 44);
    CheckValue(GetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_MOVE1),
               GetMonData(GetSavedPlayerPartyMon(3), MON_DATA_MOVE1), 45);
    CheckOriginalRoster();
    ZeroPlayerPartyMons();
    gPartiesCount[B_TRAINER_PLAYER] = 0;
    PolishedResetTowerHistory();
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
            CheckOriginalRoster();
            gTowerAudit[0] = 3;
            return;
        }
        NewGameInitData();
        gBattleTypeFlags = BATTLE_TYPE_TRAINER | BATTLE_TYPE_BATTLE_TOWER;
        gBattleScripting.specialTrainerBattleType = FACILITY_BATTLE_TOWER;
        VarSet(VAR_FRONTIER_FACILITY, FRONTIER_FACILITY_TOWER);
        VarSet(VAR_FRONTIER_BATTLE_MODE, FRONTIER_MODE_SINGLES);
        gSaveBlock2Ptr->frontier.lvlMode = FRONTIER_LVL_50;
        CheckPlayerRegistration();
        gTowerAudit[0] = 1;
    }
    u32 index = gTowerAudit[1];
    if (index == ARRAY_COUNT(sExpectedParties))
    {
        CheckTowerPoints();
        CheckOriginalRoster();
        CheckValue(TrySavingData(SAVE_NORMAL), SAVE_STATUS_OK, 26);
        SelectThree();
        // Pause with the in-battle party at forced level and without its item.
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][1];
        u32 level = 50, exp = GetSpeciesExperienceAtLevel(SPECIES_RAIKOU, level), empty = 0;
        SetMonData(mon, MON_DATA_EXP, &exp);
        CalculateMonStats(mon);
        SetMonData(mon, MON_DATA_HELD_ITEM, &empty);
        SaveGameFrontier();
        gTowerAudit[0] = 2;
        return;
    }
    u32 wins = sExpectedParties[index].wins;
    gSaveBlock2Ptr->frontier.towerWinStreaks[FRONTIER_MODE_SINGLES][FRONTIER_LVL_50] = wins;
    // VBlank advances the shipping PRNG. Isolate the seeded draw sequence
    // from the emulator frame on which this particular case starts.
    u16 interrupts = REG_IME;
    REG_IME = 0;
    SeedRng(index + 1);
    FillFrontierTrainerParty(FRONTIER_PARTY_SIZE);
    REG_IME = interrupts;
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
