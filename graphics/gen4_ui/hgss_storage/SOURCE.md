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
