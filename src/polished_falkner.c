#include "global.h"
#include "daycare.h"
#include "event_data.h"
#include "overworld.h"
#include "pokemon.h"
#include "polished_falkner.h"
#include "constants/flags.h"
#include "constants/vars.h"

u16 PolishedFalknerFlashLevel(void)
{
    return GetFlashLevel();
}

void PolishedFalknerPrepareBadges(void)
{
#if POLISHED_JOHTO_DEMO
    // Earlier demo saves gave the fifth badge solely to demonstrate Surf.
    // Field-move access now handles that separately; retain legitimately earned badges.
    if (!(VarGet(VAR_POLISHED_EARNED_BADGES) & (1 << 4)))
        FlagClear(FLAG_BADGE05_GET);
#endif
}

void PolishedFalknerAwardBadge(void)
{
    PolishedFalknerPrepareBadges();
    FlagSet(FLAG_POLISHED_ZEPHYR_BADGE);
    FlagSet(FLAG_BADGE01_GET);
    VarSet(VAR_POLISHED_EARNED_BADGES, VarGet(VAR_POLISHED_EARNED_BADGES) | 1);
}

u16 PolishedFalknerGiveEgg(void)
{
    struct Pokemon egg;
    u16 result;
    if (!FlagGet(FLAG_POLISHED_EGG_CALL) || !FlagGet(FLAG_POLISHED_ZEPHYR_BADGE)
        || FlagGet(FLAG_POLISHED_EGG_GIVEN))
        return MON_CANT_GIVE;

    CreateEgg(&egg, SPECIES_TOGEPI, TRUE);
    result = GiveCapturedMonToPlayer(&egg);
    if (result != MON_CANT_GIVE)
    {
        FlagSet(FLAG_POLISHED_EGG_GIVEN);
        FlagClear(FLAG_POLISHED_AIDE_IN_VIOLET);
    }
    return result;
}
