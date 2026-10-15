; Pinned source Rangi42/polishedcrystal 3fa43192379df5c3e7b09a08e4d5d79af4f02f42
; engine/tilesets/tileset_anims.asm lines 32-59
TilesetJohto1Anim::
TilesetJohto2Anim::
TilesetJohto3Anim::
TilesetJohto4Anim::
TilesetJohto5Anim::
	dw vTiles2 tile $14, AnimateWaterTile
	dw vTiles2 tile $1c, AnimateRainPuddleTile
	dw vTiles2 tile $1d, AnimateRainWaterTile
	dw NULL,  DoNothing
	dw NULL,  AnimateFlowerTile
	dw WhirlpoolFrames1, AnimateWhirlpoolTile
	dw WhirlpoolFrames2, AnimateWhirlpoolTile
	dw WhirlpoolFrames3, AnimateWhirlpoolTile
	dw WhirlpoolFrames4, AnimateWhirlpoolTile
	dw vTiles2 tile $1c, AnimateRainPuddleTile
	dw vTiles2 tile $1d, AnimateRainWaterTile
	dw NULL,  StandingTileFrame8
	dw vTiles2 tile $35, WriteTileToBuffer
	dw NULL,  DoNothing
	dw wTileAnimBuffer, ScrollTileDown
	dw NULL,  DoNothing
	dw wTileAnimBuffer, ScrollTileDown
	dw NULL,  DoNothing
	dw wTileAnimBuffer, ScrollTileDown
	dw NULL,  DoNothing
	dw vTiles2 tile $35, ReadTileFromBuffer
	dw NULL,  DoneTileAnimation

