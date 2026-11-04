#include "global.h"
#include "overworld.h"
#include "fieldmap.h"
#include "polished_johto_palette.h"
#include "tilesets.h"
#include "constants/map_groups.h"
#include "constants/layouts.h"
#include "constants/metatile_behaviors.h"
#include "test/test.h"

extern const u32 gObjectEventPic_PolishedChrisNormalRunning[];
extern const u32 gObjectEventPic_PolishedPokefan[];

TEST("Johto horizontal walking sheets contain complete visible 16x32 frames")
{
    const u8 *graphics = (const u8 *)gObjectEventPic_PolishedChrisNormalRunning;
    u32 frames = 18;
    PARAMETRIZE { graphics = (const u8 *)gObjectEventPic_PolishedChrisNormalRunning; frames = 18; }
    PARAMETRIZE { graphics = (const u8 *)gObjectEventPic_PolishedPokefan; frames = 9; }
    for (u32 frame = 0; frame < frames; frame++)
    {
        u32 visibleBytes = 0;
        for (u32 i = 0; i < 128; i++)
        {
            EXPECT_EQ(graphics[frame * 256 + i], 0);
            visibleBytes += graphics[frame * 256 + 128 + i] != 0;
        }
        EXPECT(visibleBytes > 0);
    }
}

TEST("Johto demo maps survive region filtering and have reciprocal door warps")
{
    const struct MapHeader *outdoor = Overworld_GetMapHeaderByGroupAndId(MAP_GROUP(MAP_POLISHED_JOHTO_DEMO_OUTDOOR), MAP_NUM(MAP_POLISHED_JOHTO_DEMO_OUTDOOR));
    const struct MapHeader *lab = Overworld_GetMapHeaderByGroupAndId(MAP_GROUP(MAP_POLISHED_JOHTO_DEMO_LAB), MAP_NUM(MAP_POLISHED_JOHTO_DEMO_LAB));
    EXPECT(outdoor != NULL);
    EXPECT(lab != NULL);
    if (outdoor == NULL || lab == NULL)
        return;
    EXPECT_EQ(outdoor->mapLayoutId, LAYOUT_POLISHED_JOHTO_DEMO_OUTDOOR);
    EXPECT_EQ(lab->mapLayoutId, LAYOUT_POLISHED_JOHTO_DEMO_LAB);
    EXPECT_EQ(outdoor->mapLayout->width, 24);
    EXPECT_EQ(outdoor->mapLayout->height, 20);
    EXPECT_EQ(lab->mapLayout->width, 10);
    EXPECT_EQ(lab->mapLayout->height, 12);
    EXPECT_EQ(outdoor->events->warpCount, 1);
    EXPECT_EQ(lab->events->warpCount, 2);
    EXPECT_EQ(outdoor->events->warps[0].mapGroup, MAP_GROUP(MAP_POLISHED_JOHTO_DEMO_LAB));
    EXPECT_EQ(outdoor->events->warps[0].mapNum, MAP_NUM(MAP_POLISHED_JOHTO_DEMO_LAB));
    EXPECT_EQ(lab->events->warps[0].mapNum, MAP_NUM(MAP_POLISHED_JOHTO_DEMO_OUTDOOR));
    EXPECT_EQ(lab->events->warps[1].warpId, 0);
    EXPECT_EQ(GetNumMetatilesInPrimary(outdoor->mapLayout), 512);
    EXPECT_EQ(GetNumPalsInPrimary(outdoor->mapLayout), 6);
}

TEST("Johto live palette registration requires layout and both exact tilesets")
{
    struct MapHeader saved = gMapHeader;
    struct MapLayout substitute;
    const struct MapHeader *outdoor = Overworld_GetMapHeaderByGroupAndId(MAP_GROUP(MAP_POLISHED_JOHTO_DEMO_OUTDOOR), MAP_NUM(MAP_POLISHED_JOHTO_DEMO_OUTDOOR));
    EXPECT(outdoor != NULL);
    if (outdoor == NULL)
        return;
    gMapHeader = *outdoor;
    EXPECT(PolishedJohtoIsRegisteredMapLayout(gMapHeader.mapLayout));
    EXPECT(!PolishedJohtoIsRegisteredMapLayout(NULL));
    substitute = *gMapHeader.mapLayout;
    EXPECT(!PolishedJohtoIsRegisteredMapLayout(&substitute));
    gMapHeader.mapLayout = &substitute;
    substitute.secondaryTileset = &gTileset_PolishedJohtoDemoLabSecondary;
    EXPECT(!PolishedJohtoIsRegisteredMapLayout(&substitute));
    substitute = *outdoor->mapLayout;
    substitute.primaryTileset = &gTileset_PolishedJohtoDemoLabPrimary;
    EXPECT(!PolishedJohtoIsRegisteredMapLayout(&substitute));
    gMapHeader = *Overworld_GetMapHeaderByGroupAndId(MAP_GROUP(MAP_ROUTE101), MAP_NUM(MAP_ROUTE101));
    EXPECT(!PolishedJohtoIsRegisteredMapLayout(gMapHeader.mapLayout));
    gMapHeader = *Overworld_GetMapHeaderByGroupAndId(MAP_GROUP(MAP_POLISHED_JOHTO_DEMO_LAB), MAP_NUM(MAP_POLISHED_JOHTO_DEMO_LAB));
    EXPECT(!PolishedJohtoIsRegisteredMapLayout(gMapHeader.mapLayout));
    gMapHeader = saved;
}
