#ifndef GUARD_POLISHED_POKEMON_DATA_H
#define GUARD_POLISHED_POKEMON_DATA_H

#include "pokemon.h"

extern const struct LevelUpMove *const gPolishedLevelUpLearnsets[NUM_SPECIES];
const struct LevelUpMove *PolishedGetLevelUpLearnset(enum Species species);

#endif
