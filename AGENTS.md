# Polished Emerald

This branch ports the complete Polished Crystal game to pokeemerald-expansion.
Read `POLISHED_CRYSTAL_PORT.md` for the source pin and implemented phases.
Read `tools/polished_port/PARITY_AUDIT.md` before selecting the next phase;
it records inherited defaults and incomplete source coverage from earlier work.

## Playable baseline and future customization

The deliverable is Polished Crystal running on the GBA engine, with Expansion's
capabilities retained for later hacking. Preserve support for additional
Pokemon, moves, larger map data and other engine extensions. Do not shrink
target tables, identifiers or tools to Game Boy Color limits to reproduce the
source game. Use explicit mappings/adapters where the storage formats differ.

The default playable game must use Polished Crystal's roster, data, balance,
art and progression. Extra Expansion content and mechanics may remain available
for future customization, but must not enter the baseline's encounters,
trainers, learnsets, evolutions, rewards or rules just because the engine
supports them. Retaining unused optional data is not itself a migration gap;
using it in place of the corresponding Polished Crystal content is a gap.

## Source of truth

- Polished Crystal defines the game's battle behavior, Pokemon data, graphics,
  sprite metadata and animation, presentation, maps, events and other content.
  This includes ordinary mechanics, not only mechanics unique to the hack.
- Use the pinned Polished Crystal source, currently v3.2.3 at
  `3fa43192379df5c3e7b09a08e4d5d79af4f02f42`, plus individually documented
  supplemental sources. Do not silently upgrade it or mix source variants.
  The baseline is the source's default release build, with `FAITHFUL` undefined;
  preserving Polished Crystal's identity includes its own rebalances.
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

- Expansion data, art and behavior still substituting for Polished Crystal in
  the playable baseline must be recorded as temporary gaps until checked
  against and replaced/adapted to the source.
- Preserve source provenance and deterministic conversion wherever possible.
  Test migrated data and behavior against pinned source fixtures; verify live
  graphics and gameplay when those are affected.
- Passing the generic Expansion test suite establishes engine regression
  coverage, not Polished Crystal parity. Do not mark a whole phase complete
  solely because its maps, battles or builds run successfully.
- Keep stable target/save identifiers where possible through explicit adapters;
  do not silently change existing saves while importing source identifiers.
