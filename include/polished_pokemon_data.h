#ifndef GUARD_POLISHED_POKEMON_DATA_H
#define GUARD_POLISHED_POKEMON_DATA_H

#include "pokemon.h"

extern const struct LevelUpMove *const gPolishedLevelUpLearnsets[NUM_SPECIES];
const struct SpeciesBaseData *PolishedGetSpeciesBaseData(enum Species species);
const struct LevelUpMove *PolishedGetLevelUpLearnset(enum Species species);
const u16 *PolishedGetEggMoves(enum Species species);
const u16 *PolishedGetTeachableLearnset(enum Species species);

#endif
