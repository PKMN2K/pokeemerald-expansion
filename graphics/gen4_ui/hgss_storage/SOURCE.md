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

Choose Box verification does not expose a standalone
NCGR+NCLR+NSCR full-screen "Choose Box grid", but sprite/runtime tracing proves
a native six-at-a-time box-overview layer. That dynamic layer is now wired live.
The title-level carousel remains only as a temporary functional fallback until
the authenticated overview interaction has been build/runtime-verified and can
safely own the complete destination-selection flow.

Members 70-73 are the box-overview thumbnail bundle, not wallpaper-selector
controls. HGSS rebuilds an individual 32x32 thumbnail from NCGR 70 whenever a
box changes, generates all 18 thumbnails during Storage initialization, and
uploads the six thumbnails belonging to the currently visible group. The live
GBA path now reproduces that mutation: member-70's source index 8 is replaced
with the box's authenticated member-75 wallpaper color, and each occupied slot
gets the exact ov14_021F8080/member-75 body-color marker. The native marker
geometry is six columns by five rows, with 2x1 markers at x=10..20 and y=11..19
in two-pixel steps. Because GBA 4bpp cannot address HGSS's 0x20+ marker indices
inside the same OBJ palette, the live thumbnail is losslessly split into a
32x32 base/wallpaper OBJ and a centered 16x16 marker OBJ containing the native
occupied middle region. This reduces OBJ VRAM without changing any visible
pixel position or BGR555 color. Members 74-77 remain the separate four-swatch
wallpaper-selector bundle. Members 85 and 86 are auxiliary NSCR window/frame
surfaces, not an all-box chooser background.

The static role binder still does not accept a fabricated `choose_box` or
`box_grid` background, because the authentic overview is sprite-driven.
Phase 3 is now complete after the full CI verification gate passed. The
temporary title-carousel/live-preview fallback has been removed: browsing with
the authenticated HGSS controls changes only the overview candidate, thumbnail
group, title, and count. It no longer scrolls the underlying full Storage box
for every candidate. A confirmed different box performs the normal Storage
transition once after the chooser closes; B cancels immediately without a
preview rollback.

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


Phase 3 is complete for the main Storage frame. The legacy SWSH
`graphics/pokemon_storage/swsh/tiles.png` and `bg1.bin` assets, their
declarations, the decompression path, and the palette-0 load sourced from the
legacy tiles have been removed. BG1 now starts from an explicitly cleared
transparent tilemap; the authentic HGSS main-frame pack begins at tile 0 and
the verified HGSS party panel follows immediately after its tile block.

No legacy SWSH frame pixels remain underneath the HGSS composition. This
finishes the required sequence for this role: authentic HGSS asset -> wire
live -> remove legacy equivalent.


## Authentic HGSS wallpaper-selector controls — phase 1

The next verified Storage role is the native wallpaper-selection control layer.
The preserved HeartGold PC Box extraction contains the exact source character
sheets for the two sprite bundles used by this interaction:

- `/a/0/1/9` members 70 NCGR + 71 NCLR + 72 NCER + 73 NANR, loaded by
  `ov14_021F41E4`;
- `/a/0/1/9` members 74 NCGR + 75 NCLR + 76 NCER + 77 NANR, loaded by
  `ov14_021F42EC`. `ov14_021F462C` also reads member 74 while wallpaper
  choices change.

`verified/wallpaper_selector_primary.png` and
`verified/wallpaper_selector_secondary.png` are byte-for-byte copies of the
user-preserved `member_070_tiles.png` and `member_074_tiles.png` extraction
outputs. They are imported as authentic source assets only in this phase; no
runtime wiring or legacy selector removal is performed yet.

The next phase must reconstruct the native NCER/NANR frames and wire those
verified frames into the live wallpaper-selection interaction before any
existing selector equivalent is removed. The machine-readable record is
`verified/wallpaper_selector.json`.


## Authentic HGSS wallpaper selector — phase 2

Runtime tracing corrects one phase-1 classification before live wiring: member 70
is not a wallpaper-selector control. `ov14_021F4958` uses it as the base for
six box-content thumbnail images. That authentic extraction is retained under
`verified/box_thumbnail_base.png` for its proper future role.

`verified/wallpaper_selector_swatch.png` has also been regenerated as a valid indexed 4bpp PNG after CI detected a corrupt PLTE CRC in the old container. The repair is derived directly from NCGR 74 plus NCER 76 cell 0: its four OAM pieces compose source tiles 0-8 in the native 24x24 geometry, preserving the verified 224-pixel mask and visible bounding box [4,5,20,19]. Only source indices 0 and 30 exist; they are losslessly compacted to 0 and 1 for the GBA mask path. No visible pixel is moved or altered.

The selector itself is the member 74/75/76/77 bundle. `ov14_021F4380`
creates four copies of its cell, spaced 46 pixels apart, and
`ov14_021F462C` rewrites the member-74 placeholder pixels whenever the
wallpaper group changes. The verified NCER 76 cell reconstructs to a 24x24
binary mask with 224 opaque pixels and visible bounds x=4..19, y=5..18.

`verified/wallpaper_selector_swatch.png` is the exact 1:1 NCER-composed
shape. Its source is 8bpp but uses only transparent index 0 and placeholder
index 30, so the GBA pack losslessly compacts the mask to 4bpp. Runtime copies
that cell into four transparent-padded 32x32 OBJ cells and assigns authentic
member-75 BGR555 colors. The live wallpaper picker is now grouped as six pages
of four choices, matching HGSS's 24-wallpaper four-at-a-time selector behavior.

This is phase 2 only. The existing standard ListMenu cursor and scroll-arrow
pair remain live underneath/alongside the authentic HGSS swatches and are
reserved for the next cleanup phase.


## Authentic HGSS wallpaper selector — phase 3

The legacy wallpaper-selector presentation has now been removed after the
authentic member-74 selector was proven live. The generic ListMenu black-arrow
cursor is disabled with `CURSOR_INVISIBLE`, while ListMenu remains only as the
input/text-selection engine. The generic
`AddScrollIndicatorArrowPairParameterized` left/right arrow pair, its
wallpaper-only tags, task field, creation path, and cleanup calls are removed.

Wallpaper behavior is unchanged: Up/Down still chooses one of the four
wallpapers on the current HGSS selector page, Left/Right still changes among
the six four-choice pages, and A/B retain their existing select/cancel
semantics. The only visible selector controls are now the authenticated HGSS
member 74/75/76/77 swatches.

This completes the required sequence for the wallpaper selector: authentic
HGSS asset -> wire live -> remove legacy equivalent. The corrected member
70/71/72/73 bundle remains preserved as the authentic box-content thumbnail
base for a later role.


## Authentic HGSS box-overview thumbnails — phase 1

The next Storage role is the native dynamic box-overview thumbnail layer.
Structural tracing of HGSS overlay 14 binds this role to `/a/0/1/9` members
70 NCGR + 71 NCLR + 72 NCER + 73 NANR. Member 70 is an 8bpp 32x32 character
surface; NCER 72 contains one 32x32, one-OAM cell; NANR 73 animation 0 displays
that cell as a single four-tick frame.

`ov14_021F4958` copies the member-70 base for one box and rewrites its pixels
from the contents of all 30 box slots. `ov14_021F49C8` performs that rebuild
for all 18 boxes at Storage initialization. `ov14_021F49E0` then uploads six
consecutive thumbnails for the active group, and `ov14_021F4A20` refreshes
an affected thumbnail after box-content changes. The corresponding overview
input state handles six box choices plus previous/next group controls.

`verified/box_thumbnail_base.png` is the already-preserved byte-for-byte
32x32 extraction of member 70, now formally bound to this role. No redraw,
resampling, recoloring, runtime wiring, or legacy removal is performed in this
phase. The machine-readable record is
`verified/box_thumbnail_base.json`.

Phase 2 must wire the authentic six-thumbnail overview live while retaining the
current title-level chooser as a fallback underneath it. Only after that native
overview is proven functional may the non-native chooser presentation be
removed in phase 3.


## Authentic HGSS box-overview thumbnails — phase 2a

The verified member-70 thumbnail base is now live in the Choose Box interaction.
Six 32x32 OBJ sprites use the untouched
`verified/box_thumbnail_base.png` pixels and the verified PNG palette. Their
centers use the exact HGSS runtime overview coordinates recovered from overlay
14: x = 43 + 34*n for n=0..5, y = 84. The live objects are also assigned the
native six-box group corresponding to the current candidate box.

This is intentionally **phase 2a**, not the completed wiring phase. HGSS
`ov14_021F4958` mutates the 8bpp member-70 working copy with wallpaper and
Pokémon-content palette indices before upload. Those extended DS OBJ palette
indices are not represented by the static NCLR 71 export alone, so this commit
does not invent substitute colors or fake content markers. The current
title-level chooser, navigation controls, title preview, and count remain live
as the fallback underneath the authentic overview bases.

The next step is phase 2b: reproduce the authenticated runtime thumbnail pixel
mutation/palette mapping on GBA, then prove all six content-sensitive thumbnails
live. No legacy chooser presentation may be removed before that is complete.

## Authentic HGSS box-overview thumbnails — phase 2b

The authenticated content-sensitive thumbnail renderer is live. Each visible
member-70 base is rebuilt with the exact member-75 wallpaper color mutation and
the native ov14_021F4A64 occupancy-marker geometry/body-color mapping. The GBA
renderer keeps the 32x32 authentic base/wallpaper pixels and uses a centered
16x16 marker OBJ only as a lossless hardware adaptation for HGSS marker palette
indices above the GBA 4bpp range.

The six thumbnails remain grouped at the native x=43+34*n, y=84 coordinates.
The authentic members 66-69 navigation controls, box title, and box count stay
live as functional HGSS/runtime information. During this phase the existing
full-box preview scroll was retained only as a verification fallback.


## Authentic HGSS box-overview thumbnails — phase 3

The full CI verification gate passed across Emerald, FireRed, LeafGreen,
release, Gen-4 UI validation/compile, documentation validation, and the general
test job. The temporary title-carousel/live-candidate preview path is therefore
removed.

Left/Right now changes the candidate inside the authenticated six-at-a-time
overview without scrolling the underlying wallpaper and Pokemon icon field.
The obsolete `previewActive`/`cancelPending` state and preview-specific
`SetUpScrollToBoxFrom` path are gone. A confirms the candidate; if it differs
from the current box, the chooser closes and the normal Storage box transition
runs once before committing the new current box. B cancels immediately, so no
rollback scroll is necessary.

This completes the required sequence for the box-overview role: authentic HGSS
asset -> wire live -> remove legacy equivalent. No synthetic thumbnail pixels,
legacy chooser preview, or repurposed Storage cursor remains in the Choose Box
presentation.

## Authentic HGSS gender indicator — phase 1

The next unresolved Storage info-panel accessory is the Pokémon gender
indicator. HGSS does **not** source this role from a standalone bitmap or from
the PC Box graphics archive `/a/0/1/9`. Overlay 14 renders it through the
normal text/font path, so the authentic replacement must preserve the original
font glyph rather than invent a sprite equivalent.

The provenance chain is now verified end-to-end. `ov14_021E5D78` opens Storage
message bank 24; `files/msgdata/msg/msg_0024.gmm` entries 82 and 83 contain
the special male/female characters `㊚` and `㊛`. `charmap.txt` maps those
characters to codes 0x00EE and 0x00EF. Storage allocates font ID 4 in
`ov14_021F4ED0`; HGSS `src/font.c` maps font ID 4 to
`NARC_graphic_font` member 4. `ov14_021F5114` selects message 0x52 for male
and 0x53 for female and sends it through `ov14_021F4F84`.

The font-member header identifies 16x16 glyph cells with 64 compressed bytes
per glyph. After the engine's exact `DecompressGlyphTile` mapping, both
gender glyphs are 9 pixels wide. Male is glyph index 237 at byte offset 15184;
female is glyph index 238 at byte offset 15248.

`verified/gender_glyphs.png` is a native-resolution 32x16 indexed export:
the left 16x16 cell is male and the right 16x16 cell is female. Its pixels are
derived directly from `graphic/font` member 4 and use the original member-7
font palette. No redraw, resampling, recoloring, or synthetic geometry is
present. The HGSS text-color constants are preserved exactly: male
`0x00070800` uses palette foreground 7 / shadow 8 / background 0, while
female `0x00030400` uses foreground 3 / shadow 4 / background 0.

This is phase 1 only. The current expansion `sGenderIcons_Gfx` path remains
live until the exact HGSS font-derived glyph presentation is wired and verified.
Only then may the legacy `graphics/pokemon_storage/swsh/gender_icons.png`
equivalent and its sprite-specific path be removed.


## Authentic HGSS gender indicator — phase 2

The verified HGSS gender glyphs are now wired into the live Storage info panel.
The live `sGenderIcons_Gfx` source is
`verified/gender_glyphs_obj.png`, a GBA-OBJ packing derivative of
`verified/gender_glyphs.png`. It contains the same exact two 16x16 HGSS font
cells and palette indices; the only change is layout from horizontal
male/female cells to vertical male/female frames so each 16x16 frame is
contiguous in GBA 1D OBJ tile order.

The live sprite now uses a 16x16 OAM cell and a dedicated
`PALTAG_HGSS_GENDER_GLYPHS` palette containing the exact
`graphic/font` member-7 BGR555 values. Female selects tile offset 4 and male
selects tile offset 0. The sprite center is shifted four pixels right relative
to the former 8x16 expansion icon so the visible left edge remains anchored
after the nickname.

This is phase 2 only. The old
`graphics/pokemon_storage/swsh/gender_icons.png` file is deliberately retained
as the legacy equivalent until the new HGSS glyph path passes build/runtime
verification. Phase 3 is its removal.

### Gender glyph OBJ container repair

Build verification exposed a malformed PNG container in the phase-2 OBJ packing derivative (libpng rejected its PLTE chunk). The live `gender_glyphs_obj.png` has been regenerated directly from the verified `gender_glyphs.png` export. The two authentic 16x16 HGSS font cells and their original palette indices are unchanged; only their layout is repacked vertically for GBA 1D OBJ tile order. The repaired indexed PNG SHA-256 is `7ef910bc3bcc0ac04cf00805cffb9596ed231a7e5e9ad33fbdb4e8dc02d2e8c7`.

## Authentic HGSS gender indicator — phase 3

Targeted Gen 4 UI asset generation and renderer compilation passed with the repaired authentic HGSS glyph OBJ export. The expansion-era `graphics/pokemon_storage/swsh/gender_icons.png` asset is now removed. The live graphics symbol has also been renamed from the inherited `sGenderIcons_Gfx` name to `sHgssGenderGlyphs_Gfx`, so the Storage gender indicator now has no remaining dependency on the legacy SWSH gender-icon asset path.

The active presentation remains the verified HGSS font-derived 16x16 male/female glyph cells and original member-7 palette values imported in phases 1-2. This completes the strict sequence for this element: authentic HGSS asset -> wire live -> remove legacy equivalent.

## Authentic HGSS shiny indicator — phase 1 (revised)

HGSS Storage itself does not draw a shiny-status marker, but the project keeps
that useful status information by repurposing **authentic HGSS artwork** from
the native Pokemon Summary screen rather than retaining the expansion/SWSH
graphic or drawing an HGSS-style substitute.

The exact source is verified through the HGSS resource graph. Summary
`sub_0208981C` stores `MonIsShiny` in bit 29 of its packed status word.
`sub_0208BD38` tests that bit and toggles the sprite stored at work offset
`0x4D4`; the adjacent `0x4D8` sprite is the Pokerus indicator. The
`_02103A70` sprite table identifies the shiny sprite as template index 52,
resource set 23, animation 0. `resdat_00000085` maps resource set 23 to
graphics 23 / palette 0 / cell 8 / animation 8. Those resolve through the
HGSS resdat tables to:

- character: `NARC_a_0_3_9` member 58
- palette: `NARC_a_1_6_2` member 61
- cell: `NARC_a_0_3_9` member 51
- animation: `NARC_a_0_3_9` member 50

The character member contains exactly two 8x8 OBJ tiles: tile 0 is the shiny
star and tile 1 is the Pokerus symbol. `verified/shiny_star.png` is a direct
1:1 indexed export of tile 0 using the original first 16-color palette bank
from member 61. DS OBJ palette index 0 remains transparent. No redraw,
resampling, recoloring, screenshot crop, or synthetic pixels are present.

Source archive checksums from the HGSS decomp's canonical filesystem manifest:
`/a/0/3/9` SHA-1 `1360486ee8eb2c19e8c5b6e36e4843d56a42c066`;
`/a/1/6/2` SHA-1 `30fd7818457b50689a8a78fe522eb9df366e58a4`.
The exported PNG SHA-256 is
`b7ee9b6d5e9017d9f8770cd898a993cd9a0b6fc034374a5fc18ae120799450fc`.

This revision supersedes the earlier absence-only phase-1 plan. The existing
`graphics/pokemon_storage/swsh/shiny_icon.png` remains live for now. Phase 2
will wire this authentic HGSS Summary star into Storage; phase 3 will remove
the legacy equivalent only after verification.

The machine-readable provenance record is
`verified/shiny_indicator.json`.


## Authentic HGSS shiny indicator — phase 2

The verified HGSS Summary shiny star is now wired into the live Storage
shiny-status path. `sSpriteSheet_ShinyIcon` now reads its 8x8 graphics from
`verified/shiny_star.png` through `sHgssShinyStar_Gfx`, and the sprite uses
a dedicated `PALTAG_HGSS_SHINY_STAR` palette loaded from the same indexed
PNG. This preserves the original HGSS OBJ palette indices instead of
displaying the authentic pixels through the former expansion palette.

`UpdateShinyIconSprite` keeps the existing Storage-specific shiny-status
condition and 8x8 panel placement. Only the presentation asset is adapted:
the rendered star and palette are now the exact HGSS Summary resource verified
in phase 1.

The legacy
`graphics/pokemon_storage/swsh/shiny_icon.png` file is deliberately retained
but is no longer the live graphics source. It remains solely as the phase-2
verification fallback. Phase 3 will delete it only after the newly wired HGSS
asset passes build/runtime verification.


## Authentic HGSS shiny indicator — phase 3

The phase-2 live HGSS shiny-star path passed the dedicated `gen4-ui` CI job
in workflow run `36505275777`: tool validation, Gen 4 UI asset generation,
and renderer compilation all succeeded.

The former expansion asset
`graphics/pokemon_storage/swsh/shiny_icon.png` is therefore removed. The
active Storage shiny indicator remains the authentic HGSS Summary 8x8 star
from `verified/shiny_star.png` with its original HGSS palette, wired through
the dedicated `PALTAG_HGSS_SHINY_STAR` path.

This completes the strict sequence for the shiny indicator:
authentic HGSS asset -> wire live -> remove legacy equivalent.


## Authentic HGSS Pokérus indicator — phase 1

The next unresolved Storage info-panel accessory is the expansion Pokérus
indicator. As with the shiny marker, HGSS Storage itself does not expose this
status icon, so the useful status is preserved by repurposing **authentic HGSS
Summary-screen artwork** rather than retaining a SWSH-era graphic or drawing
an HGSS-style substitute.

The exact native source is the companion sprite immediately beside the Summary
shiny marker. `sub_0208BD38` toggles the sprite stored at work offset
`0x4D8` when the packed Summary status field's top two bits equal 2; the
neighboring `0x4D4` sprite is the shiny marker. The `_02103A70` unmanaged
sprite-template table identifies the Pokérus sprite as template index 53:
resource set 23, animation 1, native position 252x132. Resource set 23 resolves
through `resdat_00000085` to the same HGSS Summary resource quartet used by
the shiny marker:

- character: `NARC_a_0_3_9` member 58
- palette: `NARC_a_1_6_2` member 61
- cell: `NARC_a_0_3_9` member 51
- animation: `NARC_a_0_3_9` member 50

That character member contains exactly two 8x8 OBJ tiles. Tile 0 is the shiny
star used by animation 0; tile 1 is the Pokérus symbol selected by template 53
with animation 1. `verified/pokerus_symbol.png` is a direct 1:1 indexed
export of tile 1 with the original first 16-color palette bank from member 61.
Palette index 0 remains transparent. No redraw, recoloring, resampling,
screenshot crop, or synthetic pixel work is present.

Source archive SHA-1 values from the canonical HGSS filesystem manifest remain
`1360486ee8eb2c19e8c5b6e36e4843d56a42c066` for `/a/0/3/9` and
`30fd7818457b50689a8a78fe522eb9df366e58a4` for `/a/1/6/2`.
The exported PNG SHA-256 is
`7a6ebcaad6e27c64d3321fe5af5de0afb11be8e8632721821601cec213cc8cd3`.

This is phase 1 only. The current 32x8 expansion
`graphics/pokemon_storage/swsh/pokerus_icon.png` remains live. Phase 2 will
wire the authentic 8x8 HGSS Summary symbol and adapt the live sprite geometry;
phase 3 will remove the legacy equivalent after verification.

The machine-readable provenance record is
`verified/pokerus_indicator.json`.


## Authentic HGSS Pokérus indicator — phase 2

The verified HGSS Summary Pokérus symbol is now the live Storage presentation.
`sSpriteSheet_PokerusIcon` reads the exact 8x8 tile from
`verified/pokerus_symbol.png` through `sHgssPokerusSymbol_Gfx`, and a
dedicated `PALTAG_HGSS_POKERUS_SYMBOL` loads the original palette from that
same indexed export.

The live OAM geometry is changed from the expansion's 32x8 banner to the
authentic HGSS symbol's 8x8 dimensions. `UpdatePokerusIconSprite` preserves
the existing Storage indicator center at x=72 (+ the virtual-panel offset),
y=150, so the adaptation changes the artwork/geometry without introducing a
new arbitrary placement.

The legacy
`graphics/pokemon_storage/swsh/pokerus_icon.png` file is deliberately
retained but is no longer the live source. It remains only as the phase-2
verification fallback. Phase 3 will remove it after the authentic HGSS path
passes build/runtime verification.


## Authentic HGSS Pokérus indicator — phase 3

The phase-2 live HGSS Pokérus-symbol path passed the dedicated `gen4-ui` CI
job in workflow run `36506180309`: Gen 4 UI tool validation, asset
generation, and renderer compilation all succeeded.

The former expansion asset
`graphics/pokemon_storage/swsh/pokerus_icon.png` is therefore removed. The
active Storage Pokérus indicator remains the authentic HGSS Summary 8x8 symbol
from `verified/pokerus_symbol.png`, using its original HGSS palette through
the dedicated `PALTAG_HGSS_POKERUS_SYMBOL` path.

This completes the strict sequence for the Pokérus indicator:
authentic HGSS asset -> wire live -> remove legacy equivalent.


## Authentic HGSS Storage type badges — phase 1

The next unresolved native Storage presentation is the Pokémon type badge set.
Unlike shiny/Pokérus, this role is directly present in HGSS Storage itself.
`ov14_021F3D70` renders the two type slots and calls `ov14_021F3D0C` to
replace each sprite's character data and select its palette. That helper uses
`sub_020776B4` (NARC ID 8), `sub_02077678` (type -> character member), and
`sub_0207769C` (type -> palette bank).

NARC ID 8 is `/a/0/0/8`. For HGSS Pokémon types 0..17, every mapped NCGR
decompresses to exactly 0x100 bytes of native 4bpp character data: eight
8x8 tiles, matching a 32x16 badge. Storage loads palette member 0x4A from the
same archive and uses three 16-color banks.

Phase 1 imports those resources without redrawing or palette flattening:

- `verified/type_icons_hgss_gen4.4bpp` — the 18 exact 0x100-byte character
  blocks concatenated in HGSS type-ID order (Normal through Dark, including
  the Gen-IV Mystery slot).
- `verified/type_icons_hgss_gen4.gbapal` — the exact 96-byte three-bank
  BGR555 palette payload from member 0x4A.
- `verified/type_icons.json` — member IDs, palette-bank mapping, source
  checksums, and expansion-ID mapping.

The canonical HGSS filesystem SHA-1 for `/a/0/0/8` is
`670ec02b1742a7b711caafea295113d5ab88b51b`.

Our expansion numbers types one slot later because `TYPE_NONE = 0`, so the
native HGSS badges map directly to expansion `TYPE_NORMAL = 1` through
`TYPE_DARK = 18`. HGSS predates expansion `TYPE_FAIRY = 19` and
`TYPE_STELLAR = 20`; those two are intentionally **not** fabricated in this
phase. Per the project's authenticity rule, their phase-2 presentation must be
built only from explicitly documented authentic HGSS source artwork before the
legacy type atlas can be removed.

The current `graphics/pokemon_storage/swsh/type_icons.png` path remains live.
This is phase 1 only.


## Authentic HGSS Storage Fairy badge — phase 1A adaptation

Retail HGSS has no Fairy type, so the Fairy slot cannot be satisfied by claiming a
nonexistent Nintendo badge. The supplied HGSS ROM-hack was useful as a structural
reference: its Fairy implementation occupies the normal 32x16 type-badge resource
path. Pixel comparison also showed why the hack badge cannot simply be adopted as
"authentic HGSS" artwork: its F and I match retail HGSS glyph geometry, but at
least its A differs from the retail HGSS A used by the native type badges.

The project therefore resolves Fairy by **recomposing only verified retail HGSS
pixels**. `verified/type_icon_fairy_hgss_composite.4bpp` starts from the exact
Psychic badge (HGSS type 14, /a/0/0/8 member 223), retaining its complete native
32x16 background/frame and palette-bank-1 presentation. Only the existing
PSYCHIC label's palette-index F/E pixels in rows y=4..11 are cleared back to that
badge's native base index 8. The word FAIRY is then assembled from untouched
retail badge glyph pixels:

- F: FLYING member 227, x=2..6 -> x=4..8
- A: WATER member 241, x=10..14 -> x=9..13
- I: FLYING member 227, x=17..20 -> x=14..17
- R: WATER member 241, x=24..28 -> x=18..22
- Y: FLYING member 227, x=12..16 -> x=23..27

All glyph crops use their original 8-pixel-tall y=4..11 HGSS badge pixels and
copy only the native F/E lettering indices. There is no redraw, scaling,
anti-aliasing, custom glyph geometry, or Fairy-specific recoloring. The badge
uses the unchanged retail HGSS type palette member 0x4A bank 1, whose existing
pink/purple colors make the composition visually appropriate without inventing
new colors.

This is still pre-wiring work. The legacy
`graphics/pokemon_storage/swsh/type_icons.png` remains live. Stellar is the
remaining expansion-only type gap before the type-badge set can move to phase 2
live wiring.


## HGSS Storage type badges — Stellar project policy

Stellar is intentionally **not a supported gameplay type** in this base/hack
family. Although pokeemerald-expansion exposes `TYPE_STELLAR` for compatibility
with later-generation mechanics, this project will not implement Terastal or
Stellar gameplay and therefore does not need a visible Storage badge for that
enum.

Accordingly, no HGSS-style Stellar badge will be drawn or synthesized. The
runtime type-badge path must explicitly exclude `TYPE_STELLAR` from normal
visible Storage presentation. If the enum is ever encountered unexpectedly,
it should be handled as an unsupported/fallback condition rather than inventing
artwork.

With Fairy now resolved through an authentic-HGSS-only composition and Stellar
explicitly excluded by project design, there are no remaining expanded-type
asset blockers before phase 2 live wiring. The legacy type atlas remains live
until that wiring is completed and verified.


## Authentic HGSS Storage Fairy badge — phase 1A revision

The earlier Fairy composite used the retail Psychic badge as its structural base.
That has been superseded by a cleaner provenance path using the **retail HGSS
Mystery (???) badge itself**, which is HGSS type 9 and /a/0/0/8 member 236. The
supplied Fairy-enabled HGSS hack also replaces this same native slot, making it
the most appropriate authentic structural source.

The revised `verified/type_icon_fairy_hgss_composite.4bpp` preserves the exact
Mystery badge's 32x16 geometry. Its three type-specific palette indices are
role-remapped without drawing new colors:

- Mystery index A -> HGSS bank-1 index 8: #F85888
- Mystery index B -> HGSS bank-1 index 9: #F8C0B0
- Mystery index C -> HGSS bank-1 index A: #906060

Those destination colors already exist in the unmodified retail HGSS type palette
member 0x4A. Neutral/lettering indices remain the native shared HGSS values. The
native ??? label is removed only from its central label field, and the previously
verified retail-HGSS F/A/I/R/Y glyph pixels are placed over that field.

Therefore the final Fairy badge is now:

**retail HGSS Mystery geometry -> retail HGSS pink palette roles -> retail HGSS
FAIRY glyph pixels**.

No ROM-hack Fairy pixels, custom colors, redraw, resampling, or synthetic glyph
geometry are present. The legacy Storage type atlas remains live; phase 2 wiring
has still not begun.
