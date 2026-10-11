#include "global.h"
#include "prism_mining.h"
#include "tilesets.h"
#include "test/test.h"

TEST("Prism mining tilesets classify into approved terrain families")
{
    EXPECT_EQ(GetPrismMiningTilesetFamily(&gTileset_PrismMoundCave), PRISM_MINING_TILESET_MOUND_CAVE);
    EXPECT_EQ(GetPrismMiningTilesetFamily(&gTileset_PrismFirelightCaverns), PRISM_MINING_TILESET_FIRELIGHT_CAVERNS);
    EXPECT_EQ(GetPrismMiningTilesetFamily(&gTileset_PrismKantoCave), PRISM_MINING_TILESET_KANTO_CAVE);

    EXPECT_EQ(GetPrismMiningTilesetFamily(&gTileset_PrismOlcanIsleMorning), PRISM_MINING_TILESET_OLCAN);
    EXPECT_EQ(GetPrismMiningTilesetFamily(&gTileset_PrismOlcanIsleDay), PRISM_MINING_TILESET_OLCAN);
    EXPECT_EQ(GetPrismMiningTilesetFamily(&gTileset_PrismOlcanIsleNight), PRISM_MINING_TILESET_OLCAN);
    EXPECT_EQ(GetPrismMiningTilesetFamily(&gTileset_PrismOlcanChineMorning), PRISM_MINING_TILESET_OLCAN);
    EXPECT_EQ(GetPrismMiningTilesetFamily(&gTileset_PrismOlcanChineDay), PRISM_MINING_TILESET_OLCAN);
    EXPECT_EQ(GetPrismMiningTilesetFamily(&gTileset_PrismOlcanChineNight), PRISM_MINING_TILESET_OLCAN);
}

TEST("Only approved Prism mining tilesets are recognized as mining terrain")
{
    EXPECT(IsPrismMiningTileset(&gTileset_PrismMoundCave));
    EXPECT(IsPrismMiningTileset(&gTileset_PrismFirelightCaverns));
    EXPECT(IsPrismMiningTileset(&gTileset_PrismKantoCave));
    EXPECT(IsPrismMiningTileset(&gTileset_PrismOlcanIsleDay));

    EXPECT_EQ(IsPrismMiningTileset(&gTileset_General), FALSE);
    EXPECT_EQ(IsPrismMiningTileset(NULL), FALSE);
    EXPECT_EQ(GetPrismMiningTilesetFamily(&gTileset_General), PRISM_MINING_TILESET_NONE);
    EXPECT_EQ(GetPrismMiningTilesetFamily(NULL), PRISM_MINING_TILESET_NONE);
}

TEST("Prism Olcan tileset detection covers every imported palette variant")
{
    EXPECT(IsPrismOlcanTileset(&gTileset_PrismOlcanIsleMorning));
    EXPECT(IsPrismOlcanTileset(&gTileset_PrismOlcanIsleDay));
    EXPECT(IsPrismOlcanTileset(&gTileset_PrismOlcanIsleNight));
    EXPECT(IsPrismOlcanTileset(&gTileset_PrismOlcanChineMorning));
    EXPECT(IsPrismOlcanTileset(&gTileset_PrismOlcanChineDay));
    EXPECT(IsPrismOlcanTileset(&gTileset_PrismOlcanChineNight));

    EXPECT_EQ(IsPrismOlcanTileset(&gTileset_PrismMoundCave), FALSE);
    EXPECT_EQ(IsPrismOlcanTileset(&gTileset_PrismFirelightCaverns), FALSE);
    EXPECT_EQ(IsPrismOlcanTileset(&gTileset_PrismKantoCave), FALSE);
    EXPECT_EQ(IsPrismOlcanTileset(&gTileset_General), FALSE);
    EXPECT_EQ(IsPrismOlcanTileset(NULL), FALSE);
}
