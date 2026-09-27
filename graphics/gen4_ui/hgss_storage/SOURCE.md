# HGSS Storage UI source

This directory is reserved for the authentic HeartGold/SoulSilver PC / Pokemon Storage UI conversion.

## Canonical source

- Game: Pokemon HeartGold / SoulSilver (Nintendo DS)
- Internal archive: `/a/0/1/9`
- Project Pokemon file-system description: "Box graphics (LZ compressed)"
- HeartGold raw archive listing: 87 members

References:
- https://projectpokemon.org/home/docs/gen-4/hgss-file-system-r21/
- https://projectpokemon.org/rawdb/heartgold/narc.php

## Conversion rule

The storage UI must use graphics extracted from the HGSS box archive wherever the original game provides artwork.

Do not replace HGSS visual chrome with newly drawn "HGSS-style" borders, tabs, grids, buttons, panels, arrows, or decorative pixel primitives.

Pokeemerald-side code may:
- unpack/convert NCGR/NCLR/NSCR data,
- crop or recompose authentic HGSS pixels to fit the 240x160 GBA viewport,
- remap palettes without redrawing artwork,
- position tiles/sprites,
- drive selection/highlight states,
- draw dynamic text, Pokemon icons, counts, and other runtime data,
- animate authentic UI components.

If HGSS has no equivalent for an expansion-only function, first document that absence. Any derived composition should reuse authentic HGSS UI pieces at 1:1 pixel scale where possible.

## Current status

Synthetic HGSS-style Storage drawing has been removed. Visual verification of
the user's authentic `/a/0/1/9` extraction established that the normal 6x5
box area does not use a separate visible "box grid" chrome layer: the authentic
wallpaper composition supplies the box frame/background and Pokemon icons are
positioned dynamically over it. The obsolete `box_grid` placeholder and
renderer hook have therefore been removed rather than populated with invented
art.

The party panel is now a verified authentic HGSS role: NSCR 8 + NCGR 14 +
NCLR 4 reconstruct an 88x144 panel that matches the extracted reference
pixel-for-pixel. The lossless GBA pack preserves the source indexed 4bpp tile
pixels, flip flags, and BGR555 palette values; the existing Storage renderer
loads that generated descriptor into the live BG1 party-panel region. The
legacy SWSH BG1 tilemap cells at columns 2-12, rows 0-17 are zeroed at source,
so no SWSH party-panel fallback remains underneath the authentic HGSS art.

Choose Box verification is now complete at the asset-identification layer.
The 87-member /a/0/1/9 inventory and the HGSS PC overlay call sites do not
expose a standalone NCGR+NCLR+NSCR "Choose Box grid" equivalent to the current
pokeemerald popup. HGSS composes box selection/navigation dynamically from the
normal box presentation, runtime text/count windows, and sprite-driven controls.

Members 70-73 and 74-77 are specifically excluded from the Choose Box role.
Overlay 14 loads those sprite bundles in the wallpaper-selection path; the same
path reads the selected box wallpaper and rearranges those resources when the
wallpaper choice changes. Members 85 and 86 are auxiliary NSCR window/frame
surfaces, not an all-box chooser background.

Accordingly, the static role binder no longer accepts `choose_box` (or the
already-invalid `box_grid` role). The obsolete static Choose Box descriptor
has now been removed from the live build. The chooser reuses the authenticated
normal box presentation and acts as a title-level left/right carousel: the
candidate box name and runtime count update while the HGSS wallpaper/box scene
remains visible. No synthetic all-box grid is painted or erased.

The native composition now includes live candidate previews. Left/right uses
the existing storage box-scroll path to transition the real candidate wallpaper
and Pokemon icons while `gPokemonStoragePtr->currentBox` remains unchanged.
A commits the already-previewed candidate; B first scrolls the visual preview
back to the saved current box and then cancels. This prevents a preview from
silently changing save-state selection and avoids replaying the transition after
confirmation.

The authentic Choose Box navigation/control chrome is now identified and
imported. HGSS overlay 14 loads /a/0/1/9 members 66-69 as one sprite bundle:
NCGR 66, NCER 67, NANR 68, and NCLR 69. The native templates place the left
control at x=12,y=28 with animation 1 and the right control at x=156,y=28 with
animation 3. NANR verification maps animation 1 to cell 0 (left normal),
animation 2 to cell 1 then cell 0 (left press/release), animation 3 to cell 2
(right normal), and animation 4 to cell 3 then cell 2 (right press/release).

`verified/choose_box_nav.png` is a lossless 1:1 four-frame indexed export of those
24x24 authentic HGSS cells in the order left-normal, left-pressed,
right-normal, right-pressed. NCER CEBK `shift=1` is applied to the OAM tile
indices before reading member 66, matching the native Nitro cell mapping. No
redraw, resampling, recoloring, or synthetic pixels are present. The verified controls are wired live as GBA OBJ sprites. The four 24x24 HGSS
frames are copied 1:1 into transparent 32x32 hardware cells; left/right input
uses the verified press/release frames. The former synthetic Choose Box cursor
area and the repurposed legacy storage cursor path have now been removed. The
normal storage cursor is only hidden/restored around this interaction and is no
longer used as Choose Box control chrome.
The machine-readable records are `verified/choose_box_native.json` and
`verified/choose_box_nav.json`.

The normal HGSS Storage hand pointer has now been identified at the asset
layer from the same members 66-69 sprite bundle. Overlay 14 drives sprite slot
9 with NANR animation 14 while the cursor is over the six box columns. That
animation alternates NCER cells 13 and 14 for 20 ticks each. The NCER CEBK
declares `shift=1`, so the OAM tile indices are shifted before reading NCGR
member 66. `verified/cursor.png` is an indexed 64x32 two-frame export of those
two native 32x32 cells. The authentic pointer is now wired live as the normal Storage OBJ cursor. The
verified 64x32 source strip is repacked tile-for-tile into two contiguous 32x32
GBA OBJ frames at runtime, preserving the HGSS pixels and 20-tick animation.
The NCER origin is preserved with an OBJ-only (+6,+10) visual offset, leaving
logical cursor, held-Pokemon, and held-item coordinates unchanged. All cursor
interaction modes use the authentic HGSS palette; mode identity remains on the
existing functional mode indicator rather than recoloring the hand. The legacy SWSH cursor equivalent has now been removed completely. The old
`graphics/pokemon_storage/swsh/cursor.png` file, compressed cursor sheet,
cursor palette bank, and the last multimove recolor override are gone. The
unrelated stat-label/title users of `PALTAG_MISC_1/2/3` now source their
palette from `stat_labels.png`, so no cursor data is retained under another
name. The machine-readable record is `verified/cursor.json`.

The next verified static role is the native HGSS Pokémon markings menu. The
archive composition is NSCR member 10 + NCGR member 14 + NCLR member 4. Visual
verification against the user's extracted `/a/0/1/9` gallery shows the native
88x144 panel with the six HGSS marking buttons and the two action bars.
`verified/markings_menu.png` is a lossless 1:1 composite re-indexed to a
4bpp-compatible PNG without redrawing, resampling, or color changes. The current
The authentic panel is wired live on BG0. Because the current BoxPokemon
layout persists four marking bits, the live composition uses authentic source
rows 0-6 (circle/triangle and square/heart) plus authentic rows 10-17 (the two
action bars), omitting only the unsupported star/diamond row without redrawing
or resampling any pixels. The legacy SWSH markings-menu sheet and all of its
window/mark/cursor sprites are now removed. Enabled markings use the authentic
member-4 NCLR bank-3 colors on the native button tiles, while navigation reuses
the verified authentic HGSS storage hand cursor. The verified PNG was also repacked pixel-identically as a compact CRC-valid
4-bit indexed PNG; only the PNG container/palette encoding changed.
The machine-readable record is `verified/markings_menu.json`.

The native HGSS Pokémon information panel is now verified and live.
Visual verification against the user's extracted `/a/0/1/9` gallery identifies
NSCR 9 + NCGR 14 + NCLR 4 as the 88x144 information-panel surface immediately
adjacent to the already-verified party (NSCR 8) and markings (NSCR 10) members.
`verified/mon_info_panel.png` is a lossless 1:1, 4bpp-compatible indexed export:
the seven authentic RGB colors and every source pixel are preserved with no crop,
redraw, resampling, or color changes. The source archive SHA-256 and decoded
member hashes are recorded in `verified/mon_info_panel.json`.

The verified 11x18-tile panel occupies both BG0 virtual info-screen positions.
Dynamic Pokémon text/icons are retained functionally and tightened to the native
88-pixel panel width. The legacy SWSH `mon_info.png` and `mon_info.bin`
equivalents, their C declarations, and the last shared base-map dependency have
now been removed. BG0 is initialized from an explicit transparent runtime tile
and blank tilemap, then the authentic HGSS panel is drawn over it; transient
message/menu windows continue to layer onto that functional blank surface. The
panel uses BG palette bank 11 so the multi-move text-palette restore on bank 13
cannot recolor the HGSS art. This completes the full authentic asset → wire live
→ remove legacy equivalent sequence for the Pokémon information panel.

The native HGSS Storage message window is verified and wired live.
Visual verification of the user's extracted `/a/0/1/9` gallery identifies
NSCR 11 + NCGR 14 + NCLR 4 as the full-width 256x48 rounded message bar.
`verified/message_window.png` remains a lossless 1:1 4bpp-compatible indexed
export with all five source colors and every source pixel preserved.

The GBA display exposes 30 tile columns rather than the DS screen's 32. The live
renderer therefore preserves source columns 0 and 31 (both native rounded edges)
and uses 28 of the identical interior source columns, omitting only two repeated
interior columns. No source pixel is redrawn, resampled, recolored, or
approximated. `WIN_MESSAGE` overlays text inside that authentic panel using the
panel's own cream/dark/gray palette indices.

The stale SWSH `graphics/pokemon_storage/swsh/message_window.png` file is now
deleted, and `WIN_MESSAGE` no longer loads or clears an Emerald standard frame.
The standard frame tiles at base tile 192 / palette 14 are still loaded directly
onto BG0 because the separate yes/no and generic context-menu roles have not yet
been converted; they are no longer associated with the HGSS message-window path.
This completes the full authentic asset → wire live → remove legacy equivalent
sequence for the Storage message window.

The native HGSS Storage confirmation / Yes-No panel is verified and wired
live. NSCR 12 + NCGR 14 + NCLR 4 provides the 256x56 rounded full-width choice
surface immediately following the verified NSCR 11 message bar.
`verified/yes_no.png` preserves the exact source pixels and five source colors.

As with the HGSS message bar, the GBA display uses source columns 0 and 31 for
the native rounded edges plus 28 unchanged repeated interior columns; only two
identical interior repeats are omitted to fit 240 pixels. The confirmation
question is re-rendered over the panel using NSCR 12's cream/dark/gray palette,
and the existing Yes/No text/cursor window is placed inside the panel at the
right. Input and cursor behavior are unchanged.

The Yes/No role is now fully detached from the Emerald standard-frame path.
`ShowYesNoWindow` creates only its functional text/cursor window with
`AddWindow`, prints the existing runtime Yes/No labels, and initializes the
same two-choice menu cursor directly. Input continues through
`Menu_ProcessInputNoWrap`; the local HGSS window is removed without calling
the standard-frame eraser. No `CreateYesNoMenu`, standard-frame draw, or
standard-frame clear remains in the Storage Yes/No path.

The base tile 192 / palette 14 standard frame resources remain loaded solely for
the still-unconverted generic Storage context menus, whose behavior is unchanged.
This completes the full authentic asset → wire live → remove legacy equivalent
sequence for the Storage Yes/No confirmation panel.

The next verified static role is the native HGSS Storage context-menu frame.
Visual and geometric verification of the extracted `/a/0/1/9` gallery
identifies NSCR 86 + NCGR 14 + NCLR 4 as a compact 96x80 (12x10-tile)
bordered vertical menu surface with the authentic cream interior and dotted
top/bottom trim. `verified/context_menu.png` is a lossless 1:1 indexed
repack: all seven source RGB colors and every source pixel are preserved with
no crop, redraw, resampling, or recoloring. Its archive/member hashes and role
binding are recorded in `verified/context_menu.json`.

The native HGSS Storage context-menu frame is now wired live. NSCR 86's
actual geometry is preserved rather than approximated: source column 0 is the
left edge, source continuation columns are repeated through the screen-right
edge, source rows 0/1 and 8/9 remain the two-tile top/bottom bands, and only the
verified middle row is repeated to accommodate variable menu heights. Dynamic
Storage context windows are therefore right-aligned to the GBA's tile column 29,
matching the source's open-right construction. The existing vertical
cursor-relative placement is retained and clamped only as needed to keep both
authentic top/bottom bands on-screen.

The existing menu text, cursor, list-menu, and input systems remain functional.
Their palette indices are mapped to RGB colors taken directly from NSCR 86's
authentic palette; no new colors are introduced.

The Storage context-menu role is now fully detached from Emerald standard-frame
plumbing. Both hidden `DrawStdFrameWithCustomTileAndPalette(..., 192, 14)`
calls are removed, `RemoveMenu` clears the authentic HGSS footprint directly
instead of calling the standard-frame eraser, and
`LoadUserWindowBorderGfxOnBg(0, 192, BG_PLTT_ID(14))` is no longer loaded by
Storage BG0. No Storage context-menu code depends on the legacy Emerald border
tiles. This completes the full authentic asset → wire live → remove legacy
equivalent sequence for the Storage context-menu frame.

The authentic HGSS Storage wallpaper set is now fully identified and imported at
the asset layer. HGSS uses one shared 21x20-tile NSCR (member 15), with 24
matching NCGR members 16-39 and 24 matching NCLR members 40-63. Wallpaper 01
was already checked in and live; `verified/wallpaper_02.png` through
`verified/wallpaper_24.png` now complete the native 24-wallpaper set.

Every imported wallpaper is a compact 4bpp-compatible indexed repack of the
user's extracted HGSS composition at the original 168x160 pixel dimensions.
No wallpaper is cropped, redrawn, rescaled, recolored, or synthesized.
`verified/wallpaper_set.json` records the archive hash, shared NSCR hash,
per-wallpaper NCGR/NCLR raw and decoded hashes, extracted PNG hashes, and the
exact checked-in Git blob IDs.

The complete 24-wallpaper set is now wired live through the same BG3 path that
was established for Wallpaper 01. Storage wallpaper IDs 0-23 map sequentially
to the authenticated native HGSS compositions 01-24. Each change loads exactly
one 21x20 / 168x160 4bpp composition into BG3 charblock 3 at tile 1, uses palette
bank 1, and rebuilds the 21x20 tilemap at x=9,y=0. The source pixels remain 1:1;
there is no crop, redraw, rescale, palette approximation, tile deduplication, or
legacy-art fallback.

The wallpaper picker now exposes all 24 IDs over five pages. The first sixteen
labels follow the established native normal-wallpaper order (Forest through
Simple). The final eight authenticated special wallpapers are deliberately named
Special 1 through Special 8 in the picker rather than assigning guessed theme or
Pokemon names that are not established by the asset extraction.

The wallpaper role has now completed the full authentic HGSS asset -> wire live
-> remove legacy equivalent sequence. The obsolete SWSH wallpaper directory
(20 PNG/BIN pairs) and its `bg2.bin` box-area tilemap are deleted. Storage no
longer declares or loads that SWSH BG2 map, and the old heap-backed
`wallpaperTiles` / StartLoadWallpaperGfx / UpdateWallpaperGfx plumbing is gone.

BG2 remains only as an explicitly blank functional overlay used by the existing
Storage transparency/blending behavior; it contains no SWSH wallpaper or box
art. The live visual wallpaper layer is exclusively the authenticated HGSS BG3
composition selected by wallpaper ID.

Do not restore compatibility fallbacks that draw approximated HGSS chrome.
If an authentic role is not available yet, leave that HGSS layer empty and
preserve only the underlying functional UI until the real asset is identified.


## Member mapping workflow

The public `pret/pokeheartgold` decomp currently identifies this archive as `NARC_a_0_1_9`, but does not check its 87 internal members into the repository as named source files. This means member roles must be derived from a user-provided extracted NARC rather than guessed from public filenames.

Use:

```sh
python tools/gen4_ui/map_hgss_storage_narc.py path/to/a_0_1_9.narc --extract build/hgss_storage_members
```

The mapper:
- validates the NARC structure,
- expects 87 members,
- decompresses LZ10 members,
- classifies Nitro formats such as NCGR/NCLR/NSCR/NCER/NANR,
- hashes both compressed and decoded bytes,
- writes a JSON manifest,
- optionally writes decoded members with stable numeric filenames.

Do not assign semantic names such as "box header", "cursor", or "wallpaper" until the decoded member has been visually or structurally verified.


## Verified role binding

After running `preview_hgss_storage.py` and visually confirming a real HGSS
NCGR/NCLR/NSCR combination, bind that composition to a semantic role with:

```sh
python tools/gen4_ui/bind_hgss_storage_role.py \
  build/hgss_storage \
  --role <verified-role> \
  --ncgr <verified NCGR member> \
  --nclr <verified NCLR member> \
  --nscr <verified NSCR member> \
  --crop <x> <y> <width> <height> \
  --output graphics/gen4_ui/hgss_storage/verified/<verified-role>
```

The binder rejects mismatched member types and mismatched archive manifests. Its
JSON sidecar records the archive SHA-256 and decoded member hashes so the GBA
asset remains traceable to the user's own HGSS source without committing the
ROM or NARC.

Do not create a standalone `box_grid` role for the normal 6x5 box screen.
The authentic wallpaper assets are the verified visual source for that area.
Only bind additional roles when the extraction gallery shows a real HGSS layer
with that function.


The next verified Storage role is the native HGSS main frame/background layer.
Structural verification against `pret/pokeheartgold` overlay 14 identifies
/a/0/1/9 NSCR member 2 + NCGR member 3 + NCLR member 4: `ov14_021E5C54`
loads that character/screen pair into the same Storage background layer and
loads palette member 4 for the composition.

`verified/main_frame.png` is a pixel-identical 4-bit indexed repack of the
user-preserved 256x192 extraction
`PC_Box/screens/member_002_screen__tiles_003__pal_004.png`. All nine source
RGB colors and every source pixel are preserved; there is no crop, redraw,
resampling, recoloring, or synthetic replacement artwork.

This commit is deliberately asset-only. The legacy
`graphics/pokemon_storage/swsh/tiles.png` + `bg1.bin` frame remains live until
the verified HGSS frame is wired in the next phase. Legacy removal must occur
only after that live replacement is complete. The machine-readable record is
`verified/main_frame.json`.


The authentic HGSS main Storage frame is now wired live (phase 2).
The verified NSCR 2 + NCGR 3 + NCLR 4 composition is packed into a sparse
240x160 BG1 overlay made exclusively from whole 1:1 authentic source tiles.
To fit the GBA viewport, source columns 13-14 and source rows 18-21 are omitted;
no retained pixel is redrawn, resampled, or recolored. Palette index 0 is used
only for transparent composition cells, while all nine HGSS source colors are
losslessly remapped to indices 1-9.

The live footprint deliberately mirrors and fully covers the remaining legacy
BG1 chrome: the left edge, title/header band, right edge, and bottom band.
Interior cells are transparent so the authenticated HGSS wallpaper and party
panel remain visible. The authentic party panel now loads immediately after the
main-frame tile block in BG1 VRAM, preventing tile overlap.

This is the wiring phase only. `graphics/pokemon_storage/swsh/tiles.png` and
`bg1.bin` are intentionally still loaded underneath the authentic frame and
must not be deleted until the next cleanup commit. Their visible frame role is
already covered by the authentic HGSS composition.
