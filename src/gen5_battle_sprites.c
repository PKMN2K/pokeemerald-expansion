#include "global.h"
#include "pokemon.h"

/*
 * Battle-only static Gen 5-style sprite registry.
 *
 * To opt a species/form into 96x96 battle sprites:
 *
 * 1. Add exactly 96x96 indexed PNGs:
 *      graphics/pokemon_gen5/snivy/front.png
 *      graphics/pokemon_gen5/snivy/back.png
 *
 * 2. Add explicit INCBIN declarations below:
 *
 *      static const u32 sGen5BattleFront_Snivy[] =
 *          INCBIN_U32("graphics/pokemon_gen5/snivy/front.4bpp.lz");
 *      static const u32 sGen5BattleBack_Snivy[] =
 *          INCBIN_U32("graphics/pokemon_gen5/snivy/back.4bpp.lz");
 *
 * 3. Add the matching designated initializer to sGen5BattleSprites:
 *
 *      [SPECIES_SNIVY] = {sGen5BattleFront_Snivy, sGen5BattleBack_Snivy},
 *
 * Explicit INCBIN paths are intentional: the project's dependency scanner can
 * see them and generate the .4bpp.lz assets from the PNGs with the standard
 * graphics rules.
 *
 * The PNG palette-index ordering must match the species' existing battle
 * palette. Normal party/summary/storage/Pokedex graphics are not changed.
 */

struct Gen5BattleSpriteInfo
{
    const u32 *frontPic;
    const u32 *backPic;
};

/* Add 96x96 INCBIN declarations here. */

static const struct Gen5BattleSpriteInfo sGen5BattleSprites[NUM_SPECIES] =
{
    /* Add [SPECIES_*] entries here. */
};

bool32 HasGen5BattleSprite(enum Species species)
{
    species = SanitizeSpeciesId(species);
    return sGen5BattleSprites[species].frontPic != NULL
        && sGen5BattleSprites[species].backPic != NULL;
}

const u32 *GetGen5BattleSpritePic(enum Species species, bool32 frontPic)
{
    species = SanitizeSpeciesId(species);
    if (!HasGen5BattleSprite(species))
        return NULL;

    return frontPic ? sGen5BattleSprites[species].frontPic : sGen5BattleSprites[species].backPic;
}
