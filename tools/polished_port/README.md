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

Expansion requires four rod-animation frames per direction. Because Polished Crystal supplies one authentic fishing pose per direction, all four slots reuse that pose instead of inventing transitional artwork. Polished Crystal also draws `gfx/overworld/fishing_rod.png` as a fifth OAM tile: tile `$7a` sits below the south pose and above the north pose, while tile `$7b` sits outside the west/east pose. The converter now centers Chris at `(8, 8)` inside each 32x32 frame and bakes that authentic rod tile into the remaining space. East remains the engine-flipped west pose, which also flips the rod from the original west orientation to the original east orientation. When the male player is already surfing, `SetPlayerAvatarFishing` selects the dedicated surf-fishing sheet and the fishing task later restores the saved surfing graphics.

## Field-move pose

The pinned Polished Crystal v3.2.3 tree has no dedicated Chris field-move sprite sheet. Its Chris-specific overworld art consists of normal, bike, surf, and fishing overlays. To remain faithful, the Expansion field-move state therefore uses Chris's authentic standing frames rather than importing Brendan's five-frame throwing pose or inventing new art.

`field_move.png` contains the authentic south, north, and west standing frames on 32x32 canvases. East is a runtime horizontal flip of west. A custom animation table repeats the appropriate directional frame for the same 24-frame-tick duration as Expansion's normal field-move animation, so Surf/Fly/Cut/etc. task timing remains unchanged while Chris visually stays facing the action.


## Cut grass field effect

Polished Crystal's `gfx/overworld/cut_grass.png` is a 16x16 four-tile source image. The pinned `data/sprite_anims/oam.asm` confirms that `.OAMData_Leaf` uses tile `$00`, the top-left 8x8 tile. Expansion's Cut grass particle is also an 8x8 sprite, so the converter ports that exact tile without scaling or repainting.

The live `gFieldEffectPic_CutGrass` and `gFieldEffectPic_CutGrass_Copy` symbols now use `graphics/field_effects/pics/polished_cut_grass.png`. Expansion's existing Cut palette, eight-particle orbit motion, sound, map edits, and timing remain unchanged.


## Cut tree object

Polished Crystal's `gfx/overworld/cut_tree.png` is one 16x16 tree made from four 8x8 quadrants. Its Cut animation does not redraw the tree: `.Frameset_CutTree` switches among OAM layouts that progressively move the left and right quadrants apart.

The converter precomposes those authentic OAM positions into four 32x16 frames. Frame 0 centers the intact 16x16 tree; frames 1–3 spread the halves outward exactly according to `.OAMData_CutTree2`, `.OAMData_CutTree3`, and `.OAMData_CutTree4`. Both Emerald and FRLG cuttable-tree object IDs use the new sheet while retaining their existing palette tags and Expansion's existing obstacle-removal timing.


## Strength boulder object

Polished Crystal stores its ordinary boulder, smashable rock, and fossil together in `gfx/sprites/boulder_rock_fossil.png` as three stacked 16x16 objects. The top 16x16 object is the sprite used by `strengthboulder_event`.

The converter extracts that boulder without scaling or repainting and remaps RGBDS white to GBA OBJ transparency. Both Expansion pushable-boulder graphics IDs now use `graphics/object_events/pics/misc/polished_strength_boulder.png`. Their existing palette tags, collision, Strength checks, movement behavior, and map scripting remain unchanged.
