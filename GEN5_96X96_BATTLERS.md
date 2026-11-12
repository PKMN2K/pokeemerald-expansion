# 96x96 Gen 5-style battle sprites

This fork supports optional **battle-only static 96x96 front and back Pokémon sprites** while keeping the existing 64x64 graphics for the Pokédex, summary screen, storage, and any species that has not been converted.

## Asset format

Use an indexed 16-colour PNG on a **96x96 canvas**.

The GBA cannot display a single 96x96 OBJ. The battle engine renders the image as a 3x3 grid of 32x32 subsprites. The source PNG is still a normal 96x96 image; `gbagfx` must only be told to arrange its tiles as 32x32 metatiles:

```c
const u32 gMonBattleFrontPic_Example[] =
    INCGFX_U32("graphics/pokemon/example/battle_front.png", ".4bpp.smol", "-mwidth 4 -mheight 4");
const u32 gMonBattleBackPic_Example[] =
    INCGFX_U32("graphics/pokemon/example/battle_back.png", ".4bpp.smol", "-mwidth 4 -mheight 4");

const u16 gMonBattlePalette_Example[] =
    INCGFX_U16("graphics/pokemon/example/battle_front.png", ".gbapal");
const u16 gMonBattleShinyPalette_Example[] =
    INCGFX_U16("graphics/pokemon/example/battle_front_shiny.png", ".gbapal");
```

The 96x96 path currently treats front and back art as **one static frame**. The engine duplicates that frame internally so species whose normal front animation requests frame 1 remain safe.

## Species data

Add the battle-only fields to the species entry:

```c
.battleFrontPic = gMonBattleFrontPic_Example,
.battleBackPic = gMonBattleBackPic_Example,
.battlePalette = gMonBattlePalette_Example,
.battleShinyPalette = gMonBattleShinyPalette_Example,
.battlePicSize = MON_BATTLE_PIC_96,
```

If `battlePicSize` is left at its default, the normal 64x64 path is unchanged. If a 96x96 front or back pointer is missing, that side also falls back to the normal 64x64 image.

## What is size-aware

The battle graphics buffer and OBJ allocation can change between 64x64 and 96x96 at runtime. This covers ordinary send-outs and switches as well as Transform, Illusion, Substitute transitions, and ghost/trainer fallbacks.

The existing 64x64 species graphics remain the source used outside battle.

## Positioning

A 96x96 canvas is shifted upward by 16 pixels relative to the 64x64 canvas so its bottom edge retains the existing battle ground line. The normal `frontPicYOffset` and `backPicYOffset` data are still respected.

## GBA limits

A 96x96 battler consumes 144 4bpp OBJ tiles and nine OAM entries while visible. This is substantially heavier than a 64x64 battler, so unusual scenes with many additional OBJ effects should still be tested on hardware-accurate emulators.
