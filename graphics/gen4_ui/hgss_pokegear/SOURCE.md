# Verified HGSS PokéGear sources

## Main app-switch surface — phase 1

This directory starts the authenticity-first replacement of the current
PokéGear/PokéNav presentation.

The live GBA PokéGear currently contains several programmatically drawn
"HGSS-style" panels. Those are not accepted as final production artwork under
the project's authenticity-only policy. Before replacing them, this phase
imports the exact retail HeartGold/SoulSilver source used by the PokéGear
app-switch layer.

Retail mapping is verified against `pret/pokeheartgold` revision
`9d8b7591f09b65804da2fb2dfd56f320633e0d36`, in
`src/application/pokegear/main/overlay_100_021E5900.c`.

For default skin 0, `PokegearApp_LoadSkinGraphics` selects:

- member 48 character graphics for the main PokéGear BG,
- member 30 BG palette,
- member 54 screen/tilemap data for the app-switch layer.

`PokegearApp_DrawAppButtons` then copies that member-54 screen data into the
live app-switch strip.

The three files in `verified/` are copied byte-for-byte from the retail HGSS
decomp/extraction. No pixels, palette entries, tile indices, or geometry have
been redrawn or synthesized.

This is **phase 1 only**. Nothing is wired into the GBA PokéGear yet, and no
legacy PokéNav/PokéGear equivalent is removed in this commit.

## Main app-switch surface — phase 2

The verified retail HGSS app-switch source is now live in the GBA PokéGear.

`tools/gen4_ui/make_hgss_pokegear_app_switch.py` reconstructs the normal
member-54 app-switch state from the exact member-48 tile pixels and member-30
palette. The DS source is 256x32. Its only adaptation is removing the empty
8-pixel margin at each side, yielding a 240x32 GBA-width strip. Source pixels
remain 1:1; there is no scaling, redrawing, recoloring, or interpolation.

The live strip is loaded on BG2 at tile base `0x100` using dedicated palette
bank 14 and replaces only tilemap rows 0..3. Existing option sprites, cursor
movement, menu input, descriptions, transitions, and submenu logic are
unchanged.

The previous PokéNav device chrome intentionally remains loaded underneath the
retail strip for this phase-2 verification cycle. It is not removed until CI
confirms the live integration.

### Phase-2 build portability correction

CI #620 showed that the general ROM/test jobs do not install Pillow. The
app-switch generator therefore no longer depends on `PIL.Image`.

The generator now decodes the committed 4-bit indexed member-48 PNG and writes
the 240x32 indexed output using only Python's standard library
(`struct`, `zlib`, and CRC32). The retail source members, NSCR reconstruction,
BGR555 colors, 1:1 geometry, and empty-margin-only crop are unchanged.

No legacy PokéGear artwork is removed by this correction.

## Main app-switch surface — phase 3

CI #621 passed the corrected phase-2 integration across Gen 4 UI validation,
Emerald, FireRed, LeafGreen, release, docs, and the general test suite.

The superseded legacy app-switch region has now been removed from
`graphics/pokenav/device_outline_map.bin`. Tilemap rows 0..3 are permanently
blanked with the map's canonical blank entry (`0x2000`) before the authentic
HGSS strip is installed.

This means the verified retail HGSS strip is now the **only live background**
for the app-switch surface rather than an overlay hiding old artwork. Seven
legacy tiles that were exclusive to those rows (4, 8, 9, 12, 16, 17, and 20)
are no longer referenced by the live tilemap.

The shared legacy device character sheet is not deleted yet because other tiles
still render the lower PokéNav shell. That lower shell is a separate surface
and will be replaced through its own authentic-HGSS asset → wire-live → remove
legacy-equivalent sequence.

The app-switch surface sequence is therefore complete:

**authentic HGSS asset → wire live → remove legacy equivalent**

## Fixed screen shell — phase 1

With the app-switch strip complete, the next legacy surface is the remaining
PokéNav device shell below it.

Retail HGSS provides an exact default-skin source set for the fixed PokéGear
screen layer in `PokegearApp_LoadSkinGraphics`:

- member 36 character graphics,
- member 24 BG palette,
- member 42 NSCR screen/tilemap.

For skin 0, those are loaded together on `GF_BG_LYR_SUB_0` in the retail
PokéGear. Member 36 is a 256x32 indexed source sheet and member 42 defines a
256x192 screen.

The three files in `verified/` are copied byte-for-byte from
`pret/pokeheartgold` revision `9d8b7591f09b65804da2fb2dfd56f320633e0d36`.

This is **phase 1 only** for the fixed shell. The current lower PokéNav device
artwork remains live until the verified retail shell is adapted and wired in a
separate phase.
