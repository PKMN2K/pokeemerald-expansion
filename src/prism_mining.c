#include "global.h"
#include "overworld.h"
#include "tilesets.h"
#include "constants/rtc.h"
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

// PKMN 2K mining attempt-cost hook.
//
// This is intentionally a no-op until the project decides whether mining
// spends trainer stamina, consumes a tool, reduces durability, or has no cost.
// It runs only after MiningCanInteract() succeeds.
void MiningApplyAttemptCost(void)
{
}


static const struct Tileset *GetPrismOlcanTilesetForTimeOfDay(bool32 useChinePalettes)
{
    switch (gTimeOfDay)
    {
    case TIME_MORNING:
        return useChinePalettes
            ? &gTileset_PrismOlcanChineMorning
            : &gTileset_PrismOlcanIsleMorning;
    case TIME_EVENING:
    case TIME_NIGHT:
        // Prism has no separate Evening palette bank.
        return useChinePalettes
            ? &gTileset_PrismOlcanChineNight
            : &gTileset_PrismOlcanIsleNight;
    case TIME_DAY:
    default:
        return useChinePalettes
            ? &gTileset_PrismOlcanChineDay
            : &gTileset_PrismOlcanIsleDay;
    }
}

// Runtime selectors for the already-imported Prism Olcan palette variants.
// They intentionally follow Expansion's current gTimeOfDay state without
// changing the project's global time-of-day configuration.
const struct Tileset *GetPrismOlcanIsleTilesetForCurrentTime(void)
{
    return GetPrismOlcanTilesetForTimeOfDay(FALSE);
}

const struct Tileset *GetPrismOlcanChineTilesetForCurrentTime(void)
{
    return GetPrismOlcanTilesetForTimeOfDay(TRUE);
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
