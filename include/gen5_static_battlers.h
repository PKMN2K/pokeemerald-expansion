#ifndef GUARD_GEN5_STATIC_BATTLERS_H
#define GUARD_GEN5_STATIC_BATTLERS_H

#include "global.h"
#include "constants/species.h"

#define GEN5_STATIC_BATTLER_WIDTH  96
#define GEN5_STATIC_BATTLER_HEIGHT 96
#define GEN5_STATIC_BATTLER_SIZE   (GEN5_STATIC_BATTLER_WIDTH * GEN5_STATIC_BATTLER_HEIGHT / 2)

struct Gen5StaticBattlerInfo
{
    const u32 *frontPic;
    const u32 *backPic;
    const u16 *palette;
    const u16 *shinyPalette;
};

extern const struct Gen5StaticBattlerInfo gGen5StaticBattlerInfo[NUM_SPECIES];

bool32 HasGen5StaticBattler(enum Species species, bool32 frontPic);
const u32 *GetGen5StaticBattlerPic(enum Species species, bool32 frontPic);
const u16 *GetGen5StaticBattlerPalette(enum Species species, bool32 shiny);

#endif // GUARD_GEN5_STATIC_BATTLERS_H
