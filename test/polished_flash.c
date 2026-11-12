#include "global.h"
#include "polished_flash.h"
#include "polished_pokemon_data.h"
#include "event_data.h"
#include "item.h"
#include "overworld.h"
#include "random.h"
#include "test/test.h"
#include "constants/maps.h"

TEST("Polished Flash prioritizes knowers in physical order, ignoring HP and PP")
{
    struct Pokemon party[PARTY_SIZE] = {0};
    CreateMon(&party[0], SPECIES_BELLSPROUT, 5, 0, OTID_STRUCT_PRESET(0));
    CreateMon(&party[1], SPECIES_MAREEP, 5, 0, OTID_STRUCT_PRESET(0));
    SetMonMoveSlot(&party[1], MOVE_FLASH, 3);
    u16 zero = 0;
    SetMonData(&party[1], MON_DATA_HP, &zero);
    SetMonData(&party[1], MON_DATA_PP4, &zero);
    EXPECT_EQ(PolishedFlashPartyUser(party, 2, TRUE), 1);
    EXPECT_EQ(PolishedFlashPartyUser(party, 2, FALSE), 1);
    SetMonMoveSlot(&party[0], MOVE_FLASH, 0);
    EXPECT_EQ(PolishedFlashPartyUser(party, 2, TRUE), 0);
    u8 egg = TRUE;
    SetMonData(&party[0], MON_DATA_IS_EGG, &egg);
    EXPECT_EQ(PolishedFlashPartyUser(party, 2, TRUE), 1);
    SetMonData(&party[1], MON_DATA_IS_EGG, &egg);
    EXPECT_EQ(PolishedFlashPartyUser(party, 2, TRUE), PARTY_SIZE);
}

TEST("Polished Flash TM access requires a compatible non-Egg and does not teach or spend PP")
{
    struct Pokemon party[PARTY_SIZE] = {0};
    CreateMon(&party[0], SPECIES_MAGIKARP, 5, 0, OTID_STRUCT_PRESET(0));
    CreateMon(&party[1], SPECIES_BELLSPROUT, 5, 0, OTID_STRUCT_PRESET(0));
    SetMonMoveSlot(&party[1], MOVE_VINE_WHIP, 0);
    EXPECT_EQ(PolishedFlashPartyUser(party, 2, FALSE), PARTY_SIZE);
    EXPECT_EQ(PolishedFlashPartyUser(party, 1, TRUE), PARTY_SIZE);
    EXPECT_EQ(PolishedFlashPartyUser(party, 2, TRUE), 1);
    EXPECT(!PolishedFlashKnowsMove(&party[1]));
    EXPECT_EQ(GetMonData(&party[1], MON_DATA_MOVE1), MOVE_VINE_WHIP);
    EXPECT_EQ(PolishedFlashPartyUser(party, 0, TRUE), PARTY_SIZE);
    u8 egg = TRUE;
    SetMonData(&party[1], MON_DATA_IS_EGG, &egg);
    EXPECT_EQ(PolishedFlashPartyUser(party, 2, TRUE), PARTY_SIZE);
}

TEST("Polished Flash compatibility follows every source species/form with no native fallback")
{
    struct Pokemon party[PARTY_SIZE] = {0};
    u32 checked = 0;
    for (u32 species = 1; species < NUM_SPECIES; species++)
    {
        const u16 *moves = PolishedGetTeachableLearnset(species);
        if (moves == NULL) continue;
        checked++;
        bool32 expected = FALSE;
        for (; *moves != MOVE_UNAVAILABLE; moves++)
            if (*moves == MOVE_FLASH) expected = TRUE;
        CreateMon(&party[0], species, 5, 0, OTID_STRUCT_PRESET(0));
        for (u32 i = 0; i < MAX_MON_MOVES; i++) SetMonMoveSlot(&party[0], MOVE_NONE, i);
        EXPECT_EQ(PolishedFlashPartyUser(party, 1, TRUE) == 0, expected);
    }
    EXPECT_EQ(checked, 391);
    CreateMon(&party[0], SPECIES_RALTS, 5, 0, OTID_STRUCT_PRESET(0));
    EXPECT_EQ(PolishedFlashPartyUser(party, 1, TRUE), PARTY_SIZE);
}

TEST("Polished Flash A-button dispatch preserves party, item, badge, PP and RNG state")
{
    ZeroPlayerPartyMons();
    CreateMon(&gParties[B_TRAINER_PLAYER][0], SPECIES_BELLSPROUT, 5, 0, OTID_STRUCT_PRESET(0));
    gPartiesCount[B_TRAINER_PLAYER] = 1;
    AddBagItem(ITEM_HM_FLASH, 1);
    FlagClear(FLAG_BADGE02_GET);
    FlagClear(FLAG_SYS_USE_FLASH);
    gSaveBlock1Ptr->location.mapGroup = MAP_GROUP(MAP_POLISHED_DARK_CAVE_VIOLET_ENTRANCE);
    gSaveBlock1Ptr->location.mapNum = MAP_NUM(MAP_POLISHED_DARK_CAVE_VIOLET_ENTRANCE);
    SetFlashLevel(7);
    struct Pokemon before = gParties[B_TRAINER_PLAYER][0];
    SeedRng(123);
    u32 next = Random32();
    SeedRng(123);
    EXPECT_EQ(PolishedFlashInteraction(), PolishedFlash_Ask);
    EXPECT_EQ(memcmp(&before, &gParties[B_TRAINER_PLAYER][0], sizeof(before)), 0);
    EXPECT(CheckBagHasItem(ITEM_HM_FLASH, 1));
    EXPECT(!FlagGet(FLAG_BADGE02_GET));
    EXPECT(!FlagGet(FLAG_SYS_USE_FLASH));
    EXPECT_EQ(GetFlashLevel(), 7);
    EXPECT_EQ(Random32(), next);
    FlagSet(FLAG_SYS_USE_FLASH);
    EXPECT_EQ(PolishedFlashInteraction(), NULL);
    FlagClear(FLAG_SYS_USE_FLASH);
    SetFlashLevel(0);
    EXPECT_EQ(PolishedFlashInteraction(), NULL);
    SetFlashLevel(7);
    gSaveBlock1Ptr->location.mapNum = MAP_NUM(MAP_POLISHED_JOHTO_DEMO_OUTDOOR);
    EXPECT_EQ(PolishedFlashInteraction(), NULL);
}
