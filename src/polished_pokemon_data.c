#include "global.h"
#include "pokemon.h"
#include "polished_pokemon_data.h"

#include "data/polished_level_up_learnsets.h"

const struct LevelUpMove *PolishedGetLevelUpLearnset(enum Species species)
{
    if ((u32)species >= NUM_SPECIES)
        return NULL;
    return gPolishedLevelUpLearnsets[species];
}

#include "data/polished_species_base_data.h"

const struct SpeciesBaseData *PolishedGetSpeciesBaseData(enum Species species)
{
    if ((u32)species >= NUM_SPECIES)
        return NULL;
    return gPolishedSpeciesBaseData[species];
}
