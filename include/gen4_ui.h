#ifndef GUARD_GEN4_UI_H
#define GUARD_GEN4_UI_H

enum Gen4UiPaletteMode
{
    GEN4_UI_PALETTE_4BPP,
    GEN4_UI_PALETTE_8BPP,
};

// Describes a GBA-native tiled background generated from a Gen 4 UI source.
//
// paletteOffset is expressed in palette entries, matching LoadPalette.
// tilemapOffset is expressed in tilemap entries, matching LoadBgTilemap.
// baseTile is expressed in tiles, matching LoadBgTiles.
//
// The target BG must already be configured with the matching palette mode.
struct Gen4UiBgAsset
{
    const void *tiles;
    const void *tilemap;
    const void *palette;
    u16 tilesSize;
    u16 tilemapSize;
    u16 paletteOffset;
    u16 paletteSize;
    u16 baseTile;
    u16 tilemapOffset;
    enum Gen4UiPaletteMode paletteMode;
};

bool32 Gen4UiBgMatchesPaletteMode(u32 bg, enum Gen4UiPaletteMode paletteMode);
bool32 Gen4UiLoadBgAsset(u32 bg, const struct Gen4UiBgAsset *asset);

// Temporary end-to-end renderer validation screen.
void CB2_Gen4UiTest(void);

#endif // GUARD_GEN4_UI_H
