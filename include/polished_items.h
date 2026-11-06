#ifndef GUARD_POLISHED_ITEMS_H
#define GUARD_POLISHED_ITEMS_H

#include "pokemon.h"

struct PolishedWildHeldItems
{
    u16 common, rare;
    bool8 alwaysRare;
};

const struct PolishedWildHeldItems *PolishedGetWildHeldItems(enum Species species);
// FALSE means use native data; TRUE includes an authoritative ITEM_NONE.
bool32 PolishedGenerateWildHeldItem(enum Species species, bool32 legendary, enum Item *item);

#endif
