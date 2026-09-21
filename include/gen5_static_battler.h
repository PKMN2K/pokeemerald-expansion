#ifndef GUARD_GEN5_STATIC_BATTLER_H
#define GUARD_GEN5_STATIC_BATTLER_H

#include "constants/species.h"

// Static Gen 5-style battle sprites use a 96x96, 4bpp canvas.
// The GBA cannot draw a 96x96 OBJ directly, so the renderer will expose
// the image as four hardware subsprites backed by one contiguous tile allocation.
#define GEN5_BATTLER_WIDTH          96
#define GEN5_BATTLER_HEIGHT         96
#define GEN5_BATTLER_TILE_WIDTH     (GEN5_BATTLER_WIDTH / 8)
#define GEN5_BATTLER_TILE_HEIGHT    (GEN5_BATTLER_HEIGHT / 8)
#define GEN5_BATTLER_TILE_COUNT     (GEN5_BATTLER_TILE_WIDTH * GEN5_BATTLER_TILE_HEIGHT)
#define GEN5_BATTLER_PIC_SIZE       (GEN5_BATTLER_WIDTH * GEN5_BATTLER_HEIGHT / 2)
#define GEN5_BATTLER_SUBSPRITE_COUNT 4

// Packed 96x96 battle images are stored in OBJ-friendly blocks rather than
// normal 12x12 tile-row order:
//   0x0000: top-left     64x64 (64 tiles)
//   0x0800: top-right    32x64 (32 tiles)
//   0x0C00: bottom-left  64x32 (32 tiles)
//   0x1000: bottom-right 32x32 (16 tiles)
// Total: 144 tiles / 0x1200 bytes.
#define GEN5_BATTLER_TL_TILE_OFFSET 0
#define GEN5_BATTLER_TR_TILE_OFFSET 64
#define GEN5_BATTLER_BL_TILE_OFFSET 96
#define GEN5_BATTLER_BR_TILE_OFFSET 128

struct Gen5StaticBattlerGfx
{
    const u32 *frontPic;
    const u32 *backPic;

    // Optional dedicated palettes. NULL means use the normal SpeciesInfo palette.
    const u16 *palette;
    const u16 *shinyPalette;

    // Fine positioning relative to the normal battle baseline.
    s8 frontYOffset;
    s8 backYOffset;
};

struct Gen5StaticBattlerEntry
{
    enum Species species;
    struct Gen5StaticBattlerGfx gfx;
};

// Returns NULL when a species has no 96x96 override, preserving the normal
// pokeemerald-expansion 64x64 path as the automatic fallback.
const struct Gen5StaticBattlerGfx *GetGen5StaticBattlerGfx(enum Species species);
bool32 SpeciesHasGen5StaticBattlerGfx(enum Species species);

#endif // GUARD_GEN5_STATIC_BATTLER_H
