# Polished Crystal Port Tools

This directory contains conversion tooling and manifests for moving authentic Polished Crystal assets into pokeemerald-expansion without replacing the engine.

## Route 29 first visit

`build_new_bark.py` now reproduces ten connected source maps: New Bark and its
seven interiors, Route 29 (60×18 cells), and the Route 29/46 gatehouse (10×8).
The town and route share the same traditional tiles and time palettes across
their reciprocal west/east connection. Route 29 uses the source standing-tile
ledge trigger, validates the two-cell landing and leaves other maps unchanged.
The four first-visit route residents,
two gate residents, three signs, Potion, two Cut trees and Oran Berry tree use
the pinned map coordinates and source art.

Both original west-exit approaches run the teacher's safeguard before an Elm
starter is selected. Entering New Bark records a native blackout destination
at (6,5), where native recovery heals the party without a Hoenn nurse script.
The gate's source warps 3/4 become native warps 0/1; its northern door cells are
blocked until Route 46 exists. Cherrygrove and Route 46 connections, Lyra's
post-Mystery-Egg tutorial and Tuscany's quest-dependent appearance remain
deferred. No dangling warp or placeholder destination is used.

Wild encounter species, levels and combined probabilities match all three
source tables. Native twelve-slot tables preserve the seven-slot distribution
by combining the identical final two source entries. The source evening rule
selects day for 153 of 256 random bytes and night for the remaining 103.
These tables opt in without changing other maps' global clock configuration.
The native base encounter rate is 18×16/2880 = 10%; Expansion's movement
cooldown, lead abilities, Repels and other encounter modifiers still apply.

The fruit tree yields one to three Oran Berries, falling back to the amount
that fits. A successful harvest records the native RTC day; a full pocket does
not consume it, Save/Continue preserves it, and clock reversal cannot grant a
second harvest. Picked fruit retains its source blank overlay and interaction.
Mulch replenishment is deferred. Cut trees use source pixels and the copied
background green palette, with a safe single-frame native removal animation;
the Game Boy particle animation is not imported in this phase. Native Cut
requirements apply, and either interaction or the party menu recognizes them.

The source Advanced Tips sign's unsupported GBC title-screen shortcuts are
replaced with time-of-day encounter advice. Source music remains outside this
phase. Outdoor object overlays retain source pixels with native bottom anchors.

Checks: `build_new_bark.py --check`, `test_route29.py`, native `Route 29` tests,
and the real-input `test_new_bark_emulator.py` campaign. The emulator harness
reads RAM for assertions and uses ordinary game input for state changes.

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

The live fishing path compensates for that centered body with a constant eight-pixel
vertical offset and no horizontal shift, from the first cast frame through rod
retraction. Brendan's per-frame alignment adjustments apply only to the other
graphics IDs. Surf adds its existing bobbing displacement to this fixed offset;
putting the rod away restores the normal anchor. The emulator checks all four
casting directions and the separate land/Surf restoration paths.

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


## Rock Smash obstacle

Polished Crystal stores its Strength boulder, smashable rock, and fossil together in `gfx/sprites/boulder_rock_fossil.png`. The middle 16x16 object is the smashable rock used by Rock Smash.

Polished Crystal's Rock Smash script shakes the obstacle and then removes it; it does not provide separate crumble artwork. The converter therefore repeats the authentic 16x16 rock into Expansion's four 16x16 breakable-rock frame slots in a 32x32 sheet. This preserves Polished Crystal's static-rock appearance while retaining Expansion's existing animation timing, collision, Rock Smash checks, encounter/item logic, and removal behavior.


## Fossil object

Polished Crystal stores its Strength boulder, smashable rock, and fossil together in `gfx/sprites/boulder_rock_fossil.png`. The bottom 16x16 object is the fossil.

The converter extracts that authentic 16x16 fossil without scaling or repainting and remaps RGBDS white to GBA OBJ transparency. Both Expansion fossil object graphics IDs now use `graphics/object_events/pics/misc/polished_fossil.png`, while retaining their existing palette tags, object-event behavior, map scripting, and interaction logic.


## Overworld item Poké Ball

Polished Crystal stores the ordinary overworld Poké Ball, a cut-tree object, and fruit-tree graphics together in `gfx/sprites/ball_cut_fruit.png`. The top 16x16 object is the Poké Ball used for visible item objects and scripted Poké Balls.

Expansion's shared Poké Ball object graphics use five physical 16x32 frame slots. The converter bottom-aligns the authentic 16x16 Crystal Poké Ball in each slot and repeats it across all five frames, preserving Crystal's static item-ball appearance without inventing animation frames. `OBJ_EVENT_GFX_ITEM_BALL` and `OBJ_EVENT_GFX_POKE_BALL` continue to use Expansion's existing graphics-info object, palette tag, pickup/script behavior, collision, and animation timing.


## Pokédex object

Polished Crystal stores its book, paper, and Pokédex standing-object graphics together in `gfx/sprites/book_paper_pokedex.png`. The normal standing facings use tiles $00-$03 for down, $04-$07 for up, and $08-$0b for left; Oak's Lab explicitly places the shared sprite as `SPRITEMOVEDATA_STANDING_LEFT` for `OaksLabPokedexText`. That identifies the bottom 16x16 frame as the authentic Pokédex object.

The converter extracts that frame without scaling or repainting and remaps RGBDS white to GBA OBJ transparency. Expansion's existing 16x16 Pokédex object keeps its palette tag, map-event behavior, scripting, and collision; only the live graphic is replaced.


## Sudowoodo weird-tree object

Polished Crystal's `gfx/sprites/weird_tree.png` is a 16x48 sheet containing three 16x16 frames. Route 36 uses `SPRITE_WEIRD_TREE` with `SPRITEMOVEDATA_SUDOWOODO`; its special tree-shake action alternates the first two source poses, while ordinary directional movement can use the third pose when the disguised Sudowoodo flees.

Expansion already exposes three physical 16x32 frames through `gObjectEventPic_SudowoodoTree`. The converter preserves all three authentic Crystal frames, bottom-aligning each 16x16 pose inside its corresponding 16x32 GBA frame without scaling or repainting. Expansion's existing Sudowoodo graphics info, palette tag, movement/animation tables, collision, and event scripting remain unchanged.

## Johto Traditional priority masks and live demonstration

The 128 authentic shared tiles and 160 authentic Traditional tiles occupy GBA 4bpp
IDs 1–288. Tile 0 is transparent padding. Original GBC background color 0
is opaque, so the normal tile graphics use GBA pixel index 4 and duplicate
the original RGB555 color 0 at palette index 4, without changing its color.

GBC BG attribute bit 7 prioritizes source color indices 1–3 over sprites;
source color 0 still lets sprites show through. All 38 original flags are
preserved across seven source 32x32 blocks and eleven Emerald 16x16 metatiles.
Twelve source-derived BG1 foreground masks occupy GBA graphics IDs 289–300.
Only GBC color-0 pixels become GBA transparent index 0 in those masks; the
lower BG2 graphic stays opaque and the original palette/flip bits are retained.
Unflagged positions use transparent BG1 tile 0. The matching Emerald layer
is METATILE_LAYER_TYPE_NORMAL (BG2 lower, BG1 upper).

Run source-backed regression tests with:

    python3 tools/polished_port/scripts/test_johto_tile_graphics.py
    python3 tools/polished_port/scripts/test_johto_metatile_banks.py
    python3 tools/polished_port/scripts/test_johto_priority_overlays.py

The two isolated Johto demo layouts now use this conversion pipeline; build
them with `make POLISHED_JOHTO_DEMO=1` and select NEW GAME. The outdoor garden
registers only its reviewed terrain, collision, water animation and source
time/roof palette routing. Elm's Lab uses its original block layout and fixed
indoor palette. Chris and the imported Pokefan use the exact four source red
OBJ palettes outdoors, with daytime colors restored indoors.

Unused metatile attributes remain invalid. The demo does not authorize every
Traditional block or every sprite elevation; additional source foreground
blocks and raised sprites still need their own live validation before use.
See `POLISHED_CRYSTAL_PORT.md` for the playable scope and emulator checks.
Prism mining assets are unchanged.

## Cherrygrove first visit

`build_new_bark.py` also generates the six Cherrygrove maps and their first-visit
events through `polished_cherrygrove.py`. The original guide route, residents,
signs, furniture, special indoor palettes and map artwork come from the same
v3.2.3 fixture pin. The native Town Map key item opens a Johto viewer; shops,
healing, PC storage, gifts and blackout recovery use Expansion services.

Run `test_cherrygrove.py` for source parity and `make check TESTS=Cherrygrove`
for the three native tests. `test_new_bark_emulator.py` continues through Route
29 into Cherrygrove and invokes `test_cherrygrove_emulator.py` with real input.
Its checkpoints are created by that ROM's own gameplay; no injected save or RAM
mutation substitutes for completing the tour and service interactions.
See `POLISHED_CRYSTAL_PORT.md` for complete scope and remaining story work.
