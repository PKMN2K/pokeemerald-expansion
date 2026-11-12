# Polished Crystal baseline audit

Source: Polished Crystal v3.2.3, commit
`3fa43192379df5c3e7b09a08e4d5d79af4f02f42`, **default release build**
(`FAITHFUL`, `DEBUG`, `HGSS`, `MONOCHROME` and `NOIR` undefined).
The source Makefile selects this variant by default. The word "faithful" in
the port's goal means faithful to this Polished Crystal build, not selecting
the optional `FAITHFUL` variant and discarding the hack's rebalances.

This audit follows the scope clarification recorded at `a1482a0f`. The old
workflow proved particular maps and events playable while inheriting much of
Expansion's global Pokemon and battle data. Those results remain useful
regression coverage, but are not evidence of complete source-game parity.

## Findings and disposition

| Area | Evidence from the existing port | Required correction / current disposition |
| --- | --- | --- |
| Source build variant | Previous trainer scripts read selected source text, while global data uses `GEN_LATEST`. Source files contain `FAITHFUL` branches. | Resolve the pinned default branch explicitly in importers; reject unknown conditionals. The learnset importer now does this. |
| Level-up moves | `GetSpeciesLevelUpLearnset` previously returned Expansion tables for player starters, gifts, eggs, trades and wild Pokemon even where trainer parties had source moves. | **Imported:** all 391 mapped species/form entries, including source evolution moves and cosmetic aliases. The Johto build selects these through the shared accessor. Existing Pokemon keep saved moves; subsequent learning uses the new source tables. |
| Species/form identities | Native numeric IDs do not equal source IDs or form values. Thirty source forms previously lacked target IDs. | **Registered:** all 391 identities now resolve. Thirty forms have distinct appended IDs 1573..1602, with base/form-table round trips. Their original artwork and metadata are still pending; source evolution rules are now selected separately. |
| Custom move identities | Fury Strikes, HealingLight and Fresh Snack do not exist as separate Expansion moves. | **Added:** distinct ordinary move IDs and generated source names, descriptions and basic data. Native battle adapters implement source multi-hit probability and healing fractions/rounding. Original battle animations and Fresh Snack's field use remain open. |
| Other move data and effects | `src/data/moves_info.h` still supplies most move definitions; battle configuration selects many latest-generation rules. | Import all source move data/flags and audit effect implementations, damage/stat rules, status, weather and RNG. A matching move name is insufficient. |
| Species stats and related data | Native generation presets previously supplied these values. | **Imported:** all 391 base-data mappings provide default-source stats, types, three ability slots, growth, catch/EXP/EV data, gender, egg groups/cycles and initial happiness. Gameplay uses `GetSpeciesBaseData`; additional Expansion species retain native data. Held-item pairs/probabilities are imported separately below. Pokedex/presentation data, existing item/ability effects and other battle/creation rules remain open. |
| Wild held items | Native pairs and one percent roll previously selected source species items. | **Imported:** all 391 source pairs, exact two-byte branch rules, guaranteed/legendary priority, lead boosts and 70 item mappings. Pink Bow and both leaves have distinct appended IDs and original Bag icons. Existing mapped item data/effects, source battle rounding and leaf tutors remain open. |
| Evolution conditions, breeding and teaching | Source evolution rows now replace native conditions for all 391 registered identities, with a source evaluator, shared family/Pokedex metadata and default evolution timing inside trainer battles. Source egg groups and initial cycle counts are imported. | Hidden forms for the Ekans, Magikarp and Dunsparce families, all 391 egg/teaching lists and source egg-move inheritance are now adapted. Remaining evolution options/locations, hatch-step timing, breeding attributes/regional offspring selection and item mappings remain open. |
| Trainer construction and AI | Imported parties preserve selected species, levels and moves, but use native AI/ability selection, fixed Hardy natures and DV/IV/EV adaptations. | Audit source trainer attributes and construction semantics. Recheck actual battle stats and behavior after species data import. |
| Pokemon graphics and sprite data | Source trainer portraits and several overworld sprites are live; Pokemon battle graphics, palettes, icons and animation bindings mostly remain native. | Import source pixels, normal/shiny palettes, dimensions, frame maps, sequences, timing, positions and form/gender handling. Include all unresolved forms. |
| Startup and player identity | `POLISHED_JOHTO_DEMO` starts in a test garden with a Lv10 Totodile, Surf, an Old Rod and a Mach Bike. Existing campaign replays use that harness. | Implement the real source startup separately from the test garden, with the source initial party/inventory, opening flow and supported protagonists. Add a campaign run starting from that baseline. |
| UI, Pokedex and audio | Native Bag/party/storage/battle UI, National Dex ordering and native audio are still used in imported events. A source Type Chart does not complete the UI. | Port source presentation and ordering. Keep separately requested HGSS backups unwired. |
| Map and event coverage | Thirty-two source maps and selected events through Falkner are live. Door, collision, time-palette, schedule, reward and Save/Continue checks cover those implementations. | Preserve these checks; audit their documented native substitutions and incomplete interactions. Broader Johto/Kanto geography and story, Pokégear, source services and field interactions remain open. |
| Field rules and encounters | Source encounter distributions are imported for selected maps, but native encounter modifiers, field-move gates and other engine rules still apply. Headbutt's source selector is dormant. | Audit source field behavior and enable each feature with an explicit map identity and source rules. |
| Save compatibility | Target save IDs are already allocated and tested. Source DVs/records cannot be copied byte-for-byte into native encrypted Pokemon. | Preserve stable regular move/species/item/save IDs where possible; document representation changes and migration requirements. Source behavior, not GBC table width, is the target. |
| Completion criteria | Generic native battle tests can pass with entirely native data. | Require source fixture/provenance checks, source-specific native tests, and live emulator assertions for each migration phase. Keep the broader suite as regression coverage. |
| Replay assumptions | The old tower replay relied on Expansion's early Bite and remaining Surf PP, confused the returning Start menu with field control, and could accept an interrupted interaction as complete. Capture input assumed the remembered Bag row was a ball and did not handle newly reached learn levels. | Replays now learn the source level-13 Bite through the real selector, use purchased Potions and Center visits, confirm the intended object interaction, select the actual ball stack, handle learning prompts, confirm the wild-battle replacement question and Send Out after a lead faints, and recover from a real capture blackout with bounded retries and ordinary ball purchases. Changed source data must drive test input instead of being changed to preserve the old replay. |

## This implementation's boundaries

`P_POLISHED_LEVEL_UP_LEARNSETS` defaults to `POLISHED_JOHTO_DEMO`. It chooses
source tables globally within that ROM, including normal creation, level-up,
evolution-move and move-relearner consumers. It is not a map-local substitution.
Extra Expansion species without source entries retain their existing tables
for future customization. A source form missing a target identity must be
implemented before an encounter/event can register it; it must not be resolved
as a superficially similar form.

The three custom moves are appended at Expansion's designated ordinary custom
move boundary. Existing ordinary move IDs through 847 remain unchanged. The
derived Z/Max move ranges move by three entries; these are engine-generated
battle moves, not legal saved party moves. Ordinary source moves still use
Expansion effects/data pending the next battle-data phase. The new custom moves
also use native animation assets pending the presentation import.

No existing saved Pokemon's moves are rewritten by this phase. New Pokemon
receive source moves, and subsequent level/evolution/relearner operations use
source tables. Fresh saves are required for evaluating the intended initial
conditions; old demo saves are not proof of an authentic starting game.

## Earlier learnset verification

Deterministic source regeneration, fixture checks, both native and Johto ROM
builds, and 44 focused native tests passed. The engine CI suite at `c54bd5ce`
([run 1465](https://github.com/PKMN2K/pokeemerald-expansion/actions/runs/42134823039))
passed 5,232 tests, with nine expected failures and 586 existing TODO entries.
That suite is regression evidence, not proof of unported source mechanics.

Local controller replays verified all three starters' source moves, Violet's
Poliwag capture and trade (including replacement after a lead faints and
retrying after an actual blackout), all seven Sprout trainers, learning Bite at level 13,
the rival/Flash sequence, Dark Cave, all three gym trainers, the badge/Roost
rewards and the Togepi Egg sequence. Save/Continue and repeat-interaction guards
passed. Later sections were replayed from checkpoints earned by the same ROM's
demo campaign, using ordinary movement, purchases, healing and battle inputs;
no Pokemon or inventory was injected to make those checks pass. These remain
demo campaign checks until the authentic startup is implemented.

Trainer interaction checks also recognize the intended source intro while the
field is locked; the script/last-talked probes alone missed a live Nico dialogue
in CI. Failure bundles retain both the emulator state and matching ELF so
compiler-dependent addresses can be inspected accurately. The Nico dialogue
and wild-battle replacement prompt regressions were reproduced and passed
using the actual CI ROM and matching ELF.

## Species base-data phase

`P_POLISHED_SPECIES_DATA` defaults to `POLISHED_JOHTO_DEMO`. The global
`GetSpeciesBaseData` accessor selects source records for all 391 identities.
Stats/types/abilities, growth, catch/base EXP/EV yields, gender, egg groups,
initial egg cycles and initial happiness now come from 333 pinned base files.
Native creation, trainer ability/gender selection, battle/capture/reward paths,
breeding, summary bars and Pokedex stat displays use the accessor. The direct
native HP check in `IsSpeciesEnabled` remains to avoid recursive sanitation.
Unused Expansion species fall back to native records; flags and capacities are
not reduced. The native profile remains available for engine regression tests.

Thirty appended species IDs preserve previously missing form identities.
Existing Pokemon/save IDs 1..1572 and existing form-table indices are unchanged.
Only the non-saved Egg graphics sentinel moves past the appended IDs; encrypted
eggs retain their actual species plus the Egg flag. Existing saved moves,
experience, IVs and EVs are not rewritten. Their derived stats/levels and
stored ability-slot interpretation follow the selected source profile when
recomputed; fresh saves are the baseline for progression validation.

Female fractions are mapped exactly to the target's personality thresholds,
using 32 low-byte values per source eighth (not percentages of 255). Hatch
nibbles use the source daycare formula `(n + 1) * 5`; this includes the literal
4 for Galarian Mr. Mime and the unknown nibble 15. Step timing, inheritance and
other breeding behavior are still separate gaps. Initial happiness is 70.

The new forms deliberately retain documented temporary native art and Pokedex
metadata. The later evolution phase overrides their native evolution rules.
Source held-item rows and teaching lists are
retained in the base-data manifest but are not wired by this phase. Ability
slots are imported; ability effects, trainer attributes, move definitions and
battle-rule parity remain open. These boundaries apply to every source species,
not only the newly registered forms.

### Species-phase verification

The final source generation and fixture checks pass for all 391 mappings and
333 unique base-data files. Native tests cover source creation, ability slots,
hatch cycles, every mapped identity, distinct form IDs, exact female eighths
across all 256 personality low bytes, native fallback, and Noctowl's Ghost
immunity during battle. The shared accessor also covers Battle Dome's macro
stat reads; its field parameter must not bypass the direct-read audit.

With GCC 13.2.1, both the standard Emerald and Johto ROM builds pass. The full
native suite passes 5,239 tests, with nine expected failures and 586 existing
TODO entries; no unexpected failures remain. All six AI timing limits pass.

The final Johto ROM passes all 100 recorded controller campaign checks and the
19 New Bark warp traversals. This includes all three starter choices, capture
and trade, the source level-13 Bite selector, all seven Sprout Tower trainers,
Dark Cave, all three Gym battles, badge/Roost/Togepi rewards, duplicate guards,
and Save/reset/Continue. These remain demo-start regression checks, not proof
of the still-unported authentic opening. The tested Johto ROM SHA-256 is
`53bc61bb3c5bf3c65d4f935fdd16e3db3d2478cd8d41db147c4102d28bcf7d4f`.

### CI replay repair

CI #1471 exposed a loss-replay assumption after the species import:
Cyndaquil's source Flame Body burned Lyra's Chikorita and won a battle driven
only by switching. The replay had observed the opponent only in the move menu,
so it missed that the battle had already ended. The deliberate Lyra defeat now
uses the real Totodile starter branch, and opponent validation happens at the
first action menu. The outcome assertion still requires a genuine loss, followed
by healing, the Potion handoff, Save/Continue and the no-rematch checks. Source
abilities, species data and gameplay remain unchanged.

## Evolution data and rule adapter

`P_POLISHED_EVOLUTIONS` defaults to the Johto profile. The deterministic importer
reads all 190 ordered evolution rows from the pinned default source and binds
all 391 registered species/form identities, including explicit empty tables for
terminal species and spiky-eared Pichu. Mr. Rime and Farigiraf share learnsets
with earlier species but do not inherit those species' evolution rows. Native
Expansion species outside the source registry keep their own rules and IDs.

Gameplay and the shared evolution/family/Pokedex accessor now select the source
tables. The adapter implements level, stone, trade/Linking Cord, held-item,
160-friendship, morning/day versus evening/night, stat comparison, known move,
party species, landmark and three-critical conditions. The counter increments when a critical hit lands,
not when an immune-target damage roll is calculated, and excludes link/facility
battles. A level-100 Rare Candy evolution is forced as in the source. Row order is preserved,
including Mime Jr.'s location before level and Poliwhirl's held King's Rock
before Water Stone. Source trade-held items also work on level-up, and Koffing's
Charcoal produces Galarian Weezing. Everstone behavior follows both the source
item-use precheck and the individual source evolution branches, including
Kadabra's trade protection and the source move/location/party exceptions.

Held evolution items stay on the Pokemon during eligibility checks and the
animation, and are consumed only when an evolution succeeds. Both normal and
trade animation completion paths apply this commit. Odd Souvenir has appended
item ID 879, its source name, description and 3000 price, and its three source
regional evolution uses. Its icon is a temporary native souvenir substitute.
Existing Pokemon and item save IDs are not renumbered.

The default `EVOLVE_IN_BATTLE_OPT` timing is implemented from pinned
`engine/battle/core.asm`, `evolve_battle.asm` and `abilities.asm`. Each EXP
recipient finishes all level-ups and move learning before a mid-trainer
evolution attempt. Bench recipients use their own party slot; wild battles and
the final opposing Pokemon retain deferred evolution. Cancellation/no matching
rule does not create a deferred level-up attempt; separately earned critical-hit
eligibility remains intact, matching the source flags.

The evolution UI retains battle logic allocations and script stacks while
replacing/rebuilding display resources. It restores the environment and music
sequencer, reloads the active party member, preserves stat stages/volatiles and
refreshes the evolved active mon's ability. Foe-dependent entry abilities are
skipped when the current opponent has fainted; weather and immunity effects can run.
The GBA adapter synchronizes new move PP and consumed items into the live battle
record, and prevents trainer-battle item restoration from refunding an evolution
item. Cancelled and benched evolutions do not activate the active mon's ability.
Native profiles and optional Expansion species keep their existing timing.

This is **not complete evolution-mechanic parity**. The later hidden-form
adapter below now preserves Ekans/Dunsparce and Magikarp/Gyarados form values
through creation, evolution, breeding and save storage.
The 20 maps belonging to eight evolution landmarks are recorded in the
manifest but have not yet been ported. The generator registers their exact
source map identities when they are added; no Hoenn location is substituted.
The initial-options toggle for disabling mid-battle evolution, the EV-disabled
option's special IV comparison and source evolution animation are also pending.
Default EV-enabled Tyrogue uses actual stats. General ability/battle rules remain
part of the battle-profile audit; this phase implements their evolution trigger.

Source fixtures retain evolution, item-use, default-option, battle and animation
code with verified Git blob hashes, so these boundaries can be checked directly
against the pin. Dedicated native tests cover the rules above and source/native
profile isolation; Python checks verify row counts, alias boundaries and reject
unknown source syntax. Both standard Emerald and Johto builds pass with GCC
14.2.1. With CI's GCC 13.2.1, all 38 focused Polished tests and the full
5,260-test native suite pass, with nine expected failures and 586 existing
TODO entries. All six AI timing limits pass unchanged.

The native battle tests exercise active and successive bench evolution,
evolution moves, ability activation and permanent held-item consumption through
trainer cleanup. A separate mGBA button-input replay completes and cancels the
real evolution UI, then checks the restored battle display, party/battle data,
held item, ability/weather, environment, music and deferred level-up flag. It
patches only test-runner selection/headless flags, never gameplay state. CI
runs this replay and retains its screenshots and JSON evidence.

The final Johto ROM passes all 12 demo checks, all 100 campaign checks through
Falkner and the Togepi Egg, and all 19 New Bark warp traversals, including
Save/reset/Continue and repeat-interaction guards. These are regression checks
from the existing demo startup, not evidence of the unported authentic opening
or every later-game evolution. Johto ROM SHA-256:
`0b032c95554198c552c3e86a034f5090efbfd9b54447687c20f6471d3447c843`.

## CI dependency installation

[CI #1474](https://github.com/PKMN2K/pokeemerald-expansion/actions/runs/42230082975)
passed, but its FireRed job spent 43 minutes 57 seconds fetching packages from
`azure.archive.ubuntu.com` at a reported 26.3 kB/s. This was a toolchain download
delay before compilation. The five dependency-installing jobs now pin
`ubuntu-24.04` and share `.github/scripts/install-dependencies.sh`.

The installer tries Ubuntu's main archive first and Azure once as a fallback,
with signed Noble repositories and the same package lists as before. Metadata
fetches have a 90-second deadline and package downloads a 180-second deadline
per mirror, with bounded connection waits and retries. Only after all packages
are downloaded does it install from cache without further downloads. Each
installation step also has a 12-minute outer limit. Recovery tests cover the
normal path, a real simulated download timeout, metadata failure, both mirrors
failing without installation, and propagation of an installation failure.

## Hidden forms and inheritance

`P_POLISHED_FORMS` defaults to the Johto profile. A generated mapping joins the
31 registered identities in the Ekans/Arbok, Magikarp/Gyarados and
Dunsparce/Dudunsparce families to the pinned source form numbers. It does not
allocate new species IDs or treat form numbers as Expansion table indices.

Wild Ekans/Arbok choose forms 1 or 2 equally, Magikarp chooses among its 20
patterns, and Dunsparce/Dudunsparce choose three segments on exactly one of 25
outcomes. Scripted wild creation shares those rules. Explicit forms, including
plain form 1, bypass the roll. Encounter records now accept `polished_form`,
and the source grass importer retains it; this extends ROM data only. Ordinary
gifts and generic/trainer creation retain their specified registered identity.
Trainer-specific regional defaults and further source creation rules remain
part of the trainer audit, rather than randomizing every newly created mon.

Five unused bits in encrypted Pokemon substruct 0 store the source value.
Species/item IDs, all existing fields and the 80-byte boxed/100-byte party
record sizes stay unchanged. Zero in an older save reads the registered
identity's form without rewriting or rerolling that Pokemon. Thus an old
patterned Magikarp retains its known pattern, while an old ordinary Gyarados
has no recoverable earlier pattern and defaults to plain. Native profiles keep
the new field unused. The 31 registered source identities are selected only
under the source profile; optional Expansion species keep native behavior.

Evolution rows retain the source `NO_FORM` marker. Previewing or cancelling an
evolution leaves the saved record unchanged. Successful evolution preserves
Ekans' future Arbok pattern, Dunsparce's segment value and all Magikarp patterns
inside ordinary Gyarados. Daycare chooses the mother/non-Ditto parent's form,
or independently chooses either parent when their *source species* matches,
even when their registered target variant IDs differ. Everstone is not required
for this form choice. Egg creation resolves the form before choosing moves,
stats and other species data. Native capture, boxing, daycare egg hatching and flash
save/reload preserve the stored value.

The pin contains a daycare quirk: `DayCare_GenerateEgg` holds the selected
form in register `d` across `CheckInvalidVariants` and the egg-pointer lookups,
then restores it for `NO_FORM` offspring. The comments describe rejecting named
Arbok and Red Gyarados forms, but the executed path retains those values. The
adapter therefore carries Arbok forms 3–5 into Ekans and Red Gyarados form 21
into a plain-identity Magikarp, which can evolve back to Red Gyarados. Verified
source fixtures and register-flow checks preserve this behavior explicitly;
this is not a silent upgrade or correction to a newer source version.

Original form artwork, palettes and animation metadata remain pending. Unown's
unlock-dependent generation, inheritance of other attributes,
hatch-step timing and the broader creation/battle rules are separate gaps.
Unported encounter and trainer maps must retain explicit source forms when
imported; no claim is made that those maps have already been migrated.

### Hidden-form verification

Pinned source generation and encounter-regression checks pass. Dedicated
native tests exercise every Magikarp pattern, all 25 segment outcomes, hidden
evolution targets, daycare with both Ditto slots and genders, native hatching,
explicit encounters, old zero-field saves and native-profile isolation. All
24 encryption/substruct orders retain neighboring fields and the existing
record sizes. Native flash save/reload preserves both party and boxed forms.
Real controller input completes and cancels patterned Magikarp's evolution,
then verifies the restored battle and saved pattern in the encrypted record.

Both standard Emerald and Johto ROM builds pass with GCC 14.2.1. With CI's
GCC 13.2.1, the 51 focused Polished tests and full native suite pass: 5,273
passed, nine expected failures and 586 existing TODO entries. All six AI
timing limits pass unchanged. The additional capture assertions exercise the
ordinary native capture handoff for every Magikarp pattern. The unchanged
Johto campaign passes all 100 checks and 19 warp checks; all 12 demo checks
also pass (ROM SHA-256 `cffcd80ecad935fb897a50c2793e035b9c9bc1b171f409c9a23dfd08dcda0483`).

Those hatching checks covered player-owned daycare eggs. The later learning
phase below reproduces and fixes the traded-egg encryption defect.

## Egg moves and teaching compatibility

`build_pokemon_learning.py` joins all 391 mapped identities to the pinned
`EggSpeciesMovesPointers` and base-data teaching bits. Cosmetic forms explicitly
share the source base row. The manifest retains the 75 TM, six HM and 31 tutor
move mappings without renumbering native item or move IDs. Shared gameplay
accessors select the imported lists under `P_POLISHED_LEARNING_DATA`, enabled
by default in the Johto profile. Mapped empty lists are authoritative, including
Magikarp; unmapped Expansion species retain their native data.
The optional native DexNav retains its initial move when the source egg list
is empty instead of calling the random-array selector with a zero count.

The pinned plain Slowpoke list has two `dp` entries. `InheritEggMove` skips only
the first two bytes, so the second pair (`SLOWPOKE` = `$4f`, `GALARIAN_FORM` = 3)
is interpreted as Sleep Powder and Double Slap. These moves are retained for
plain Slowpoke's family and excluded from the separate Galarian list. The
generator asserts this exact source anomaly and rejects unknown extra headers.

The live daycare adapter follows `InitEggMoves`: level-one defaults, common
level-up moves in physical parent-one order, father's egg moves, then the
mother/non-Ditto parent's egg moves. Evolution-only entries cannot be inherited
as shared level moves. Duplicates do not gain priority; later overflow can
remove an earlier duplicate. PP is filled for the resulting moves. There is no
source TM-inheritance or held-Light-Ball move-injection branch. The native
shared-level scan also had `i++` in its inner `j` loop; this is corrected and
covered with a native-profile Magikarp inheritance regression.

The traded-egg regression failed before the ownership fix with invalid move
1739 during `BoxMonRestorePP`. The OT-ID setter now XORs the old/new key
difference through the encrypted payload and preserves actual shiny status.
It does not reorder substructs, alter checksums or bless already corrupt data.
Creation sets the OT before the first encryption. Hatching, capture and the
battle controller use the same safe setter; boxed/party sizes remain 80/100.

Source fixture hashes, deterministic regeneration and 60 focused native tests
pass. New cases cover all 391 accessor mappings, regional/empty lists, physical
slot order, duplicate overflow, both genders with Ditto in either slot,
forbidden native inheritance, all 24 encrypted layouts with both shiny states,
corruption detection and a traded-egg hatch/party/PC flash save round trip.

The full native suite passes 5,282 tests, with nine expected failures and 586
existing TODOs (5,877 total). After the final optional DexNav guard, all 60
focused tests pass again; standard and Johto production builds succeed.
The controller campaign passes all 100 checks and 19 warp checks on ROM
`8545ae60eec4bc07ba8c38e1a8bb60b90555ed1844fc276b9b45e350c12f734e`.
The final Johto rebuild, `ec19f73c2ed754a65176ae87c5d6d8e04815e0b681b52b30f3070be9e3b564d4`,
also passes all 12 demo checks. A binary comparison shows that its only change
from the campaign ROM is the diagnostic source-line literal at `0x1118a8`
(2289 to 2295) in `Task_OpenDexNavFromStartMenu`; DexNav is disabled in this
profile, and the gameplay code/data are otherwise identical. The guard remains
available when the optional feature is enabled for future customization.

Remaining scope: source TM item numbering/descriptions/inventory, tutor map
placement and reminder UI; broader offspring selection, IVs, abilities,
natures, balls and hatch timing; ordinary move PP/effects and source artwork.
The imported compatibility lists do not establish parity for those systems.

## Wild held-item identities and generation

`build_pokemon_held_items.py` imports all 391 source pairs with 70 real item
mappings, `NO_ITEM` and a separate `ALWAYS_ITEM_2` flag. The pinned fixtures
include item constants, attributes, names, descriptions, icon pointers,
original PNGs and RGB555 palettes. The shared species/form base-data fixtures
retain the battle routine, ability lookup and percent macro.

`P_POLISHED_WILD_HELD_ITEMS` is on for Johto. Guaranteed item two precedes the
legendary encounter's item-one rule; neither consumes RNG. Otherwise, the
first byte uses threshold 127 (ordinary) or 153 (boosted), and a failed first
roll alone permits a second byte with threshold 25 or 127. This reproduces
`percent = n * 255 / 100`, including empty/equal slots. The exact numerators
over 65,536 byte pairs are 32,512/3,225/29,799 for common/rare/none ordinarily,
and 39,168/13,081/13,287 when boosted. GBA random bytes replace the GBC RNG
sequence without replacing those probabilities.

The physical lead's Compound Eyes or Super Luck works even when fainted, but
not when an Egg/empty. The default source's independent Amulet Coin check also
boosts, without stacking. Native species-based legendary guesses and native
Altering Cave item tables do not participate. Explicit nonzero scripted items
remain intact. Unmapped species and the disabled profile preserve native data;
unmapped legendary encounters remain itemless. The disabled optional DexNav
uses this generator if enabled and cannot reroll its selected empty result.

Appended items 880..882 are Pink Bow, Silver Leaf and Gold Leaf; prior regular
item IDs and all saved Pokemon layouts are unchanged. Source names, prices,
descriptions, Bag-icon pixels and palettes are generated. Pink Bow uses the
native 20% Fairy-type power adapter; the source also specifies Fairy and 6/5,
but complete damage-stage order/rounding parity is still pending. Existing
mapped item definitions/effects are not certified by their identity mappings.
Source leaf tutors, source TM numbering/inventory and broader item mechanics
remain open. Source UI layout and contextual icon-corner handling remain part
of presentation work; the new graphics reproduce the original Bag assets.

Held-item validation: all source fixture checks and deterministic generators
pass. The full native suite passes 5,293 tests, with nine expected failures and
586 existing TODOs (5,888 total). After the final source plural-name metadata
import (Pink Bows, Silver Leaves, Gold Leaves), all 71 focused Polished tests
pass again. Standard Emerald and Johto production builds succeed; all 12
visual demo checks pass on final ROM
`066342bbcae086738bdf55b014d0f943b37d656710886265113a5ba612fac452`.
The complete controller campaign on this same final ROM passes all 100 checks
and 19 warp checks, including Falkner, Zephyr Badge/Roost, Togepi and native
Save/Continue. All 26 CI source/asset commands also pass on the final files.

## Bag HP-recovery items

`P_POLISHED_ITEM_HEALING` defaults to the Johto profile. The generator selects
all 14 HP-use entries from the pinned item-effect dispatch table and reads
their amounts from source attributes. Party use, player/opponent battle use
and the native AI's healing estimate share the source amount adapter.
Potions, drinks, bitter healing herbs, Oran Berry and Berry Juice use their
source fixed amounts. Max Potion's `-1` parameter means 999 HP in the source;
Sitrus heals one quarter and Figy one third, rounded down with a one-HP minimum.
The GBA calculation retains capacity above the source's 767-HP routine limit.

Figy now supports the source party-menu and battle Bag use. The native profile
retains its held-only Figy behavior, and unmapped items keep native handling.
No saved item identifiers change. Existing HP caps and fainted/full-health
party checks remain in the shared use handler.

This phase covers **Bag HP-recovery amounts and Figy use availability**, not
all healing/held-item semantics. Held Figy still has Expansion's flavor-based
confusion behavior pending the held-effect pass. Berry activation timing,
herb happiness, status/revival/PP effects, trainer item decisions, source UI,
item icons and the remainder of the item catalog remain separate gaps.

Validation covers source-fixture regeneration, six new native healing tests,
both standard/Johto production builds and all 12 visual demo checks. All six
AI frame limits pass with CI's GCC 13.2.1. GCC 14.2.1 reports 29 frames against
the 28-frame smart-doubles limit on both this change and the untouched
`55d71ec` baseline; no performance ceilings were relaxed. The native profile's
healing lookup is inlined away, while tests can still select either profile.
The tested Johto ROM SHA-256 is
`24d566080db200fbae5245c48e0c43e132e8a5592fafb9dbcf0ad14f24c8eabd`.

## Next priorities

1. Complete the remaining evolution options above, broader item definitions/
   effects and source TM/tutor inventories. Complete breeding/hatching behavior
   beyond base data and inherited moves.
2. Import remaining move data and implement a source battle profile, including
   trainer creation and AI. Validate source probabilities and rounding directly.
3. Import Pokemon graphics and complete the associated sprite metadata.
4. Establish the real source startup and rerun the existing campaign from its
   authentic initial state; continue correcting field/UI/audio gaps and the
   remaining campaign with the same source-based completion criteria.

Expansion's extra content and capacities remain available for later hacking.
Unused optional content is not a parity defect; using it in place of Polished
Crystal data or behavior in the playable baseline is.
