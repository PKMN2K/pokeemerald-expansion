#include "global.h"
#include "event_data.h"
#include "item.h"
#include "item_use.h"
#include "overworld.h"
#include "constants/flags.h"
#include "constants/items.h"
#include "constants/maps.h"
#include "constants/map_event_ids.h"
#include "test/test.h"

TEST("Academy doors and Earl preserve the existing Violet map IDs")
{
    const struct MapHeader *city = Overworld_GetMapHeaderByGroupAndId(MAP_GROUP(MAP_POLISHED_VIOLET_CITY), MAP_NUM(MAP_POLISHED_VIOLET_CITY));
    const struct MapHeader *school = Overworld_GetMapHeaderByGroupAndId(MAP_GROUP(MAP_POLISHED_EARLS_POKEMON_ACADEMY), MAP_NUM(MAP_POLISHED_EARLS_POKEMON_ACADEMY));
    const struct WarpEvent *door = &city->events->warps[3];
    EXPECT_EQ(city->events->warps[0].mapNum, MAP_NUM(MAP_POLISHED_VIOLET_POKECENTER_1F));
    EXPECT_EQ(city->events->warps[1].mapNum, MAP_NUM(MAP_POLISHED_ROUTE_31_VIOLET_GATE));
    EXPECT_EQ(city->events->warps[2].mapNum, MAP_NUM(MAP_POLISHED_ROUTE_31_VIOLET_GATE));
    EXPECT_EQ(door->x, 30);
    EXPECT_EQ(door->y, 17);
    EXPECT_EQ(door->mapNum, MAP_NUM(MAP_POLISHED_EARLS_POKEMON_ACADEMY));
    EXPECT_EQ(door->warpId, 0);
    EXPECT_EQ(school->events->warpCount, 2);
    for (u32 i = 0; i < school->events->warpCount; ++i)
    {
        EXPECT_EQ(school->events->warps[i].mapNum, MAP_NUM(MAP_POLISHED_VIOLET_CITY));
        EXPECT_EQ(school->events->warps[i].warpId, 3);
    }
    EXPECT_EQ(city->events->objectEvents[LOCALID_VIOLET_EARL-1].flagId, FLAG_POLISHED_EARL_IN_ACADEMY);
}

TEST("Academy Type Chart is a reusable Key Item with a retry after a full pocket")
{
    struct ItemSlot original[BAG_KEYITEMS_COUNT];
    struct BagPocket *pocket = &gBagPockets[POCKET_KEY_ITEMS];
    memcpy(original, gSaveBlock1Ptr->bag.keyItems, sizeof(original));
    for (u32 i = 0; i < ARRAY_COUNT(original); ++i)
        BagPocket_SetSlotItemIdAndCount(pocket, i, ITEM_TOWN_MAP, 1);
    EXPECT_EQ(GetItemPocket(ITEM_POLISHED_TYPE_CHART), POCKET_KEY_ITEMS);
    EXPECT(GetItemFieldFunc(ITEM_POLISHED_TYPE_CHART) == ItemUse_TypeChart);
    EXPECT(!AddBagItem(ITEM_POLISHED_TYPE_CHART, 1));
    BagPocket_SetSlotItemIdAndCount(pocket, 0, ITEM_NONE, 0);
    EXPECT(AddBagItem(ITEM_POLISHED_TYPE_CHART, 1));
    EXPECT(CheckBagHasItem(ITEM_POLISHED_TYPE_CHART, 1));
    memcpy(gSaveBlock1Ptr->bag.keyItems, original, sizeof(original));
}
