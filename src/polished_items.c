#include "global.h"
#include "battle.h"
#include "battle_util.h"
#include "config_changes.h"
#include "main.h"
#include "polished_items.h"
#include "random.h"

#include "data/polished_wild_held_items.h"

struct PolishedItemHealing
{
    u16 amount;
    u8 divisor;
};

#include "data/polished_item_healing.h"

struct PolishedMedicineFriendship
{
    u16 item;
    s8 changes[3];
};

#include "data/polished_medicine_friendship.h"

bool32 PolishedApplyMedicineFriendshipInternal(struct Pokemon *mon, enum Item item)
{
    for (u32 i = 0; i < ARRAY_COUNT(sPolishedMedicineFriendship); i++)
    {
        const struct PolishedMedicineFriendship *data = &sPolishedMedicineFriendship[i];
        if (data->item != item)
            continue;
        if (GetMonData(mon, MON_DATA_IS_EGG))
            return TRUE;

        s32 friendship = GetMonData(mon, MON_DATA_FRIENDSHIP);
        u32 column = (friendship >= POLISHED_MEDICINE_FRIENDSHIP_LOW)
                   + (friendship >= POLISHED_MEDICINE_FRIENDSHIP_HIGH);
        // The source's negative branch never calls GetExtraHappiness.
        friendship = max(0, friendship + data->changes[column]);
        SetMonData(mon, MON_DATA_FRIENDSHIP, &friendship);

        // Keep Return/Frustration's live value synchronized for this exact
        // party member, including the other active slot in a double battle.
        if (gMain.inBattle)
        {
            for (enum BattlerId battler = 0; battler < gBattlersCount; battler++)
                if (GetBattlerMon(battler) == mon)
                    gBattleMons[battler].friendship = friendship;
        }
        return TRUE;
    }
    return FALSE;
}

u32 PolishedGetItemHealAmountInternal(enum Item item, u32 maxHP)
{
    if ((u32)item >= ITEMS_COUNT)
        return 0;
    const struct PolishedItemHealing *data = &sPolishedItemHealing[item];
    if (data->divisor != 0)
        return max(1, maxHP / data->divisor);
    return data->amount;
}

bool32 PolishedFigyCanHeal(u32 hp, u32 maxHP, bool32 gluttony, bool32 forced)
{
    if (hp == 0 || maxHP == 0)
        return FALSE;
    // StealHPHealingItem/ReconsumeHPHealingItem bypass the pinch check but
    // explicitly reject full HP. Normal consumption uses FloorBC's minimum 1.
    if (forced)
        return hp < maxHP;
    return hp <= max(1, maxHP / (gluttony ? 2 : 4));
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
