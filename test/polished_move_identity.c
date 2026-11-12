#include "global.h"
#include "battle.h"
#include "move.h"
#include "polished_moves.h"
#include "polished_pokemon_data.h"
#include "pokemon_storage_system.h"
#include "save.h"
#include "test/test.h"

// Source $f9 is Brick Break in the pinned non-FAITHFUL build. These are all
// eight source level-up occurrences, including the regional forms.
static const struct
{
    enum Species species;
    u8 level;
} sBrickBreakLearnLevels[] =
{
    {SPECIES_CUBONE, 41},
    {SPECIES_MAROWAK, 49},
    {SPECIES_HITMONLEE, 21},
    {SPECIES_PINSIR, 26},
    {SPECIES_SIRFETCHD, 40},
    {SPECIES_MAROWAK_ALOLA, 49},
    {SPECIES_FARFETCHD_GALAR, 40},
    {SPECIES_ZAPDOS_GALAR, 43},
};

TEST("Polished move identity creates all affected species with Brick Break and source PP")
{
    u32 row = 0;
    for (u32 i = 0; i < ARRAY_COUNT(sBrickBreakLearnLevels); i++)
        PARAMETRIZE { row = i; }
    SetConfig(CONFIG_POLISHED_LEVEL_UP_LEARNSETS, TRUE);
    SetConfig(CONFIG_B_POLISHED_MOVE_PP, TRUE);
    struct Pokemon mon;
    CreateMon(&mon, sBrickBreakLearnLevels[row].species, sBrickBreakLearnLevels[row].level, 0, OTID_STRUCT_PRESET(0));
    GiveMonInitialMoveset(&mon);
    u32 found = 0;
    for (u32 slot = 0; slot < MAX_MON_MOVES; slot++)
    {
        enum Move move = GetMonData(&mon, MON_DATA_MOVE1 + slot);
        EXPECT_NE(move, MOVE_ROCK_SMASH);
        if (move == MOVE_BRICK_BREAK)
        {
            found++;
            EXPECT_EQ(GetMonData(&mon, MON_DATA_PP1 + slot), 15);
        }
    }
    EXPECT_EQ(found, 1);
    EXPECT_EQ(gMovesInfo[MOVE_BRICK_BREAK].additionalEffects[0].moveEffect, MOVE_EFFECT_BREAK_SCREEN);
    EXPECT_EQ(GetMovePower(MOVE_BRICK_BREAK), 75);
}

TEST("Polished move identity offers and learns Brick Break at every affected source level")
{
    u32 row = 0;
    for (u32 i = 0; i < ARRAY_COUNT(sBrickBreakLearnLevels); i++)
        PARAMETRIZE { row = i; }
    SetConfig(CONFIG_POLISHED_LEVEL_UP_LEARNSETS, TRUE);
    SetConfig(CONFIG_B_POLISHED_MOVE_PP, TRUE);
    struct Pokemon mon;
    CreateMon(&mon, sBrickBreakLearnLevels[row].species, sBrickBreakLearnLevels[row].level - 1, 0, OTID_STRUCT_PRESET(0));
    GiveMonInitialMoveset(&mon);
    // All eight already have four moves: first exercise the replacement
    // prompt, then an empty slot through the ordinary learning API.
    EXPECT_EQ(MonTryLearningNewMoveAtLevel(&mon, TRUE, sBrickBreakLearnLevels[row].level), MON_HAS_MAX_MOVES);
    EXPECT_EQ(gMoveToLearn, MOVE_BRICK_BREAK);
    SetMonMoveSlot(&mon, MOVE_NONE, 3);
    EXPECT_EQ(MonTryLearningNewMoveAtLevel(&mon, TRUE, sBrickBreakLearnLevels[row].level), MOVE_BRICK_BREAK);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_MOVE4), MOVE_BRICK_BREAK);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_PP4), 15);
    EXPECT_EQ(MonTryLearningNewMoveAtLevel(&mon, FALSE, sBrickBreakLearnLevels[row].level), MOVE_NONE);
}

TEST("Polished move identity teaches source TM31 to exactly 207 species and preserves native fallback")
{
    SetConfig(CONFIG_POLISHED_LEVEL_UP_LEARNSETS, TRUE);
    SetConfig(CONFIG_POLISHED_LEARNING_DATA, TRUE);
    SetConfig(CONFIG_B_POLISHED_MOVE_PP, TRUE);
    u32 mapped = 0, compatible = 0, levelEntries = 0;
    for (u32 species = 1; species < NUM_SPECIES; species++)
    {
        const struct LevelUpMove *learnset = PolishedGetLevelUpLearnset(species);
        if (learnset == NULL)
            continue;
        mapped++;
        for (u32 i = 0; learnset[i].move != LEVEL_UP_MOVE_END; i++)
        {
            EXPECT_NE(learnset[i].move, MOVE_ROCK_SMASH);
            if (learnset[i].move == MOVE_BRICK_BREAK)
                levelEntries++;
        }
        EXPECT(!CanLearnTeachableMove(species, MOVE_ROCK_SMASH));
        if (CanLearnTeachableMove(species, MOVE_BRICK_BREAK))
            compatible++;
    }
    EXPECT_EQ(mapped, 391);
    EXPECT_EQ(levelEntries, 8);
    EXPECT_EQ(compatible, 207);
    EXPECT(CanLearnTeachableMove(SPECIES_CUBONE, MOVE_BRICK_BREAK));
    EXPECT(!CanLearnTeachableMove(SPECIES_MAGIKARP, MOVE_BRICK_BREAK));
    EXPECT_EQ(PolishedGetMovePP(MOVE_BRICK_BREAK), 15);
    EXPECT_EQ(PolishedGetMovePP(MOVE_ROCK_SMASH), 0);
    EXPECT_EQ(GetSpeciesTeachableLearnset(SPECIES_MUDKIP), gSpeciesInfo[SPECIES_MUDKIP].teachableLearnset);
    SetConfig(CONFIG_POLISHED_LEARNING_DATA, FALSE);
    SetConfig(CONFIG_POLISHED_LEVEL_UP_LEARNSETS, FALSE);
    EXPECT_EQ(GetSpeciesTeachableLearnset(SPECIES_CUBONE), gSpeciesInfo[SPECIES_CUBONE].teachableLearnset);
    EXPECT_EQ(GetSpeciesLevelUpLearnset(SPECIES_CUBONE), gSpeciesInfo[SPECIES_CUBONE].levelUpLearnset);
}

TEST("Polished move identity keeps existing Rock Smash and Brick Break distinct through party and PC saves")
{
    bool32 enabled = FALSE;
    PARAMETRIZE { enabled = FALSE; }
    PARAMETRIZE { enabled = TRUE; }
    SetConfig(CONFIG_POLISHED_LEVEL_UP_LEARNSETS, enabled);
    SetConfig(CONFIG_POLISHED_LEARNING_DATA, enabled);
    EXPECT_EQ(MOVE_ROCK_SMASH, 249);
    EXPECT_EQ(MOVE_BRICK_BREAK, 280);
    ZeroPlayerPartyMons();
    struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][0];
    CreateMon(mon, SPECIES_CUBONE, 41, 0, OTID_STRUCT_PRESET(0));
    SetMonMoveSlot(mon, MOVE_ROCK_SMASH, 0);
    SetMonMoveSlot(mon, MOVE_BRICK_BREAK, 1);
    u32 pp = 3;
    SetMonData(mon, MON_DATA_PP1, &pp);
    CalculatePlayerPartyCount();
    struct BoxPokemon expected = mon->box;
    SetBoxMonAt(0, 0, &mon->box);
    Save_ResetSaveCounters();
    EXPECT_EQ(TrySavingData(SAVE_NORMAL), SAVE_STATUS_OK);
    ZeroPlayerPartyMons();
    ZeroBoxMonAt(0, 0);
    EXPECT_EQ(LoadGameSave(SAVE_NORMAL), SAVE_STATUS_OK);
    EXPECT_EQ(memcmp(&expected, &mon->box, sizeof(expected)), 0);
    EXPECT_EQ(memcmp(&expected, GetBoxedMonPtr(0, 0), sizeof(expected)), 0);
    EXPECT_EQ(GetMonData(mon, MON_DATA_SANITY_IS_BAD_EGG), FALSE);
    EXPECT_EQ(GetMonData(mon, MON_DATA_MOVE1), MOVE_ROCK_SMASH);
    EXPECT_EQ(GetMonData(mon, MON_DATA_MOVE2), MOVE_BRICK_BREAK);
    EXPECT_EQ(GetMonData(mon, MON_DATA_PP1), 3);
}
