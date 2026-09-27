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

The party panel and Choose Box paths still accept verified assets only. Their
generated include files intentionally remain zero descriptors until the
corresponding authentic HGSS members are visually verified and bound.

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
