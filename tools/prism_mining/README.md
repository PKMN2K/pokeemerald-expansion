# Pokémon Prism mining import

This directory tracks the import of the **non-player mining presentation assets** from Pokémon Prism into this pokeemerald-expansion branch.

## Goal

Bring Prism's mining rocks, wall/object states, break/debris effects, ore/gem/item reveal art, and any mining-specific supporting effects into the same final ROM as the Polished Crystal art migration.

This is **not** a separate game or build. The Prism mining files are kept in their own namespace only for provenance, maintenance, and debugging.

## Scope

Included:
- mineable rock / cliff / wall artwork
- damaged or cracked states
- break / debris / dust effects
- ore, gem, fossil, or item-reveal artwork used by the mining presentation
- non-player animation frames required to reproduce the mining visuals

Excluded:
- Prism player graphics
- Prism trainer graphics
- Prism mining EXP / leveling rules
- unrelated Prism progression systems
- unrelated Prism UI or overworld assets

## Source priority

1. **Pokémon Prism v0.95.0254 ROM supplied for this migration**
   - Game Boy title: `PM_PRISM`
   - size: 2,097,152 bytes
   - MD5: `7777fe98c1985ed73d024e2518a3a83b`
   - SHA-1: `752076692ae3387cf426ce5f51a98c6b60e8df6a`
2. RainbowDevs v0.95.0254 release source package, when an asset or symbol is available there.
3. `bottlebrushes/pokemon-prism-assets`, a derived graphics archive whose manifest identifies its source as the published Prism v0.95.0254 source tree. Use it as an indexing/reference aid and verify imported art against the authoritative ROM/source.
4. `kanzure/pokemon-prism` commit `97af399678fd87c125a05ef5c27ca8e5011ff588` as a historical code reference only. That old disassembly still bulk-includes most ROM banks and is not the authoritative source for the 0.95.0254 mining art.

## Import rules

- Do not resample or repaint authentic Prism pixels unless the target engine requires a packing-only transformation.
- Keep source frame order and animation semantics documented.
- Convert only the minimum format/layout needed by pokeemerald-expansion.
- Keep Prism-derived files under a dedicated `graphics/prism_mining/` and/or `src/prism_mining/` namespace unless an existing Expansion object must be wired directly.
- Every imported asset group gets a manifest recording source, frame layout, target path, and retained runtime behavior.
- CI must build the same shared ROM after each committed integration step.

## Next extraction pass

Identify and verify the first self-contained non-player mining visual group (rock/wall state plus any break effect) from the v0.95.0254 source/ROM before wiring it into Expansion.


## Mound Cave mining tileset (tileset 21)

Mound Cave 1F uses Prism's `TILESET_CAVE`, which is tileset `0x15` / decimal 21. The authentic source graphic is `gfx/tilesets/21.png` (128x56). Its blob SHA exactly matches the v0.95.0254-derived `bottlebrushes/pokemon-prism-assets` copy, so the imported PNG is preserved byte-for-byte under `graphics/prism_mining/mound_cave_tileset21.png`.

Mining is collision-driven rather than object-animation-driven: Prism marks eligible 16x16 quadrants of cave metatiles as `MINING`. The mining script plays `SFX_BEAT_UP`, rolls the result, and leaves the cave tile unchanged. There is therefore no separate Prism crack/break animation to import for normal mining. The manifest records the 49 tileset-21 metatiles containing at least one mineable quadrant so the later Emerald mechanic can reproduce the same eligible wall geometry.
