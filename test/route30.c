#include "global.h"
#include "event_data.h"
#include "item.h"
#include "overworld.h"
#include "polished_route29.h"
#include "polished_johto_palette.h"
#include "rtc.h"
#include "constants/items.h"
#include "constants/maps.h"
#include "constants/vars.h"
#include "test/test.h"

TEST("Route 30 fruit trees keep separate saved harvests and consume Mulch only on a picked tree")
{
    struct Time clock = gSaveBlock2Ptr->localTimeOffset;
    struct ItemSlot berries[BAG_BERRIES_COUNT], items[BAG_ITEMS_COUNT];
    const u16 vars[] = {VAR_POLISHED_ROUTE29_FRUIT_DAY, VAR_POLISHED_ROUTE30_ORAN_DAY, VAR_POLISHED_ROUTE30_PECHA_DAY, VAR_POLISHED_ROUTE31_PERSIM_DAY};
    u16 saved[ARRAY_COUNT(vars)], selected = gSpecialVar_0x8005;
    u32 i;
    memcpy(berries, gSaveBlock1Ptr->bag.berries, sizeof(berries));
    memcpy(items, gSaveBlock1Ptr->bag.items, sizeof(items));
    for (i = 0; i < BAG_ITEMS_COUNT; i++)
        BagPocket_SetSlotItemIdAndCount(&gBagPockets[POCKET_ITEMS], i, ITEM_NONE, 0);
    for (i = 0; i < BAG_BERRIES_COUNT; i++)
        BagPocket_SetSlotItemIdAndCount(&gBagPockets[POCKET_BERRIES], i, ITEM_NONE, 0);
    for (i = 0; i < ARRAY_COUNT(vars); i++)
    {
        saved[i] = VarGet(vars[i]);
        VarSet(vars[i], 0);
    }
    RtcCalcLocalTimeOffset(10, 12, 0, 0);
    AddBagItem(ITEM_POLISHED_MULCH, 1);
    gSpecialVar_0x8005 = 1;
    PolishedFruitUseMulch();
    EXPECT_EQ(gSpecialVar_Result, FALSE);
    EXPECT(CheckBagHasItem(ITEM_POLISHED_MULCH, 1));
    PolishedFruitPick();
    EXPECT(gSpecialVar_Result >= 1 && gSpecialVar_Result <= 3);
    EXPECT_EQ(VarGet(vars[1]), 11);
    EXPECT_EQ(VarGet(vars[0]), 0);
    EXPECT_EQ(VarGet(vars[2]), 0);
    PolishedFruitUseMulch();
    EXPECT_EQ(gSpecialVar_Result, TRUE);
    EXPECT(!CheckBagHasItem(ITEM_POLISHED_MULCH, 1));
    EXPECT_EQ(VarGet(vars[1]), 0);
    EXPECT(PolishedFruitAvailable());
    PolishedFruitPick();
    PolishedFruitUseMulch();
    EXPECT_EQ(gSpecialVar_Result, FALSE);
    EXPECT_EQ(VarGet(vars[1]), 11);
    gSpecialVar_0x8005 = 2;
    PolishedFruitPrepare();
    EXPECT_EQ(gSpecialVar_0x8006, ITEM_PECHA_BERRY);
    PolishedFruitPick();
    EXPECT(CheckBagHasItem(ITEM_PECHA_BERRY, 1));
    EXPECT_EQ(VarGet(vars[2]), 11);
    RtcCalcLocalTimeOffset(9, 12, 0, 0);
    EXPECT(!PolishedFruitAvailable());
    RtcCalcLocalTimeOffset(11, 0, 0, 0);
    EXPECT(PolishedFruitAvailable());
    gSpecialVar_0x8005 = 3;
    PolishedFruitPrepare();
    EXPECT_EQ(gSpecialVar_0x8006, ITEM_PERSIM_BERRY);
    EXPECT(PolishedFruitAvailable());
    PolishedFruitPick();
    EXPECT(gSpecialVar_Result >= 1 && gSpecialVar_Result <= 3);
    EXPECT(!PolishedFruitAvailable());
    gSpecialVar_0x8005 = ARRAY_COUNT(vars);
    PolishedFruitPick();
    EXPECT_EQ(gSpecialVar_Result, 0);
    PolishedFruitUseMulch();
    EXPECT_EQ(gSpecialVar_Result, FALSE);
    for (i = 0; i < ARRAY_COUNT(vars); i++)
        VarSet(vars[i], saved[i]);
    memcpy(gSaveBlock1Ptr->bag.berries, berries, sizeof(berries));
    memcpy(gSaveBlock1Ptr->bag.items, items, sizeof(items));
    gSaveBlock2Ptr->localTimeOffset = clock;
    gSpecialVar_0x8005 = selected;
    RtcCalcLocalTime();
}

TEST("Route 30 doors are reciprocal and only the outdoor map uses source time palettes")
{
    const u16 maps[] = {MAP_POLISHED_ROUTE_30, MAP_POLISHED_ROUTE_30_BERRY_SPEECH_HOUSE, MAP_POLISHED_MR_POKEMONS_HOUSE};
    struct MapHeader original = gMapHeader;
    u32 i, j, count = 0;
    for (i = 0; i < ARRAY_COUNT(maps); i++)
    {
        const struct MapHeader *map = Overworld_GetMapHeaderByGroupAndId(MAP_GROUP(maps[i]), MAP_NUM(maps[i]));
        gMapHeader = *map;
        EXPECT_EQ(PolishedJohtoIsRegisteredMapLayout(map->mapLayout), i == 0);
        for (j = 0; j < map->events->warpCount; j++)
        {
            const struct WarpEvent *warp = &map->events->warps[j];
            const struct MapHeader *dest = Overworld_GetMapHeaderByGroupAndId(warp->mapGroup, warp->mapNum);
            EXPECT(warp->warpId >= 0 && warp->warpId < dest->events->warpCount);
            const struct WarpEvent *back = &dest->events->warps[warp->warpId];
            EXPECT_EQ(back->mapGroup, MAP_GROUP(maps[i]));
            EXPECT_EQ(back->mapNum, MAP_NUM(maps[i]));
            EXPECT_EQ(dest->mapLayout->map[back->y * dest->mapLayout->width + back->x] & 0xFC00, 0x3000);
            count++;
        }
    }
    EXPECT_EQ(count, 6);
    gMapHeader = original;
}
