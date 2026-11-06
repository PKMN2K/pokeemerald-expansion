#include "global.h"
#include "battle.h"
#include "config_changes.h"
#include "overworld.h"
#include "polished_evolution.h"
#include "polished_johto_palette.h"
#include "rtc.h"
#include "constants/maps.h"

struct PolishedEvolution
{
    u16 method, argument, extra;
    enum Species targetSpecies;
};

struct PolishedEvolutionLocation
{
    u8 mapGroup, mapNum, landmark;
};

#include "data/polished_evolutions.h"

const struct Evolution *PolishedGetSpeciesEvolutions(enum Species species)
{
    if ((u32)species >= NUM_SPECIES)
        return NULL;
    return sPolishedEvolutionMetadata[species];
}

bool32 PolishedEvolutionTimeMatches(u32 period, u32 hour)
{
    enum TimeOfDay time = PolishedJohtoTimeFromHour(hour);
    bool32 morningDay = time == TIME_MORNING || time == TIME_DAY;
    return period == POLISHED_TR_ANYTIME
        || (period == POLISHED_TR_MORNDAY && morningDay)
        || (period == POLISHED_TR_EVENITE && !morningDay);
}

u32 PolishedGetEvolutionLandmark(void)
{
    for (u32 i = 0; sPolishedEvolutionLocations[i].landmark != POLISHED_LANDMARK_NONE; i++)
    {
        if (gSaveBlock1Ptr->location.mapGroup == sPolishedEvolutionLocations[i].mapGroup
         && gSaveBlock1Ptr->location.mapNum == sPolishedEvolutionLocations[i].mapNum)
            return sPolishedEvolutionLocations[i].landmark;
    }
    return POLISHED_LANDMARK_NONE;
}

const u8 *PolishedEvolutionLandmarkName(u32 landmark)
{
    switch (landmark)
    {
    case POLISHED_LANDMARK_MAGNET_TUNNEL: return COMPOUND_STRING("Magnet Tunnel");
    case POLISHED_LANDMARK_DIM_CAVE: return COMPOUND_STRING("Dim Cave");
    case POLISHED_LANDMARK_ILEX_FOREST: return COMPOUND_STRING("Ilex Forest");
    case POLISHED_LANDMARK_ICE_PATH: return COMPOUND_STRING("Ice Path");
    case POLISHED_LANDMARK_SINJOH_RUINS: return COMPOUND_STRING("Sinjoh Ruins");
    case POLISHED_LANDMARK_MYSTRI_STAGE: return COMPOUND_STRING("Mystri Stage");
    case POLISHED_LANDMARK_RUGGED_ROAD: return COMPOUND_STRING("Rugged Road");
    case POLISHED_LANDMARK_SNOWTOP_MOUNTAIN: return COMPOUND_STRING("Snowtop Mountain");
    default: return COMPOUND_STRING("Unknown");
    }
}

enum Species PolishedFindEvolution(struct Pokemon *mon, enum EvolutionMode mode, enum Item item, u32 hour, u32 landmark, u32 criticalHits)
{
    enum Species species = GetMonData(mon, MON_DATA_SPECIES);
    enum Item heldItem = GetMonData(mon, MON_DATA_HELD_ITEM);
    u32 level = GetMonData(mon, MON_DATA_LEVEL);
    bool32 usingItem = mode == EVO_MODE_ITEM_CHECK || mode == EVO_MODE_ITEM_USE;
    bool32 trading = mode == EVO_MODE_TRADE;
    const struct PolishedEvolution *rules;

    if ((u32)species >= NUM_SPECIES || GetMonData(mon, MON_DATA_IS_EGG))
        return SPECIES_NONE;
    rules = sPolishedEvolutionRules[species];
    if (rules == NULL || (mode != EVO_MODE_NORMAL && mode != EVO_MODE_BATTLE_ONLY
        && mode != EVO_MODE_BATTLE_SPECIAL && !usingItem && !trading))
        return SPECIES_NONE;
    // Source flags any party member at three critical hits, then checks its
    // complete ordered evolution list, just as on a level-up.
    if (mode == EVO_MODE_BATTLE_SPECIAL && criticalHits < 3)
        return SPECIES_NONE;
    // EvoStoneEffect performs this guard before calling the source evaluator.
    if (usingItem && heldItem == ITEM_EVERSTONE)
        return SPECIES_NONE;

    for (; rules->method != POLISHED_EVOLVE_END; rules++)
    {
        bool32 matches = FALSE;
        u32 argument = rules->argument;
        if (rules->method == POLISHED_EVOLVE_TRADE)
        {
            // These are also checked before force-evolution/link-mode gating
            // in the source. Held trade items work on level-up and item use.
            if (heldItem != ITEM_EVERSTONE)
                matches = argument == ITEM_LINKING_CORD
                    ? (trading || (usingItem && item == ITEM_LINKING_CORD))
                    : heldItem == argument;
        }
        else if (trading)
            continue;
        else if (rules->method == POLISHED_EVOLVE_ITEM)
            matches = usingItem && item == argument;
        else if (usingItem)
            continue;
        else switch (rules->method)
        {
        case POLISHED_EVOLVE_LEVEL:
            matches = level >= argument && heldItem != ITEM_EVERSTONE;
            break;
        case POLISHED_EVOLVE_HAPPINESS:
            matches = GetMonData(mon, MON_DATA_FRIENDSHIP) >= POLISHED_EVOLUTION_FRIENDSHIP
                && heldItem != ITEM_EVERSTONE && PolishedEvolutionTimeMatches(argument, hour);
            break;
        case POLISHED_EVOLVE_STAT:
        {
            u32 attack = GetMonData(mon, MON_DATA_ATK);
            u32 defense = GetMonData(mon, MON_DATA_DEF);
            matches = level >= argument && heldItem != ITEM_EVERSTONE
                && ((rules->extra == POLISHED_ATK_GT_DEF && attack > defense)
                 || (rules->extra == POLISHED_ATK_LT_DEF && attack < defense)
                 || (rules->extra == POLISHED_ATK_EQ_DEF && attack == defense));
            break;
        }
        case POLISHED_EVOLVE_HOLDING:
            matches = heldItem == argument && PolishedEvolutionTimeMatches(rules->extra, hour);
            break;
        case POLISHED_EVOLVE_LOCATION:
            matches = landmark != POLISHED_LANDMARK_NONE && landmark == argument;
            break;
        case POLISHED_EVOLVE_MOVE:
            for (u32 i = 0; i < MAX_MON_MOVES; i++)
                if (GetMonData(mon, MON_DATA_MOVE1 + i) == argument)
                    matches = TRUE;
            break;
        case POLISHED_EVOLVE_CRIT:
            matches = criticalHits >= argument;
            break;
        case POLISHED_EVOLVE_PARTY:
            for (u32 i = 0; i < gPartiesCount[B_TRAINER_PLAYER]; i++)
                if (GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_SPECIES) == argument)
                    matches = TRUE;
            break;
        }
        if (matches)
            return rules->targetSpecies;
    }
    return SPECIES_NONE;
}

enum Species PolishedGetEvolutionTargetSpecies(struct Pokemon *mon, enum EvolutionMode mode, enum Item item, bool32 *canStopEvo)
{
    u32 criticalHits = 0;
    enum Species target;
    if ((mode == EVO_MODE_BATTLE_SPECIAL || mode == EVO_MODE_BATTLE_ONLY)
        && gBattleOutcome != B_OUTCOME_WON && gBattleOutcome != B_OUTCOME_CAUGHT)
        return SPECIES_NONE;
    for (u32 i = 0; i < gPartiesCount[B_TRAINER_PLAYER]; i++)
        if (mon == &gParties[B_TRAINER_PLAYER][i])
            criticalHits = gPartyCriticalHits[i];
    RtcCalcLocalTime();
    target = PolishedFindEvolution(mon, mode, item, gLocalTime.hours, PolishedGetEvolutionLandmark(), criticalHits);
    if (target != SPECIES_NONE && canStopEvo != NULL)
        *canStopEvo = mode != EVO_MODE_ITEM_USE && mode != EVO_MODE_ITEM_CHECK && mode != EVO_MODE_TRADE
            && !(mode == EVO_MODE_NORMAL && item == ITEM_RARE_CANDY && GetMonData(mon, MON_DATA_LEVEL) == MAX_LEVEL);
    return target;
}

void PolishedCommitEvolution(struct Pokemon *mon, enum Species targetSpecies)
{
    enum Species species = GetMonData(mon, MON_DATA_SPECIES);
    enum Item heldItem = GetMonData(mon, MON_DATA_HELD_ITEM);
    const struct PolishedEvolution *rules;
    if (!GetConfig(POLISHED_EVOLUTIONS) || (u32)species >= NUM_SPECIES)
        return;
    rules = sPolishedEvolutionRules[species];
    if (rules == NULL)
        return;
    // The source modifies a temporary mon and copies it back only after the
    // animation succeeds. CHECK_EVO, DO_EVO and B cancellation must keep items.
    for (; rules->method != POLISHED_EVOLVE_END; rules++)
        if ((rules->method == POLISHED_EVOLVE_HOLDING
            || (rules->method == POLISHED_EVOLVE_TRADE && rules->argument != ITEM_LINKING_CORD))
            && rules->targetSpecies == targetSpecies && heldItem == rules->argument)
        {
            enum Item none = ITEM_NONE;
            SetMonData(mon, MON_DATA_HELD_ITEM, &none);
            // Trainer battles normally restore original held items. An item
            // spent on a completed source evolution is permanently consumed.
            if (PolishedBattleEvolutionIsRunning())
                for (u32 i = 0; i < PARTY_SIZE; i++)
                    if (mon == &gParties[B_TRAINER_PLAYER][i]
                        && gBattleStruct->itemLost[B_TRAINER_PLAYER][i].originalItem == heldItem)
                        gBattleStruct->itemLost[B_TRAINER_PLAYER][i].originalItem = ITEM_NONE;
            return;
        }
}
