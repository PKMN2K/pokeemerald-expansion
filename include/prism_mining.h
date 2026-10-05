#ifndef GUARD_PRISM_MINING_H
#define GUARD_PRISM_MINING_H

struct Tileset;

const struct Tileset *GetPrismOlcanIsleTilesetForCurrentTime(void);
const struct Tileset *GetPrismOlcanChineTilesetForCurrentTime(void);
const struct Tileset *ResolvePrismOlcanTilesetForCurrentTime(const struct Tileset *tileset);

#endif // GUARD_PRISM_MINING_H
