#ifndef GUARD_POLISHED_JOHTO_PALETTE_H
#define GUARD_POLISHED_JOHTO_PALETTE_H

#include "global.h"
#include "constants/rtc.h"

// Authentic Polished Crystal families; see generated C table.
enum PolishedJohtoPaletteFamily
{
    JOHTO_PALETTE_STANDARD,
    JOHTO_PALETTE_VIOLET_ECRUTEAK,
    JOHTO_PALETTE_OVERCAST,
    JOHTO_PALETTE_FAMILY_COUNT,
};

#define JOHTO_SOURCE_MAP_GROUP_COUNT 37
#define JOHTO_SOURCE_OVERCAST_COUNT 3
#define JOHTO_GBA_PALETTE_COUNT 7
#define JOHTO_GBA_PALETTE_COLORS 16

// Compose RGB555 colors without modifying the game's palette buffers.
// Source map groups are 1..37 (NOT Emerald map groups); weather indices
// are original GetOvercastIndex values, 0 for no overcast or 1..3.
bool8 PolishedJohtoComposePalette(enum PolishedJohtoPaletteFamily family,
                                enum TimeOfDay time, u8 sourceMapGroup,
                                u8 sourceOvercastIndex,
                                u16 output[JOHTO_GBA_PALETTE_COUNT][JOHTO_GBA_PALETTE_COLORS]);

// Explicit opt-in only: no Emerald map layouts have been registered yet.
// The source-mapping table remains empty until the authentic tileset,
// map layouts, transparency and priority/layer behavior are verified.
bool8 PolishedJohtoIsRegisteredMapLayout(const struct MapLayout *mapLayout);
bool8 PolishedJohtoTryApplyRegisteredMapPalette(const struct MapLayout *mapLayout,
                                             bool8 skipFaded);

#endif // GUARD_POLISHED_JOHTO_PALETTE_H
