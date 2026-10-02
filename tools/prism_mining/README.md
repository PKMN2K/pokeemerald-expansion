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


## Emerald mining metatile behavior

Expansion now reserves `MB_PRISM_MINING` as a dedicated metatile behavior for Prism-style mineable surfaces. It is appended after the existing behavior IDs, so no existing behavior numbers are renumbered. `MetatileBehavior_IsPrismMining()` provides the runtime classifier that later mining interaction code can call.

This step deliberately does **not** assign the behavior to existing Hoenn/FRLG cave tiles and does not implement rewards, pick durability, EXP, or player graphics. Mineable tiles will be opted in explicitly when Prism-derived cave geometry is wired into maps.


## First placeable Prism mining surface

A dedicated Expansion secondary tileset, `gTileset_PrismMoundCave`, now exposes the first authentic Prism mining surface without modifying any existing Hoenn/FRLG cave tiles.

The initial 16x16 metatile is the **top-left quadrant of Prism Mound Cave block 0x1D**. Prism's block data uses source tiles `0x0c, 0x0d, 0x1c, 0x1d`; its collision entry marks all four 16x16 quadrants of block `0x1D` as `MINING`. The GBA metatile references those exact source tiles and is tagged `MB_PRISM_MINING`.

The full authentic 128x56 Prism tilesheet is used directly as this secondary tileset's tile source. Existing maps are unchanged: this surface becomes active only when a map deliberately selects `gTileset_PrismMoundCave` and places metatile 0. For this geometry-only step, the tileset temporarily uses Expansion's Cave palette bank; Prism's time-of-day cave palette translation remains a separate art step.


## Authentic Prism Mound Cave night palettes

The dedicated `gTileset_PrismMoundCave` now uses a Prism-derived palette bank instead of borrowing Expansion's Cave palettes. Mound Cave is declared with `PALETTE_NITE`, so palettes 0-7 come from the `;Night` dungeon palette row in Prism's `tilesets/bg.pal`.

Prism's tileset PNG is 2-bit grayscale. Expansion's `gbagfx` inverts grayscale when converting to 4bpp, so original Prism color indices 0-3 become GBA indices 12-15. Each converted JASC palette therefore stores its four authentic Prism RGB555 colors in slots 12-15; slots 0-11 are unused by this imported tilesheet. Source RGB5 channels are expanded with `(v << 3) | (v >> 2)`, which round-trips exactly to the original 5-bit values.

Palette slots 8-15 are currently zero-filled because Prism tileset 21 only references source palettes 0, 3, 4, and 6; keeping all 16 target slots present satisfies Expansion's secondary-tileset palette layout without inventing colors.


## Firelight Caverns mining tileset (tileset 33)

The next authentic mining terrain group is Prism's `TILESET_FIRELIGHT_CAVERNS` (`0x21` / decimal 33). Its source graphic `gfx/tilesets/33.png` is a 128x56, 2-bit grayscale tilesheet and is now staged byte-for-byte as `graphics/prism_mining/firelight_caverns_tileset33.png`.

The source collision table contains **55 mineable metatiles**. As with Mound Cave, mining is collision-driven: there is no separate crack/debris animation and normal mining does not mutate the terrain tile. The new manifest records the exact mineable quadrants for later Emerald metatile conversion.

Firelight Caverns is now wired as an isolated live Expansion secondary tileset. No existing maps select it yet, and no mining rewards, EXP, pick durability, player graphics, or unrelated Prism systems are imported.


## Authentic Prism Firelight Caverns day palettes

`gTileset_PrismFirelightCaverns` now uses a dedicated Prism-derived day palette bank instead of borrowing Expansion's Cave palettes. Prism's Firelight cave maps use `PALETTE_DAY`, and the dungeon color table maps day to palette records `$08-$0F` in `tilesets/bg.pal`.

As with the Mound Cave import, Prism's 2-bit grayscale tile indices are translated by `gbagfx` into GBA indices 12-15, so each source four-color palette is stored in target palette entries 12-15. RGB5 channels are expanded with `(v << 3) | (v >> 2)` for exact GBA round-tripping. Target palette banks 8-15 remain zero-filled because this Prism tileset uses the eight dungeon palette slots.


## Firelight mining surface batch 1

`gTileset_PrismFirelightCaverns` now contains **9 placeable mining metatiles**. Metatile 0 remains block `0x1D` TL; metatiles 1-8 add authentic mineable quadrants from blocks `0x17`, `0x23`, `0x31`, and `0x34`.

This batch is intentionally conservative: every included source quadrant is marked `MINING`, and every source tile attribute is exactly `0x06` (palette 6 with no flip flags). That lets the conversion remain byte-for-byte in tile choice while using the already translated authentic Firelight day palette. Existing maps remain untouched.


## Firelight mining surface conversion complete

All **127 mineable 16x16 quadrants** from Prism Firelight Caverns tileset 33 are now available as placeable `MB_PRISM_MINING` metatiles in `gTileset_PrismFirelightCaverns`.

A source-attribute audit found that the mineable quadrants use only attributes `0x06` and `0x04`: palette 6 or palette 4, with **no X flip, Y flip, or alternate VRAM-bank usage**. Because both authentic Prism day palettes were already imported, every remaining mining quadrant could be converted directly without inventing flip handling or repainting tiles.

The first nine metatile IDs were preserved exactly; the remaining 118 surfaces were appended. Existing maps are still untouched and must opt into the Firelight secondary tileset explicitly.


## Kanto Cave mining tileset (tileset 27)

The next staged Prism mining terrain group is `TILESET_CAVE_KANTO` (`0x1B` / decimal 27). Its authentic source graphic `gfx/tilesets/27.png` is a 128x40, 2-bit grayscale tilesheet and is now staged byte-for-byte as `graphics/prism_mining/kanto_cave_tileset27.png`.

Its collision table contains **101 mineable metatiles** covering **242 mineable 16x16 quadrants**, including **25 fully mineable metatiles**. Prism uses this terrain in Kanto cave locations including Silk Tunnel, Mt. Boulder, and Eagulou Gym F1.

This step is asset-and-manifest staging only. Kanto Cave is not yet wired as a live Expansion tileset, and no mining rewards, EXP, durability, player graphics, or unrelated Prism systems are imported.


## First live Kanto Cave mining surface

`gTileset_PrismKantoCave` is live with one placeable `MB_PRISM_MINING` surface from Prism block `0x02`, top-left quadrant. Its source tiles are `0x04, 0x29, 0x31, 0x05`, all using source palette attribute `0x05` with no flip or bank flags.

## Dedicated Kanto Cave Night palettes

`gTileset_PrismKantoCave` now uses its own `gTilesetPalettes_PrismKantoCave` table. The 16 palette files are exact blob-for-blob copies of the already verified Prism Night dungeon palette translation used by Mound Cave, because Kanto Cave source maps use the same `PALETTE_NITE` / `$10-$17` dungeon palette context.

This removes the temporary shared-symbol bridge without changing any colors. Palette banks 0-7 contain the authentic Prism Night palettes and 8-15 remain the same zero-filled unused banks.


## Kanto Cave mining surface batch 1

`gTileset_PrismKantoCave` now contains **9 placeable mining metatiles**. Metatile 0 remains the original block `0x02` TL surface; metatiles 1-8 add block `0x02` TR/BL/BR, all four quadrants of block `0x17`, and block `0x1B` TL.

Every appended source quadrant is marked `MINING`, and every constituent source tile uses attribute `0x05` (palette 5, bank 0, no X/Y flips). Existing maps remain untouched.


## Kanto Cave mining surface batch 2

`gTileset_PrismKantoCave` now contains **24 placeable mining metatiles**. IDs 0-8 are preserved exactly; IDs 9-23 add block `0x1B` TR/BL/BR plus all four quadrants from blocks `0x1F`, `0x20`, and `0x21`.

All 15 appended quadrants are source `MINING` surfaces and every constituent tile uses attribute `0x05` (palette 5, bank 0, no X/Y flips), so no speculative attribute translation was required. Existing maps remain untouched.


## Kanto Cave mining surface batch 3

`gTileset_PrismKantoCave` now contains **46 placeable mining metatiles**. IDs 0-23 are preserved exactly; IDs 24-45 append 22 verified `MINING` quadrants from source blocks `0x22`, `0x23`, `0x25`, `0x26`, `0x2B`-`0x2E`, `0x30`, and `0x31`.

Every appended tile attribute is still `0x05` (palette 5, bank 0, no X/Y flips), so this batch remains a direct source-tile conversion with no speculative flip handling. Existing maps remain untouched.


## Kanto Cave mining surface batch 4

`gTileset_PrismKantoCave` now contains **78 placeable mining metatiles**. IDs 0-45 are preserved exactly; IDs 46-77 append 32 verified `MINING` quadrants from source blocks `0x32`, `0x34`-`0x35`, `0x37`-`0x3B`, and `0x3F`-`0x43`.

This batch introduces authentic palette-6 mining surfaces alongside palette 5. Every converted quadrant is uniform `0x05` or `0x06`, with no X/Y flips, alternate VRAM bank, or priority flags. Target entries therefore map directly to `0x52xx` or `0x62xx`. Existing maps remain untouched.


## Kanto Cave mining surface batch 5

`gTileset_PrismKantoCave` now contains **132 placeable mining metatiles**. IDs 0-77 are preserved exactly; IDs 78-131 append 54 verified `MINING` quadrants from source blocks `0x44` through `0x58` where listed by Prism's collision data.

All appended quadrants are uniform source attribute `0x05` or `0x06`, with no X/Y flips, alternate VRAM bank, or priority flags. Target entries therefore remain direct `0x52xx`/`0x62xx` translations. Existing maps remain untouched.


## Kanto Cave mining surface batch 6

`gTileset_PrismKantoCave` now contains **167 placeable mining metatiles**. IDs 0-131 are preserved exactly; IDs 132-166 append 35 verified `MINING` quadrants from source blocks `0x59` through `0x6F` where listed by Prism's collision data.

All appended quadrants are uniform source attribute `0x05` or `0x06`, with no X/Y flips, alternate VRAM bank, or priority flags. Target entries remain direct `0x52xx`/`0x62xx` translations. Existing maps remain untouched.
