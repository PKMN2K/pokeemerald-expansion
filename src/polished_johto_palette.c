#include "global.h"
#include "polished_johto_palette.h"
#include "palette.h"
#include "rtc.h"

// Source-generated, pinned RGB555 data; no custom/redrawn colors.
#include "data/tilesets/polished_johto_palette_tables.h"

// Explicit layout registrations are intentionally EMPTY at this stage.
// A future imported Johto layout must have its exact Emerald layout ID,
// exact two tileset pointers, source map-group index and verified palette
// family entered here. Never reuse the 1-based source group as an Emerald
// map group, and never auto-enable on a generic Emerald/Hoenn map.
struct PolishedJohtoMapPaletteRegistration
{
    u16 emeraldLayoutId;
    const struct Tileset *primaryTileset;
    const struct Tileset *secondaryTileset;
    u8 sourceMapGroup;
    u8 family;
    u8 overcastIndex; // Original GetOvercastIndex: 0 = normal, 1..3 overcast
};

static const struct PolishedJohtoMapPaletteRegistration sJohtoMapRegistrations[] =
{
    // Sentinel. No live layouts are registered; Prism mining is unaffected.
    {0},
};

static const struct PolishedJohtoMapPaletteRegistration *FindJohtoMapRegistration(const struct MapLayout *layout)
{
    u32 i;

    if (layout == NULL || gMapHeader.mapLayout != layout)
        return NULL;

    // Using the layout *and* both tileset identities prevents a Hoenn map
    // with a similar numeric map-group/map-number from matching by accident.
    for (i = 0; i < ARRAY_COUNT(sJohtoMapRegistrations); i++)
    {
        const struct PolishedJohtoMapPaletteRegistration *registration = &sJohtoMapRegistrations[i];

        if (registration->emeraldLayoutId == 0
         || registration->primaryTileset == NULL
         || registration->secondaryTileset == NULL)
            continue;

        if (registration->emeraldLayoutId == gMapHeader.mapLayoutId
         && registration->primaryTileset == layout->primaryTileset
         && registration->secondaryTileset == layout->secondaryTileset)
            return registration;
    }

    return NULL;
}

bool8 PolishedJohtoIsRegisteredMapLayout(const struct MapLayout *mapLayout)
{
    return FindJohtoMapRegistration(mapLayout) != NULL;
}

// Emerald enum: MORNING, DAY, EVENING, NIGHT.
// Pinned Polished Crystal palette order: morn, day, nite, eve.
// Do NOT use enum TimeOfDay directly as the source palette array index.
static bool8 GetJohtoTimeIndices(enum TimeOfDay time, u8 *sourcePeriod, u8 *roofPeriod)
{
    switch (time)
    {
    case TIME_MORNING:
        *sourcePeriod = 0;
        *roofPeriod = 0;
        return TRUE;
    case TIME_DAY:
        *sourcePeriod = 1;
        *roofPeriod = 0; // Source roof colors: morn/day share one pair
        return TRUE;
    case TIME_NIGHT:
        *sourcePeriod = 2;
        *roofPeriod = 1;
        return TRUE;
    case TIME_EVENING:
        *sourcePeriod = 3;
        *roofPeriod = 2;
        return TRUE;
    default:
        return FALSE;
    }
}

bool8 PolishedJohtoComposePalette(enum PolishedJohtoPaletteFamily family,
                                enum TimeOfDay time, u8 sourceMapGroup,
                                u8 sourceOvercastIndex,
                                u16 output[JOHTO_GBA_PALETTE_COUNT][JOHTO_GBA_PALETTE_COLORS])
{
    const u16 (*roofPairs)[2];
    u8 sourcePeriod, roofPeriod;
    u32 bank, color;

    if (output == NULL || family >= JOHTO_PALETTE_FAMILY_COUNT
     || sourceMapGroup == 0 || sourceMapGroup > JOHTO_SOURCE_MAP_GROUP_COUNT
     || sourceOvercastIndex > JOHTO_SOURCE_OVERCAST_COUNT)
        return FALSE;

    // Overcast is selected ONLY by a verified original weather result.
    if ((sourceOvercastIndex > 0 && family != JOHTO_PALETTE_OVERCAST)
     || (sourceOvercastIndex == 0 && family == JOHTO_PALETTE_OVERCAST))
        return FALSE;

    if (!GetJohtoTimeIndices(time, &sourcePeriod, &roofPeriod))
        return FALSE;

    // GBA's 16-color banks: preserve all four original CGB RGB555 colors
    // exactly; the twelve unused color entries are zero-padded.
    for (bank = 0; bank < JOHTO_GBA_PALETTE_COUNT; bank++)
    {
        for (color = 0; color < JOHTO_GBA_PALETTE_COLORS; color++)
        {
            if (color < 4)
                output[bank][color] = sPolishedJohtoPaletteColors[family][sourcePeriod][bank][color];
            else
                output[bank][color] = 0;
        }
    }

    // Polished Crystal replaces ONLY colors 1 and 2 of BG palette 6.
    // Other source colors, including roof colors 0 and 3, remain untouched.
    if (sourceOvercastIndex > 0)
        roofPairs = sPolishedJohtoOvercastRoofColors[sourceOvercastIndex - 1];
    else
        roofPairs = sPolishedJohtoRoofColors[sourceMapGroup];

    output[6][1] = roofPairs[roofPeriod][0];
    output[6][2] = roofPairs[roofPeriod][1];
    return TRUE;
}

// The renderer hooks call this only after the ordinary map palette loader.
// With NO registration this is a strict no-op. Future registration must
// resolve source map-specific special palettes, weather, priority layers,
// and GBA palette-0 backdrop policy first.
bool8 PolishedJohtoTryApplyRegisteredMapPalette(const struct MapLayout *mapLayout, bool8 skipFaded)
{
    const struct PolishedJohtoMapPaletteRegistration *registration = FindJohtoMapRegistration(mapLayout);
    u16 ALIGNED(4) palette[JOHTO_GBA_PALETTE_COUNT][JOHTO_GBA_PALETTE_COLORS];

    if (registration == NULL)
        return FALSE;

    if (!PolishedJohtoComposePalette((enum PolishedJohtoPaletteFamily)registration->family,
                                     GetTimeOfDay(), registration->sourceMapGroup,
                                     registration->overcastIndex, palette))
        return FALSE;

    if (skipFaded)
        CpuCopy16(palette, gPlttBufferUnfaded, sizeof(palette));
    else
        LoadPaletteFast(palette, 0, sizeof(palette));

    return TRUE;
}
