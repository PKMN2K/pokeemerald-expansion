# Static 96x96 Gen 5 battle sprites

This fork supports opt-in static 96x96 Pokémon front and back sprites in battle while keeping the existing 64x64 assets for the Pokédex, party menu, summary screen, storage, and other UI.

## Add a species

1. Create `graphics/pokemon_gen5_static/<folder>/`.
2. Add `front.png` and `back.png` as indexed 96x96 images using one 16-colour palette (including transparency).
3. Add `normal.pal` and `shiny.pal`.
4. Add one line to `src/data/pokemon/gen5_static_battlers.h`:

   `GEN5_STATIC_BATTLER(SPECIES_NAME_WITHOUT_PREFIX, folder)`

Example:

`GEN5_STATIC_BATTLER(BULBASAUR, bulbasaur)`

The battle renderer repacks the 12x12 tile image into four GBA OBJ-compatible chunks (64x64, 32x64, 64x32, 32x32) and displays them as one logical battler.

Existing 64x64 species do not change. If a 64x64 battler Transforms into a species registered for 96x96 graphics, the engine safely falls back to that target species' normal 64x64 battle graphic because the original sprite did not reserve the larger OBJ tile allocation. A battler that entered battle using 96x96 graphics can change back and forth safely.
