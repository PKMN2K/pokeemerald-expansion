#include "global.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "item.h"
#include "polished_route29.h"
#include "random.h"
#include "string_util.h"
#include "constants/event_objects.h"
#include "constants/items.h"
#include "constants/map_event_ids.h"
#include "constants/maps.h"
#include "constants/vars.h"

enum TimeOfDay PolishedRoute29EncounterTime(u32 hour, u32 roll)
{
    // Source MORN=5, DAY=9, EVE=17, NITE=21. GetTimeOfDayNotEve
    // compares an 8-bit random value with `60 percent` (153).
    if (hour < 5 || hour >= 21)
        return TIME_NIGHT;
    if (hour < 9)
        return TIME_MORNING;
    if (hour < 17 || (roll & 255) < 153)
        return TIME_DAY;
    return TIME_NIGHT;
}

static u32 FruitDay(void)
{
    RtcCalcLocalTime();
    return gLocalTime.days < 0 ? 0 : gLocalTime.days;
}

bool32 PolishedRoute29FruitAvailable(void)
{
    // Zero is unpicked. Day+1 distinguishes the first harvest on day zero.
    // Moving the clock backwards cannot grant an extra daily harvest.
    return FruitDay() >= VarGet(VAR_POLISHED_ROUTE29_FRUIT_DAY);
}

void PolishedRoute29RefreshFruit(void)
{
    u8 id;
    u16 graphics = PolishedRoute29FruitAvailable() ? OBJ_EVENT_GFX_POLISHED_FRUIT : OBJ_EVENT_GFX_POLISHED_FRUIT_PICKED;
    VarSet(VAR_OBJ_GFX_ID_F, graphics);
    if (!TryGetObjectEventIdByLocalIdAndMap(LOCALID_ROUTE29_FRUIT, MAP_NUM(MAP_POLISHED_ROUTE_29), MAP_GROUP(MAP_POLISHED_ROUTE_29), &id)
        && gObjectEvents[id].graphicsId != graphics)
        ObjectEventSetGraphicsId(&gObjectEvents[id], graphics);
}

void PolishedRoute29PickFruit(void)
{
    u32 quantity;
    gSpecialVar_Result = 0;
    if (!PolishedRoute29FruitAvailable())
        return;
    // Like the source, fall back 3->2->1 without consuming a tree on failure.
    for (quantity = Random() % 3 + 1; quantity; quantity--)
    {
        if (AddBagItem(ITEM_ORAN_BERRY, quantity))
        {
            VarSet(VAR_POLISHED_ROUTE29_FRUIT_DAY, FruitDay() + 1);
            CopyItemNameHandlePlural(ITEM_ORAN_BERRY, gStringVar3, quantity);
            gSpecialVar_Result = quantity;
            PolishedRoute29RefreshFruit();
            return;
        }
    }
}
