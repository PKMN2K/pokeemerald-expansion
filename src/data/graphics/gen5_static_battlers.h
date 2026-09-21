// Static 96x96 Gen 5-style battler assets.
//
// Keep the symbol names in this exact form so the registry in
// src/data/pokemon/gen5_static_battlers.h can refer to them.
//
// Bulbasaur is the first renderer validation species. Its current 96x96
// test canvases intentionally contain the existing 64x64 art bottom-aligned
// so positioning and tile assembly can be verified before final Gen 5 art
// is substituted.
static const u32 sGen5StaticFront_BULBASAUR[] =
    INCGFX_U32("graphics/pokemon_gen5_static/bulbasaur/front.png", ".4bpp.smol");
static const u32 sGen5StaticBack_BULBASAUR[] =
    INCGFX_U32("graphics/pokemon_gen5_static/bulbasaur/back.png", ".4bpp.smol");
static const u16 sGen5StaticPalette_BULBASAUR[] =
    INCGFX_U16("graphics/pokemon_gen5_static/bulbasaur/normal.pal", ".gbapal");
static const u16 sGen5StaticShinyPalette_BULBASAUR[] =
    INCGFX_U16("graphics/pokemon_gen5_static/bulbasaur/shiny.pal", ".gbapal");
