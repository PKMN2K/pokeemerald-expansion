#include "global.h"
#include "polished_sprout.h"

u32 PolishedSproutLandSlot(u32 roll)
{
    // Unlike the early routes, the source's final 3% and 2% slots have
    // different levels. Preserve both in the native twelve-slot table.
    static const u8 limits[] = {20, 40, 50, 60, 70, 80, 85, 90, 95, 98, 99};
    u32 slot;
    roll %= 100;
    for (slot = 0; slot < ARRAY_COUNT(limits); slot++)
        if (roll < limits[slot])
            break;
    return slot;
}
