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
