# Polished Emerald

This branch ports the complete Polished Crystal game to pokeemerald-expansion.
Read `POLISHED_CRYSTAL_PORT.md` for the source pin and implemented phases.

## Source of truth

- Polished Crystal defines the game's battle behavior, Pokemon data, graphics,
  sprite metadata and animation, presentation, maps, events and other content.
  This includes ordinary mechanics, not only mechanics unique to the hack.
- Use the pinned Polished Crystal source, currently v3.2.3 at
  `3fa43192379df5c3e7b09a08e4d5d79af4f02f42`, plus individually documented
  supplemental sources. Do not silently upgrade it or mix source variants.
- pokeemerald-expansion supplies the GBA runtime and implementation framework.
  Port the source behavior into C/data/scripts when necessary. Retain a native
  implementation only where it reproduces the relevant Polished Crystal
  behavior; Expansion defaults and generation presets are not specifications.
- Import species/forms, stats, types, abilities, evolutions, all learnsets,
  breeding/growth/catch data, moves, items, trainer data, AI and battle rules
  from Polished Crystal. Map identifiers explicitly; matching names alone do
  not establish equivalent behavior or data.
- Import original Pokemon front/back sprites, icons, overworld sprites,
  trainer/player art, normal/shiny and form/gender palettes, and their source
  dimensions, frames, animation sequences/timing and positioning metadata.
  Convert storage/layout as the GBA requires while preserving source pixels,
  colors and visible behavior. Generic modern sprites or native animations
  are unfinished substitutes, not completion of an import.
- Separately requested additions retain their documented scope and source.
  Do not treat this requirement as permission to wire the standalone HGSS
  music backup files into the game.

## Completion and verification

- Remaining Expansion data, art and behavior in the port must be recorded as
  temporary gaps until checked against and replaced/adapted to the source.
- Preserve source provenance and deterministic conversion wherever possible.
  Test migrated data and behavior against pinned source fixtures; verify live
  graphics and gameplay when those are affected.
- Passing the generic Expansion test suite establishes engine regression
  coverage, not Polished Crystal parity. Do not mark a whole phase complete
  solely because its maps, battles or builds run successfully.
- Keep stable target/save identifiers where possible through explicit adapters;
  do not silently change existing saves while importing source identifiers.
