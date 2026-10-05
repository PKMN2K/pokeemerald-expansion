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


## Kanto Cave mining surface batch 7

`gTileset_PrismKantoCave` now contains **198 placeable mining metatiles**. IDs 0-166 are preserved exactly; IDs 167-197 append all 31 verified `MINING` quadrants from source blocks `0x70` through `0x7F` where listed by Prism's collision data.

All appended quadrants are uniform source attribute `0x05` or `0x06`, with no X/Y flips, alternate VRAM bank, or priority flags. Target entries remain direct `0x52xx`/`0x62xx` translations. Existing maps remain untouched.

The remaining Kanto Cave mining work is now the earlier source quadrants that were skipped before the sequential `0x1B`-`0x7F` pass; those can be audited and appended next.


## Kanto Cave mining surface completion

`gTileset_PrismKantoCave` now contains **all 242 Prism mining quadrants** as placeable `MB_PRISM_MINING` metatiles. IDs 0-197 are preserved exactly; IDs 198-241 append the final 44 earlier source quadrants that were skipped during the sequential expansion passes.

The final 44 are uniform source attribute `0x05` or `0x06`, with no X/Y flips, alternate VRAM bank, or priority flags. The complete target files are 3,872 bytes of metatile data and 484 bytes of metatile attributes.

Kanto Cave mining terrain conversion is now complete. Existing maps remain untouched; map placement and mining gameplay integration stay isolated for later steps.


## Olcan Isle / Olcan Chine staging

Prism tileset 55 (`TILESET_OLCAN_ISLE`, source ID `0x37`) is staged at `graphics/prism_mining/olcan_isle_tileset55.png` for the next mining terrain group.

Its collision table contains **110 `MINING` quadrants across 59 metatiles**. The source tileset is used by Olcan Isle (`PALETTE_AUTO`), Olcan Chine (`PALETTE_AUTO`), and Olcan Chine Entrance (`PALETTE_NITE`), so live palette wiring is intentionally deferred until its day/night palette usage is audited.

No maps or live target tilesets are changed by this staging step.


## Olcan palette and attribute audit

Olcan's flagged source attributes are now decoded and preserved rather than flattened. `0x25` is palette 5 + X-flip; `0x0E` is palette 6 + VRAM bank 1; `0x2E` is palette 6 + VRAM bank 1 + X-flip. Prism's loader places the first `0x80` tiles in VRAM bank 0 and the next `0x80` in bank 1, so bank-1 tile IDs are linearized by adding `0x80` in the GBA target (`0x07 -> 0x87`, `0x08 -> 0x88` in the observed mining surfaces).

The exact Prism `olcan_isle.pal` and `tunod.pal` sources are retained, and all **Morning / Day / Night** variants for their eight palettes are translated to JASC-PAL under `data/tilesets/secondary/prism_olcan_isle/palette_variants/`. Olcan Isle uses its dedicated palette source; Olcan Chine uses Tunod's palette source.

For the imported 2bpp-indexed art, each four-color source palette is stored in target palette slots 12-15. Live wiring remains isolated for the next step; no existing maps are changed.


## First live Olcan mining surface

Olcan now has a live shared mining tileset source. Prism block `0x04` top-left is exposed as target metatile `0`, using source tiles `0x2C, 0x0B, 0x1A, 0x1B`, all palette 5, with `MB_PRISM_MINING` behavior.

Two target secondary tilesets share the same 176-tile graphics and metatile data: `gTileset_PrismOlcanIsleDay` uses the authentic Olcan Isle Day palettes, while `gTileset_PrismOlcanChineDay` uses the authentic Tunod/Olcan Chine Day palettes. Palette slots 8-15 are unused and zero-filled.

Existing maps remain untouched; these tilesets are available only for deliberate later map assignment.


## Olcan mining surfaces batch 1

`gTileset_PrismOlcanIsleDay` and `gTileset_PrismOlcanChineDay` now share **10 live `MB_PRISM_MINING` metatiles**. IDs 1-9 append the next source-order Prism mining quadrants after the original block `0x04` TL surface.

This batch validates source X-flip preservation in live data: Prism attribute `0x25` is translated to GBA metatile entries with the horizontal-flip bit set (for example `0x561B`, `0x562B`, `0x563B`). No bank-1 surface appears in this first expansion batch.

Existing maps remain untouched.


## Olcan mining surfaces batch 2

The shared Olcan target now contains **20 live `MB_PRISM_MINING` metatiles**. IDs 10-19 append the next ten source-order mining quadrants, from Prism block `0x0C` TL through block `0x14` BR.

This batch adds three more surfaces containing source X-flip attribute `0x25`; those remain encoded with the GBA horizontal-flip bit. No VRAM-bank-1 mining surface appears yet.

Existing maps remain untouched.


## Olcan mining surfaces batch 3

The shared Olcan target now contains **32 live `MB_PRISM_MINING` metatiles**. IDs 20-31 append twelve source-order mining quadrants from Prism block `0x16` BL through block `0x1D` TR.

This batch preserves additional X-flipped surfaces and introduces mixed source palette usage inside a mining surface: blocks `0x17` and `0x1B` combine palette 3 and palette 5 entries exactly as Prism does. No VRAM-bank-1 mining surface appears yet; the first such surface is source-order target ID 96.

Existing maps remain untouched.


## Olcan mining surfaces batch 4

The shared Olcan target now contains **48 live `MB_PRISM_MINING` metatiles**. IDs 32-47 append the next sixteen source-order mining quadrants, beginning at Prism block `0x1E` TL and continuing through block `0x39` TL.

The source-order gap from block `0x1E` to `0x2E` is intentional: blocks in between contain no `MINING` collision quadrants. Four surfaces in this batch preserve source X-flip attribute `0x25`; no new bank-1 or mixed-palette cases occur here.

Existing maps remain untouched.


## Olcan mining surfaces batch 5

The shared Olcan target now contains **64 live `MB_PRISM_MINING` metatiles**. IDs 48-63 append sixteen source-order mining quadrants from Prism block `0x39` TR through block `0x47` BR.

This is a flip-heavy batch: ten of the sixteen new surfaces preserve Prism's source X-flip attribute `0x25`. No bank-1 or mixed-palette mining surfaces occur in this range.

Existing maps remain untouched.


## Olcan mining surfaces batch 6

The shared Olcan target now contains **80 live `MB_PRISM_MINING` metatiles**. IDs 64-79 append sixteen source-order mining quadrants from Prism block `0x4C` BL through block `0x60` TL.

Two surfaces in this batch preserve source X-flip attribute `0x25`. No bank-1 or mixed-palette mining surfaces occur in this range.

Existing maps remain untouched.


## Olcan mining surfaces batch 7

The shared Olcan target now contains **96 live `MB_PRISM_MINING` metatiles**. IDs 80-95 append sixteen source-order mining quadrants from Prism block `0x60` BL through block `0x81` TL.

Three surfaces in this batch preserve source X-flip attribute `0x25`. No bank-1 entries are included intentionally: target ID `96` (Prism block `0x81` BL) is the first bank-1 mining surface, so it is reserved for an isolated validation step.

Existing maps remain untouched.


## Olcan first bank-1 mining surface

Target metatile **ID 96** is the first live Olcan mining surface that exercises Prism's VRAM bank-1 attribute.

Prism block `0x81` BL uses source tiles `0x4A, 0x4B, 0x08, 0x07` with attributes `0x05, 0x05, 0x0E, 0x0E`. The lower two tiles are palette 6, bank 1, so they linearize to target tile IDs `0x88` and `0x87`. The resulting target entries are `0x524A, 0x524B, 0x6288, 0x6287`.

This single-surface commit intentionally isolates the first bank-1 transition for CI validation. Existing maps remain untouched.


## Olcan mining surface conversion complete

The shared Olcan target now contains **all 110 Prism `MINING` collision quadrants** as live `MB_PRISM_MINING` metatiles. Final IDs 97-109 cover Prism block `0x81` BR through block `0xA5` BR.

Target ID `97` preserves the remaining mixed bank/palette/flip case: source attributes `0x05, 0x05, 0x2E, 0x0E` translate the lower-left bank-1, palette-6, X-flipped tile to `0x6687` and the lower-right bank-1, palette-6 tile to `0x6288`.

Olcan mining-surface coverage is now **110 / 110**. Existing maps remain untouched; map assignment and dynamic morning/night palette switching are separate follow-up work.


## Mound Cave mining surfaces batch 1

Mound Cave (`TILESET_CAVE`, Prism source ID `0x15`) is the next incomplete mining terrain group. Its source collision data contains **104 `MINING` quadrants across 49 metatiles**.

The pre-existing live target metatile `0` remains Prism block `0x1D` TL. IDs `1-16` now append the first sixteen source-order mining quadrants (`0x01` TR through `0x0C` BR), skipping no source surfaces in that range.

Mound Cave source mining attributes are palette-only (`0x04` and `0x06`); this first expansion batch uses palette 6 throughout and requires no bank or flip translation. Authentic Prism Night dungeon palettes remain wired through `gTilesetPalettes_PrismMoundCave`.

Existing maps remain untouched.


## Mound Cave mining surfaces batch 2

Target IDs `17-32` append the next sixteen source-order Mound Cave `MINING` quadrants, from Prism block `0x0D` BL through `0x19` TL. Together with preserved target ID `0` (`0x1D` TL), live Mound Cave coverage is now **33 / 104**.

Target ID `28` (Prism block `0x17` TR) is the first mixed-palette Mound Cave surface: source attributes `0x04, 0x04, 0x06, 0x06` translate to `0x421A, 0x421B, 0x6201, 0x6200`. This batch still requires no bank or flip translation.

Existing maps remain untouched. The next source-order surface is Prism block `0x19` TR; when expansion reaches `0x1D` TL it must be skipped because that surface is already preserved as target ID `0`.


## Mound Cave mining surfaces batch 3

Target IDs `33-48` append the next sixteen **unique** Prism Mound Cave mining quadrants. The source walk begins at `0x19` TR and ends at `0x21` BR. Prism `0x1D` TL is deliberately skipped during this batch because that exact surface is already preserved as target ID `0`.

This raises Mound Cave coverage to **49 / 104** source `MINING` quadrants with **49 live target metatiles** and no duplicate copy of the preserved surface. Source attributes in this batch remain palette-only; no bank or flip translation is required.

Existing maps remain untouched. The next source-order surface is Prism block `0x22` TR.


## Mound Cave mining surfaces batch 4

Target IDs `49-64` append the next sixteen source-order Prism Mound Cave mining quadrants, from `0x22` TR through `0x2E` BL. This raises live coverage to **65 / 104** source `MINING` quadrants.

The preserved Prism `0x1D` TL surface remains target ID `0`; no duplicate is introduced. Existing maps remain untouched.

The next source-order surface is Prism block `0x2E` BR.


## Mound Cave mining surfaces batch 5

Target IDs `65-80` append the next sixteen source-order Prism Mound Cave mining quadrants, from `0x2E` BR through `0x36` TR. This raises live coverage to **81 / 104** source `MINING` quadrants.

The preserved Prism `0x1D` TL surface remains target ID `0`; existing maps remain untouched.

The next source-order surface is Prism block `0x36` BR.


## Mound Cave mining surfaces batch 6

Target IDs `81-96` append the next sixteen source-order Prism Mound Cave mining quadrants, from `0x36` BR through `0x52` BR. This raises live coverage to **97 / 104** source `MINING` quadrants.

The preserved Prism `0x1D` TL surface remains target ID `0`; existing maps remain untouched.

Seven Mound Cave mining quadrants remain, beginning at Prism block `0x53` BL and ending at `0x57` BR.


## Mound Cave mining surfaces final batch

Target IDs `97-103` add the final seven Prism Mound Cave mining quadrants, from `0x53` BL through `0x57` BR. Mound Cave now has **104 / 104** source `MINING` quadrants converted and live in the reusable target tileset.

The preserved Prism `0x1D` TL surface remains target ID `0`, with every other source mining quadrant represented exactly once across IDs `1-103`. Existing maps remain untouched.

Mound Cave mining-surface conversion is complete; the next step is the next incomplete Prism mining terrain group or later map/palette wiring.


## Intentionally skipped Prism cave variants

Prism tilesets `TILESET_CAVE2` through `TILESET_CAVE7` (source IDs `0x30-0x35`) are intentionally **not** being imported into the polished-crystal-port mining asset set. The existing Firelight Caverns, Kanto Cave, Mound Cave, and Olcan Isle terrain groups provide sufficient mining visual variety, so these additional cave variants are excluded to avoid redundant asset accumulation.

Do not treat CAVE2-CAVE7 as incomplete migration work unless this decision is explicitly revisited later.


## Mining interaction integration

The imported `MB_PRISM_MINING` behavior is now connected to the normal A-button metatile interaction path. Interacting with any live Prism mining surface calls `EventScript_PrismMining` and displays a neutral mining-surface message.

This integration step deliberately adds **no** Prism mining EXP, rewards, pickaxe/item requirements, random encounters, or tile mutation. It establishes a safe engine-level interaction hook that later mining mechanics can replace or extend without changing the imported art.


## Intentionally skipped Prism sidescroll tileset

Prism `TILESET_SIDESCROLL` (source ID `0x20`) is intentionally **not** being imported into the polished-crystal-port mining asset set.

Do not treat the sidescroll tileset as incomplete migration work unless this decision is explicitly revisited later.


## Mining surface import scope closed

No additional Pokémon Prism mining terrain or mineable surface tilesets are to be imported into `polished-crystal-port`.

The existing imported terrain pool—Firelight Caverns, Kanto Cave, Mound Cave, and Olcan Isle—is the final mining-surface set for this project. Previously skipped cave variants and `TILESET_SIDESCROLL` remain excluded, and other Prism tilesets containing `MINING` collision should likewise not be treated as pending surface migration work.

Future Prism mining migration should focus on mining-specific non-terrain assets and integration, such as debris/dust effects, ore/gem/fossil/item-reveal artwork, other non-player mining animation frames, and the mining interaction/mechanics layer.


## Prism non-terrain mining artwork audit

The pinned Pokémon Prism source implementation was audited before importing any additional effect artwork.

Prism's `MiningScript` does **not** invoke a dedicated pickaxe swing, rock-debris/dust animation, ore/gem/fossil reveal graphic, or mining-specific item-reveal animation. Its visual flow remains on the existing mineable surface, plays `SFX_BEAT_UP`, resolves the extracted item, and then uses normal text/item handling.

The source asset `gfx/field/boulderdust.png` is **not mining-specific**. Prism loads `BoulderDustGFX` as a generic indoor overworld emote and it is not referenced by `event/mining.asm`. It should therefore not be imported or documented as authentic Prism mining artwork.

No extra non-terrain mining-effect graphics are pending from this source audit. The authentic Prism mining visual asset migration is therefore represented by the already imported mining surfaces; future work can focus on interaction/mechanics or on new custom effects if explicitly desired.

## Custom mining reward policy

The original Pokémon Prism mining reward table is **not part of this port's gameplay design**. The imported Prism work supplies mining surfaces, interaction provenance, and presentation behavior only.

Reward selection is project-owned through `MiningRollCustomReward()`. Until the PKMN 2K mining loot table is explicitly designed, that hook returns `ITEM_NONE`; Prism's probabilities, mining-level reward windows, map-specific special rewards, and Prism-only item substitutions must not be reintroduced by default.

## Mining eligibility hook

Mining access is now routed through `MiningCanInteract()` before any mining sound or reward roll occurs. The hook currently returns `TRUE`, so gameplay is unchanged: every `MB_PRISM_MINING` surface remains usable.

This deliberately leaves the eventual requirement open. A later PKMN 2K design can gate mining behind a key item, Trainer Skill, quest flag, or another condition by changing one function rather than modifying imported tilesets or map interactions.
