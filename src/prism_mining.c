#include "global.h"
#include "constants/items.h"

// PKMN 2K mining eligibility hook.
//
// Mining is currently available on every MB_PRISM_MINING surface. Keep the
// decision here so a later design can require a key item, Trainer Skill,
// quest flag, or other project-specific condition without touching map art or
// the interaction script.
bool32 MiningCanInteract(void)
{
    return TRUE;
}

// PKMN 2K mining reward hook.
//
// The Prism reward table is intentionally not retained. This function is a
// project-owned extension point for the custom mining loot table. Until that
// table is defined, mining returns ITEM_NONE rather than inheriting Prism's
// probabilities or substituting guessed rewards.
u16 MiningRollCustomReward(void)
{
    return ITEM_NONE;
}
