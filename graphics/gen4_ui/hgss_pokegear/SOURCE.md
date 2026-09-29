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

## Fixed screen shell — phase 2

The verified retail default-skin screen shell is now live on the GBA PokéGear
without scaling or redrawing.

`tools/gen4_ui/make_hgss_pokegear_screen_shell.py` adapts member 42 at tile
granularity. It keeps NSCR columns 1..30, so the 256-pixel DS canvas becomes
the 240-pixel GBA viewport by omitting one 8-pixel edge column on each side.
Vertically it keeps source rows 0..11 and 16..23; rows 12..15 are a uniform
32-pixel spacer in the retail screen map, so removing only that spacer converts
192 pixels to 160 while every retained artwork pixel remains 1:1.

The member-36 NCGR-derived indexed PNG is compiled directly as 4bpp tiles at
BG2 tile base `0x40`. The adapted tilemap preserves the retail tile identity,
horizontal/vertical flip bits, and palette-bank semantics while offsetting tile
IDs by that GBA tile base. Member 24 palette bank 13 is emitted directly from
the verified NCLR and loaded into BG palette bank 13.

The authentic app-switch strip is loaded afterward and continues to own rows
0..3. The old PokéNav device shell still loads first underneath the HGSS shell
for this phase-2 verification cycle; its code and assets are not removed yet.


## Fixed screen shell — phase 3

CI #624 passed the phase-2 fixed-shell integration across the full workflow,
including Gen 4 UI validation, Emerald, FireRed, LeafGreen, release, docs, and
the general test suite.

The superseded PokéNav device shell has therefore been removed from
`src/pokenav_menu_handler_gfx.c`. Its palette/tile/tilemap declarations and
the state-1 BG2 load are gone. State 1 remains only as a sequencing gate so the
existing asynchronous load cadence is preserved before later layers are
installed.

A branch-wide check of the PokéNav source found `device_outline` referenced
only by that main-menu graphics module. With its final live use removed, the
now-unreferenced legacy source files are deleted as well:

- `graphics/pokenav/device_outline.png`
- `graphics/pokenav/device_outline_map.bin`

The authentic retail HGSS member-36/member-24/member-42 shell is now the only
live BG2 device-shell artwork. The already-authentic app-switch strip still
loads afterward on rows 0..3. No other PokéGear/PokéNav surfaces are changed by
this cleanup.

The fixed screen-shell sequence is therefore complete:

**authentic HGSS asset → wire live → remove legacy equivalent**


## Main UI sprite layer — phase 1

With the authentic app-switch background and fixed screen shell complete, the
remaining launcher still draws its interactive option layer from the legacy
PokéNav sprite sheet and palettes in `src/pokenav_menu_handler_gfx.c`
(`gPokenavOptions_Gfx` / `gPokenavOptions_Pal`).

Retail HeartGold/SoulSilver has a distinct common PokéGear UI sprite resource
set. At pret/pokeheartgold revision
`9d8b7591f09b65804da2fb2dfd56f320633e0d36`,
`PokegearUIManager_LoadInitialSkinGfx` in
`src/application/pokegear/main/overlay_100_021E6914.c` loads, for default
skin 0:

- member 6 character graphics (the committed source PNG compiled to NCGR),
- member 0 OBJ palette,
- member 12 NCER cell layout,
- member 13 NANR animation data.

`PokegearApp_LoadGraphics` in
`src/application/pokegear/main/overlay_100_021E5900.c` then creates the
shared PokéGear UI sprites from that resource set, including the app-switch
cursor sprites and the clock/day/status widgets.

The four verified files in `verified/` are copied byte-for-byte from that
retail HGSS source revision. They are source evidence only in this phase:
nothing is wired into the GBA launcher yet, and no legacy PokéNav option
sprite/palette is removed.

The next phase will adapt only the retail cells/frames that are useful on the
240x160 GBA launcher and wire those authentic pixels live while retaining the
legacy sprite layer underneath until CI passes.
