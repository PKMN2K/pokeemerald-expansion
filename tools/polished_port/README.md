# Polished Crystal Port Tools

This directory contains conversion tooling and manifests for moving authentic Polished Crystal assets into pokeemerald-expansion without replacing the engine.

## Source pin

Polished Crystal v3.2.3:
`3fa43192379df5c3e7b09a08e4d5d79af4f02f42`

## Rule

Converted assets are staged under new Polished Crystal-specific names first. They are only wired into the live game after their format, palette, frame order, and dimensions are verified.

## First conversion: male player overworld

Authentic Polished Crystal sources:

- `gfx/sprites/chris.png` — normal walking/standing source
- `gfx/sprites/chris_bike.png` — bicycle source
- `gfx/sprites/chris_surf.png` — surfing source

The corresponding compressed GBC build assets are referenced by `gfx/sprites.asm` as:

- `gfx/sprites/chris.2bpp.lz`
- `gfx/sprites/chris_bike.2bpp.lz`
- `gfx/sprites/chris_surf.2bpp.lz`

pokeemerald-expansion currently separates the male player into these live sheets:

- `graphics/object_events/pics/people/brendan/walking.png`
- `graphics/object_events/pics/people/brendan/running.png`
- `graphics/object_events/pics/people/brendan/field_move.png`
- `graphics/object_events/pics/people/brendan/fishing.png`
- `graphics/object_events/pics/people/brendan/surfing.png`
- `graphics/object_events/pics/people/brendan/mach_bike.png`
- `graphics/object_events/pics/people/brendan/acro_bike.png`

Do not overwrite Brendan assets during conversion. The first generated output will use a new `polished_chris` asset namespace, after which it can be wired into the player graphics tables.

## Planned workspace

- `manifests/` — source/target mappings and frame-layout metadata
- `scripts/` — deterministic conversion scripts
- `generated/` — optional intermediate/reference output, not live game assets

The first script will convert `chris.png` into an Expansion-compatible walking sheet while preserving the original Polished Crystal pixels and palette intent.
