#include "global.h"
#include "gen5_static_battlers.h"

#define GEN5_STATIC_BATTLER(species, folder) \
    static const u32 sGen5StaticFront_##species[] = INCGFX_U32("graphics/pokemon_gen5_static/" #folder "/front.png", ".4bpp.smol"); \
    static const u32 sGen5StaticBack_##species[] = INCGFX_U32("graphics/pokemon_gen5_static/" #folder "/back.png", ".4bpp.smol"); \
    static const u16 sGen5StaticPalette_##species[] = INCGFX_U16("graphics/pokemon_gen5_static/" #folder "/normal.pal", ".gbapal"); \
    static const u16 sGen5StaticShinyPalette_##species[] = INCGFX_U16("graphics/pokemon_gen5_static/" #folder "/shiny.pal", ".gbapal");
#include "data/pokemon/gen5_static_battlers.h"
#undef GEN5_STATIC_BATTLER

const struct Gen5StaticBattlerInfo gGen5StaticBattlerInfo[NUM_SPECIES] =
{
#define GEN5_STATIC_BATTLER(species, folder) \
    [SPECIES_##species] = \
    { \
        .frontPic = sGen5StaticFront_##species, \
        .backPic = sGen5StaticBack_##species, \
        .palette = sGen5StaticPalette_##species, \
        .shinyPalette = sGen5StaticShinyPalette_##species, \
    },
#include "data/pokemon/gen5_static_battlers.h"
#undef GEN5_STATIC_BATTLER
};

bool32 HasGen5StaticBattler(enum Species species, bool32 frontPic)
{
    if ((u32)species >= NUM_SPECIES)
        return FALSE;

    if (frontPic)
        return gGen5StaticBattlerInfo[species].frontPic != NULL;
    else
        return gGen5StaticBattlerInfo[species].backPic != NULL;
}

const u32 *GetGen5StaticBattlerPic(enum Species species, bool32 frontPic)
{
    if (!HasGen5StaticBattler(species, frontPic))
        return NULL;

    return frontPic
        ? gGen5StaticBattlerInfo[species].frontPic
        : gGen5StaticBattlerInfo[species].backPic;
}

const u16 *GetGen5StaticBattlerPalette(enum Species species, bool32 shiny)
{
    if ((u32)species >= NUM_SPECIES)
        return NULL;

    return shiny
        ? gGen5StaticBattlerInfo[species].shinyPalette
        : gGen5StaticBattlerInfo[species].palette;
}
