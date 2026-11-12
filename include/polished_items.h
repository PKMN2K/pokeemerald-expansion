#ifndef GUARD_POLISHED_ITEMS_H
#define GUARD_POLISHED_ITEMS_H

#include "pokemon.h"
#include "config_changes.h"

struct PolishedWildHeldItems
{
    u16 common, rare;
    bool8 alwaysRare;
};

const struct PolishedWildHeldItems *PolishedGetWildHeldItems(enum Species species);
// FALSE means use native data; TRUE includes an authoritative ITEM_NONE.
bool32 PolishedGenerateWildHeldItem(enum Species species, bool32 legendary, enum Item *item);
u32 PolishedGetItemHealAmountInternal(enum Item item, u32 maxHP);
// Zero means native fallback. Keep that path free of out-of-line calls in
// native builds, including repeated AI item estimates. Tests can select either
// profile without paying for the generic configuration switch on each lookup.
static ALWAYS_INLINE u32 PolishedGetItemHealAmount(enum Item item, u32 maxHP)
{
    bool32 enabled = P_POLISHED_ITEM_HEALING;
#if TESTING
    if (gConfigChangesTestOverride != NULL)
        enabled = gConfigChangesTestOverride->polishedItemHealing;
#endif
    return enabled ? PolishedGetItemHealAmountInternal(item, maxHP) : 0;
}

#endif
