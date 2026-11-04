# Polished Crystal -> pokeemerald-expansion Port

This branch is the clean base for a visual/content port of Polished Crystal onto pokeemerald-expansion.

## Engine base

- Repository: PKMN2K/pokeemerald-expansion
- Branch: polished-crystal-port
- Base release: pokeemerald-expansion 1.17.0
- Base commit: e8bd1cd7b03fc032ea37e3ecd38b379b5d01a1e7

## Polished Crystal source pin

- Repository: Rangi42/polishedcrystal
- Release: v3.2.3
- Source commit: 3fa43192379df5c3e7b09a08e4d5d79af4f02f42

Do not silently advance this source pin. Any future source upgrade should be deliberate and recorded here.

### Supplemental asset pins

Some authentic Polished Crystal assets were added after v3.2.3. When one is deliberately imported, pin it separately rather than silently moving the whole project baseline.

- Chris running sprite:
  - Repository: Rangi42/polishedcrystal
  - Development commit: 9dfcd39459a505f2cd0901de2d878cb93f9e4773
  - Source: `gfx/sprites/chris_run.png`
  - Reason: dedicated PLAYER_RUN / SPRITE_CHRIS_RUN artwork is not present in v3.2.3.

## Porting rule

pokeemerald-expansion remains the engine. Do not translate Polished Crystal wholesale from RGBDS assembly into C.

Port, adapt, or reproduce:
- authentic graphics and palettes
- overworld sprites and animation sets
- tilesets, metatiles, maps, and collision intent
- UI presentation
- battle presentation
- data/content choices
- scripts/events
- mechanics that are genuinely unique to Polished Crystal

Prefer pokeemerald-expansion's native implementation when it already provides an equivalent mechanic.

## First milestone

Boot the clean expansion engine into a test map that uses:
1. an authentic Polished Crystal player overworld sprite,
2. an authentic Polished Crystal outdoor tileset and palette,
3. one NPC,
4. grass, water, a sign, and a door,
5. correct movement/collision.

No broader game-content port begins until this vertical slice is working.

## Playable visual slice

The first milestone now has an opt-in ROM build:

```sh
make -j4 POLISHED_JOHTO_DEMO=1
```

Open `pokeemerald-johto-demo.gba` and select **NEW GAME**. Chris starts in a
24-by-20 Johto garden with Totodile, Surf, an Old Rod and a Mach Bike. The sign
and NPC have working dialogue; the laboratory door leads into the original
10-by-12 Elm's Lab block layout and returns outside. The pond's southern opening
supports native Surf entry and exit. The lab NPC heals the party.

The garden is a demonstration arrangement of unchanged source blocks, **not a
replacement for New Bark Town or a claim that its story has been ported**. Its
roof uses New Bark's nine source roof tiles and palette group 24. The interior
uses the source's fixed daytime indoor palette. Existing Hoenn startup and saves
remain available in ordinary builds; the two maps occupy a newly appended group.
FireRed and LeafGreen retain their own region filtering and do not offer this
Emerald demo mode. Prism mining assets and rewards are unchanged.

The live integration includes:

- Source-authentic outdoor graphics, roof replacement, laboratory tiles and NPC.
- All four outdoor RGB555 periods for backgrounds, Chris and the NPC, explicit
  roof routing, and animated water. Source-colored OBJ tags bypass the engine's
  extra time tint only on the registered outdoor map; indoors they use day colors.
- Source-backed wall, grass and water behavior; land elevation 3 and water 1.
- Separate map-grid collision and tile behavior, with unsupported unused tile
  attributes left invalid. This is not permission to register every source block.
- Fixed horizontal-frame packing for Chris walking/running and the imported NPC.

`build_johto_demo.py --check` reproduces the binary assets from pinned fixtures.
GBA regression tests check real map dispatch, reciprocal warps, palette opt-in
guards, and complete visible sprite frames. `test_johto_demo_emulator.py` boots a
fresh ROM and uses real button input to verify movement, interaction, doors,
Surf, bike mounting/dismounting, and separate land/surf fishing poses and their
return to the prior movement state. All four casting directions retain the same
body anchor, including the native Surf bob. It checks live palette RAM against
the pinned source and animated water
in VRAM, including nighttime transitions to and from the fixed-palette lab.
CI builds the demo and uploads its ROM, screenshots and validation JSON.
The emulator probe uses libmgba; it does not patch game RAM or inject scripts.

## Whole-port completion status

### New Bark Town geography

The opt-in demo now also reaches the original **20-by-18 New Bark Town** and
all seven connected interiors: Elm's Lab, both floors of the player's home,
the neighbors' house, both floors of Lyra's home, and Elm's house. Speak to the
second guide on the east side of the demonstration garden to visit; the town
teacher offers a return trip. These two travel prompts are demo navigation.

The maps use the v3.2.3 block layouts, original door/stair coordinates and
source collision permissions. The neighbors' house correctly uses the shared
`JohtoHouse4.ablk` source layout. All 19 warp events retain their source
destinations, including wide door exits and Elm's sideways entrance. House
tiles, palettes and 21 static sign/furnishing interactions are imported from
the same pin. Elm's healing machine invokes Expansion's native party healing.
The town opts into New Bark's four outdoor/roof palettes; all seven interiors
use the source's fixed daytime colors. Existing map/layout IDs stay unchanged.

This completes the town's **geography and traversal phase**, not its story or
population. Conditional NPCs, starter selection, the rival/Lyra scenes, Mom's
events, hidden items, room decorations and Route 29/27 connections are still
pending. The unconnected town boundaries remain closed by source border
collision. No story flags or Headbutt source-map assignments are invented.
New Bark's original `TREEMON_SET_CITY` encounter data remains in the dormant
selector. Registering its geography does not enable that data in live play;
the audit checks that the selector still has no production caller. The seven
interiors have no source tree encounter sets.

`build_new_bark.py --check` reproduces the maps, tilesets, static scripts and
source manifest. `test_new_bark_emulator.py` follows all 19 directed warps with
real input from a fresh demo game, checks the destination and landing cell,
compares live day/night palette RAM, exercises reading/healing interactions,
and returns to the garden. CI includes both this check and the original demo
movement/fishing checks. Fixtures and outputs are recorded in
`tools/polished_port/manifests/new_bark.json`.

The playable visual slice and dormant Headbutt encounter selection are complete
parts of the port. **The whole Polished Crystal migration is not complete.**
Remaining areas include the broader tileset/map and NPC conversion, live unique
field interactions (including Headbutt), Crystal UI and battle presentation, and
the selected game content and scripts. No complete Johto/Kanto campaign,
Pokégear, Wing Case, or source story progression is claimed by this demo.

## Dormant Headbutt encounter phase

The encounter-data and selection phase now has one compiled path from an
original Polished Crystal map identity to an Expansion species/form and level:

- 43 original map group/number assignments select nine tree groups.
- City shares Canyon's data; the eight distinct groups contain 16 normal/rare
  distributions and 54 original rows. Rock Smash remains separate.
- All 22 tree species resolve to Expansion IDs. The audit also preserves the
  three separate Rock Smash species, for 25 candidate species total.
- Hisuian Voltorb and Electrode resolve to their dedicated Expansion species IDs.
  Original form value 4 is never used as an Expansion form-table index. The
  existing engine's stats, abilities, graphics and learnsets remain authoritative.
- Tree score, 10/50/80 percent battle gates, weighted selection, and all 17 badge
  counts compose into `JohtoHeadbuttSelectEncounterFromSourceMap`.
- Missing sets, unsuccessful rolls, invalid inputs and configured-out species
  return distinct results. Only a valid encounter writes the output; no rerolls,
  base-form substitutions, RNG consumption or save changes occur.

Validation covers 2,193,000 map/score/roll/badge combinations, 655,360 coordinate
cases, all 562 source maps without a Headbutt set, and 271 disabled-species row
cases in compiled production C. ROM regression tests check the real species
data and creation of Hisuian Pokemon, badge levels and RNG preservation. CI also
checks that the generated C tables match the pinned source fixtures.

Regenerate or check the tables with:

```sh
python3 tools/polished_port/scripts/export_johto_headbutt_encounter_tables.py
python3 tools/polished_port/scripts/export_johto_headbutt_encounter_tables.py --check
python3 tools/polished_port/scripts/test_johto_headbutt_encounters.py
```

The API deliberately accepts **source** coordinates and map IDs. Live Emerald
maps must receive an explicit mapping when they are registered; matching map
numbers or names is not a valid bridge. The demonstration maps above have no
Headbutt encounter mapping: a custom garden must not silently inherit a source
route's encounter set. Headbutt field-move entries, tree animation, Wing Case
inventory, rewards and battle scripts remain deferred to their own live
interaction phase. Prism mining assets and rewards are outside this work.

Additional fixtures retain the same v3.2.3 pin: `data/wild/badge_base_levels.asm`,
the `SetBadgeBaseLevel`/`AdjustLevelForBadges` excerpt from
`engine/overworld/wildmons.asm`, and `data/pokemon/base_stats/voltorb_hisuian.asm`
and `electrode_hisuian.asm`. They substantiate progression and form identity;
they do not import Polished Crystal's battle balance.
