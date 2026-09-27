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

This is the first native-composition wiring stage. The next stage is to make a
hovered candidate transition the live box preview (wallpaper/icons) through the
existing box-scroll path, then bind the authentic navigation controls. The
machine-readable verification record is `verified/choose_box_native.json`.

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
