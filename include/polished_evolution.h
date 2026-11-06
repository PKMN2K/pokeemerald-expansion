#ifndef GUARD_POLISHED_EVOLUTION_H
#define GUARD_POLISHED_EVOLUTION_H

#include "pokemon.h"

enum PolishedEvolutionMethod
{
    POLISHED_EVOLVE_END,
    POLISHED_EVOLVE_LEVEL,
    POLISHED_EVOLVE_ITEM,
    POLISHED_EVOLVE_TRADE,
    POLISHED_EVOLVE_HOLDING,
    POLISHED_EVOLVE_HAPPINESS,
    POLISHED_EVOLVE_STAT,
    POLISHED_EVOLVE_LOCATION,
    POLISHED_EVOLVE_MOVE,
    POLISHED_EVOLVE_CRIT,
    POLISHED_EVOLVE_PARTY,
};

enum PolishedEvolutionTime { POLISHED_TR_ANYTIME = 1, POLISHED_TR_MORNDAY, POLISHED_TR_EVENITE };
enum PolishedEvolutionStat { POLISHED_ATK_GT_DEF = 1, POLISHED_ATK_LT_DEF, POLISHED_ATK_EQ_DEF };
enum PolishedEvolutionLandmark
{
    POLISHED_LANDMARK_NONE,
    POLISHED_LANDMARK_MAGNET_TUNNEL,
    POLISHED_LANDMARK_DIM_CAVE,
    POLISHED_LANDMARK_ILEX_FOREST,
    POLISHED_LANDMARK_ICE_PATH,
    POLISHED_LANDMARK_SINJOH_RUINS,
    POLISHED_LANDMARK_MYSTRI_STAGE,
    POLISHED_LANDMARK_RUGGED_ROAD,
    POLISHED_LANDMARK_SNOWTOP_MOUNTAIN,
};

const struct Evolution *PolishedGetSpeciesEvolutions(enum Species species);
enum Species PolishedGetEvolutionTargetSpecies(struct Pokemon *mon, enum EvolutionMode mode, enum Item item, bool32 *canStopEvo);
// Explicit context keeps the source predicate independent of RTC/map storage.
enum Species PolishedFindEvolution(struct Pokemon *mon, enum EvolutionMode mode, enum Item item, u32 hour, u32 landmark, u32 criticalHits);
void PolishedCommitEvolution(struct Pokemon *mon, enum Species targetSpecies);
bool32 PolishedEvolutionTimeMatches(u32 period, u32 hour);
u32 PolishedGetEvolutionLandmark(void);
const u8 *PolishedEvolutionLandmarkName(u32 landmark);

#endif
