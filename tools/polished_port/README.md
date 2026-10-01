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
- `gfx/sprites/chris_run.png` — dedicated running source; supplemental pin `9dfcd39459a505f2cd0901de2d878cb93f9e4773`
- `gfx/sprites/chris_bike.png` — bicycle source
- `gfx/sprites/chris_surf.png` — surfing source
- `gfx/overworld/chris_fish.png` — fishing-on-land frames
- `gfx/overworld/chris_surf_fish.png` — fishing-while-surfing frames

The corresponding compressed GBC build assets are referenced by `gfx/sprites.asm` as:

- `gfx/sprites/chris.2bpp.lz`
- `gfx/sprites/chris_run.2bpp.lzp` (supplemental development source)
- `gfx/sprites/chris_bike.2bpp.lz`
- `gfx/sprites/chris_surf.2bpp.lz`
- `gfx/overworld/chris_fish.2bpp`
- `gfx/overworld/chris_surf_fish.2bpp`

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

The walking and running conversions preserve the original Polished Crystal pixels and palette intent.

The bike conversion maps `gfx/sprites/chris_bike.png` into a 9-frame 32x32 Expansion sheet. Each authentic 16x16 Crystal frame is horizontally centered and bottom-aligned. Both Mach Bike and Acro Bike player states use this same authentic Crystal cycling set. Because Crystal has no Acro trick poses, Acro-only animation slots reuse authentic cycling frames with compatible timing rather than inventing new artwork.

## Surf conversion

`gfx/sprites/chris_surf.png` is a six-frame 16x96 Polished Crystal sheet. Expansion's surfing renderer uses six physical 32x32 frames plus a picture-table remap. The converter therefore preserves all six source frames and writes them in physical order:

1. south idle
2. south step
3. north idle
4. north step
5. west idle
6. west step

Each 16x16 Crystal frame is horizontally centered and bottom-aligned in its 32x32 GBA frame. Expansion's existing separate surf-blob/bobbing field effect remains unchanged.

## Fishing conversion

Polished Crystal's `gfx/overworld/chris_fish.png` and `chris_surf_fish.png` are 16x24 overlay sheets, not standalone full sprites. Each contains three 16x8 strips. The original engine writes those strips over the bottom half of Chris's south, north, and west standing frames.

The converter reconstructs the authentic full 16x16 poses by combining:
- land fishing: top half of `chris.png` + matching `chris_fish.png` strip
- surf fishing: top half of `chris_surf.png` + matching `chris_surf_fish.png` strip

Expansion requires four rod-animation frames per direction. Because Polished Crystal supplies one authentic fishing pose per direction, all four slots reuse that pose instead of inventing transitional artwork. East remains the engine-flipped west pose. When the male player is already surfing, `SetPlayerAvatarFishing` selects the dedicated surf-fishing sheet and the fishing task later restores the saved surfing graphics.
