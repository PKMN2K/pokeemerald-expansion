#include "global.h"
#include "overworld.h"
#include "fieldmap.h"
#include "polished_johto_palette.h"
#include "constants/map_groups.h"
#include "constants/layouts.h"
#include "test/test.h"

TEST("New Bark source maps have valid reciprocal warp destinations and walkable landings")
{
    static const u16 mapIds[] = {
        MAP_POLISHED_NEW_BARK_TOWN, MAP_POLISHED_ELMS_LAB,
        MAP_POLISHED_PLAYERS_HOUSE_1F, MAP_POLISHED_PLAYERS_HOUSE_2F,
        MAP_POLISHED_PLAYERS_NEIGHBORS_HOUSE, MAP_POLISHED_LYRAS_HOUSE_1F,
        MAP_POLISHED_LYRAS_HOUSE_2F, MAP_POLISHED_ELMS_HOUSE,
    };
    u32 total = 0;
    for (u32 i = 0; i < ARRAY_COUNT(mapIds); i++)
    {
        u16 mapId = mapIds[i];
        const struct MapHeader *map = Overworld_GetMapHeaderByGroupAndId(MAP_GROUP(mapId), MAP_NUM(mapId));
        EXPECT(map != NULL);
        if (map == NULL)
            return;
        EXPECT_EQ(GetNumMetatilesInPrimary(map->mapLayout), 512);
        for (u32 j = 0; j < map->events->warpCount; j++)
        {
            const struct WarpEvent *warp = &map->events->warps[j];
            const struct MapHeader *destination = Overworld_GetMapHeaderByGroupAndId(warp->mapGroup, warp->mapNum);
            EXPECT(destination != NULL);
            if (destination == NULL)
                return;
            EXPECT(warp->warpId >= 0 && warp->warpId < destination->events->warpCount);
            if (warp->warpId < 0 || warp->warpId >= destination->events->warpCount)
                return;
            const struct WarpEvent *target = &destination->events->warps[warp->warpId];
            EXPECT_EQ(target->mapGroup, MAP_GROUP(mapId));
            EXPECT_EQ(target->mapNum, MAP_NUM(mapId));
            EXPECT(target->x >= 0 && target->x < destination->mapLayout->width);
            EXPECT(target->y >= 0 && target->y < destination->mapLayout->height);
            if (target->x < 0 || target->x >= destination->mapLayout->width
             || target->y < 0 || target->y >= destination->mapLayout->height)
                return;
            u16 cell = destination->mapLayout->map[target->y * destination->mapLayout->width + target->x];
            EXPECT_EQ(cell & 0xC00, 0);
            EXPECT_EQ(cell >> 12, 3);
            total++;
        }
    }
    EXPECT_EQ(total, 19);
}

TEST("New Bark time palettes are limited to the town and exclude every new interior")
{
    struct MapHeader saved = gMapHeader;
    u16 mapId = MAP_POLISHED_NEW_BARK_TOWN;
    bool32 outdoor = TRUE;
    PARAMETRIZE { mapId = MAP_POLISHED_NEW_BARK_TOWN; outdoor = TRUE; }
    PARAMETRIZE { mapId = MAP_POLISHED_ELMS_LAB; outdoor = FALSE; }
    PARAMETRIZE { mapId = MAP_POLISHED_PLAYERS_HOUSE_1F; outdoor = FALSE; }
    PARAMETRIZE { mapId = MAP_POLISHED_PLAYERS_HOUSE_2F; outdoor = FALSE; }
    PARAMETRIZE { mapId = MAP_POLISHED_PLAYERS_NEIGHBORS_HOUSE; outdoor = FALSE; }
    PARAMETRIZE { mapId = MAP_POLISHED_LYRAS_HOUSE_1F; outdoor = FALSE; }
    PARAMETRIZE { mapId = MAP_POLISHED_LYRAS_HOUSE_2F; outdoor = FALSE; }
    PARAMETRIZE { mapId = MAP_POLISHED_ELMS_HOUSE; outdoor = FALSE; }
    const struct MapHeader *map = Overworld_GetMapHeaderByGroupAndId(MAP_GROUP(mapId), MAP_NUM(mapId));
    EXPECT(map != NULL);
    if (map != NULL)
    {
        gMapHeader = *map;
        EXPECT_EQ(PolishedJohtoIsRegisteredMapLayout(gMapHeader.mapLayout), outdoor);
    }
    gMapHeader = saved;
}
