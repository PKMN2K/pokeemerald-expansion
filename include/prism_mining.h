#ifndef GUARD_PRISM_MINING_H
#define GUARD_PRISM_MINING_H

struct Tileset;

const struct Tileset *GetPrismOlcanIsleTilesetForCurrentTime(void);
const struct Tileset *GetPrismOlcanChineTilesetForCurrentTime(void);
const struct Tileset *ResolvePrismOlcanTilesetForCurrentTime(const struct Tileset *tileset);
bool32 IsPrismOlcanTileset(const struct Tileset *tileset);
bool32 IsPrismMiningTileset(const struct Tileset *tileset);
bool32 CurrentMapUsesPrismMiningTileset(void);
bool32 CurrentMapUsesPrismOlcanTileset(void);

#endif // GUARD_PRISM_MINING_H
