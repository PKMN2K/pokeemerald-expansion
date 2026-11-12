#include "global.h"
#include "polished_tm.h"
#include "constants/items.h"
#include "constants/pokeball.h"
#include "data/polished_tm_rules.h"

void PolishedTMFriendship(struct Pokemon *mon)
{
    if (!GetMonData(mon, MON_DATA_SPECIES) || GetMonData(mon, MON_DATA_IS_EGG))
        return;
    u32 friendship = GetMonData(mon, MON_DATA_FRIENDSHIP);
    u32 tier = friendship >= POLISHED_TM_FRIENDSHIP_HIGH ? 2
        : friendship >= POLISHED_TM_FRIENDSHIP_LOW ? 1 : 0;
    friendship += sPolishedTMFriendship[tier];
    // Source bonuses are additive, including when the base change is zero.
    if (GetMonData(mon, MON_DATA_HELD_ITEM) == ITEM_SOOTHE_BELL)
        friendship++;
    if (GetMonData(mon, MON_DATA_POKEBALL) == BALL_LUXURY)
        friendship++;
    friendship = min(friendship, MAX_FRIENDSHIP);
    SetMonData(mon, MON_DATA_FRIENDSHIP, &friendship);
}
