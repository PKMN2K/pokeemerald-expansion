#include "global.h"
#include "config_changes.h"
#include "battle.h"
#include "daycare.h"
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

TEST("Polished learnsets contain 391 terminated, ordered live source mappings")
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
    EXPECT_EQ(count, 391);
}

TEST("Polished base data preserves default-source rebalances in created Pokemon")
{
    struct Pokemon mon;
    SetConfig(CONFIG_POLISHED_SPECIES_DATA, TRUE);
    CreateMonWithIVs(&mon, SPECIES_BUTTERFREE, 50, 0, OTID_STRUCT_PRESET(0), 0);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_MAX_HP), 120);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_ATK), 50);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_DEF), 55);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_SPEED), 85);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_SPATK), 115);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_SPDEF), 115);
    EXPECT_EQ(GetSpeciesType(SPECIES_BUTTERFREE, 1), TYPE_PSYCHIC);
    EXPECT_EQ(GetSpeciesType(SPECIES_NOCTOWL, 0), TYPE_GHOST);
    EXPECT_EQ(GetSpeciesType(SPECIES_LEDIAN, 1), TYPE_FIGHTING);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_EXP), 125000);
    EXPECT_EQ(GetLevelFromMonExp(&mon), 50);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_FRIENDSHIP), 70);
}

TEST("Polished base data supplies source abilities and hatch cycles")
{
    struct Pokemon mon;
    u32 abilityNum = 1;
    SetConfig(CONFIG_POLISHED_SPECIES_DATA, TRUE);
    CreateRandomMon(&mon, SPECIES_CHIKORITA, 5);
    SetMonData(&mon, MON_DATA_ABILITY_NUM, &abilityNum);
    EXPECT_EQ(GetMonAbility(&mon), ABILITY_NATURAL_CURE);
    CreateEgg(&mon, SPECIES_GYARADOS, FALSE);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_FRIENDSHIP), 5);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_SPECIES), SPECIES_GYARADOS);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_IS_EGG), TRUE);
    CreateEgg(&mon, SPECIES_MR_MIME_GALAR, FALSE);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_FRIENDSHIP), 25);
}

TEST("Polished forms retain distinct saved identities and source base data")
{
    struct Pokemon mon;
    enum Species species;
    PARAMETRIZE { species = SPECIES_MEWTWO_ARMORED; }
    PARAMETRIZE { species = SPECIES_GYARADOS_RED; }
    PARAMETRIZE { species = SPECIES_ARBOK_KOGA; }
    PARAMETRIZE { species = SPECIES_PIKACHU_SURF; }
    PARAMETRIZE { species = SPECIES_MAGIKARP_SKELLY; }
    PARAMETRIZE { species = SPECIES_MAGIKARP_RAINDROP; }
    SetConfig(CONFIG_POLISHED_SPECIES_DATA, TRUE);
    SetConfig(CONFIG_POLISHED_LEVEL_UP_LEARNSETS, TRUE);
    CreateRandomMon(&mon, species, 30);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_SPECIES), species);
    EXPECT(IsSpeciesEnabled(species));
    EXPECT(GetMonData(&mon, MON_DATA_MAX_HP) > 0);
    EXPECT_EQ(GetFormSpeciesId(GET_BASE_SPECIES_ID(species), GetFormIdFromFormSpeciesId(species)), species);
    EXPECT(PolishedGetLevelUpLearnset(species) != NULL);
    EXPECT(GetSpeciesBaseData(species) == PolishedGetSpeciesBaseData(species));
    EXPECT_EQ(SPECIES_GLIMMORA_MEGA, 1572);
    EXPECT_EQ(SPECIES_GYARADOS_RED, 1573);
    EXPECT_EQ(SPECIES_MAGIKARP_RAINDROP, 1602);
}

TEST("Polished gender probabilities preserve exact source eighths")
{
    enum Species species;
    u32 femaleCount = 0, maleCount = 0, genderlessCount = 0;
    u32 expectedFemale, expectedMale, expectedGenderless;
    PARAMETRIZE { species = SPECIES_CHIKORITA; expectedFemale = 32; expectedMale = 224; expectedGenderless = 0; }
    PARAMETRIZE { species = SPECIES_MACHOP; expectedFemale = 64; expectedMale = 192; expectedGenderless = 0; }
    PARAMETRIZE { species = SPECIES_PIKACHU; expectedFemale = 128; expectedMale = 128; expectedGenderless = 0; }
    PARAMETRIZE { species = SPECIES_CLEFAIRY; expectedFemale = 192; expectedMale = 64; expectedGenderless = 0; }
    PARAMETRIZE { species = SPECIES_NIDORAN_F; expectedFemale = 256; expectedMale = 0; expectedGenderless = 0; }
    PARAMETRIZE { species = SPECIES_NIDORAN_M; expectedFemale = 0; expectedMale = 256; expectedGenderless = 0; }
    PARAMETRIZE { species = SPECIES_MEWTWO_ARMORED; expectedFemale = 0; expectedMale = 0; expectedGenderless = 256; }
    SetConfig(CONFIG_POLISHED_SPECIES_DATA, TRUE);
    for (u32 personality = 0; personality < 256; personality++)
    {
        u8 gender = GetGenderFromSpeciesAndPersonality(species, personality);
        femaleCount += gender == MON_FEMALE;
        maleCount += gender == MON_MALE;
        genderlessCount += gender == MON_GENDERLESS;
    }
    EXPECT_EQ(femaleCount, expectedFemale);
    EXPECT_EQ(maleCount, expectedMale);
    EXPECT_EQ(genderlessCount, expectedGenderless);
}

TEST("Polished Armored Mewtwo uses its own source stats types and EV yield")
{
    SetConfig(CONFIG_POLISHED_SPECIES_DATA, TRUE);
    EXPECT_EQ(GetSpeciesType(SPECIES_MEWTWO_ARMORED, 1), TYPE_STEEL);
    EXPECT_EQ(GetSpeciesType(SPECIES_MEWTWO, 1), TYPE_PSYCHIC);
    EXPECT_EQ(GetSpeciesBaseDefense(SPECIES_MEWTWO_ARMORED), 100);
    EXPECT_EQ(GetSpeciesBaseSpAttack(SPECIES_MEWTWO_ARMORED), 134);
    EXPECT_EQ(GetSpeciesBaseData(SPECIES_MEWTWO_ARMORED)->catchRate, 3);
    EXPECT_EQ(GetSpeciesBaseData(SPECIES_MEWTWO_ARMORED)->expYield, 220);
    EXPECT_EQ((u32)GetSpeciesBaseData(SPECIES_MEWTWO_ARMORED)->evYield_Defense, 1);
    EXPECT_EQ((u32)GetSpeciesBaseData(SPECIES_MEWTWO_ARMORED)->evYield_SpAttack, 2);
    EXPECT_EQ((u32)GetSpeciesBaseData(SPECIES_MEWTWO_ARMORED)->evYield_SpDefense, 1);
}

TEST("Polished base data covers all 391 identities and preserves optional Expansion data")
{
    u32 count = 0;
    SetConfig(CONFIG_POLISHED_SPECIES_DATA, TRUE);
    for (u32 species = 1; species < NUM_SPECIES; species++)
    {
        const struct SpeciesBaseData *data = PolishedGetSpeciesBaseData(species);
        if (data == NULL)
            continue;
        count++;
        EXPECT(IsSpeciesEnabled(species));
        EXPECT(GetSpeciesBaseData(species) == data);
        EXPECT(data->baseHP > 0);
    }
    EXPECT_EQ(count, 391);
    EXPECT(PolishedGetSpeciesBaseData(SPECIES_MUDKIP) == NULL);
    EXPECT(GetSpeciesBaseData(SPECIES_MUDKIP) == &gSpeciesInfo[SPECIES_MUDKIP].baseData);
    EXPECT(PolishedGetSpeciesBaseData(NUM_SPECIES) == NULL);
    EXPECT(PolishedGetSpeciesBaseData(0xffff) == NULL);
    SetConfig(CONFIG_POLISHED_SPECIES_DATA, FALSE);
    EXPECT(GetSpeciesBaseData(SPECIES_BUTTERFREE) == &gSpeciesInfo[SPECIES_BUTTERFREE].baseData);
    EXPECT_EQ(GetSpeciesType(SPECIES_BUTTERFREE, 1), TYPE_FLYING);
}
