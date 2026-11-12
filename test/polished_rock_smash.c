#include "global.h"
#include "polished_rock_smash.h"
#include "polished_pokemon_data.h"
#include "event_data.h"
#include "item.h"
#include "save.h"
#include "test/test.h"
#include "constants/maps.h"
#include "constants/vars.h"

TEST("Polished rocks prioritize a later knower over earlier TM compatibility and ignore HP")
{
    struct Pokemon party[PARTY_SIZE] = {0};
    CreateMon(&party[0], SPECIES_CUBONE, 5, 0, OTID_STRUCT_PRESET(0));
    CreateMon(&party[1], SPECIES_HITMONLEE, 5, 0, OTID_STRUCT_PRESET(0));
    SetMonMoveSlot(&party[1], MOVE_BRICK_BREAK, 2);
    u16 hp = 0;
    SetMonData(&party[1], MON_DATA_HP, &hp);
    EXPECT_EQ(PolishedRockSmashPartyUser(party, 2, TRUE), 1);
    EXPECT_EQ(PolishedRockSmashPartyUser(party, 2, FALSE), 1);
    SetMonMoveSlot(&party[0], MOVE_BRICK_BREAK, 0);
    EXPECT_EQ(PolishedRockSmashPartyUser(party, 2, TRUE), 0);
    u8 egg = TRUE;
    SetMonData(&party[0], MON_DATA_IS_EGG, &egg);
    EXPECT_EQ(PolishedRockSmashPartyUser(party, 2, TRUE), 1);
    SetMonData(&party[1], MON_DATA_IS_EGG, &egg);
    EXPECT_EQ(PolishedRockSmashPartyUser(party, 2, TRUE), PARTY_SIZE);
}

TEST("Polished rocks require owned TM31 for compatibility and never accept native Rock Smash")
{
    struct Pokemon party[PARTY_SIZE] = {0};
    CreateMon(&party[0], SPECIES_MAGIKARP, 5, 0, OTID_STRUCT_PRESET(0));
    CreateMon(&party[1], SPECIES_CUBONE, 5, 0, OTID_STRUCT_PRESET(0));
    SetMonMoveSlot(&party[1], MOVE_ROCK_SMASH, 0);
    EXPECT_EQ(PolishedRockSmashPartyUser(party, 2, FALSE), PARTY_SIZE);
    EXPECT_EQ(PolishedRockSmashPartyUser(party, 2, TRUE), 1);
    EXPECT(!PolishedRockSmashKnowsMove(&party[1]));
    EXPECT_EQ(PolishedRockSmashPartyUser(party, 1, TRUE), PARTY_SIZE);
    u8 egg = TRUE;
    SetMonData(&party[1], MON_DATA_IS_EGG, &egg);
    EXPECT_EQ(PolishedRockSmashPartyUser(party, 2, TRUE), PARTY_SIZE);
    EXPECT_EQ(PolishedRockSmashPartyUser(party, 0, TRUE), PARTY_SIZE);
}

TEST("Polished rocks accept exactly the 207 source TM31 species and preserve all badges")
{
    u32 compatible = 0;
    struct Pokemon party[PARTY_SIZE] = {0};
    FlagClear(FLAG_BADGE03_GET);
    for (u32 species = 1; species < NUM_SPECIES; species++)
    {
        if (PolishedGetTeachableLearnset(species) == NULL) continue;
        CreateMon(&party[0], species, 5, 0, OTID_STRUCT_PRESET(0));
        bool32 expected = FALSE;
        const u16 *moves = PolishedGetTeachableLearnset(species);
        if (moves)
            for (; *moves != MOVE_UNAVAILABLE; moves++)
                if (*moves == MOVE_BRICK_BREAK)
                    expected = TRUE;
        EXPECT_EQ(PolishedRockSmashPartyUser(party, 1, TRUE) == 0, expected);
        compatible += expected;
    }
    EXPECT_EQ(compatible, 207);
    EXPECT(!FlagGet(FLAG_BADGE03_GET));
}

TEST("Polished rocks exhaust source encounter slots and every item byte")
{
    u32 battles = 0, krabby = 0, geodude = 0, shuckle = 0;
    for (u32 i = 0; i < 256; i++)
    {
        battles += PolishedRockSmashBattleRoll(i);
        u8 level = 77;
        enum Species species = PolishedRockSmashSpecies(i, &level);
        EXPECT_EQ(level, i < 100 ? 15 : 77);
        krabby += species == SPECIES_KRABBY;
        geodude += species == SPECIES_GEODUDE;
        shuckle += species == SPECIES_SHUCKLE;
        if (i >= 100) EXPECT_EQ(species, SPECIES_NONE);
    }
    EXPECT_EQ(battles, 4);
    EXPECT_EQ(krabby, 70); EXPECT_EQ(geodude, 25); EXPECT_EQ(shuckle, 5);
    const enum Item items[] = {ITEM_HELIX_FOSSIL, ITEM_DOME_FOSSIL, ITEM_OLD_AMBER, ITEM_BIG_NUGGET,
        ITEM_RARE_BONE, ITEM_NUGGET, ITEM_STAR_PIECE, ITEM_BIG_PEARL, ITEM_STARDUST,
        ITEM_HARD_STONE, ITEM_SOFT_SAND, ITEM_PEARL, ITEM_POLISHED_BRICK_PIECE, ITEM_NONE};
    const u8 weights[] = {1,1,1,1,2,4,6,12,18,24,24,48,64,50};
    u32 byte = 0;
    for (u32 row = 0; row < ARRAY_COUNT(items); row++)
        for (u32 i = 0; i < weights[row]; i++)
            EXPECT_EQ(PolishedRockSmashItem(byte++), items[row]);
    EXPECT_EQ(byte, 256);
}

TEST("Polished rocks roll a battle or item with no extra item RNG after encounters")
{
    for (u32 seed = 0; seed < 1000; seed++)
    {
        SeedRng(seed);
        u8 gate;
        do { gate = Random() & 0xff; } while (gate >= 250);
        bool32 battle = gate % 10 < 4;
        u8 expectedRoll;
        do { expectedRoll = Random() & 0xff; } while (battle && expectedRoll >= 200);
        if (battle) expectedRoll %= 100;
        u32 next = Random32();
        SeedRng(seed);
        enum Species species;
        enum Item item;
        u8 level, expectedLevel;
        bool32 result = PolishedRockSmashRollOutcome(&species, &level, &item);
        EXPECT_EQ(result, battle);
        EXPECT_EQ(Random32(), next);
        if (battle)
        {
            EXPECT_EQ(species, PolishedRockSmashSpecies(expectedRoll, &expectedLevel));
            EXPECT_EQ(level, expectedLevel);
            EXPECT_EQ(item, ITEM_NONE);
        }
        else
        {
            EXPECT_EQ(species, SPECIES_NONE);
            EXPECT_EQ(level, 0);
            EXPECT_EQ(item, PolishedRockSmashItem(expectedRoll));
        }
    }
}

TEST("Polished rocks register only Dark Cave and reset only map-visit flags")
{
    EXPECT(PolishedRockSmashMap(MAP_GROUP(MAP_POLISHED_DARK_CAVE_VIOLET_ENTRANCE), MAP_NUM(MAP_POLISHED_DARK_CAVE_VIOLET_ENTRANCE)));
    EXPECT(!PolishedRockSmashMap(MAP_GROUP(MAP_POLISHED_JOHTO_DEMO_OUTDOOR), MAP_NUM(MAP_POLISHED_JOHTO_DEMO_OUTDOOR)));
    for (u32 f = FLAG_TEMP_1; f <= FLAG_TEMP_4; f++) FlagSet(f);
    FlagSet(FLAG_POLISHED_CAVE_DONE);
    VarSet(VAR_POLISHED_ROCK_SMASH_ACTIVE, 1);
    ClearTempFieldEventData();
    for (u32 f = FLAG_TEMP_1; f <= FLAG_TEMP_4; f++) EXPECT(!FlagGet(f));
    EXPECT_EQ(VarGet(VAR_POLISHED_ROCK_SMASH_ACTIVE), 0);
    EXPECT(FlagGet(FLAG_POLISHED_CAVE_DONE));
    EXPECT_EQ(ITEM_POLISHED_GOLD_LEAF, 882);
    EXPECT_EQ(ITEM_POLISHED_BRICK_PIECE, 883);
    EXPECT_EQ(GetItemPrice(ITEM_POLISHED_BRICK_PIECE), 100);
}
