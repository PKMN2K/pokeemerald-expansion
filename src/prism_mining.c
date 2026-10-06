#include "global.h"
#include "overworld.h"
#include "prism_mining.h"
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
    // Ensure map-load palette selection uses the current RTC period rather than
    // whatever gTimeOfDay happened to contain before the warp.
    UpdateTimeOfDay(FALSE);

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


const struct Tileset *ResolvePrismOlcanTilesetForCurrentTime(const struct Tileset *tileset)
{
    if (tileset == &gTileset_PrismOlcanIsleMorning
     || tileset == &gTileset_PrismOlcanIsleDay
     || tileset == &gTileset_PrismOlcanIsleNight)
        return GetPrismOlcanIsleTilesetForCurrentTime();

    if (tileset == &gTileset_PrismOlcanChineMorning
     || tileset == &gTileset_PrismOlcanChineDay
     || tileset == &gTileset_PrismOlcanChineNight)
        return GetPrismOlcanChineTilesetForCurrentTime();

    return tileset;
}


bool32 IsPrismOlcanTileset(const struct Tileset *tileset)
{
    return tileset == &gTileset_PrismOlcanIsleMorning
        || tileset == &gTileset_PrismOlcanIsleDay
        || tileset == &gTileset_PrismOlcanIsleNight
        || tileset == &gTileset_PrismOlcanChineMorning
        || tileset == &gTileset_PrismOlcanChineDay
        || tileset == &gTileset_PrismOlcanChineNight;
}

// Identifies the complete, intentionally limited PKMN 2K Prism mining
// secondary-tileset pool. Keep this centralized so later map/debug logic does
// not need to duplicate the approved terrain list.
bool32 IsPrismMiningTileset(const struct Tileset *tileset)
{
    return tileset == &gTileset_PrismMoundCave
        || tileset == &gTileset_PrismFirelightCaverns
        || tileset == &gTileset_PrismKantoCave
        || IsPrismOlcanTileset(tileset);
}

bool32 CurrentMapUsesPrismMiningTileset(void)
{
    if (gMapHeader.mapLayout == NULL)
        return FALSE;

    return IsPrismMiningTileset(gMapHeader.mapLayout->secondaryTileset);
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
