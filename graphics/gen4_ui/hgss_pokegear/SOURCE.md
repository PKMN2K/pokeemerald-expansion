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
