/* Generated source Brick Piece identity; existing item IDs are unchanged. */
    [ITEM_POLISHED_BRICK_PIECE] =
    {
        .name = ITEM_NAME("Brick Piece"),
        .pluralName = ITEM_PLURAL_NAME("Brick Pieces"),
        .price = 100,
        .description = COMPOUND_STRING("A chunk of tile.\nSell low."),
        .pocket = POCKET_ITEMS,
        .type = ITEM_USE_BAG_MENU,
        .fieldUseFunc = ItemUseOutOfBattle_CannotUse,
        .iconPic = gItemIcon_PolishedBrickPiece,
        .iconPalette = gItemIconPalette_PolishedBrickPiece,
    },
