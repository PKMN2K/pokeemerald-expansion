#ifndef GUARD_POLISHED_GROWTH_H
#define GUARD_POLISHED_GROWTH_H

#include "pokemon.h"

// Source identities begin at zero EXP at level one. Other levels and optional
// identities use the full native growth tables, without rewriting saved EXP.
u32 GetSpeciesExperienceAtLevel(enum Species species, u32 level);

#endif
