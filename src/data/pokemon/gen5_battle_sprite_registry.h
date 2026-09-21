/*
 * Static Gen 5-style 96x96 battle sprite registry.
 *
 * Add one entry for each species/form that has dedicated battle artwork:
 *
 *     GEN5_BATTLE_SPRITE(SPECIES_SNIVY, snivy)
 *
 * Each entry expects:
 *     graphics/pokemon_gen5/<folder>/front.png
 *     graphics/pokemon_gen5/<folder>/back.png
 *
 * The normal build rules convert those PNGs to .4bpp.lz automatically.
 * Keep each PNG exactly 96x96, indexed/4bpp-compatible, and use the same
 * palette-index ordering as the species' existing battle palette.
 *
 * This file is intentionally an X-macro list (no include guard).
 */
