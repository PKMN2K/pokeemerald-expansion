#include "global.h"
#include "polished_tm.h"
#include "item.h"
#include "test/test.h"
#include "constants/pokeball.h"

TEST("Polished TM lesson friendship uses source thresholds and additive bonuses")
{
    struct Pokemon mon;
    CreateMon(&mon, SPECIES_BELLSPROUT, 5, 0, OTID_STRUCT_PRESET(0));
    for (u32 friendship = 0; friendship <= 255; friendship++)
    for (u32 bell = 0; bell < 2; bell++)
    for (u32 luxury = 0; luxury < 2; luxury++)
    {
        u16 held = bell ? ITEM_SOOTHE_BELL : ITEM_NONE;
        u8 ball = luxury ? BALL_LUXURY : BALL_POKE;
        SetMonData(&mon, MON_DATA_FRIENDSHIP, &friendship);
        SetMonData(&mon, MON_DATA_HELD_ITEM, &held);
        SetMonData(&mon, MON_DATA_POKEBALL, &ball);
        PolishedTMFriendship(&mon);
        EXPECT_EQ(GetMonData(&mon, MON_DATA_FRIENDSHIP), min(255, friendship + (friendship < 160) + bell + luxury));
    }
}

TEST("Polished TM lesson friendship ignores Eggs and empty party slots")
{
    struct Pokemon mon = {0};
    PolishedTMFriendship(&mon);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_FRIENDSHIP), 0);
    CreateMon(&mon, SPECIES_BELLSPROUT, 5, 0, OTID_STRUCT_PRESET(0));
    u8 egg = TRUE;
    SetMonData(&mon, MON_DATA_IS_EGG, &egg);
    u32 before = GetMonData(&mon, MON_DATA_FRIENDSHIP);
    PolishedTMFriendship(&mon);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_FRIENDSHIP), before);
}

TEST("Polished TM source rules cover only migrated machines with stable IDs")
{
    EXPECT_EQ(ITEM_TM_BRICK_BREAK, 612);
    EXPECT_EQ(ITEM_TM_ROOST, 632);
    for (u32 item = 0; item < ITEMS_COUNT; item++)
        EXPECT_EQ(IsPolishedTM(item), item == ITEM_TM31 || item == ITEM_TM51 || item == ITEM_TM70);
    EXPECT_EQ(GetItemTMHMMoveId(ITEM_TM31), MOVE_BRICK_BREAK);
    EXPECT_EQ(GetItemTMHMMoveId(ITEM_TM51), MOVE_ROOST);
    EXPECT_EQ(GetTMHMItemIdFromMoveId(MOVE_BRICK_BREAK), ITEM_TM31);
    EXPECT_EQ(GetTMHMItemIdFromMoveId(MOVE_ROOST), ITEM_TM51);
    EXPECT_EQ(GetMachineDisplayNumber(ITEM_TM31), 31);
    EXPECT_EQ(GetMachineDisplayNumber(ITEM_TM51), 51);
    EXPECT(!IsItemHM(ITEM_TM31));
    EXPECT(!IsItemHM(ITEM_TM51));
#if POLISHED_JOHTO_DEMO
    EXPECT_EQ(GetItemPrice(ITEM_TM31), 0);
    EXPECT_EQ(GetItemPrice(ITEM_TM51), 0);
#else
    EXPECT_EQ(GetItemPrice(ITEM_TM31), 3000);
    EXPECT_EQ(GetItemPrice(ITEM_TM51), 3000);
#endif
}

TEST("Polished TM70 sparse mapping preserves native machine IDs and categories")
{
    EXPECT_EQ(ITEM_TM70, 651);
    EXPECT_EQ(ITEM_HM_FLASH, 686);
    EXPECT_EQ(GetItemTMHMMoveId(ITEM_HM_FLASH), MOVE_FLASH);
    EXPECT(IsItemHM(ITEM_HM_FLASH));
    EXPECT_EQ(GetMachineDisplayNumber(ITEM_HM_FLASH), 5);
    EXPECT_EQ(GetMachineDisplayNumber(ITEM_TM_ROOST), 51);
    EXPECT(GetMachineSortKey(ITEM_TM_ROOST) < GetMachineSortKey(ITEM_HM_CUT));
    for (u32 i = 1; i <= NUM_ALL_MACHINES; i++)
    {
        enum Item item = GetTMHMItemId(i);
        EXPECT_EQ(GetItemTMHMIndex(item), i);
        EXPECT_EQ(GetItemTMHMMoveId(item), GetTMHMMoveId(i));
    }
#if POLISHED_JOHTO_DEMO
    EXPECT_EQ(GetItemTMHMMoveId(ITEM_TM70), MOVE_FLASH);
    EXPECT_EQ(GetTMHMItemIdFromMoveId(MOVE_FLASH), ITEM_TM70);
    EXPECT_EQ(GetMachineDisplayNumber(ITEM_TM70), 70);
    EXPECT(!IsItemHM(ITEM_TM70));
    EXPECT(!CannotForgetMove(MOVE_FLASH));
#else
    EXPECT_EQ(GetItemTMHMMoveId(ITEM_TM70), MOVE_NONE);
    EXPECT_EQ(GetTMHMItemIdFromMoveId(MOVE_FLASH), ITEM_HM_FLASH);
#endif
}
