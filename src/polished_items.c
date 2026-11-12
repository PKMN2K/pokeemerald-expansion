#include "global.h"
#include "config_changes.h"
#include "polished_items.h"
#include "random.h"

#include "data/polished_wild_held_items.h"

struct PolishedItemHealing
{
    u16 amount;
    u8 divisor;
};

#include "data/polished_item_healing.h"

u32 PolishedGetItemHealAmountInternal(enum Item item, u32 maxHP)
{
    if ((u32)item >= ITEMS_COUNT)
        return 0;
    const struct PolishedItemHealing *data = &sPolishedItemHealing[item];
    if (data->divisor != 0)
        return max(1, maxHP / data->divisor);
    return data->amount;
}

const struct PolishedWildHeldItems *PolishedGetWildHeldItems(enum Species species)
{
    return (u32)species < NUM_SPECIES ? sPolishedWildHeldItems[species] : NULL;
}

static bool32 LeadBoostsHeldItems(void)
{
    struct Pokemon *lead = &gParties[B_TRAINER_PLAYER][0];
    // The default source checks the physical lead's item even when its
    // ability is unavailable. Fainting does not disable either boost.
    if (GetMonData(lead, MON_DATA_HELD_ITEM) == ITEM_AMULET_COIN)
        return TRUE;
    if (GetMonData(lead, MON_DATA_SPECIES) == SPECIES_NONE
        || GetMonData(lead, MON_DATA_SANITY_IS_EGG))
        return FALSE;
    enum Ability ability = GetMonAbility(lead);
    return ability == ABILITY_COMPOUND_EYES || ability == ABILITY_SUPER_LUCK;
}

bool32 PolishedGenerateWildHeldItem(enum Species species, bool32 legendary, enum Item *item)
{
    const struct PolishedWildHeldItems *data = PolishedGetWildHeldItems(species);
    if (!GetConfig(POLISHED_WILD_HELD_ITEMS) || data == NULL)
        return FALSE;
    if (data->alwaysRare)
        *item = data->rare;
    else if (legendary)
        *item = data->common;
    else
    {
        bool32 boosted = LeadBoostsHeldItems();
        // LoadEnemyWildmon uses two independent, short-circuited byte
        // rolls. RGBDS's percent macro multiplies by 255, not 256.
        if (RandomUniform(RNG_POLISHED_HELD_ITEM_COMMON, 0, 255)
            < (boosted ? POLISHED_HELD_BOOST_COMMON : POLISHED_HELD_COMMON))
            *item = data->common;
        else if (RandomUniform(RNG_POLISHED_HELD_ITEM_RARE, 0, 255)
            < (boosted ? POLISHED_HELD_BOOST_RARE : POLISHED_HELD_RARE))
            *item = data->rare;
        else
            *item = ITEM_NONE;
    }
    return TRUE;
}
