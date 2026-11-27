#include "global.h"
#include "battle.h"
#include "battle_setup.h"
#include "battle_util.h"
#include "data.h"
#include "decompress.h"
#include "event_data.h"
#include "load_save.h"
#include "main.h"
#include "new_game.h"
#include "move.h"
#include "pokemon.h"
#include "random.h"
#include "save.h"

void CreateTrainerPartyForPlayer(void);
struct ExpectedMon
{
    u16 species, item;
    u8 nature, gender, ability, shiny, level[2];
    u32 iv;
    u8 ev[6];
    u16 moves[2][MAX_MON_MOVES];
};
static const struct ExpectedMon sMons[] = {
/* MONS */
};
struct Team { u16 trainer, first; u8 count; };
static const struct Team sTeams[] = {
/* TEAMS */
};
static const struct { u16 pic; u32 tiles, palette; } sArt[] = {
/* PICTURES */
};
static const struct { u8 atk, def; u16 result; } sInverChart[] = {
/* MATCHUPS */
};
EWRAM_DATA volatile u32 gRosterResult[8] = {0};
static EWRAM_DATA u32 sArtBuffer[TRAINER_PIC_SIZE / 4] = {0};

static void Check(u32 actual, u32 expected, u32 field)
{
    if (actual != expected && gRosterResult[3] == 0)
    {
        gRosterResult[3] = field;
        gRosterResult[6] = actual;
        gRosterResult[7] = expected;
    }
}

static void CheckTeam(const struct Team *t, u32 badges)
{
    gSpecialVar_0x8004 = t->trainer;
    CreateTrainerPartyForPlayer();
    u32 count = min(PARTY_SIZE, t->count);
    Check(GetTrainerPartySizeFromId(t->trainer), count, 1);
    u32 present = 0;
    for (u32 i = 0; i < count; i++)
    {
        gRosterResult[2] = i;
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];
        u32 species = GetMonData(mon, MON_DATA_SPECIES);
        u32 index = i;
        if (t->trainer == TRAINER_PC_INVER_1)
        {
            for (index = 0; index < t->count && sMons[t->first + index].species != species; index++)
                ;
            if (index == t->count || (present & (1u << index)))
            {
                Check(species, SPECIES_NONE, 2);
                return;
            }
            present |= 1u << index;
        }
        const struct ExpectedMon *expected = &sMons[t->first + index];
        Check(species, expected->species, 3);
        Check(GetMonData(mon, MON_DATA_LEVEL), expected->level[badges], 4);
        Check(GetMonData(mon, MON_DATA_HELD_ITEM), expected->item, 5);
        Check(GetNature(mon), expected->nature, 6);
        Check(GetMonGender(mon), expected->gender, 7);
        Check(GetMonData(mon, MON_DATA_ABILITY_NUM), expected->ability, 8);
        Check(GetMonData(mon, MON_DATA_IS_SHINY), expected->shiny, 9);
        Check(GetMonData(mon, MON_DATA_IVS), expected->iv, 10);
        Check(GetMonData(mon, MON_DATA_FRIENDSHIP), 160, 11);
        for (u32 j = 0; j < NUM_STATS; j++)
            Check(GetMonData(mon, MON_DATA_HP_EV + j), expected->ev[j], 12 + j);
        for (u32 j = 0; j < MAX_MON_MOVES; j++)
        {
            Check(GetMonData(mon, MON_DATA_MOVE1 + j), expected->moves[badges][j], 20 + j);
            Check(GetMonData(mon, MON_DATA_PP1 + j), GetMovePP(expected->moves[badges][j]), 24 + j);
        }
    }
    if (t->trainer == TRAINER_PC_INVER_1 && !gRosterResult[3])
        gRosterResult[6] |= 127 ^ present;
}

void CB2_MoveMachineAudit(void)
{
    if (gRosterResult[0] >= 2 || gRosterResult[3])
        return;
    if (!gRosterResult[0])
    {
        SetSaveBlocksPointers(0);
        ResetMenuAndMonGlobals();
        Save_ResetSaveCounters();
        Sav2_ClearSetDefault();
        MoveSaveBlocks_ResetHeap();
        if (LoadGameSave(SAVE_NORMAL) == SAVE_STATUS_OK)
        {
            for (u32 i = 0; i < POLISHED_ROSTER_COUNT; i++)
            {
                Check(HasTrainerBeenFought(POLISHED_ROSTER_FIRST + i), i % 3 == 0, 37);
                gRosterResult[7]++;
            }
            Check(HasTrainerBeenFought(TRAINER_POLISHED_FALKNER), TRUE, 38);
            gRosterResult[0] = 3;
            return;
        }
        NewGameInitData();
        gBattleTypeFlags = BATTLE_TYPE_TRAINER;
        gMain.inBattle = TRUE;
        TRAINER_BATTLE_PARAM.opponentA = TRAINER_PC_INVER_1;
        for (u32 i = 0; i < ARRAY_COUNT(sInverChart); i++)
            Check(GetTypeModifier(sInverChart[i].atk, sInverChart[i].def), sInverChart[i].result, 39);
        gBattleTypeFlags |= BATTLE_TYPE_LINK;
        Check(GetTypeModifier(TYPE_GROUND, TYPE_FLYING), UQ_4_12(0.0), 40);
        gBattleTypeFlags = BATTLE_TYPE_TRAINER;
        gMain.inBattle = FALSE;
        Check(GetTypeModifier(TYPE_NORMAL, TYPE_GHOST), UQ_4_12(0.0), 41);
        TRAINER_BATTLE_PARAM.opponentA = TRAINER_POLISHED_FALKNER;
        SeedRng(59);
        gRosterResult[0] = 1;
    }
    if (gRosterResult[5] < ARRAY_COUNT(sArt))
    {
        u32 i = gRosterResult[5];
        DecompressDataWithHeaderWram(GetTrainerFrontPicData(sArt[i].pic), sArtBuffer);
        Check(Crc32B((const u8 *)sArtBuffer, sizeof(sArtBuffer)), sArt[i].tiles, 31);
        Check(Crc32B((const u8 *)GetTrainerFrontPicPalette(sArt[i].pic), 32), sArt[i].palette, 32);
        gRosterResult[5]++;
        return;
    }
    if (gRosterResult[4] < 2 * ARRAY_COUNT(sTeams))
    {
        u32 pass = gRosterResult[4] / ARRAY_COUNT(sTeams);
        u32 index = gRosterResult[4] % ARRAY_COUNT(sTeams);
        gRosterResult[1] = index;
        VarSet(VAR_POLISHED_EARNED_BADGES, pass ? 0xffff : 0);
        CheckTeam(&sTeams[index], pass);
        if (sTeams[index].trainer == TRAINER_PC_INVER_1)
            for (u32 i = 0; i < 128 && !gRosterResult[3] && gRosterResult[6] != 127; i++)
                CheckTeam(&sTeams[index], pass);
        gRosterResult[4]++;
        return;
    }
    for (u32 i = 0; i < POLISHED_ROSTER_COUNT; i++)
    {
        u16 id = POLISHED_ROSTER_FIRST + i;
        Check(HasTrainerBeenFought(id), FALSE, 33);
        SetTrainerFlag(id);
        if (i % 3)
            ClearTrainerFlag(id);
    }
    SetTrainerFlag(TRAINER_POLISHED_FALKNER);
    Check(TrySavingData(SAVE_NORMAL), SAVE_STATUS_OK, 34);
    gRosterResult[0] = 2;
}
