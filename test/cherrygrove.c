#include "global.h"
#include "event_data.h"
#include "heal_location.h"
#include "overworld.h"
#include "polished_johto_palette.h"
#include "polished_town_map.h"
#include "constants/heal_locations.h"
#include "constants/event_bg.h"
#include "constants/flags.h"
#include "constants/items.h"
#include "constants/maps.h"
#include "test/test.h"

TEST("Cherrygrove source doors are reciprocal and have walkable native landings")
{
    static const u16 maps[] = {
        MAP_POLISHED_CHERRYGROVE_CITY, MAP_POLISHED_CHERRYGROVE_MART,
        MAP_POLISHED_CHERRYGROVE_POKECENTER_1F, MAP_POLISHED_GUIDE_GENTS_HOUSE,
        MAP_POLISHED_CHERRYGROVE_GYM_SPEECH_HOUSE, MAP_POLISHED_CHERRYGROVE_EVOLUTION_SPEECH_HOUSE,
    };
    u32 i, total = 0;
    for (i = 0; i < ARRAY_COUNT(maps); i++)
    {
        const struct MapHeader *map = Overworld_GetMapHeaderByGroupAndId(MAP_GROUP(maps[i]), MAP_NUM(maps[i]));
        u32 j;
        EXPECT(map != NULL);
        for (j = 0; j < map->events->warpCount; j++)
        {
            const struct WarpEvent *warp = &map->events->warps[j];
            const struct MapHeader *target = Overworld_GetMapHeaderByGroupAndId(warp->mapGroup, warp->mapNum);
            EXPECT(target != NULL);
            EXPECT(warp->warpId >= 0 && warp->warpId < target->events->warpCount);
            const struct WarpEvent *back = &target->events->warps[warp->warpId];
            EXPECT_EQ(back->mapGroup, MAP_GROUP(maps[i]));
            EXPECT_EQ(back->mapNum, MAP_NUM(maps[i]));
            EXPECT_EQ(target->mapLayout->map[back->y * target->mapLayout->width + back->x] & 0xFC00, 0x3000);
            total++;
        }
    }
    EXPECT_EQ(total, 15);
    const struct MapHeader *city = Overworld_GetMapHeaderByGroupAndId(MAP_GROUP(MAP_POLISHED_CHERRYGROVE_CITY), MAP_NUM(MAP_POLISHED_CHERRYGROVE_CITY));
    u32 hiddenItems = 0;
    for (i = 0; i < city->events->bgEventCount; i++)
    {
        const struct BgEvent *event = &city->events->bgEvents[i];
        if (event->kind != BG_EVENT_HIDDEN_ITEM)
            continue;
        EXPECT_EQ(event->x, 35);
        EXPECT_EQ(event->y, 2);
        EXPECT_EQ((u32)event->bgUnion.hiddenItem.item, ITEM_NUGGET);
        EXPECT_EQ(event->bgUnion.hiddenItem.hiddenItemId + FLAG_HIDDEN_ITEMS_START, FLAG_POLISHED_CHERRYGROVE_NUGGET);
        hiddenItems++;
    }
    EXPECT_EQ(hiddenItems, 1);
    const struct MapHeader *center = Overworld_GetMapHeaderByGroupAndId(MAP_GROUP(MAP_POLISHED_CHERRYGROVE_POKECENTER_1F), MAP_NUM(MAP_POLISHED_CHERRYGROVE_POKECENTER_1F));
    EXPECT(center->mapLayout->map[7 * 12] & 0xC00);
}

TEST("Cherrygrove Center blackout stays at its walkable Johto nurse approach")
{
    struct WarpData saved = gSaveBlock1Ptr->lastHealLocation;
    struct WarpData warp;
    SetLastHealLocationWarp(HEAL_LOCATION_POLISHED_CHERRYGROVE);
    EXPECT_EQ(GetHealLocationIndexByWarpData(&gSaveBlock1Ptr->lastHealLocation), HEAL_LOCATION_POLISHED_CHERRYGROVE);
    EXPECT_EQ(GetHealNpcLocalId(HEAL_LOCATION_POLISHED_CHERRYGROVE), 0);
    SetWhiteoutRespawnWarpAndHealerNPC(&warp);
    EXPECT_EQ(warp.mapGroup, MAP_GROUP(MAP_POLISHED_CHERRYGROVE_POKECENTER_1F));
    EXPECT_EQ(warp.mapNum, MAP_NUM(MAP_POLISHED_CHERRYGROVE_POKECENTER_1F));
    EXPECT_EQ(warp.x, 5);
    EXPECT_EQ(warp.y, 3);
    const struct MapHeader *map = Overworld_GetMapHeaderByGroupAndId(warp.mapGroup, warp.mapNum);
    EXPECT_EQ(map->mapLayout->map[warp.y * map->mapLayout->width + warp.x] & 0xFC00, 0x3000);
    gSaveBlock1Ptr->lastHealLocation = saved;
}

TEST("Cherrygrove time palettes exclude fixed interiors and the Johto map excludes other regions")
{
    struct MapHeader saved = gMapHeader;
    struct WarpData location = gSaveBlock1Ptr->location;
    u16 id = MAP_POLISHED_CHERRYGROVE_CITY;
    bool32 outdoor = TRUE, johto = TRUE;
    PARAMETRIZE { id = MAP_POLISHED_CHERRYGROVE_CITY; }
    PARAMETRIZE { id = MAP_POLISHED_CHERRYGROVE_POKECENTER_1F; outdoor = FALSE; }
    PARAMETRIZE { id = MAP_POLISHED_CHERRYGROVE_MART; outdoor = FALSE; }
    PARAMETRIZE { id = MAP_POLISHED_GUIDE_GENTS_HOUSE; outdoor = FALSE; }
    PARAMETRIZE { id = MAP_POLISHED_CHERRYGROVE_GYM_SPEECH_HOUSE; outdoor = FALSE; }
    PARAMETRIZE { id = MAP_POLISHED_CHERRYGROVE_EVOLUTION_SPEECH_HOUSE; outdoor = FALSE; }
    PARAMETRIZE { id = MAP_LITTLEROOT_TOWN; outdoor = FALSE; johto = FALSE; }
    gMapHeader = *Overworld_GetMapHeaderByGroupAndId(MAP_GROUP(id), MAP_NUM(id));
    EXPECT_EQ(PolishedJohtoIsRegisteredMapLayout(gMapHeader.mapLayout), outdoor);
    gSaveBlock1Ptr->location.mapGroup = MAP_GROUP(id);
    gSaveBlock1Ptr->location.mapNum = MAP_NUM(id);
    EXPECT_EQ(PolishedTownMapApplies(), johto);
    gMapHeader = saved;
    gSaveBlock1Ptr->location = location;
}
