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

The existing `src/swsh_storage_system.c` contains several synthetic `DrawHgss...` helpers from an earlier approximation pass. These are temporary migration targets, not canonical HGSS art.

Before replacing them, extract and identify the members of `/a/0/1/9`, then map each member to the corresponding PC/Storage screen component.


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
  --role box_grid \
  --ncgr <verified NCGR member> \
  --nclr <verified NCLR member> \
  --nscr <verified NSCR member> \
  --crop <x> <y> <width> <height> \
  --output graphics/gen4_ui/hgss_storage/verified/box_grid
```

The binder rejects mismatched member types and mismatched archive manifests. Its
JSON sidecar records the archive SHA-256 and decoded member hashes so the GBA
asset remains traceable to the user's own HGSS source without committing the
ROM or NARC.

The first migration target is `box_grid`, replacing the synthetic line/cross
tile construction in `DrawHgssStorageBoxGrid`. Do not wire that renderer to
a member triple until the gallery has visually verified the role.
