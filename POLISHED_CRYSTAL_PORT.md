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
numbers or names is not a valid bridge. No Johto maps, field-move menu entries,
tree animation, Wing Case inventory, rewards or battle scripts are activated by
this phase. Those belong to the subsequent field-interaction/registration phase,
which still requires in-emulator validation. Prism mining assets and rewards are
outside this work.

Additional fixtures retain the same v3.2.3 pin: `data/wild/badge_base_levels.asm`,
the `SetBadgeBaseLevel`/`AdjustLevelForBadges` excerpt from
`engine/overworld/wildmons.asm`, and `data/pokemon/base_stats/voltorb_hisuian.asm`
and `electrode_hisuian.asm`. They substantiate progression and form identity;
they do not import Polished Crystal's battle balance.
