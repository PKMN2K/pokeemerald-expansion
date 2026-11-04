#include "global.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "fieldmap.h"
#include "heal_location.h"
#include "item.h"
#include "overworld.h"
#include "polished_route29.h"
#include "rtc.h"
#include "wild_encounter.h"
#include "constants/heal_locations.h"
#include "constants/items.h"
#include "constants/maps.h"
#include "constants/layouts.h"
#include "constants/vars.h"
#include "test/test.h"

TEST("Route 29 ledges jump from the standing tile onto walkable, unoccupied ground")
{
    struct MapHeader oldMap = gMapHeader;
    struct BackupMapLayout oldGrid = gBackupMapLayout;
    struct ObjectEvent oldObject = gObjectEvents[OBJECT_EVENTS_COUNT - 1];
    gMapHeader = *Overworld_GetMapHeaderByGroupAndId(MAP_GROUP(MAP_POLISHED_ROUTE_29), MAP_NUM(MAP_POLISHED_ROUTE_29));
    // Read the production map directly, without the renderer's seven-cell border.
    gBackupMapLayout.width = gMapHeader.mapLayout->width;
    gBackupMapLayout.height = gMapHeader.mapLayout->height;
    gBackupMapLayout.map = (u16 *)gMapHeader.mapLayout->map;
    EXPECT_EQ(GetLedgeJumpDirection(12, 4, DIR_SOUTH), DIR_NONE);
    EXPECT_EQ(GetLedgeJumpDirection(12, 5, DIR_SOUTH), DIR_SOUTH);
    EXPECT_EQ(GetLedgeJumpDirection(12, 5, DIR_NORTH), DIR_NONE);
    EXPECT_EQ(GetLedgeJumpDirection(14, 5, DIR_SOUTH), DIR_SOUTH);
    EXPECT_EQ(GetLedgeJumpDirection(8, 6, DIR_WEST), DIR_WEST);
    gObjectEvents[OBJECT_EVENTS_COUNT - 1].active = TRUE;
    gObjectEvents[OBJECT_EVENTS_COUNT - 1].currentCoords.x = 12;
    gObjectEvents[OBJECT_EVENTS_COUNT - 1].currentCoords.y = 6;
    EXPECT_EQ(GetLedgeJumpDirection(12, 5, DIR_SOUTH), DIR_NONE);
    gObjectEvents[OBJECT_EVENTS_COUNT - 1] = oldObject;
    // Outside this imported map the native next-tile convention is unchanged.
    gMapHeader.mapLayoutId = LAYOUT_POLISHED_NEW_BARK_TOWN;
    EXPECT_EQ(GetLedgeJumpDirection(12, 4, DIR_SOUTH), DIR_SOUTH);
    gMapHeader = oldMap;
    gBackupMapLayout = oldGrid;
}

TEST("Route 29 preserves all clock boundaries and the source evening mixture")
{
    u32 hour, roll;
    for (hour = 0; hour < 24; hour++)
    {
        u32 day = 0, night = 0, morning = 0;
        for (roll = 0; roll < 256; roll++)
        {
            enum TimeOfDay t = PolishedRoute29EncounterTime(hour, roll);
            day += t == TIME_DAY;
            night += t == TIME_NIGHT;
            morning += t == TIME_MORNING;
        }
        EXPECT_EQ(morning, hour >= 5 && hour < 9 ? 256 : 0);
        EXPECT_EQ(day, hour >= 9 && hour < 17 ? 256 : hour >= 17 && hour < 21 ? 153 : 0);
        EXPECT_EQ(night, hour < 5 || hour >= 21 ? 256 : hour >= 17 && hour < 21 ? 103 : 0);
    }
}

TEST("Route 29 berry harvesting survives full pockets, fallback quantities and clock reversal")
{
    struct Time originalClock = gSaveBlock2Ptr->localTimeOffset;
    struct ItemSlot original[BAG_BERRIES_COUNT];
    u16 originalDay = VarGet(VAR_POLISHED_ROUTE29_FRUIT_DAY);
    u16 originalGfx = VarGet(VAR_OBJ_GFX_ID_F);
    u32 i;
    memcpy(original, gSaveBlock1Ptr->bag.berries, sizeof(original));
    RtcCalcLocalTimeOffset(10, 12, 0, 0);
    VarSet(VAR_POLISHED_ROUTE29_FRUIT_DAY, 0);
    for (i = 0; i < BAG_BERRIES_COUNT; i++)
        BagPocket_SetSlotItemIdAndCount(&gBagPockets[POCKET_BERRIES], i, ITEM_ORAN_BERRY, MAX_BAG_ITEM_CAPACITY);
    EXPECT(PolishedRoute29FruitAvailable());
    PolishedRoute29PickFruit();
    EXPECT_EQ(gSpecialVar_Result, 0);
    EXPECT_EQ(VarGet(VAR_POLISHED_ROUTE29_FRUIT_DAY), 0);
    // Exactly one free place: every random 1-3 result must successfully fit one.
    BagPocket_SetSlotItemIdAndCount(&gBagPockets[POCKET_BERRIES], 0, ITEM_ORAN_BERRY, MAX_BAG_ITEM_CAPACITY - 1);
    PolishedRoute29PickFruit();
    EXPECT_EQ(gSpecialVar_Result, 1);
    EXPECT_EQ(VarGet(VAR_POLISHED_ROUTE29_FRUIT_DAY), 11);
    EXPECT(!PolishedRoute29FruitAvailable());
    PolishedRoute29PickFruit();
    EXPECT_EQ(gSpecialVar_Result, 0);
    RtcCalcLocalTimeOffset(9, 12, 0, 0);
    EXPECT(!PolishedRoute29FruitAvailable());
    RtcCalcLocalTimeOffset(11, 0, 0, 0);
    EXPECT(PolishedRoute29FruitAvailable());
    memcpy(gSaveBlock1Ptr->bag.berries, original, sizeof(original));
    gSaveBlock2Ptr->localTimeOffset = originalClock;
    VarSet(VAR_POLISHED_ROUTE29_FRUIT_DAY, originalDay);
    VarSet(VAR_OBJ_GFX_ID_F, originalGfx);
    RtcCalcLocalTime();
}

TEST("Route 29 blackout returns to a walkable Johto recovery point without a Hoenn healer")
{
    struct WarpData saved = gSaveBlock1Ptr->lastHealLocation;
    struct WarpData warp;
    SetLastHealLocationWarp(HEAL_LOCATION_POLISHED_NEW_BARK_TOWN);
    EXPECT_EQ(GetHealLocationIndexByWarpData(&gSaveBlock1Ptr->lastHealLocation), HEAL_LOCATION_POLISHED_NEW_BARK_TOWN);
    EXPECT_EQ(GetHealNpcLocalId(HEAL_LOCATION_POLISHED_NEW_BARK_TOWN), 0);
    SetWhiteoutRespawnWarpAndHealerNPC(&warp);
    EXPECT_EQ(warp.mapGroup, MAP_GROUP(MAP_POLISHED_NEW_BARK_TOWN));
    EXPECT_EQ(warp.mapNum, MAP_NUM(MAP_POLISHED_NEW_BARK_TOWN));
    EXPECT_EQ(warp.x, 6);
    EXPECT_EQ(warp.y, 5);
    const struct MapHeader *town = Overworld_GetMapHeaderByGroupAndId(warp.mapGroup, warp.mapNum);
    EXPECT_EQ(town->mapLayout->map[warp.y * town->mapLayout->width + warp.x] & 0xC00, 0);
    gSaveBlock1Ptr->lastHealLocation = saved;
}
