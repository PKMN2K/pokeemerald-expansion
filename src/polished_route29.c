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

static const struct PolishedFruitTree
{
    u16 dayVar, item, map, localId, gfxVar, fullGfx, emptyGfx;
} sFruitTrees[] =
{
    {VAR_POLISHED_ROUTE29_FRUIT_DAY, ITEM_ORAN_BERRY, MAP_POLISHED_ROUTE_29, LOCALID_ROUTE29_FRUIT,
     VAR_OBJ_GFX_ID_F, OBJ_EVENT_GFX_POLISHED_FRUIT, OBJ_EVENT_GFX_POLISHED_FRUIT_PICKED},
    {VAR_POLISHED_ROUTE30_ORAN_DAY, ITEM_ORAN_BERRY, MAP_POLISHED_ROUTE_30, LOCALID_ROUTE30_ORAN,
     VAR_OBJ_GFX_ID_E, OBJ_EVENT_GFX_POLISHED_FRUIT, OBJ_EVENT_GFX_POLISHED_FRUIT_PICKED},
    {VAR_POLISHED_ROUTE30_PECHA_DAY, ITEM_PECHA_BERRY, MAP_POLISHED_ROUTE_30, LOCALID_ROUTE30_PECHA,
     VAR_OBJ_GFX_ID_D, OBJ_EVENT_GFX_POLISHED_FRUIT_PINK, OBJ_EVENT_GFX_POLISHED_FRUIT_PICKED_PINK},
};

static bool32 FruitAvailable(u32 index)
{
    // Zero is unpicked. Day+1 distinguishes the first harvest on day zero.
    // Moving the clock backwards cannot grant an extra daily harvest.
    return FruitDay() >= VarGet(sFruitTrees[index].dayVar);
}

void PolishedFruitRefreshAll(void)
{
    u32 i;
    for (i = 0; i < ARRAY_COUNT(sFruitTrees); i++)
    {
        const struct PolishedFruitTree *tree = &sFruitTrees[i];
        u8 id;
        u16 graphics = FruitAvailable(i) ? tree->fullGfx : tree->emptyGfx;
        VarSet(tree->gfxVar, graphics);
        if (!TryGetObjectEventIdByLocalIdAndMap(tree->localId, MAP_NUM(tree->map), MAP_GROUP(tree->map), &id)
            && gObjectEvents[id].graphicsId != graphics)
            ObjectEventSetGraphicsId(&gObjectEvents[id], graphics);
    }
}

static void PickFruit(u32 index)
{
    u32 quantity;
    const struct PolishedFruitTree *tree = &sFruitTrees[index];
    gSpecialVar_Result = 0;
    if (!FruitAvailable(index))
        return;
    // Like the source, fall back 3->2->1 without consuming a tree on failure.
    for (quantity = Random() % 3 + 1; quantity; quantity--)
    {
        if (AddBagItem(tree->item, quantity))
        {
            VarSet(tree->dayVar, FruitDay() + 1);
            CopyItemNameHandlePlural(tree->item, gStringVar3, quantity);
            gSpecialVar_Result = quantity;
            PolishedFruitRefreshAll();
            return;
        }
    }
}

bool32 PolishedRoute29FruitAvailable(void) { return FruitAvailable(0); }
void PolishedRoute29RefreshFruit(void) { PolishedFruitRefreshAll(); }
void PolishedRoute29PickFruit(void) { PickFruit(0); }

bool32 PolishedFruitAvailable(void)
{
    return gSpecialVar_0x8005 < ARRAY_COUNT(sFruitTrees) && FruitAvailable(gSpecialVar_0x8005);
}

void PolishedFruitPrepare(void)
{
    gSpecialVar_0x8006 = gSpecialVar_0x8005 < ARRAY_COUNT(sFruitTrees) ? sFruitTrees[gSpecialVar_0x8005].item : ITEM_NONE;
    PolishedFruitRefreshAll();
}

void PolishedFruitPick(void)
{
    gSpecialVar_Result = 0;
    if (gSpecialVar_0x8005 < ARRAY_COUNT(sFruitTrees))
        PickFruit(gSpecialVar_0x8005);
}

void PolishedFruitUseMulch(void)
{
    gSpecialVar_Result = FALSE;
    if (gSpecialVar_0x8005 >= ARRAY_COUNT(sFruitTrees) || FruitAvailable(gSpecialVar_0x8005))
        return;
    // Source FertilizedFruitTree clears only this picked flag. Consume one
    // item only when a picked tree can actually be replenished.
    if (RemoveBagItem(ITEM_POLISHED_MULCH, 1))
    {
        VarSet(sFruitTrees[gSpecialVar_0x8005].dayVar, 0);
        gSpecialVar_Result = TRUE;
        PolishedFruitRefreshAll();
    }
}
