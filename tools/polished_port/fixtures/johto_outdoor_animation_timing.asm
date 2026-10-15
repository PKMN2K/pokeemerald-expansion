; Authentic original Polished Crystal animation dispatch/selection snippets.
; Rangi42/polishedcrystal at 3fa43192379df5c3e7b09a08e4d5d79af4f02f42
; Exact source: engine/tilesets/tileset_anims.asm
; Extracted whole original contiguous line ranges; not re-authored.

; Original lines 344-360
DoneTileAnimation:
; Reset the animation command loop.
	xor a
	ldh [hTileAnimFrame], a
	ret

StandingTileFrame:
	ld hl, wTileAnimationTimer
	inc [hl]
	ret

StandingTileFrame8:
	ld a, [wTileAnimationTimer]
	inc a
	and %111
	ld [wTileAnimationTimer], a
	ret

; Original lines 502-525
AnimateWaterTile:
	ld hl, sp + 0
	ld b, h
	ld c, l

	; period 4, every 2 frames, offset to 1 tile (16 bytes)
	ld a, [wTileAnimationTimer]
	maskbits 4, 1
	add a
	add a
	add a

	add LOW(.WaterTileFrames)
	ld l, a
	adc HIGH(.WaterTileFrames)
	sub l
	ld h, a

	ld sp, hl
	jmp WriteTileToDE

.WaterTileFrames:
INCBIN "gfx/tilesets/water/johto_water.2bpp"


; Original lines 526-547
AnimateRainPuddleTile:
	ld hl, sp + 0
	ld b, h
	ld c, l

	; period 8, offset to 1 tile (16 bytes)
	ld a, [wTileAnimationTimer]
	maskbits 8
	swap a

	add LOW(.RainPuddleTileFrames)
	ld l, a
	adc HIGH(.RainPuddleTileFrames)
	sub l
	ld h, a

	ld sp, hl
	jmp WriteTileToDE

.RainPuddleTileFrames:
INCBIN "gfx/tilesets/rain/rain_puddle.2bpp"


; Original lines 548-569
AnimateRainWaterTile:
	ld hl, sp + 0
	ld b, h
	ld c, l

	; period 8, offset to 1 tile (16 bytes)
	ld a, [wTileAnimationTimer]
	maskbits 8
	swap a

	add LOW(.RainWaterTileFrames)
	ld l, a
	adc HIGH(.RainWaterTileFrames)
	sub l
	ld h, a

	ld sp, hl
	jmp WriteTileToDE

.RainWaterTileFrames:
INCBIN "gfx/tilesets/rain/rain_water.2bpp"


; Original lines 865-890
AnimateFlowerTile:
	ld hl, sp + 0
	ld b, h
	ld c, l

	; period 2, every 2 frames, offset to 1 tile (16 bytes)
	ld a, [wTileAnimationTimer]
	maskbits 2, 1
	add a
	add a
	add a

	add LOW(.FlowerTileFrames)
	ld l, a
	adc HIGH(.FlowerTileFrames)
	sub l
	ld h, a

	ld sp, hl
	ld hl, vTiles2 tile $03
	jmp WriteTile

.FlowerTileFrames:
INCBIN "gfx/tilesets/flower/1.2bpp"
INCBIN "gfx/tilesets/flower/2.2bpp"


; Original lines 1054-1081
AnimateWhirlpoolTile:
	ld hl, sp + 0
	ld b, h
	ld c, l

	ld l, e
	ld h, d
	ld a, [hli]
	ld e, a
	ld a, [hli]
	ld d, a

	; period 4, offset to 1 tile (16 bytes)
	ld a, [wTileAnimationTimer]
	maskbits 4
	swap a

	add [hl]
	inc hl
	ld h, [hl]
	ld l, a
	adc h
	sub l
	ld h, a

	ld sp, hl
	jmp WriteTileToDE


; Original lines 1241-1249
WhirlpoolFrames1: dw vTiles2 tile $31, WhirlpoolTiles1
WhirlpoolFrames2: dw vTiles2 tile $32, WhirlpoolTiles2
WhirlpoolFrames3: dw vTiles2 tile $41, WhirlpoolTiles3
WhirlpoolFrames4: dw vTiles2 tile $42, WhirlpoolTiles4

WhirlpoolTiles1: INCBIN "gfx/tilesets/whirlpool/1.2bpp"
WhirlpoolTiles2: INCBIN "gfx/tilesets/whirlpool/2.2bpp"
WhirlpoolTiles3: INCBIN "gfx/tilesets/whirlpool/3.2bpp"
WhirlpoolTiles4: INCBIN "gfx/tilesets/whirlpool/4.2bpp"
