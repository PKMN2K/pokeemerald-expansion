#include "global.h"
#include "heal_location.h"
#include "overworld.h"
#include "polished_johto_palette.h"
#include "constants/heal_locations.h"
#include "constants/maps.h"
#include "test/test.h"

TEST("Violet City uses the four Crystal clock periods at their original boundaries")
{
    static const enum TimeOfDay times[] =
    {
        TIME_NIGHT, TIME_NIGHT, TIME_NIGHT, TIME_NIGHT, TIME_NIGHT,
        TIME_MORNING, TIME_MORNING, TIME_MORNING, TIME_MORNING,
        TIME_DAY, TIME_DAY, TIME_DAY, TIME_DAY, TIME_DAY, TIME_DAY, TIME_DAY, TIME_DAY,
        TIME_EVENING, TIME_EVENING, TIME_EVENING, TIME_EVENING,
        TIME_NIGHT, TIME_NIGHT, TIME_NIGHT,
    };
    for (u32 hour = 0; hour < ARRAY_COUNT(times); ++hour)
        EXPECT_EQ(PolishedJohtoTimeFromHour(hour), times[hour]);
}

TEST("Violet City blackout and cartridge Continue have a registered Center landing")
{
    const struct HealLocation *heal = GetHealLocationByMap(MAP_GROUP(MAP_POLISHED_VIOLET_POKECENTER_1F),
                                                           MAP_NUM(MAP_POLISHED_VIOLET_POKECENTER_1F));
    EXPECT(heal != NULL);
    EXPECT_EQ(heal->x, 5);
    EXPECT_EQ(heal->y, 3);
    SetLastHealLocationWarp(HEAL_LOCATION_POLISHED_VIOLET);
    EXPECT_EQ(GetHealLocationIndexByWarpData(&gSaveBlock1Ptr->lastHealLocation), HEAL_LOCATION_POLISHED_VIOLET);
}
