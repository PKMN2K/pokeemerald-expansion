#ifndef GUARD_POLISHED_FORMS_H
#define GUARD_POLISHED_FORMS_H

#include "pokemon.h"

// Source form numbers, not Expansion form-table indices. Zero is NO_FORM in
// encounter data and an uninitialized field in older encrypted save records.
u8 PolishedGetBoxMonForm(struct BoxPokemon *mon);
u8 PolishedInitialForm(enum Species species);
enum Species PolishedSpeciesForForm(enum Species species, u8 form);
enum Species PolishedEggSpecies(enum Species species);
u8 PolishedSelectWildForm(enum Species *species, u8 explicitForm);
u8 PolishedInheritEggForm(struct DayCare *daycare, enum Species *eggSpecies);

#endif
