#include "global.h"
#include "bg.h"
#include "gen4_ui.h"
#include "palette.h"

bool32 Gen4UiBgMatchesPaletteMode(u32 bg, enum Gen4UiPaletteMode paletteMode)
{
    if (IsInvalidBg(bg))
        return FALSE;

    return GetBgAttribute(bg, BG_ATTR_PALETTEMODE) == paletteMode;
}

bool32 Gen4UiLoadBgAsset(u32 bg, const struct Gen4UiBgAsset *asset)
{
    if (asset == NULL || IsInvalidBg(bg))
        return FALSE;

    if (!Gen4UiBgMatchesPaletteMode(bg, asset->paletteMode))
        return FALSE;

    if (asset->palette != NULL && asset->paletteSize != 0)
        LoadPalette(asset->palette, asset->paletteOffset, asset->paletteSize);

    if (asset->tiles != NULL && asset->tilesSize != 0)
    {
        if (LoadBgTiles(bg, asset->tiles, asset->tilesSize, asset->baseTile) == 0xFFFF)
            return FALSE;
    }

    if (asset->tilemap != NULL && asset->tilemapSize != 0)
    {
        if (LoadBgTilemap(bg, asset->tilemap, asset->tilemapSize, asset->tilemapOffset) == 0xFFFF)
            return FALSE;
    }

    return TRUE;
}
