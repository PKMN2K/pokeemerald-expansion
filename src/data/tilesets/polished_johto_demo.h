// Generated binary inputs: tools/polished_port/scripts/build_johto_demo.py

static const u32 sPolishedJohtoDemoOutdoorPrimaryTiles[] = INCBIN_U32("data/tilesets/primary/polished_johto_demo_outdoor/tiles.bin");
static const u16 sPolishedJohtoDemoOutdoorPrimaryPalettes[] = INCBIN_U16("data/tilesets/primary/polished_johto_demo_outdoor/palettes.bin");
static const u16 sPolishedJohtoDemoOutdoorPrimaryMetatiles[] = INCBIN_U16("data/tilesets/primary/polished_johto_demo_outdoor/metatiles.bin");
static const u16 sPolishedJohtoDemoOutdoorPrimaryAttributes[] = INCBIN_U16("data/tilesets/primary/polished_johto_demo_outdoor/metatile_attributes.bin");
const struct Tileset gTileset_PolishedJohtoDemoOutdoorPrimary =
{
    .isCompressed = FALSE,
    .isSecondary = FALSE,
    .tiles = sPolishedJohtoDemoOutdoorPrimaryTiles,
    .palettes = (const u16 (*)[16])sPolishedJohtoDemoOutdoorPrimaryPalettes,
    .metatiles = sPolishedJohtoDemoOutdoorPrimaryMetatiles,
    .metatileAttributes = sPolishedJohtoDemoOutdoorPrimaryAttributes,
    .callback = InitTilesetAnim_PolishedJohtoOutdoorPrimary,
};

static const u32 sPolishedJohtoDemoOutdoorSecondaryTiles[] = INCBIN_U32("data/tilesets/secondary/polished_johto_demo_outdoor/tiles.bin");
static const u16 sPolishedJohtoDemoOutdoorSecondaryPalettes[] = INCBIN_U16("data/tilesets/secondary/polished_johto_demo_outdoor/palettes.bin");
static const u16 sPolishedJohtoDemoOutdoorSecondaryMetatiles[] = INCBIN_U16("data/tilesets/secondary/polished_johto_demo_outdoor/metatiles.bin");
static const u16 sPolishedJohtoDemoOutdoorSecondaryAttributes[] = INCBIN_U16("data/tilesets/secondary/polished_johto_demo_outdoor/metatile_attributes.bin");
const struct Tileset gTileset_PolishedJohtoDemoOutdoorSecondary =
{
    .isCompressed = FALSE,
    .isSecondary = TRUE,
    .tiles = sPolishedJohtoDemoOutdoorSecondaryTiles,
    .palettes = (const u16 (*)[16])sPolishedJohtoDemoOutdoorSecondaryPalettes,
    .metatiles = sPolishedJohtoDemoOutdoorSecondaryMetatiles,
    .metatileAttributes = sPolishedJohtoDemoOutdoorSecondaryAttributes,
    .callback = NULL,
};

static const u32 sPolishedJohtoDemoLabPrimaryTiles[] = INCBIN_U32("data/tilesets/primary/polished_johto_demo_lab/tiles.bin");
static const u16 sPolishedJohtoDemoLabPrimaryPalettes[] = INCBIN_U16("data/tilesets/primary/polished_johto_demo_lab/palettes.bin");
static const u16 sPolishedJohtoDemoLabPrimaryMetatiles[] = INCBIN_U16("data/tilesets/primary/polished_johto_demo_lab/metatiles.bin");
static const u16 sPolishedJohtoDemoLabPrimaryAttributes[] = INCBIN_U16("data/tilesets/primary/polished_johto_demo_lab/metatile_attributes.bin");
const struct Tileset gTileset_PolishedJohtoDemoLabPrimary =
{
    .isCompressed = FALSE,
    .isSecondary = FALSE,
    .tiles = sPolishedJohtoDemoLabPrimaryTiles,
    .palettes = (const u16 (*)[16])sPolishedJohtoDemoLabPrimaryPalettes,
    .metatiles = sPolishedJohtoDemoLabPrimaryMetatiles,
    .metatileAttributes = sPolishedJohtoDemoLabPrimaryAttributes,
    .callback = NULL,
};

static const u32 sPolishedJohtoDemoLabSecondaryTiles[] = INCBIN_U32("data/tilesets/secondary/polished_johto_demo_lab/tiles.bin");
static const u16 sPolishedJohtoDemoLabSecondaryPalettes[] = INCBIN_U16("data/tilesets/secondary/polished_johto_demo_lab/palettes.bin");
static const u16 sPolishedJohtoDemoLabSecondaryMetatiles[] = INCBIN_U16("data/tilesets/secondary/polished_johto_demo_lab/metatiles.bin");
static const u16 sPolishedJohtoDemoLabSecondaryAttributes[] = INCBIN_U16("data/tilesets/secondary/polished_johto_demo_lab/metatile_attributes.bin");
const struct Tileset gTileset_PolishedJohtoDemoLabSecondary =
{
    .isCompressed = FALSE,
    .isSecondary = TRUE,
    .tiles = sPolishedJohtoDemoLabSecondaryTiles,
    .palettes = (const u16 (*)[16])sPolishedJohtoDemoLabSecondaryPalettes,
    .metatiles = sPolishedJohtoDemoLabSecondaryMetatiles,
    .metatileAttributes = sPolishedJohtoDemoLabSecondaryAttributes,
    .callback = NULL,
};
