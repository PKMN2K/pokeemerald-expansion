#include "global.h"
#include "config/pokemon.h"
#include "gen5_static_battler.h"

#if P_GEN5_STATIC_BATTLERS

// Species opt in here one at a time. Keeping this registry separate from
// SpeciesInfo means summary, party, storage, Pokédex, and other 64x64 users
// continue to use pokeemerald-expansion's normal graphics.
//
// Example entry once 96x96 assets exist:
//
// {
//     .species = SPECIES_BULBASAUR,
//     .gfx =
//     {
//         .frontPic = gGen5FrontPic_Bulbasaur,
//         .backPic = gGen5BackPic_Bulbasaur,
//         .palette = NULL,
//         .shinyPalette = NULL,
//         .frontYOffset = 0,
//         .backYOffset = 0,
//     },
// },
//
// SPECIES_NONE terminates the sparse registry and must remain last.
static const struct Gen5StaticBattlerEntry sGen5StaticBattlerRegistry[] =
{
    {
        .species = SPECIES_NONE,
    },
};

#endif // P_GEN5_STATIC_BATTLERS

const struct Gen5StaticBattlerGfx *GetGen5StaticBattlerGfx(enum Species species)
{
#if P_GEN5_STATIC_BATTLERS
    u32 i;

    if (species == SPECIES_NONE)
        return NULL;

    for (i = 0; sGen5StaticBattlerRegistry[i].species != SPECIES_NONE; i++)
    {
        if (sGen5StaticBattlerRegistry[i].species == species)
            return &sGen5StaticBattlerRegistry[i].gfx;
    }
#endif

    return NULL;
}

bool32 SpeciesHasGen5StaticBattlerGfx(enum Species species)
{
    return GetGen5StaticBattlerGfx(species) != NULL;
}
