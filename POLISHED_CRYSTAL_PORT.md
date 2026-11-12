# Polished Crystal -> pokeemerald-expansion Port

This branch ports the complete Polished Crystal game onto pokeemerald-expansion.
Polished Crystal is the source of truth for gameplay, Pokemon data, graphics,
sprite metadata, presentation and content. pokeemerald-expansion supplies the
GBA runtime and implementation framework.

## Intended deliverable

The baseline is **Polished Crystal running in a GBA engine**, with its complete
data and identity migrated. Expansion's added capacity and capabilities remain
available for later hack customization.

| Responsibility | Target |
| --- | --- |
| Default playable game | Polished Crystal's roster, data, balance, battle behavior, art, UI, maps, events and progression |
| Engine capacity | Retain Expansion's support for more Pokemon, more moves, larger map data and other extensions |
| Future customization | Additional content and mechanics stay available for deliberate use after the source baseline is established |

Source fidelity does not require GBC memory banking, narrow identifiers, table
limits or identical binary records. Preserve the GBA engine's extensible data
structures and use explicit source-to-target mappings. Conversely, retained
Expansion content must not silently enter the source campaign through default
species data, encounters, trainer parties, learnsets, evolutions, rewards or
battle rules. Unused optional content can remain in the repository; its presence
does not make it part of the Polished Crystal baseline.

## Engine base

- Repository: PKMN2K/pokeemerald-expansion
- Branch: polished-crystal-port
- Base release: pokeemerald-expansion 1.17.0
- Base commit: e8bd1cd7b03fc032ea37e3ecd38b379b5d01a1e7

## Polished Crystal source pin

- Repository: Rangi42/polishedcrystal
- Release: v3.2.3
- Source commit: 3fa43192379df5c3e7b09a08e4d5d79af4f02f42
- Source variant: default release build (`FAITHFUL` undefined), including the
  hack's own rebalanced data; other optional source build modifiers are off.

Do not silently advance this source pin. Any future source upgrade should be deliberate and recorded here.

### Supplemental asset pins

Some authentic Polished Crystal assets were added after v3.2.3. When one is deliberately imported, pin it separately rather than silently moving the whole project baseline.

- Chris running sprite:
  - Repository: Rangi42/polishedcrystal
  - Development commit: 9dfcd39459a505f2cd0901de2d878cb93f9e4773
  - Source: `gfx/sprites/chris_run.png`
  - Reason: dedicated PLAYER_RUN / SPRITE_CHRIS_RUN artwork is not present in v3.2.3.

## Porting rule

The user's scope clarification on 2026-11-05 explicitly includes the battle
engine, all Pokemon data and sprites, and all supporting sprite data. This
applies to ordinary mechanics as well as Polished Crystal-specific changes.

Port or reproduce the pinned source in the GBA runtime. RGBDS assembly cannot
run directly on the GBA; implement its required behavior in C and native
scripts/data. Reuse an Expansion implementation only when its behavior matches
Polished Crystal. A matching move/species name or a generation preset is not
evidence of parity, and `GEN_LATEST` is not the port's target specification.

| Area | Required source coverage |
| --- | --- |
| Battle engine | Damage/stat calculations, turn order, RNG/probabilities, status, move effects, abilities, held items, weather, AI, capture and battle rewards |
| Pokemon data | Species/forms, stats, types, abilities, evolutions, level/egg/TM/tutor learnsets, growth, breeding, catch/EXP/EV data, held items and Pokedex data |
| Pokemon graphics | Front/back sprites, icons, overworld art, normal/shiny palettes and supported form/gender variants |
| Sprite data | Dimensions, frame/tile mappings, animation sequences and timing, palette selection, positioning/anchors and visible effects |
| Other graphics and presentation | Trainer/player/NPC art, tilesets, maps, collision, UI, battle presentation and source audio |
| Game content | Trainer parties and AI, encounter tables, items, scripts/events, progression and the complete Johto/Kanto campaign |

Conversions may change binary layout, compression, numeric IDs and canvas
padding where required by the GBA. Preserve the source pixels, colors and
observable behavior; record the source-to-target mapping. Target storage
differences such as DV/IV representation must also preserve the intended
source behavior rather than silently changing balance.

Existing separately requested additions retain their documented scope. In
particular, the standalone HGSS music backups are not authorized for live
integration by this clarification.

### Battle and Pokemon parity: current gaps

The campaign through Falkner is playable, but this full scope is **not yet
implemented**. The [baseline audit](tools/polished_port/PARITY_AUDIT.md) records
the inherited assumptions and the work still required after the learnset
import described below:

- `include/config/battle.h` still selects many Expansion generation defaults.
  They need comparison with Polished Crystal's actual battle implementation.
- Source base stats, types, ability slots and level-up/evolution-move tables now
  use shared accessors in the Johto build. Expansion still supplies ability
  effects and sprite/animation bindings. Egg/teaching data and evolution rules are
  imported separately below, including default mid-trainer timing; hidden-form
  and optional-setting gaps remain explicit.
- Pokemon battle sprites, palettes, icons and animation metadata still need
  the comprehensive source import. Imported trainer and overworld artwork does
  not establish completion of Pokemon battle graphics.
- Native ability/nature selection, trainer AI and the current DV-to-IV/EV
  adaptations need a source-parity audit. Earlier phase descriptions identify
  the current implementation, not an accepted final substitution.
- The 16 generic battle-test repairs in `d6911800` maintain the runtime. Their
  passing tests do not verify Polished Crystal rules or approve extra mechanics
  absent from the source.

The next battle/data work must use the pinned source inventory and current live
bindings to import remaining data/assets and adapt mechanics with source-based tests.
Every temporary native fallback for Polished Crystal content remains an open
parity gap. Unused Expansion content retained for later customization is not a
gap. Retain the existing campaign replay as regression coverage when changing
the battle foundation.

### Source level-up data and species identities

`build_pokemon_learnsets.py` imports the pinned default release's level-up and
evolution moves through its species/form pointer tables. It resolves source
conditionals explicitly and retains blob hashes for all inputs. All **391**
source species/form identities now resolve: **361** bind to existing Expansion
species IDs and **30** have new, separate IDs. These include Armored Mewtwo,
the source red Gyarados form, and cosmetic Arbok, Pikachu and Magikarp variants.
Their original art remains pending; registering data does not complete artwork.

`P_POLISHED_LEVEL_UP_LEARNSETS` is enabled in the Johto demo build. The shared
accessor now supplies source moves for new starters, gifts, eggs, trades and
wild Pokemon, subsequent level-up, evolution moves and relearning. Source
examples include Cyndaquil learning Ember at level 10, Chikorita learning
HealingLight at level 12 and both screens at level 17, and Togepi learning
Fresh Snack at level 29. Extra Expansion species keep their own tables.
Existing saved moves are not rewritten.

Fury Strikes, HealingLight and Fresh Snack have distinct new ordinary move IDs,
with generated source names, descriptions and basic data. Their native battle
adapters preserve the source six-slot multi-hit distribution, floor-before-
doubling sun healing, Utility Umbrella's rain/sun limits and rounded-down
half-HP healing. Existing ordinary move IDs remain unchanged. Their original
battle animations and Fresh Snack's field use are still pending, as are other
source move data/effects. Evolution rules are imported in the separate phase below.

Checks include deterministic regeneration, all source identity counts and
variant handling, native construction/learning/form/capacity tests, custom-move
battle tests, and player starter-move assertions in the real-input emulator
campaign. Passing these checks completes this data import for the currently
mapped identities; it does not complete the broader Pokemon or battle port.

### Source species base data

`build_pokemon_base_data.py` joins the pinned `BaseData` INCLUDE order to the
source species/form indices. Default-release conditionals are resolved before
conversion, and all inputs have Git blob hashes. The resulting 391 mappings
cover base stats, both types, all three ability slots, growth, catch rate, base
experience, EV yields, gender, egg groups, egg-cycle counts and initial happiness.

`P_POLISHED_SPECIES_DATA` defaults to the Johto demo profile. A shared accessor
selects compact source records for all gameplay consumers: Pokemon/trainer
creation, battle typing/stats/rewards/capture, growth, breeding and stat displays.
Other Expansion species and the native profile retain their existing values.
Examples include Bug/Psychic Butterfree with 110 Special Attack and Special
Defense, Ghost/Flying Noctowl, Chikorita's second ability Natural Cure, and
Armored Mewtwo's Psychic/Steel typing and distinct stats/EV yields.

Source female fractions map exactly to GBA personality thresholds: each eighth
uses 32 of the 256 possible low-byte values. Fixed-gender species retain the
native female-only and genderless sentinels. Source hatch nibbles
become `(n + 1) * 5` cycles, including literal nibble 4 for Galarian Mr. Mime;
`HATCH_UNKNOWN` becomes 80. Step timing and breeding rules still need their own
source parity work. Initial happiness uses source `BASE_HAPPINESS` (70).

All existing Pokemon IDs 1..1572 remain unchanged; new forms occupy 1573..1602.
The non-saved Egg graphics sentinel moves to 1603. Saved eggs contain their
actual species and an Egg flag, not that sentinel. Existing form-table indices
are retained by appending the new entries. A saved Pokemon's species, moves,
experience and EVs are not rewritten. Its derived stats and level calculations
use the selected profile when recalculated; use a fresh save to evaluate source
progression, especially when a species changes growth curve.

The form registrations still use temporary native art and metadata. The later
evolution phase overrides their native rules. Egg/teaching lists and held-item
tables/probabilities are imported in the later phases below. Ability effects,
remaining item mechanics and trainer/battle rules remain open.
See `tools/polished_port/PARITY_AUDIT.md`; this is a base-data import, not a
claim that the full Pokemon or battle-engine migration is complete.

Regenerate and check with:

```sh
python3 tools/polished_port/scripts/build_pokemon_learnsets.py --check
python3 tools/polished_port/scripts/build_pokemon_base_data.py --check
python3 tools/polished_port/scripts/test_pokemon_learnsets.py
python3 tools/polished_port/scripts/test_pokemon_base_data.py
python3 tools/polished_port/scripts/build_pokemon_evolutions.py --check
python3 tools/polished_port/scripts/test_pokemon_evolutions.py
```


### Source evolution rules

`build_pokemon_evolutions.py` imports all 190 ordered evolution rows and binds
all 391 registered source identities. `P_POLISHED_EVOLUTIONS` selects them in
the Johto build through the shared evolution accessor and source rule adapter.
It covers source level/stone/trade-held-item conditions, 160 friendship,
morning/day versus evening/night, move/stat/party/location requirements and
three critical hits. Terminal aliases and spiky-eared Pichu cannot inherit a
native evolution. Held evolution items are consumed after animation success,
so checks and cancellation preserve them. Odd Souvenir has stable appended
item ID 879 and the source's three regional evolution uses.

The default source timing is now active: after all levels/moves from an EXP
award, a party member can evolve before a trainer's next Pokemon. This includes
bench recipients. Wild battles and the final trainer opponent still defer
evolution until after battle. The ordinary evolution UI supports cancellation
and learning evolution moves, then restores the battlefield and music. An
active evolved Pokemon reloads its stats/types/moves/ability without resetting
stat stages or volatile battle state, and runs its source entry-ability timing.
Consumed evolution items cannot be restored by native trainer-battle cleanup.

The [baseline audit](tools/polished_port/PARITY_AUDIT.md#evolution-data-and-rule-adapter)
records the remaining work: unported landmark maps, optional initial options
(including disabling mid-battle
evolution and EV-disabled stat comparison), and source animation/item artwork.
No Hoenn location substitutes for an unported source landmark. These phases
do not claim complete evolution or battle-mechanic parity.

## Hidden-form lifecycle

The Johto profile now preserves source form values for Ekans/Arbok,
Dunsparce/Dudunsparce and Magikarp/Gyarados through wild creation, evolution,
breeding, hatching, capture and PC/save storage. Wild Magikarp uses all 20
patterns, and the three-segment chance is the source's 1 in 25. Explicit
encounter forms bypass random generation, including explicitly plain forms.

The adapter uses five unused encrypted Pokemon bits; existing species IDs and
boxed/party record sizes remain unchanged. Older zero-field saves retain their
registered visible identity without rerolling. Evolution previews and B
cancellation preserve the saved value. Daycare follows the source parent/form
selection and its pinned special-form inheritance quirk, documented in the
[baseline audit](tools/polished_port/PARITY_AUDIT.md#hidden-forms-and-inheritance).
Original form art and the remaining breeding/creation rules are still pending.

## Egg moves and teaching compatibility

`P_POLISHED_LEARNING_DATA` selects pinned egg moves and compatibility with
Polished Crystal's 75 TMs, six HMs and 31 tutors for all 391 source identities.
The shared accessors cover daycare, move teaching and the Dex's move lists.
Source lists can be explicitly empty; they never fall through to Expansion
data. Expansion-only species retain their native lists. Existing species,
move and item IDs, saved moves and Pokemon record sizes are unchanged.

The source egg-move sequence is live: level-one defaults, shared level-up moves
in physical parent-one order, father egg moves, then mother/non-Ditto egg moves.
Duplicates retain their slot and overflow removes the oldest move. Source eggs
do not inherit arbitrary TM moves or gain Volt Tackle from a Light Ball.
The pinned plain Slowpoke table's extra `dp` is preserved as the two bytes the
source engine reads: Sleep Powder and Double Slap. This is documented in the
generated learning manifest rather than silently correcting the source.

Traded eggs also retain their encrypted payload and shiny state when ownership
changes on hatching. The shared OT setter now changes the encryption key safely;
new-record initialization remains separate. A native daycare loop-counter error
that could overrun the shared-move scan is corrected as well.

The source TM item catalog/numbering, tutor locations, reminder UI, remaining
breeding attributes and regional offspring selection are still open. Ordinary
move PP and effects remain native pending the move-data phase. See the
[baseline audit](tools/polished_port/PARITY_AUDIT.md#egg-moves-and-teaching-compatibility)
for scope and verification.

## Source wild held items

`build_pokemon_held_items.py` joins all 391 source species/form identities to
pinned held-item pairs. The manifest records explicit mappings for 70 actual
items plus `NO_ITEM`, source item IDs and attributes, and the separate
`ALWAYS_ITEM_2` sentinel. `P_POLISHED_WILD_HELD_ITEMS` enables this data/rule
adapter by default in the Johto build. Unmapped Expansion species retain their
native tables, and explicitly assigned nonzero encounter items are preserved.

The source guarantee sentinel takes priority over the legendary encounter
flag. Legendary encounters otherwise take item one without rolling; this is
an encounter flag, not a species classification. Ordinary encounters make two
independent, short-circuited byte rolls. The RGBDS `percent` macro multiplies
by 255: ordinary thresholds are 127/25, boosted thresholds 153/127. Even empty
or equal-item pairs follow these rolls. Compound Eyes, Super Luck or an Amulet
Coin on physical party slot zero provide the one boosted profile. A fainted
lead still counts; an Egg cannot supply an ability. The default source checks
the Amulet Coin independently. Native Altering Cave tables cannot override
mapped source data. Optional DexNav uses the same generator and preserves its
already-rolled empty result; it remains disabled in the baseline.

Pink Bow, Silver Leaf and Gold Leaf have appended stable IDs 880..882 with
source singular/plural names, prices, descriptions and original 24x24 Bag
icons/RGB555 palettes.
Pink Bow boosts **Fairy**, not Normal, using the native 20% type-power adapter.
The leaves' tutor exchanges remain part of the unported source map services.
Existing item IDs and Pokemon/save record layouts are unchanged.

Source hash/regeneration checks cover all mappings, the 131,072 ordinary and
boosted byte pairs, and exact icon pixels/colors. Native tests cover lead
selection, boundaries, guaranteed/legendary ordering, equal/empty pairs,
short-circuited RNG consumption, doubles, explicit items, native isolation,
Altering Cave/DexNav bypasses, capture/PC flash saves, live item-icon loading,
wild battle initialization and Pink Bow's type-specific damage adapter.
All 71 focused Polished tests pass; the full native suite passes 5,293 tests
with nine expected failures and 586 existing TODOs. The final Johto ROM passes
all 100 campaign checks, 19 warp checks and 12 visual demo checks. See the
parity audit for the final ROM hash and validation sequence.

This does not complete the item catalog or battle profile: existing mapped
items still need source price/description/art/effect audits; source damage
order and rounding, TM inventory/numbering, tutor placements and general item
mechanics remain explicit gaps. The GBA RNG supplies the byte draws; its
sequence is not a reproduction of the GBC RNG.

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

This completes the town's **geography and traversal phase**. The ambient
resident phase below populates the source's unconditional dialogue NPCs.
The window rival, opening starter sequence and Mom's opening are covered below.
Later story-dependent NPCs, hidden items, room decorations and Route 27
are still pending. Route 29 is connected in the phase described below.
The remaining unconnected town boundaries remain closed
by source border collision. No Headbutt source-map assignments are invented.
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

### New Bark ambient residents

Six source residents are now live: the green-clothed man and daytime schoolboy
in town, the neighbors' daughter and evening/night husband, and Elm's wife and
son. Their source dialogue, positions, walking axes/radii and facing directions
are preserved using Expansion's native NPC scripts and movement. The existing
town teacher now looks around, while retaining the demo's return-trip prompt.

The four additional walking sheets are converted directly from pinned source
pixels; Elm's wife reuses the existing Pokefan F frames with her original green
palette. Red, blue, green and brown source colors remain distinct. Outdoor
actors use the imported time palettes; interiors retain source daytime colors.
New graphics IDs and palette tags are appended without renumbering old IDs.

As in source `LoadObjectMasks`/`CheckObjectTime`, schedules are evaluated when
loading a map. The schoolboy appears from **05:00 through 16:59**; the husband
appears from **17:00 through 04:59**. Map-local temporary flags are recomputed
from RTC hours, independent of Expansion's configurable time-period cutoffs.
Returning from an ordinary menu keeps the current mask. Save/Continue computes
a fresh mask and reconciles the restored actor, so a saved daytime/nighttime
presence cannot override the clock. A clock change never removes a speaker in
the middle of a conversation. No persistent story flags are assigned.

`build_new_bark.py --check` also reproduces these sprites, palettes, events and
scripts from the pinned fixtures. The mGBA check exercises all six conversations
and walking bounds, verifies outdoor period colors and indoor daytime colors,
checks both schedules around all four source time boundaries and midnight, and
tests native Save/reset/Continue with visibility changes in both directions.
The source schoolboy's Pokégear dialogue is retained; it does not add Pokégear
functionality. Further story-dependent cast members remain pending.

### Rival at Elm's lab window

The first New Bark rival encounter is live at the original `(3,2)` window
position, with the pinned Rival walking sheet and source red time palettes.
The map permits only the original west approach at `(2,2)`. Both source text
passages play in order: the rival initially watches the window, then turns
toward the player before the shove.

The player faces north, backs down one cell to `(2,3)`, and jumps two cells to
`(2,5)` without turning around. The rival moves left into the vacated cell,
faces down, and returns right to `(3,2)`. Crystal's `jump_step_down` performs
both a jump and a landing step, so the port uses native `jump_2_down`, not a
one-cell hop. The short source follow sequence is expressed as two simultaneous
scripted movements; it does not replace the global follower. `SFX_TACKLE` maps
to Expansion's own Tackle impact, `SE_M_COMET_PUNCH`.

`FLAG_HIDE_POLISHED_NEW_BARK_RIVAL` reserves previously unused persistent bit
`0x20` in both Emerald and FRLG without changing save size or existing IDs.
It corresponds to source `EVENT_RIVAL_NEW_BARK_TOWN`. The window encounter
repeats until the Mystery Egg visit at Mr. Pokémon's house, whose source setter
now makes the rival leave town.

The real-input emulator suite verifies the two exact dialogue buffers, all
four source sprite palettes, intermediate positions/facings, jump height,
native impact playback, input locking and control restoration. It repeats the
scene immediately, after leaving/re-entering town, after native Save/Continue,
and while riding the demo's Mach Bike. Frame traces and screenshots accompany
the validation report. Later rival progression remains a separate unfinished
phase; Mom's opening is described below.

### Elm and Lyra's introduction and starter choice

Walk to the lab entrance at `(6,4)` in New Bark Town. Lyra approaches from her
source position and leads the player inside. Elm's full release dialogue,
refusal loop, email reaction and request play before choosing a starter.
Elm and Lyra follow their original paths and finish at the source positions.
Trying to leave before choosing triggers Elm's original reminder.

All three choices use the pinned colored ball artwork, a species preview and
cry, confirmation, native gift/nickname handling, and Lyra's matching choice:

| Player's Lv5 starter | Held item | Lyra's choice | Lyra's nickname |
| --- | --- | --- | --- |
| Cyndaquil | Oran Berry | Chikorita | Chicory |
| Totodile | Oran Berry | Cyndaquil | Cinder |
| Chikorita | Oran Berry | Totodile | Toto |

The existing demo Totodile and equipment remain available. The gift uses the
normal party/PC transfer behavior; if neither has space, the choice is not
committed and no ball disappears. Declining a starter leaves every choice
available. Elm gives directions and registers his contact, then the lab reaches
source scene 6. The remaining ball is descriptive and cannot grant another gift.
The real lab's healing machine becomes usable after the gift; the garden lab's
testing facilities remain available independently.

Elm, Lyra, the aide and the three balls use exact v3.2.3 source pixels and
daytime indoor palettes. Lyra uses the existing red time palette outdoors.
Ball frames use only the source standing ball, not its cut-tree/fruit frames.
`new_bark_intro.py` translates the selected source movement and dialogue labels;
`build_new_bark.py --check` reproduces all 106 products, including the battle
and home assets described below.

Three previously unused vars shared by Emerald and FRLG hold town scene
(`0x40DB`), lab scene (`0x40DC`) and starter choice (`0x40E5`). Contact bit `0x21`
is also unused by both base games. Save sizes and existing IDs are unchanged.
Ball visibility derives from the saved choice on entry/Continue. Native menu
returns retain the current cutscene state, including the nickname-screen return.
Use NEW GAME for this expanded demo; old exploratory saves predate these actors
and scenes.

The mGBA suite takes all three starter branches using real button input. It
checks refusal and decline, both Cyndaquil approach directions, the healing gate,
actor positions and palettes, nickname entry, the
actual encrypted Pokémon's species/level/item/checksum, preservation of the demo
party, duplicate-gift prevention, and native Save/reset/Continue plus reentry.
Snapshots include cartridge savedata and are taken only through gameplay.

Starter selection hands off to the first Lyra battle below, followed by the
Route 29, Cherrygrove and Mr. Pokémon journey described in the later sections.
The usable Pokégear phone interface remains pending.
Elm's contact is stored for that later interface. Sound cues and Pokémon battle
art currently use Expansion's assets; this does not claim a Polished Crystal
soundtrack or a complete battle-art migration.

### Lyra's first battle and the aide's Potion

The original `(4,6)` lab trigger now starts Lyra's first battle. Her corresponding
Lv5 Chikorita (Chicory), Cyndaquil (Cinder), or Totodile (Toto) retains its source
nickname and two starting moves. The native trainer parties use no held item,
zero EVs and IV 22 to represent the source class's DV 11 stat contribution.
Expansion supplies its ordinary abilities, natures, AI and battle rules.
The party and learnset excerpts record the v3.2.3 source blob hashes and line
numbers in `lyra_battle_excerpts.json`.

Lyra's original 56×56 trainer portrait is centered horizontally and aligned to
the bottom of a native 64×64 canvas without scaling. Its original two colors,
white and black are preserved as RGB555; only the source background index is
transparent. The portrait ID and three trainer IDs are appended, leaving all
existing IDs and the maximum trainer-flag allocation unchanged. The remaining
six Emerald slots are used by Mikey, Don, the three first-rival teams and Joey
in the Route 30 and return phases. FireRed/LeafGreen retain their own roster.

Both winning and losing return to the lab. Lyra uses her original dialogue,
walks five steps down and leaves; the entire party is healed before source
scene 5 becomes active. Crossing `(4,8)` or `(5,8)` brings the aide over by the
corresponding source path. He gives **one Potion**, returns to `(2,9)`, and
advances to source scene 2. A failed native Bag insertion leaves scene 5 pending
so the player can retry; a successful gift cannot repeat.

Lyra's visibility derives from the existing saved lab scene and a map-local
flag. No permanent flag or save field was added. Save/Continue before and after
the Potion retains her departure and the correct pending/completed handoff.
The native battle helper needed one correction: `RIVAL_BATTLE_TUTORIAL` is a
composite value containing `RIVAL_BATTLE_HEAL_AFTER`. Testing any overlapping
bit incorrectly activated Emerald's Birch/Zigzagoon tutorial for a healing-only
rival battle. The check now requires the complete tutorial value, preserving
FRLG's explicit tutorial mode. A native setup regression exercises all three
flag configurations.

The mGBA suite checks the live portrait pixels/palette, nicknames and moves for
all three opponents, an actual full-party defeat, both aide approaches, native
Save/Continue, reentry and duplicate-gift guards.
The resident movement probe also observes walking lanes from nearby and allows
bounded native random pauses while still requiring displacement and checking
source movement bounds. This addresses the prior CI #1438 movement assertion.
Mom's opening and Route 29 are now covered below. Later campaign scenes and the
phone interface remain unported.

### Mom's opening, home schedule and weekday setup

The player's downstairs home now contains the authentic Mom and Matron sprites,
using the source's red daytime indoor palette. All four original coordinate
triggers and direct interaction start Mom's release introduction. The two
eastern approaches finish at `(10,4)`; the northern approaches finish at `(9,3)`.
The slow steps, facing changes, exclamation and final left-facing pose are
preserved. The release-text importer correctly excludes Mom's DEBUG-only text.

The one-time handoff records Pokégear ownership, the Phone Card and Mom's
contact. It then offers all seven weekdays with confirmation and retry,
the source's DST yes/no and confirmation loop, and both Running Shoes answers.
The weekday picker uses Expansion's native multichoice interface. A saved
weekday offset follows the actual RTC without changing its date, time or daily
event counters. `PolishedJohtoGetWeekday` exposes that chosen weekday for later
Johto events. Initial DST selection records a flag without adding an hour,
exactly as the source's `Special_InitialSetDSTFlag` does.

Mom's opening explains the hold-B Running Shoes controls and invites the player
home for a later daylight-saving adjustment. That adjustment becomes available
through her post-errand conversation. The always-run option and general
Pokégear screens remain future UI work.

After a map reload, Mom uses her source positions: morning `(3,2)` at 05:00,
day `(9,4)` at 09:00, evening `(6,3)` at 17:00 and night `(1,2)` at 21:00.
She remains at the opening position until that visit ends, including after
menu returns. Save/Continue recomputes the correct visibility without showing
multiple Moms. The visiting neighbor gives all three source greetings and
returns to face right. Mom's repeat text changes from Elm's waiting message to
the errand conversation after the starter. Delivering the Mystery Egg unlocks her bank.

Four unused shared flags (`0x22`–`0x25`) and two unused vars (`0x40F7`–`0x40F8`)
hold ownership/contact/DST, home scene and weekday offset. Save sizes and all
existing IDs are unchanged. The source RTC routine and clock-text excerpts
retain the same v3.2.3 pin and blob hashes.

The native tests cover all weekday values, invalid choices, preservation of
the original clock and rollover through eight subsequent days. Real-input
mGBA checks exercise every approach and weekday, both retry loops, DST choices,
Running Shoes answers, schedule boundaries, neighbor dialogue, pre/post-starter
repeat interactions, and Save/Continue before and after the event. Screenshots
include the native weekday picker and live clock confirmation.

The emulator now fixes its hardware RTC before New Game seeds the RNG. Its
deliberate-loss input switches while both Pokémon are healthy, then uses the
remaining Pokémon's status move with an explicit PP check. This removes the
observed exhausted-Leer loop without changing production battle behavior.

### Route 29 first visit and gatehouse

Route 29's 60×18-cell geography now connects seamlessly to New Bark. Its four
first-visit residents, three signs, Potion, two Cut trees and daily Oran Berry
tree use the pinned coordinates and source sprites. The 10×8-cell gatehouse
includes both residents, including native interaction across the officer's
counter. Source gate warps 3/4 map to native 0/1; both northern door cells stay
blocked until Route 46 is imported. There are now ten source maps and 22
reciprocal door/stair events, in addition to the two garden demo maps.

The original teacher safeguard runs at both town exit approaches until the
player receives an Elm starter. New Bark records a native blackout destination
at (6,5), where the engine restores the party without entering a Hoenn scene.
The route shares the town's traditional tile banks, roof group and four time
palettes. Its ledges use Crystal's standing-tile jump rule, with a walkable,
unoccupied two-cell landing. Other maps retain Emerald's next-tile ledge rule.

All three source grass tables preserve species, levels and combined weights.
The duplicate final slots allow exact species/level probabilities in twelve
native slots. Evening selects the day table on 153/256 rolls and the night
table otherwise, matching the source byte threshold. Native rate 18 gives a
10% unmodified check; Expansion's encounter cooldowns, abilities and Repels
remain in effect. Only this route opts into these clock rules.

Berry harvests give one to three Oran Berries, fall back to an amount that
fits, and record the RTC day only after successful Bag insertion. Same-day
reuse and clock reversal cannot duplicate berries; Save/Continue persists the
harvest and the next RTC day replenishes it. Cut uses native field-move
requirements, source tree pixels/background green colors, persistent removal,
and a one-frame native removal animation rather than unimported GBC particles.
The Advanced Tips sign replaces unsupported GBC title shortcuts with encounter
time advice. Mulch, Lyra and Tuscany become available through the later story events below. Route 46 and source music remain pending.

Shared unused flags `0x26`, `0x27`, `0xE9` hold the Potion and two Cut trees;
unused var `0x40F9` holds the last successful berry day plus one. Existing IDs
and save sizes are unchanged. Reproducible checks cover all live encounter
weights, source connections, gate remapping and flag allocation. Native tests
cover clock boundaries/evening rolls, Bag-full fallback, daily clock reversal,
blackout routing and real-map ledge landings. Real-input mGBA checks cover the
opening guard, connection, source residents, gate, pickups, Save/Continue,
daily regrowth, native nighttime battles and complete traversal back to town.

### Cherrygrove first visit, Town Map and services

Cherrygrove's original 40×22 city connects to Route 29's western edge. Its Mart,
Pokémon Center, Guide Gent's house and both speech houses retain their source
layouts and fifteen reciprocal door events. This brings the port to sixteen
source maps and 37 door/stair events, plus the two garden demonstration maps.
Route 30 is connected in the next phase below; the bay remains closed. The Center communication floor is blocked
until its story requirements and destination are imported.

The automatic east-entry trigger and direct Guide Gent conversation follow
all six original tour stops. He grants one native Town Map key item, then goes
home. If the Bag cannot accept it, the gift remains available at his house.
The Map Card ownership flag and completed scene persist through Save/Continue.
The portable map and Center wall map display the pinned Johto artwork, original
tile flips and palettes, and all 67 Johto landmark names/coordinates. Direction
buttons select landmarks; A or B returns to play. This is a usable map adapter;
the complete Pokégear clock/phone/radio interface remains pending.

The Mart sells the four original first-visit items through Expansion's native
shop: Potion, Antidote, Paralyze Heal and Awakening. The Center nurse uses the
four source greetings, Yes/No prompt, left/bowing sprite poses, native healing
fanfare and full-party healing. Its PC opens native storage, the journal records
the original Red article flag, and entering sets a Johto blackout destination
at (5,3). Native whiteout recovery heals the party there without a Hoenn NPC.
The original healing-machine ball animation and source music remain pending.

All first-visit residents use source sprites, positions, colors and dialogue,
including the animated Pidgey. The two children retain their conversation
facings. Signs, shelves and ordinary furniture use source text. The hidden
Nugget and Fisher's Mystic Water use native Bag handling and persistent flags;
the Fisher records completion only after successful insertion.
The Nugget's northeastern ledge requires the northern water approach through
Route 30; the land-route tests audit its event encoding.

The city registers source roof group 26. Its time refresh cannot be skipped
when another clock reader has already updated the shared time variable. The
Center copies the source red bank into its Johto floor bank, and the generic
Mart uses its original blue override. Interiors retain their fixed day colors.
The water table preserves the original Tentacool/Tentacruel levels and 60/30/10
weights. Native encounter rate 11 is the closest integer to the source's 6%
base probability; Expansion cooldowns, abilities and Repels remain active.
Fishing encounter tables and later story-dependent events are still pending.

Five unused flags shared by Emerald and FRLG (`0x4A7`–`0x4AB`) hold guide-home,
Map Card, Nugget, Mystic Water and journal state; unused var `0x40FA` holds the
city scene. Existing IDs and save sizes stay unchanged. Deterministic generation
now reproduces 163 source-map products. Host audits compare the guide's route,
doors, source encounters and special palettes. Native tests check reciprocal
landings, blackout routing and map/palette isolation. The real-input mGBA
campaign exercises the tour, map viewer, services, residents and saved state.

### Route 30, Mulch and Mr. Pokémon's first visit

The original 26×54 Route 30 connects north of Cherrygrove with the source's
five-block offset, translated to ten native cells. Both 8×8 houses and all six
reciprocal door events are live. The port now contains nineteen source maps and
43 directed door/stair events, plus the two garden maps. Route 31 stays closed.
Original signs, the Antidote, hidden Potion, Cut tree and two fruit trees retain
their coordinates. Joey's Rattata/Pidgey battle blocks the first-visit western
path; his two attack movements return both Pokémon to their source positions.
Mikey and Don have native trainer battles, source portraits, species, levels
and early learnsets. Gen II DV 6 contributes native IV 12, matching the earlier
Lyra conversion convention. Joey becomes a trainer after the Egg is delivered;
trainer rematches remain pending. Battles use Expansion's mechanics and music.

All three grass tables preserve the source species/level probabilities. The
same source clock boundaries and evening mixture used by Route 29 now apply to
Route 30 as well. Its water tables retain Poliwag/Poliwhirl and 60/30/10
weights; native rate 4 is the closest integer to the source's 2% base check.
Native encounter cooldowns, modifiers and Repels remain active. The route uses
roof group 26, including time-dependent purple Rattata and pink Pecha colors.
Indoor colors remain fixed. The Town Map identifies these three maps as Route 30.

The Berry Master gives one original Mulch item, recording the gift only after
successful Bag insertion. Mulch has its original item pixels and palette. When
talking to a picked tree, No preserves the item; Yes consumes one and replenishes
only that tree, exactly as the source's FertilizedFruitTree flag reset does.
The shared interaction supports Route 29's Oran tree and both Route 30 trees.
Each keeps its own saved daily harvest; full-pocket fallback and clock-reversal
protection remain active. Using Mulch from the Bag is unavailable in the source
as well. Both Mulch and Mystery Egg append new item IDs without changing any
existing item's ID.

Mr. Pokémon's first-entry scene grants the Mystery Egg key item, preserves the
release dialogue and Oak's movements, removes the source Pokédex prop and Oak,
and heals the full party. The first-frame scene starts before the native doorway
step, so the original right/up approach is preserved. A full key pocket leaves
the quest pending for another visit. Oak unlocks Expansion's usable
National Pokédex so the Johto Pokémon can be viewed; original Johto ordering and
Pokédex presentation remain pending. Saving and returning preserve the handoff
and prevent repeat gifts. The source rival leaves New Bark's window, the remaining
lab ball disappears, and the visiting neighbor returns home.

Four formerly unused shared flags (`0x4AC`–`0x4AF`) store the route's pickups,
Cut tree and Mulch gift. Four unused vars (`0x40FB`–`0x40FE`) store independent
fruit dates, Mr. Pokémon's scene and the return-errand stage. Oak and the prop
use temporary visibility flags derived from the saved scene. The return stage
queues Elm's call; the city and lab scenes independently arm the rival and
theft investigation. Fishing tables and source music remain pending.

Deterministic generation reproduces 196 source-map products. Host checks compare
connections, source encounter weights, item pixels/palettes, safe flag allocation
and walkable scene paths. Native tests exercise independent harvests, conditional
Mulch consumption, clock reversal and reciprocal door landings. The real-input
emulator campaign exercises the outbound trip, gifts, choices, Oak, the native
Pokédex, healing, Save/Continue, repeat visits and the return connection.

### Return to Elm: stolen starter and the Mystery Egg

On the first exit from Mr. Pokémon's house, Elm calls about the disaster at his
lab. The source call text is displayed once using an Expansion message box and
notification sound. Saving, resetting and re-entering the house do not repeat
it. There is no general-purpose Pokégear phone interface yet.

Returning east through Cherrygrove at `(33,6)` or `(33,7)` starts the first
rival battle. All three source parties lead with a male Lv4 Rattata; the
second Pokémon is Lv5 and holds an Oran Berry:

| Player's starter | Rival's stolen starter |
| --- | --- |
| Cyndaquil | Totodile |
| Totodile | Chikorita |
| Chikorita | Cyndaquil |

The portrait uses the original 56×56 pixels and RGB555 colors, padded to the
64×64 native canvas. Trainer moves come from the pinned learnsets. A win or
loss continues the ID-card scene in town, heals the full party, and permanently
clears the encounter. No whiteout or repeat battle is required.

At Elm's lab, either aisle leads to the officer's accusation and Lyra's
intervention. The native naming screen shows the rival's overworld sprite.
Confirming an empty name retains the source default `Silver`; the officer
uses the chosen name in his dialogue. The name occupies eight previously
reserved bytes in SaveBlock2. The following clock field and save size do not
move. The Egg is removed once, Elm recoils and returns to his original tile,
and subsequent conversations use his studying-the-Egg text. The broken window,
New Bark teacher and Route 30 youngster also respond to the quest state.

Returning to Route 30 now reveals Joey at `(2,28)` in place of his staged
Rattata/Pidgey fight. His first trainer battle uses his source Lv5 male
Rattata with Guts, Adamant nature, Tackle, Leer and Quick Attack. His number
exchange and later rematches remain part of the phone migration.

Three previously unused flags shared with FRLG (`0x4BD`–`0x4BF`) record the
rival encounter, theft investigation and Egg delivery. The four trainer IDs
`860`–`863` fill the existing Emerald allocation. Before adding more trainers,
extend trainer tracking deliberately; an additional ID would cross that limit.

`test_return.py` checks source parties, choreography, flags and trainer capacity.
The campaign emulator uses normal controller input to reach all three rival
variants, both battle outcomes, both lab approaches, name entry, saved Egg
delivery and Joey's battle.

### Leaving New Bark after the errand

After the Egg delivery, Mom offers to save money. Either reply opens the bank.
Saying Yes sends one quarter of subsequent trainer prizes to Mom after prize
multipliers. Pay Day earnings and money lost in a whiteout follow Expansion's
normal rules. Like the source, the bank saturates at ¥9,999,999. Once it is
full, the entire next prize goes to the wallet. The native wallet uses its
existing ¥999,999 cap.

On the first visit, Mom encourages the player if spoken to again. Leave the
house for New Bark and return to access Get, Save, Change or Cancel. The native
seven-digit amount picker displays both balances, uses left/right to select a
place and up/down to change the amount. Zero and B cancel. An overdraft or an
amount that would overflow its destination leaves both accounts intact and
lets the player retry. Selecting No when Mom asks about money opens the source
daylight-saving conversation. Changing the clock moves exactly one hour and
is blocked if it would change the date.

At either of the two original Route 29 triggers, Lyra offers to demonstrate
catching a Lv5 Pidgey. Accepting follows her into the grass and starts the
native throw-only tutorial. The 48×48 Crystal back sprite and its two original
colors appear over a 64×64 native canvas. Its throw cue holds the same source
pixels across both animation commands. The native Bag opens directly to
Lyra's ten Poké Balls and one Premier Ball. The demonstration catches Pidgey
for Lyra, restores the player's Bag and party, and returns to her dialogue.
Declining skips the demonstration and records the source refusal flag. Either
choice gives five Poké Balls once. Lyra waits if the Bag lacks space.

After the player first discusses savings with Mom, Tuscany appears on Route 29
on the chosen Tuesday and gives one Silk Scarf. The New Bark teacher directs
the player to visit Mom while outbound Pokégear calls await their UI phase.
Mom's phone shopping and gift deliveries are deferred with that phone system.

The bank uses two unused native vars, 0x4091 and 0x409B, for the low and high
words of the balance. Flags 0x4C0–0x4C6 hold the opening, saving choice,
first visit, tutorial result and Tuscany events. These assignments use existing
save space. The source fixture audit checks both movement paths, portrait pixels
and allocations. The emulator campaign checks actual deposits and withdrawals,
Joey's prize, Lyra's accepted/declined branches from both rows, party and Ball
preservation, Tuesday's gift, and native Save/Continue.

### Route 31 and the Violet gate

Walk north from Route 30 after returning the Mystery Egg. Route 31 uses the
original 44×18 layout, group 26 outdoor colors, standing-tile ledges, and the
morning, day and night grass and pond encounter tables. Both rows of the gate connect Route 31 to Violet City. The Dark Cave
entrance awaits its map.

The route has its Potion, Poké Ball, both Cut trees, daily Persim tree, three
signs and two residents. If the player left home without opening the bank,
Mom calls on arrival and offers to save prize money. Either answer opens the
account and persists. The sleepy man's later mail exchange needs Goldenrod.
Wade's phone registration and rematches await the Pokégear phone interface.

Bug Catcher Wade uses his four source Pokémon. After Joey, Mikey, Don and
Wade have all been defeated, Finch offers a battle. His Geodude and Mareep
each hold an Air Balloon and use their source moves, natures, abilities and
stat investments. Their starting levels are 8 and 9. Badge-based levels feed
team creation, the battle transition and the prize calculation. A win grants
one Air Balloon. A full pocket leaves the reward available on the next visit.

Emerald's original trainer flags end at 0x85F. The two new trainers use
IDs 866/867 and independent saved bits 0x4CC/0x4CD. The partner IDs between
Joey and Wade remain available to older recorded battles. Item, obstacle,
reward and introduction flags occupy the other reserved slots in 0x4C7–0x4CE.
The Persim harvest uses var 0x40FF. Var 0x409D stores the 16 earned Johto
and Kanto badges, separately from the demo's starting Surf permission. Older
cartridge saves enter Route 31 with zero earned badges.

### Violet City and its Pokémon Center

Continue west through the Route 31 gate to reach Violet City. Either row
lands at its original city tile. A cartridge save made inside the previous
gate opens both western doors on Continue. The city uses its original 40×36
layout, group 10 roof tiles, Violet/Ecruteak colors and source priority masks.
The imported outdoor maps change colors at 05:00, 09:00, 17:00 and 21:00.
The two trainers who share a street corner take turns at 05:00 and 17:00.

The city has its signs, seven ambient residents, two hidden items, PP Up,
Rare Candy, Cut tree and daily Cheri tree. Its pond has the original Poliwag
and Poliwhirl encounter levels and weights. Entering the city records the
visit for future Fly integration. The Town Map selects Violet City here and
inside its Center.

The Center offers healing, PC storage, a wall map and Falkner's journal.
Visiting makes it the blackout destination. The city entrance, both Center
exit tiles, journal and berry harvest support cartridge Save/Continue.

### Earl’s Pokémon Academy and Type Chart

Speak to Earl by the Violet Gym at (13,16). Answer Yes to his question
about beating Falkner and he stays there. Answer No and he spins, leads
you to the Academy and gives you the Type Chart. His movement follows
the original route from any of the four adjacent squares. Afterward Earl
teaches inside. If the Key Items pocket was full, speak to him there after
making room to receive the chart. His location and the chart survive saving
and reloading.

Open the chart from Key Items in the field or during a battle. The original
160×144 diagram is centered on the GBA screen. A and B both return to the
same Bag pocket. During a battle you can then choose your action for the
current turn. The chart uses the original picture, palettes and object overlays.

Both Academy exits return to the same Violet doorway. Inside, Earl teaches
about party order and experience. The blackboard covers five status conditions,
the notebook has four pages, the four students retain their conversations,
and reading Walker’s journal sets its saved flag. You can enter before Earl
takes his tour. A cartridge save from the previous Violet release can also
open the formerly blocked Academy door on Continue.

### Violet Mart and Flower Mail

The Mart opens at its original Violet City doorway at (9,17). Both interior
exit tiles return there. The source clerk, Granny and male Ace Trainer use
their original artwork, positions, movement and dialogue. Its shared Mart
tileset uses the original blue palette and interactive merchandise shelves.

The native shop sells all eleven source stock entries. Flower Mail has its
original item icon, colors and stationery tiles arranged for the GBA screen.
It uses native Easy Chat, held mail, reading and PC mailbox handling. The
consumable Escape Rope is a separate item in the Items pocket, priced at
1,000 and removed after a successful escape. Both new items have appended
IDs for cartridge-save compatibility. Escape destinations become available
as the campaign's caves and dungeons are imported.

The Johto profile now applies the pinned source's Violet Mart price
corrections through the shared item-price accessor: Paralyze Heal costs 300,
Awakening costs 100 and X Defense costs 2,000. The other eight stock prices
already agree. The Johto profile also restores half-price resale (including
Flower Mail at 25), while ordinary Expansion builds retain their native
prices and resale configuration. This is an initial source shop-economy
correction, **not** a complete item metadata/effect parity audit.

The next source-price phase additionally wires the pinned held-item manifest's
**70 mapped item prices** through `GetItemPrice` in the Johto profile. Its
zero-initialized source-price table leaves items without a source mapping on
their native Expansion price, and non-Johto builds do not compile the adapter.
The table is regenerated deterministically from the previously pinned and
hash-validated item fixture inventory; CI checks regeneration and runtime
source-profile selection. Price parity alone does not imply parity for item
icons, descriptions, field use, hold effects or battle mechanics.


The city map-load script refreshes the Mart entrance in earlier saved maps.
Its new warp is appended after the Center, eastern gate and Academy warps.
The emulator campaign exercises both exits, customer and shelf dialogue,
every stock entry, cancellation, insufficient funds, selling, mail reading
and cartridge Save/Continue. Native tests cover held mail, the PC mailbox,
full item pockets and existing warp identities.

### Violet's nickname and trade houses

The houses at (3,15) and (21,29) use the source Traditional House tiles,
layout, furniture and residents. Strawberry wanders and plays Eevee's cry.
Gramps appears from 17:00 through 04:59, including when continuing a save;
ordinary menu returns retain the schedule established for the current visit.
Both exit tiles in each house return to its original city doorway. Existing
map, layout and doorway IDs remain unchanged, and city loading refreshes the
two new entrances for earlier saves.

The source map is named `VioletOnixTradeHouse`, but its actual Kyle trade in
the pinned Polished Crystal revision is **Poliwag for Voltorb**, nicknamed
Mimic. It keeps Kyle's trainer ID 48926, Hasty nature, Aftermath hidden
ability, Premier Ball and Persim Berry. All six source DVs of 14 become IVs
of 28, preserving their base stat contribution. Mimic matches the offered
Poliwag's level. The trade uses Expansion's party/PC selector, moves, stat
calculation and animation. Its persistent completion flag prevents a second
trade and restores Kyle's source follow-up dialogue after Save/Continue.

### Sprout Tower first visit

The north door at (23,1) now opens all three original 16-by-16 floors. Their
tiles, palettes, collision, stair destinations, statues, painting, residents
and four item balls come from the pinned source. The central pillar uses
all ten source tile strips and the original 16-command animation order,
including the five-frame forward/backward sway. Existing map and warp IDs
remain unchanged; loading Violet refreshes the new door in older saves.

Sages Chow, Nico, Edmond, Jin, Troy and Neal and Elder Li use their source
parties, levels and learned moves, with authentic Sage/Elder battle art.
The class DVs become doubled native IVs; the source modern-EV conversion
gives 16 EVs per stat for Sages and 18 for Li. Li retains his Super Potion.
Expansion supplies the battle AI, ability selection, battle effects and
prize-money calculation. Each trainer has an independent persistent flag
outside the original trainer/partner ranges.

The upstairs encounters retain all seven source species/level probabilities
for morning, day and night, including the distinct 3% and 2% Bellsprout
levels. A native slot selector shared with Dark Cave preserves that distinction.
The native encounter rate is 4 (about 2.22% before native cooldowns and
modifiers); the source rate is 2%. The first floor has no wild encounters.

The summit coordinate scene retains the Elder's lecture, the player's and
Rival's paths, and the Rival's Escape Rope departure. His visibility and
the completed scene survive saving and loading. Defeating Li awards Flash
once. If the Bag cannot accept it, his next interaction retries the gift
without requiring another battle. The source reusable TM70 is represented
by Expansion's reusable HM05 Flash, using native teaching, field-move and
badge rules; the dialogue explains the native usage. This phase does not
implement Crystal's press-A-without-teaching Flash mechanic. The actual
Escape Rope item works from the tower and returns to its Violet entrance.

Mako has her original first-visit conversation. Her later challenge and Rare
Candy still require the five Ecruteak Kimono Girl victories and will be
connected with that campaign phase. Source music remains a separate phase.
Granny uses the imperial source text, matching the current engine setting.

### Dark Cave, Violet Gym and the Togepi Egg

The Route 31 cave entrance and Violet Gym now open their original source
maps. Dark Cave retains the source night palettes, collision, four pickups,
hidden Elixir and four smashable rocks. Its morning/day/night land slots
preserve all seven source species/level probabilities, and its Magikarp
water slots preserve the 60/30/10 level distribution. Native base rates are
7/180 for land and 4/180 for water, approximating the source 4% and 2% before
native movement, ability and encounter modifiers. The Blackthorn and Route
46 exits remain closed until those destinations are ported.

The Gym guide turns the player away until Falkner returns. At Dark Cave's
source trigger, he warns and steps the player back before Flash is obtained.
After the Sprout Tower reward, the original darkness branch allows the
rescue: Pidgeotto attacks Ursaring, both Pokémon leave, and Falkner follows
the source exit path. This does not require teaching Flash before the badge.
Native Flash field use retains Expansion's badge requirement. The rescue
and all three actors' departure persist through Save/Continue.

Abe, Rod and Falkner use authentic overworld and battle graphics. Abe has
Spearow at level 9; Rod has Pidgey 7 and Hoothoot 8. Falkner has Natu 10,
Hoothoot 11 and Pidgeotto 13, their explicit source moves, 32 HP EVs, doubled
class DVs (22 IVs), Pidgeotto's Pecha Berry and his Super Potion. Bird Keepers
use doubled class DVs (16 IVs) and the source modern conversion of 5 EVs per
stat. Native Expansion AI, ability selection and prizes still apply.

Victory grants the Zephyr Badge in the native first badge slot, updates
Johto's earned-badge bitmask and awards reusable TM51 Roost. The reserved
TM51 item ID is activated without renumbering old TMs or HMs. Retries after a
failed reward do not repeat the battle or badge. The old demonstration-only
fifth badge is removed when entering Violet/Gym; demonstration Surf access
is now separate from badge progress. The native Trainer Card still uses
Emerald badge art; Johto badge graphics belong to the presentation phase.

On leaving the Gym, Elm's one-time call sends his aide to Violet Center and
removes him from the lab. The source offer and refusal lead to one native
Togepi Egg, sent to the PC if the party is full. Full party and PC leave the
gift pending; success alone records it and returns the aide to the lab.
Egg creation and hatching use Expansion's native engine. The later hatch
report, Odd Souvenir reward, Elm battles and outbound phone interface are
still later campaign work. Source music remains a separate phase.

The campaign now covers 32 source maps and 82 reciprocal door/stair events,
plus the two garden maps. Existing map, warp, trainer, item and save IDs are
preserved. Shared save flags are allocated only from unused Emerald/FRLG
slots. Native tests cover reward idempotence, TM compatibility and all Egg
storage boundaries; controller-driven tests exercise the playable sequence.

The playable visual slice and dormant Headbutt encounter selection are complete
parts of the port. **The whole Polished Crystal migration is not complete.**
Remaining areas include the full battle engine behavior and AI, Pokemon data,
Pokemon sprites and their animation metadata, broader tileset/map and NPC
conversion, live field interactions (including Headbutt), Crystal UI and battle
presentation, and all remaining game content and scripts. No complete
Johto/Kanto campaign, Pokégear, Wing Case, or source story progression is
claimed by this demo.

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
  implementation now uses source base data when `P_POLISHED_SPECIES_DATA` is
  enabled and source level-up moves with `P_POLISHED_LEVEL_UP_LEARNSETS`.
  Graphics and ability implementations remain pending source parity work.
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
and `electrode_hisuian.asm`. The global base-data importer now imports these
species' source stats as well; battle-effect parity remains separate work.
