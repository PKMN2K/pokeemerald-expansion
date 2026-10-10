#ifndef GUARD_PRISM_MINING_H
#define GUARD_PRISM_MINING_H

struct Tileset;

enum PrismMiningTilesetFamily
{
    PRISM_MINING_TILESET_NONE,
    PRISM_MINING_TILESET_MOUND_CAVE,
    PRISM_MINING_TILESET_FIRELIGHT_CAVERNS,
    PRISM_MINING_TILESET_KANTO_CAVE,
    PRISM_MINING_TILESET_OLCAN,
};

const struct Tileset *GetPrismOlcanIsleTilesetForCurrentTime(void);
const struct Tileset *GetPrismOlcanChineTilesetForCurrentTime(void);
const struct Tileset *ResolvePrismOlcanTilesetForCurrentTime(const struct Tileset *tileset);
bool32 IsPrismOlcanTileset(const struct Tileset *tileset);
enum PrismMiningTilesetFamily GetPrismMiningTilesetFamily(const struct Tileset *tileset);
bool32 IsPrismMiningTileset(const struct Tileset *tileset);
bool32 CurrentMapUsesPrismMiningTileset(void);
bool32 CurrentMapUsesPrismOlcanTileset(void);

#endif // GUARD_PRISM_MINING_H
