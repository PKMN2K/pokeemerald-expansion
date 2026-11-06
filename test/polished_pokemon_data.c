#include "global.h"
#include "config_changes.h"
#include "battle.h"
#include "move.h"
#include "pokemon.h"
#include "polished_pokemon_data.h"
#include "test/test.h"

TEST("Polished learnsets create starters, gifts and wild Pokemon with source moves")
{
    enum Species species;
    u32 level;
    enum Move a, b, c, d;
    PARAMETRIZE { species = SPECIES_CYNDAQUIL; level = 9; a = MOVE_TACKLE; b = MOVE_LEER; c = MOVE_SMOKESCREEN; d = MOVE_NONE; }
    PARAMETRIZE { species = SPECIES_CYNDAQUIL; level = 10; a = MOVE_TACKLE; b = MOVE_LEER; c = MOVE_SMOKESCREEN; d = MOVE_EMBER; }
    PARAMETRIZE { species = SPECIES_CHIKORITA; level = 12; a = MOVE_GROWL; b = MOVE_RAZOR_LEAF; c = MOVE_POISON_POWDER; d = MOVE_POLISHED_HEALING_LIGHT; }
    PARAMETRIZE { species = SPECIES_TOTODILE; level = 13; a = MOVE_LEER; b = MOVE_WATER_GUN; c = MOVE_RAGE; d = MOVE_BITE; }
    PARAMETRIZE { species = SPECIES_PIDGEY; level = 5; a = MOVE_TACKLE; b = MOVE_GUST; c = MOVE_NONE; d = MOVE_NONE; }
    PARAMETRIZE { species = SPECIES_TOGEPI; level = 1; a = MOVE_GROWL; b = MOVE_CHARM; c = MOVE_NONE; d = MOVE_NONE; }
    PARAMETRIZE { species = SPECIES_POLIWAG; level = 1; a = MOVE_SWEET_KISS; b = MOVE_WATER_GUN; c = MOVE_NONE; d = MOVE_NONE; }
    struct Pokemon mon;
    SetConfig(CONFIG_POLISHED_LEVEL_UP_LEARNSETS, TRUE);
    CreateRandomMon(&mon, species, level);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_MOVE1), a);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_MOVE2), b);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_MOVE3), c);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_MOVE4), d);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_PP4), GetMovePP(d));
}

TEST("Polished learnsets offer all source moves at one level and source evolution moves")
{
    struct Pokemon mon;
    SetConfig(CONFIG_POLISHED_LEVEL_UP_LEARNSETS, TRUE);
    CreateRandomMon(&mon, SPECIES_CYNDAQUIL, 9);
    EXPECT_EQ(MonTryLearningNewMoveAtLevel(&mon, TRUE, 10), MOVE_EMBER);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_MOVE4), MOVE_EMBER);
    EXPECT_EQ(MonTryLearningNewMoveAtLevel(&mon, FALSE, 10), MOVE_NONE);
    CreateRandomMon(&mon, SPECIES_CHIKORITA, 16);
    EXPECT_EQ(MonTryLearningNewMoveAtLevel(&mon, TRUE, 17), MON_HAS_MAX_MOVES);
    EXPECT_EQ(gMoveToLearn, MOVE_REFLECT);
    EXPECT_EQ(MonTryLearningNewMoveAtLevel(&mon, FALSE, 17), MON_HAS_MAX_MOVES);
    EXPECT_EQ(gMoveToLearn, MOVE_LIGHT_SCREEN);
    EXPECT_EQ(MonTryLearningNewMoveAtLevel(&mon, FALSE, 17), MOVE_NONE);
    EXPECT_EQ(GetSpeciesLevelUpLearnset(SPECIES_BLASTOISE)[0].level, 0);
    EXPECT_EQ(GetSpeciesLevelUpLearnset(SPECIES_BLASTOISE)[0].move, MOVE_FLASH_CANNON);
}

TEST("Polished learnsets preserve regional forms, unmapped species and Expansion capacity")
{
    SetConfig(CONFIG_POLISHED_LEVEL_UP_LEARNSETS, TRUE);
    EXPECT(GetSpeciesLevelUpLearnset(SPECIES_VOLTORB) != GetSpeciesLevelUpLearnset(SPECIES_VOLTORB_HISUI));
    EXPECT(GetSpeciesLevelUpLearnset(SPECIES_UNOWN) == GetSpeciesLevelUpLearnset(SPECIES_UNOWN_QUESTION));
    EXPECT(PolishedGetLevelUpLearnset(SPECIES_MUDKIP) == NULL);
    EXPECT(GetSpeciesLevelUpLearnset(SPECIES_MUDKIP) == gSpeciesInfo[SPECIES_MUDKIP].levelUpLearnset);
    EXPECT(PolishedGetLevelUpLearnset(NUM_SPECIES) == NULL);
    EXPECT(PolishedGetLevelUpLearnset(0xffff) == NULL);
    SetConfig(CONFIG_POLISHED_LEVEL_UP_LEARNSETS, FALSE);
    EXPECT(GetSpeciesLevelUpLearnset(SPECIES_CYNDAQUIL) == gSpeciesInfo[SPECIES_CYNDAQUIL].levelUpLearnset);
    EXPECT_EQ(MOVE_MALIGNANT_CHAIN, 847); // Existing ordinary move/save IDs stay stable.
    EXPECT(MOVE_POLISHED_FRESH_SNACK < MOVES_COUNT);
}

TEST("Polished learnsets contain 361 terminated, ordered live source mappings")
{
    u32 count = 0;
    SetConfig(CONFIG_POLISHED_LEVEL_UP_LEARNSETS, TRUE);
    for (u32 species = 1; species < NUM_SPECIES; species++)
    {
        const struct LevelUpMove *moves = PolishedGetLevelUpLearnset(species);
        if (moves == NULL)
            continue;
        count++;
        EXPECT(GetSpeciesLevelUpLearnset(species) == moves);
        u32 previousLevel = 0;
        u32 i;
        for (i = 0; i < 128 && moves[i].move != LEVEL_UP_MOVE_END; i++)
        {
            EXPECT(moves[i].level >= previousLevel);
            EXPECT(moves[i].level <= MAX_LEVEL);
            EXPECT(moves[i].move > MOVE_NONE && moves[i].move < MOVES_COUNT);
            previousLevel = moves[i].level;
        }
        EXPECT(i < 128);
    }
    EXPECT_EQ(count, 361);
}
