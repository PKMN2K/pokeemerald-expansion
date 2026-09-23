// Static 96x96 Gen 5-style battler assets.
//
// Keep the symbol names in this exact form so the registry in
// src/data/pokemon/gen5_static_battlers.h can refer to them.
//
// Zigzagoon is the renderer validation species because the Route 101 Birch
// rescue battle displays it immediately. Its 96x96 test canvases contain the
// existing 64x64 art bottom-aligned so composite positioning can be checked
// before final Gen 5-style artwork is substituted.
static const u32 sGen5StaticFront_ZIGZAGOON[] =
    INCGFX_U32("graphics/pokemon_gen5_static/zigzagoon/front.png", ".4bpp.smol");
static const u32 sGen5StaticBack_ZIGZAGOON[] =
    INCGFX_U32("graphics/pokemon_gen5_static/zigzagoon/back.png", ".4bpp.smol");
static const u16 sGen5StaticPalette_ZIGZAGOON[] =
    INCGFX_U16("graphics/pokemon_gen5_static/zigzagoon/normal.pal", ".gbapal");
static const u16 sGen5StaticShinyPalette_ZIGZAGOON[] =
    INCGFX_U16("graphics/pokemon_gen5_static/zigzagoon/shiny.pal", ".gbapal");
