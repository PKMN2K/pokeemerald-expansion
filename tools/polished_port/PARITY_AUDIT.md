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

The [complete move/machine data audit](reports/moves-machines-7e32866f.md)
checks all 255 source moves, 75 TM and six HM identities, and all 43,792
species/form compatibility combinations across the 112 machines/tutors against
production Johto accessors. At `7e32866f`, 22 move rows have basic-data differences,
35 TM and two HM identities differ, and only TM31/TM51/TM70 use the explicit
source lesson adapter. All source PP/PP Up and compatibility checks match.
These are not claims of complete move effect or machine UI parity. Some move
table differences are representation differences requiring behavioral review.

The subsequent [move-balance correction](#source-move-balance-corrections)
imports twelve of those rows and removes their seventeen field discrepancies.
The subsequent Hidden Power suite imports its remaining power/type fields and
the default source's dynamic type calculation. The Pain Split suite below
imports its Ghost type and source HP-sharing rules. The fixed-damage adapter
below accounts for Sonic Boom/Dragon Rage, leaving six differing
move rows. Machine identity/lesson gaps remain as recorded in the original report.

CI now runs `audit_moves_machines.py` with an explicit known-gap inventory.
New or changed findings fail; resolved gaps are permitted. Strict mode without
`--known-gaps` exits nonzero for the currently unfinished work. The generated
Markdown/JSON evidence is included in the Johto artifact and job summary.

| Area | Evidence from the existing port | Required correction / current disposition |
| --- | --- | --- |
| Source build variant | Previous trainer scripts read selected source text, while global data uses `GEN_LATEST`. Source files contain `FAITHFUL` branches. | Resolve the pinned default branch explicitly in importers; reject unknown conditionals. The learnset importer now does this. |
| Level-up moves | `GetSpeciesLevelUpLearnset` previously returned Expansion tables for player starters, gifts, eggs, trades and wild Pokemon even where trainer parties had source moves. | **Imported:** all 391 mapped species/form entries, including source evolution moves and cosmetic aliases. The Johto build selects these through the shared accessor. Existing Pokemon keep saved moves; subsequent learning uses the new source tables. |
| Species/form identities | Native numeric IDs do not equal source IDs or form values. Thirty source forms previously lacked target IDs. | **Registered:** all 391 identities now resolve. Thirty forms have distinct appended IDs 1573..1602, with base/form-table round trips. Their original artwork and metadata are still pending; source evolution rules are now selected separately. |
| Custom move identities | Fury Strikes, HealingLight and Fresh Snack do not exist as separate Expansion moves. | **Added:** distinct ordinary move IDs and generated source names, descriptions and basic data. Native battle adapters implement source multi-hit probability and healing fractions/rounding. Original battle animations and Fresh Snack's field use remain open. |
| Other move data and effects | `src/data/moves_info.h` still supplies most move definitions; battle configuration selects many latest-generation rules. | All 255 source PP records and source PP Up maxima are imported; Return happiness power is adapted. Import the remaining source move data/flags and audit effect implementations, damage/stat rules, status, weather and RNG. A matching move name is insufficient. |
| Species stats and related data | Native generation presets previously supplied these values. | **Imported:** all 391 base-data mappings provide default-source stats, types, three ability slots, growth, catch/EXP/EV data, gender, egg groups/cycles and initial happiness. Gameplay uses `GetSpeciesBaseData`; additional Expansion species retain native data. Held-item pairs/probabilities are imported separately below. Pokedex/presentation data, existing item/ability effects and other battle/creation rules remain open. |
| Wild held items | Native pairs and one percent roll previously selected source species items. | **Imported:** all 391 source pairs, exact two-byte branch rules, guaranteed/legendary priority, lead boosts and 70 item mappings. Pink Bow and both leaves have distinct appended IDs and original Bag icons. Existing mapped item data/effects, source battle rounding and leaf tutors remain open. |
| Evolution conditions, breeding and teaching | Source evolution rows now replace native conditions for all 391 registered identities, with a source evaluator, shared family/Pokedex metadata and default evolution timing inside trainer battles. Source egg groups and initial cycle counts are imported. | Hidden forms for the Ekans, Magikarp and Dunsparce families, all 391 egg/teaching lists and source egg-move inheritance are now adapted. Remaining evolution options/locations, hatch-step timing, breeding attributes/regional offspring selection and item mappings remain open. |
| Trainer construction and AI | Imported parties preserve selected species, levels and moves, but use native AI/ability selection, fixed Hardy natures and DV/IV/EV adaptations. | Audit source trainer attributes and construction semantics. Recheck actual battle stats and behavior after species data import. |
| Pokemon graphics and sprite data | Source trainer portraits and several overworld sprites are live; Pokemon battle graphics, palettes, icons and animation bindings mostly remain native. | Import source pixels, normal/shiny palettes, dimensions, frame maps, sequences, timing, positions and form/gender handling. Include all unresolved forms. |
| Startup and player identity | `POLISHED_JOHTO_DEMO` starts in a test garden with a Lv10 Totodile, Surf, an Old Rod and a Mach Bike. Existing campaign replays use that harness. | Implement the real source startup separately from the test garden, with the source initial party/inventory, opening flow and supported protagonists. Add a campaign run starting from that baseline. |
| UI, Pokedex and audio | Native Bag/party/storage/battle UI, National Dex ordering and native audio are still used in imported events. A source Type Chart does not complete the UI. | Port source presentation and ordering. Keep separately requested HGSS backups unwired. |
| Map and event coverage | Thirty-two source maps and selected events through Falkner are live. Door, collision, time-palette, schedule, reward and Save/Continue checks cover those implementations. | Preserve these checks; audit their documented native substitutions and incomplete interactions. Broader Johto/Kanto geography and story, Pokégear, source services and field interactions remain open. |
| Field rules and encounters | Dark Cave rocks use source eligibility, repeats and encounter/reward rolls. Flash now uses source eligibility, confirmation, full-map lighting and persistence; source TM70 is live, with old HM05 saves still supported and presentation gaps explicit. Other selected encounters/field moves still use native modifiers and gates. Headbutt's source selector is dormant. | Audit source field behavior and enable each feature with an explicit map identity and source rules. |
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
battle moves, not legal saved party moves. Apart from source PP and Return's
happiness power below, ordinary source moves still use Expansion effects/data pending further
battle-data work. The new custom moves
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
all healing/held-item semantics. The following held Figy phase removes its
native flavor-based confusion in the source profile. Berry event timing,
other friendship events, status/revival/PP effects, trainer item decisions, source UI,
item icons and the remainder of the item catalog remain separate gaps.

Validation covers source-fixture regeneration, six new native healing tests,
both standard/Johto production builds and all 12 visual demo checks. All six
AI frame limits pass with CI's GCC 13.2.1. GCC 14.2.1 reports 29 frames against
the 28-frame smart-doubles limit on both this change and the untouched
`55d71ec` baseline; no performance ceilings were relaxed. The native profile's
healing lookup is inlined away, while tests can still select either profile.
The tested Johto ROM SHA-256 is
`24d566080db200fbae5245c48e0c43e132e8a5592fafb9dbcf0ad14f24c8eabd`.

## Held Figy Berry

The same `P_POLISHED_ITEM_HEALING` profile now maps Figy's held effect to the
pinned source's `HELD_BERRY` behavior. It restores `max(1, floor(maxHP / 3))`
without flavor/nature confusion. Natural consumption uses the inclusive
quarter-HP threshold, or half HP with Gluttony; each threshold also has a
one-HP minimum. The source steal/reconsume routines bypass the pinch check
but reject full HP. Bug Bite and Cud Chew use those forced-consumption rules.
Fainted Pokemon cannot heal. The switch-in AI estimates the same source
threshold and amount, capped at missing HP; this is an estimate correction,
not an import of Polished Crystal's trainer AI.

Figy keeps its existing item ID and native dispatch category. Only its live
effect is overridden before the native flavor-confusion handler. Other flavor
berries and the disabled source profile retain native behavior. The source
effect does not apply the native Ripen multiplier or native Heal Block test;
those are absent from the pinned `HELD_BERRY` routine. Existing engine dispatch
still supplies Unnerve/Klutz checks, berry scripts and consumption timing.
Complete ability interactions, source event ordering, text and animation are
separate gaps. No saved IDs or Pokemon layouts change.

The source guard verifies the hashed item attributes, natural and forced
healing dispatch, Gluttony threshold, minimum-one arithmetic and absence of
flavor confusion. Eight new native tests cover all 25 natures, inclusive
thresholds, damage crossing the threshold, one-time consumption, Unnerve/Klutz,
Bug Bite above pinch and at full HP, Cud Chew, all source rounding inputs and
profile isolation. Other held healing items remain unaudited; Berry Juice's
source Berries pocket versus the existing target Items pocket also needs an
explicit save-compatible migration before changing its dispatch.

Validation: all 33 CI source/asset commands pass. With CI's GCC 13.2.1, the
full native suite passes 5,307 tests, with nine expected failures and 586
existing TODOs (5,902 total), including all six AI timing limits. Standard
Emerald and Johto production builds succeed with GCC 14.2.1, and all 12 Johto
demo emulator checks pass. The tested Johto ROM SHA-256 is
`bf1a860dc9addf6d14a631eee8663e3563a81cf3f74da9bdd9008b219c796512`.
These demo checks cover the existing overworld; the eight native tests above
exercise the new Figy behavior in mGBA. The full controller campaign remains
part of automatic CI.

## Bitter-medicine happiness

The four source bitter medicines now use generated happiness penalties under
`P_POLISHED_ITEM_HEALING`. The importer reads the pinned item-effect dispatch,
event constants, happiness table and negative-change routine. It verifies
their source Git blob hashes and keeps the original 100/160 thresholds.
Energy Powder and Heal Powder subtract 5 below 160 and 10 at or above 160;
Energy Root subtracts 10/15 and Revival Herb subtracts 15/20. Previously the
native 200 threshold delayed the larger penalty, and native capture-location
or Luxury Ball bonuses could soften a loss. The source negative branch skips
all happiness bonuses and clamps at zero.

The party-item handler and the separate battle HP/status handlers apply the
penalty only when the medicine successfully has an effect. The source egg
exclusion is retained. Stored
party happiness and the exact active battler's happiness are synchronized,
so current-battle consumers do not retain a stale value. The pointer mapping
also supports both battle sides and the partner slot in doubles without
changing another Pokemon's happiness. Benched targets retain their new value
when subsequently switched in. Status curing also guards the absent active
battler index before clearing Nightmare; treating a benched Pokemon must not
write past the active-battler array. No existing saved happiness or item IDs are
rewritten; the existing save representation is unchanged.

Native profiles, unmapped Expansion medicines and all other friendship events
retain their existing behavior. This phase does not certify source gain
events, friendship bonuses outside these losses, Return's damage calculation,
complete status/revival semantics, battle-facility rules or source
item-use text/animations. Those remain separate migration work.

Eight new native tests cover all 256 happiness values for each medicine,
the zero clamp, ignored bonuses, unsuccessful party/battle uses, egg exclusion,
native and optional-medicine isolation, both active battle sides, all four
medicines on benched targets followed by switching, and the targeted partner
in doubles.

Validation uses GCC 13.2.1, matching CI. The full native run passed 5,315 tests,
with nine expected failures and 586 existing TODOs (5,910 total), including
all six AI timing limits. After the final bench-status bounds guard, all 93
Polished tests, 23 native status-cure tests and six Full Restore tests pass
on the final code. Both production ROMs were rebuilt after that guard, and
all 12 Johto demo emulator checks pass. All 34 source/asset checks pass.
The final tested Johto ROM SHA-256 is
`00c07e1c09b4d480cffc7f5d8583010265474a84b750351de2873713fab46d42`.
Automatic CI reruns the complete native suite and controller campaign on the
published commit; local demo checks are regression coverage for the existing
overworld, while the medicine tests execute the new effects in mGBA.

## Return happiness power

`B_POLISHED_RETURN` defaults to `POLISHED_JOHTO_DEMO` and imports the pinned
Return formula: `min(102, floor(happiness * 102 / 160))`. Both battle sides and
the shared AI damage estimator read the active battler's current happiness.
The bitter-medicine synchronization above therefore changes Return immediately:
Energy Powder at 160 happiness changes power from 102 to 95 after its 10-point
penalty. No saved happiness, move IDs or Pokemon structures are rewritten.

The new hashed `engine/battle/move_effects/return.asm` and
`data/moves/effects.asm` fixtures join the existing move table, constants and
damage-command fixtures. `build_return_power.py --check` verifies the formula,
cap, Normal/physical/100 accuracy/20 PP record and damage-command order. Raw
power is zero at happiness 0 or 1. The source skips its main damage formula,
then its `stab` command floors a successful non-immune hit to one after
modifiers. This is preserved even with high Attack, Technician, Choice Band,
critical hits, STAB and resistance; Ghost immunity is not bypassed. AI's
separate post-roll path uses that same floor. Do not substitute vanilla Gen2's
zero-damage behavior merely because the source is a GBC game.

Only `MOVE_RETURN` receives this behavior. Pika Papow and Veevee Volley share
Expansion's effect handler but keep their own native scaling. Frustration is
absent from the pinned source and remains optional Expansion content, not a
missing source move. The native profile, fixed-power overrides and Z/Max move
power paths remain available. Source trainer parties with custom Return moves
will also need their explicit 160 happiness imported when those parties are
ported; the currently imported parties contain no Return. General trainer
creation/AI, other move data, source damage modifiers and rounding, effects,
animations and presentation are still incomplete. This phase establishes
Return's happiness-power rule and zero-power floor, not complete battle parity.

Seven new native mGBA tests cover all 256 happiness values on both sides in
both profiles, the 160 cap, boosted/resisted low-happiness hits, immunity,
live medicine interaction, optional moves/fixed power and actual AI move
selection between Return and Tackle under both profiles.

Validation on the final code uses GCC 13.2.1, matching CI: all 5,322 native
tests pass, with nine expected failures and 586 existing TODOs (5,917 total),
including all six AI timing checks. Both standard Emerald and Johto production
ROMs build, all 35 source/asset checks pass, and all 12 Johto demo emulator
checks pass. The tested Johto ROM SHA-256 is
`545d0b2f6a66cb55dfedf85fc77441bbfced99a6cbd857cc99ba2836338ff113`.
The seven native tests execute Return behavior in mGBA; the demo checks cover
the existing overworld. Automatic CI additionally reruns the full controller
campaign on the published commit.

## Move PP data and maximum PP

`B_POLISHED_MOVE_PP` defaults to `POLISHED_JOHTO_DEMO`. A generated sparse
table maps all 255 pinned default move records to stable Expansion move IDs.
`GetMovePP` now selects those values globally: initial moves, learning, trainer
construction, battle/summary displays and PP consumers share the source data.
For example, Hone Claws has 40 PP, Rock Blast 25, Metronome 20, Rapid Spin 20
and Take Down 15. Native presets previously supplied 15, 10, 10, 40 and 20
respectively. Unmapped Expansion moves, Z/Max moves, invalid IDs and the
disabled source profile retain their native/sanitized behavior.

`CalculatePPWithBonus` uses the source's per-PP-Up increment for mapped moves:
`base + min(floor(base / 5), 7) * count`. A source 40-PP move therefore has
40/47/54/61 PP with zero/one/two/three PP Ups. The 61 limit follows the source
behavior without importing its packed six-bit storage: Expansion's separate
current-PP bytes and bonus fields, move IDs and extra capacity are unchanged.
An optional 40-PP Leafage still reaches 64 PP. Existing saved current PP is
not rewritten on load; subsequent ordinary healing/refill uses the selected
maximum, while new/replaced moves start with their source base PP.

`build_move_pp.py --check` verifies the existing hashed move constants, default
move table and item-effect fixture, including PP Up/Max counts, Sketch's
exclusion, maximum lookup and the original 7-point increment cap. It generates
the table, constants and a provenance manifest with all four maximum values
per source move. Party/Box refill and PP restoration item paths already use
the shared accessors; no duplicate item formula was introduced. PP Up/Max
preserve spent PP by adding only the increase in maximum PP. The native
one-PP guard also excludes Sketch; source Struggle is not a learnable move.
Positive friendship changes from these items, their UI/text/animations,
broader PP-use effects and battle event ordering are not certified by this
data/maximum phase.

The source has a noteworthy identity alias: in its default build, constant
`ROCK_SMASH` (source ID 249) is named `BRICK_BREAK` in the move table. This
PP table explicitly maps that record to `MOVE_BRICK_BREAK`, not the native
Rock Smash effect. Earlier learnset/teaching imports resolved some
`ROCK_SMASH` tokens by name. The move-identity correction below now resolves
those references through the source table too. Both moves have 15 PP, so the
correction does not change the PP table.

Nine new native tests cover all records and profiles, all four bonus counts
in every slot, all 256 packed bonus bytes for a 40-PP move, invalid/optional
IDs, move initialization and depletion, party/Box refill, PP Up/Max spent-PP
preservation and no-effect guards, Ether/Elixir limits, both battle sides,
benched restoration followed by switching and the targeted doubles partner.
The battle-test item selector now resolves the requested move on the selected
recipient, rather than the item user's active Pokemon. This fixes test input
generation for Ether used on a bench member or partner with different moves;
the production item-selection UI is unchanged.

Final validation uses GCC 13.2.1, matching CI. All 5,331 native tests pass,
with nine expected failures and 586 existing TODOs (5,926 total), including
all six AI timing checks. Both standard Emerald and Johto ROMs build; all
36 source/asset checks and all 12 Johto demo emulator checks pass. The tested
Johto ROM SHA-256 is
`6a5668b200b0235f7a7c3f927da6c361deb7aee85d58a45649543fd37589397c`.
The nine new tests execute the PP behavior in mGBA. The local demo checks
cover existing overworld behavior; automatic CI additionally reruns the full
controller campaign on the published commit.

## Standalone emulator compiler repair

CI #1530 on `66a7091e` passed all 5,331 native tests and both production
ROM builds, but failed before the evolution and New Bark controller replays
could execute. Their standalone ARM compiler commands do not use Make's
`-DPOLISHED_JOHTO_DEMO` option. The new shared PP accessor exposed that
missing definition as a C compilation error.

`config/general.h` now supplies Make's native default only when the option
is absent. Explicit native and Johto selections remain intact. Both affected
CI jobs generate the normal headers and compile all nine emulator layout
helpers before building ROMs or running the long test suite. The preflight
also checks omitted/zero/one profile values and reports compiler stderr.
The test job installs Pillow for the campaign helpers' imports.

The preflight passes from a fresh checkout with GCC 13.2.1. Both real
evolution replays pass, including completion/cancellation and persistent
form-pattern checks. Rebuilding standard Emerald and Johto produces exactly
the same ROM bytes as `66a7091e`; this repair changes neither game rules nor
saved data. The full campaign replay remains a required automatic CI check.
CI #1531 on `202f2f1b` subsequently passed every required job, including
the full campaign and evolution replays.

## Default source move identities

The shared importer now resolves all 255 moves and the empty-move sentinel
through the pinned default constant IDs, aliases and move-table rows before
mapping them to Expansion IDs. `move_identities.json` records that contract;
the existing learnset regeneration check verifies it in CI. Unknown source
symbols fail lookup instead of silently selecting a same-named native move.
Level-up/evolution moves, egg moves, teaching lists, move-based evolution
conditions and PP generation share this resolver.

Source ID 249 has constant `ROCK_SMASH`, alias `BRICK_BREAK`, and default move
table entry Brick Break. Both source spellings now resolve to stable target
`MOVE_BRICK_BREAK` (280). This corrects Cubone at 41, Marowak and Alolan
Marowak at 49, Hitmonlee at 21, Pinsir at 26, Sirfetch'd and Galarian Farfetch'd
at 40, and Galarian Zapdos at 43. Source TM31 now supplies Brick Break
compatibility to its 207 eligible species/forms. All other learnset entries,
egg lists, evolution conditions and PP records regenerate unchanged.

Target `MOVE_ROCK_SMASH` (249) remains a distinct optional native move. No
target IDs or save layouts are changed, and existing saved moves are not
rewritten. The four native test cases cover creation and learning at all
eight source levels, all 391 source compatibility lists, native fallback,
and encrypted party/PC save reload with both target moves present. Broader
move effects and source TM inventory/numbering, tutor placement and field
Rock Smash behavior remain separate work.

At the end of the move-identity phase, the four breakable rocks in
`PolishedDarkCaveVioletEntrance` still called native `EventScript_RockSmash`.
The field adapter below now uses the default source identity (Brick Break)
and source rules. The rescue campaign alone did not certify these rocks;
dedicated controller fixtures cover their interaction branches.

Validation with GCC 13.2.1: all 5,335 native tests pass (nine expected failures
and 586 existing TODOs; 5,930 total), including the four new tests and all six
AI timing checks. All 36 source/asset checks, both production ROM builds,
all 12 Johto demo checks, and the full controller campaign through Falkner
and the post-battle rewards pass. The tested Johto ROM SHA-256 is
`8d86d6d044b4eefc792cc17de2175b0fe20c97adb023a90f1bd155d106eea848`.

## Live Dark Cave rock interactions

The four original rocks now use the pinned default-source Brick Break routine.
Physical party order is preserved: a non-Egg knower anywhere in the party wins
before the fallback scan for a source-compatible Pokemon while TM31 is owned.
Fainted Pokemon qualify; no badge is required; the TM is neither taught nor
consumed. The party menu offers the move only to a knower and uses that selected
Pokemon without a Yes/No question. Native-profile Rock Smash remains available.

The first accepted field interaction displays the source prompt, nickname/move
text, Pokemon picture and cry. Later rocks in the same visit recheck eligibility
and skip the prompt/picture. The source 40-frame, two-pixel vertical earthquake
and ten-frame rock blink precede disappearance. Existing `FLAG_TEMP_1` through
`FLAG_TEMP_4` record the broken rocks; `VAR_TEMP_5` records repeat-use state.
Warp/connection loading clears them; battle reload and Save/Continue retain them.

A rejection-sampled byte range gives the source 4-in-10 battle gate. Encounters
are level-15 Krabby/Geodude/Shuckle at 70/25/5, independent of Repel, badges or
native rock encounter modifiers. A battle uses ordinary wild capture/escape
rules and resumes the event script on return. Only a no-battle result rolls the
source 256-weight item table. Source receipt/full-pocket text is used, and full
inventory does not restore the rock. Brick Piece is appended as item 883 with
its source name, plural, price, description, 24x24 pixels and RGB555 palette.
Existing item/save IDs remain unchanged.

Original assembly, text, animation and item-art files are hash-checked against
the established source pin. `build_rock_smash.py` regenerates tables, text, icon
and manifest. Source contract checks and native tests cover eligibility/order,
all 207 compatible species, every encounter/item roll, random-call ordering and
map-visit reset behavior. Controller fixtures link unchanged production Johto
objects to a separate test boot module; supplied test parties are explicitly
fixture state, not proof of obtaining TM31 in the current campaign. These checks
are separate from the ordinary campaign replay.

Validation: 5,341 native tests pass, with nine expected failures and 586
existing TODOs. All 38 source checks, ten standalone ARM probes, both production
builds, twelve demo checks and the campaign regression through Falkner/Togepi
pass. The eight dedicated rock scenarios cover all four objects, cancellation,
party-menu use, TM-only access, rejection, automatic use with battle return,
full inventory, Save/Continue and reentry. Reentry uses a separately documented
fixture at the exit because the eastern rocks are separated from it by source
terrain; it does not claim a naturally obtained Surf route. Brick Piece's live
24x24 pixels, transparent 32x32 padding and original RGB555 colors are read back
from OBJ VRAM/palette memory. Full-pocket failure leaves inventory unchanged.
Final rock fixtures validate Johto ROM SHA-256
`0e5c8021101ac9f62e74cee6a4a3cff793f83ce4f80daa0ce5203510e9f88991`;
the broader campaign regression used the preceding build before the dedicated
item-icon renderer was added. CI reruns the complete pipeline on publication.

Remaining presentation/engine gaps are explicit: Pokemon pictures and cries,
Strength/item audio and message-box styling still use native assets/UI;
existing mapped item properties and broader wild creation/battle behavior retain
their previously recorded gaps. This phase does not enable dormant Headbutt or
claim source parity for unrelated field moves, additional maps or Prism mining.

## Dark Cave Flash field rules

The registered Dark Cave map now offers Flash as the A-button fallback after
object/background/metatile/Surf interactions. The pinned `CheckPartyMove` order
selects the first non-Egg knower in physical party order, then the first
source-compatible non-Egg when the Flash machine is owned. HP, PP and badges
are not gates. Declining leaves the cave dark; accepting does not teach Flash,
spend PP, or consume the item. The party menu requires the selected Pokemon to
know Flash and skips the Yes/No prompt. All 391 source teaching tables remain
authoritative; optional native species do not gain source TM compatibility.

The field script shows the Pokemon, presents the source Flash message, fades to
white and lights the entire map. Emerald's small lit circle is removed. The
existing Flash flag and flash-level save fields persist through menus, battles
and Save/Continue; entering outdoors resets them. Loading an older demo save
with Flash already active normalizes its native radius to full brightness on
this registered source map. Map, item, species and save IDs remain unchanged.
Elder Li's explanatory dialogue now comes directly from his original inline
text, including pressing A without teaching the move.

The subsequent TM70 identity suite replaces the HM05 reward adapter and
Flash's HM forgetting restriction. Full source machine inventory remains
incomplete; old HM05 saves retain their legacy item and field access. Pre-use darkness, Pokemon pictures/cries, the Flash sound,
message styling and exact fade timing also retain native presentation. The
Aerodactyl chamber exception is explicitly deferred with its unported map.
This phase does not enable Headbutt or certify other field moves.

`build_flash.py --check` verifies pinned assembly, input priority, reset rules
and generated dialogue. Four native tests cover party order, zero HP/PP, Eggs,
all 391 species/form compatibility tables, map/lit-state gates and no mutation of
party, items, badges or RNG during selection. `test_flash_emulator.py` uses
separate boot fixtures linked to production objects; subsequent actions use
normal controller input. The entrance/reentry and rock/battle fixtures are
separate because source terrain prevents walking between those areas without
additional traversal. Fixture parties/items are not campaign acquisition proof.

Validation with GCC 13.2.1: both production ROMs build and all 5,345 native
tests pass (nine expected failures and 586 existing TODOs; 5,940 total).
All 39 source/asset checks, eleven standalone ARM layout probes, twelve demo
checks and seven dedicated Flash scenarios pass. The dedicated checks use
Johto ROM SHA-256
`d19ed9b9b950810fe55d7d8c952cab754136ee316b7369e1d78202bcc5788d1d`.
The same final ROM passes all 100 campaign checks and 19 warp traversals,
including the Sprout reward, Dark Cave rescue, Falkner, Roost and Togepi Egg.
These remain demo-start campaign regressions until the authentic opening is ported.

## Source TM70 identity and teaching

TM70 Flash now occupies the existing target item ID 651, with source number,
move description, original 24x24 pixels and Normal-type RGB555 colors. Elder Li
awards it once; his original TM wording is restored. Machine lookup entries,
display numbers and HM classification are independent, so the sparse entry
neither renumbers native machines nor inherits HM presentation. Existing
HM05/save ID 686 is retained and still grants field Flash access. There is no
save or bag-capacity change. Move-relearner machine candidates are deduplicated
when a legacy and migrated item teach the same move.

Successful teaching uses the imported compatibility lists and source lesson
friendship: +1/+1/+0 at the 100/160 thresholds, +1 each for Soothe Bell and
Luxury Ball, saturation at 255, and no met-location bonus. Source bonuses apply
even when the base gain is zero. Eggs, incompatible/already-knowing Pokemon
and cancellation do not change Pokemon or inventory. Flash is ordinarily
forgettable in this profile; native builds keep their existing HM behavior.

The complete pinned Bag call chain clears LEARNING_TM before LearnMove, making
its PP-conservation branch inactive for ordinary TM teaching. Empty and
replacement slots therefore receive full base PP, with replacement PP Ups
cleared, as in the actual source flow. The importer records this distinction
rather than implementing the dormant comment as a new rule.

Fixtures are git-blob-verified against the existing source pin. Deterministic
generation covers the item, icon, palette, description and friendship table.
Native tests cover all 1,024 friendship/bonus combinations, Egg/empty guards
and stable native machine mappings. Dedicated emulator fixtures are linked to
unchanged production Johto objects and use controller-only input after boot;
they verify learning, reuse, refusal/cancellation, ordinary forgetting, save
persistence and live original icon pixels/colors. They are separate from the
campaign regression proving the actual Elder reward.

Remaining gaps: full source TM/HM/tutor inventory and machine-owned-bit storage,
Bag/party/learning UI, text, sounds and animation. The reusable nontradable bag
item is an explicit storage adapter. Older HM05 items keep native presentation
and lesson friendship; no legacy inventory is rewritten. Other move data and
battle mechanics retain the existing audit boundaries.

Validation with GCC 13.2.1: both production ROMs build; 5,348 native tests pass
(nine expected failures and 586 existing TODOs; 5,943 total). All 40 source/asset
checks, twelve standalone ARM probes, twelve demo checks, eight Flash field
fixtures and six TM70 teaching fixtures pass. The same final Johto ROM passes
the complete 100-check campaign and 19 warp traversals, including Elder Li's
TM70 reward, Falkner and Togepi. ROM SHA-256:
`b1b96ab1c4c6150d2e557bebdab93137edd5ffedb795ea9990dfd333fe8f10a1`.
The campaign still uses the documented demo start, not the authentic opening.

## Source TM31 / TM51 identity and teaching

Source TM31 Brick Break and TM51 Roost now retain their existing target/save
IDs 612 and 632 while importing source descriptions, original 24x24 machine
pixels and Fighting/Flying RGB555 palettes. The pinned default release maps
ROCK_SMASH to Brick Break. Falkner's actual `verbosegivetmhm TM_ROOST` awards
TM51; the older source event label `EVENT_GOT_TM31_ROOST` is not a number map.

Both items use the shared source TM lesson path introduced for TM70: reusable
ownership, source compatibility, +1/+1/+0 friendship with additive Soothe Bell
and Luxury Ball bonuses, full source base PP and replacement-slot PP Up reset.
Roost has five base PP; Brick Break has fifteen. The common icon/friendship
code now has generic names. Source rules apply only to these three migrated
machines in the Johto profile; native items and legacy HM05 retain their rules.

`build_source_tms.py` reuses hash-verified pinned machine, move, palette and
Violet Gym fixtures, with an additional pinned type-order fixture. Native tests
cover the exact three-item scope and stable IDs/categories, alongside the full
friendship matrix. Fourteen controller-only TM31/TM51 fixtures link production
Johto objects and check empty/replacement lessons, source PP, ordinary forgetting,
reuse, incompatible/known-move/Egg rejection, cancellation, refusal to give,
Save/Continue, ROM item prose/price and live icon pixels/palettes. The campaign
checks the source item/art on Falkner's actual one-time reward separately.

Remaining gaps: TM31 acquisition beyond the currently imported campaign,
remaining source machines/tutors and source owned-bit storage, Bag/party/lesson
layout, dialogue/audio/animation, and separately audited move battle effects.
Fixture parties and machine ownership do not establish campaign acquisition.

Validation with GCC 13.2.1: both production ROMs build; all 5,349 native tests
pass (nine expected failures and 586 existing TODOs; 5,944 total). All 41
source/asset checks, twelve ARM layout probes, twelve demo checks, eight rock
fixtures, eight Flash fixtures, six TM70 fixtures and fourteen TM31/TM51
fixtures pass. Final Johto ROM SHA-256:
`cb159439b932cc6cb0cb29e2d8f7e11e074d6a950dca0e4577e0fc7f2f376fd5`.
The same final ROM passes all 101 campaign checks and 19 warp traversals,
including Elder Li, Falkner's original TM51 artwork/definition and Togepi.
These remain demo-start regressions until the authentic source opening is ported.

## Source move-balance corrections

`build_move_balance.py` resolves the pinned default release and generates source
constants for twelve ordinary rows: Cut, Fly, Take Down, Sing, Supersonic,
Strength, Fire Spin, Dig, Scald, Crabhammer, Octazooka and Whirlpool. Seventeen
changed fields are selected directly in the ROM's `gMovesInfo` initializers.
`B_POLISHED_MOVE_BALANCE` defaults to the Johto build; ordinary/native builds
retain their existing values. No save fields or persistent IDs change.

| Move | Imported power | Imported accuracy | Imported type |
| --- | --- | --- | --- |
| Cut | 60 | 100 | Steel |
| Fly | 90 | 100 | Flying |
| Take Down | 90 | 100 | Normal |
| Sing | 0 | 75 | Normal |
| Supersonic | 0 | 65 | Normal |
| Strength | 80 | 100 | Fighting |
| Fire Spin | 40 | 90 | Fire |
| Dig | 90 | 100 | Ground |
| Scald | 70 | 100 | Water |
| Crabhammer | 100 | 95 | Water |
| Octazooka | 75 | 95 | Water |
| Whirlpool | 40 | 90 | Water |

The existing shared getters read the selected ROM values for actual damage,
hit checks, AI estimates, summary screens and machine information. Matching
native categories and other fields are retained and checked against the source.
Compile-time selection adds no per-lookup branch or secondary table read. All
six existing AI timing benchmarks run with their original ceilings in both the
native test ROM and a separate source-balance test ROM. The latter uses
`TEST=1 POLISHED_MOVE_BALANCE_TEST=1`, a separate object directory and ELF, so
other Johto profiles cannot mask balance regressions or contaminate native tests.

The original move effect implementations and flags are retained: this completes
these four data fields for twelve moves, not every effect's source behavior.
Remaining source gaps include secondary chances, priority/flags, source accuracy
RNG and rounding/modifiers, damage rules and animations. Ten other differing rows
remain for separate review; fixed damage and typeless/bypass sentinels must not
be mechanically copied into native storage. Broader machine identity/lesson gaps
remain unchanged.

Six test groups run in both ROM profiles to verify the twelve imported rows,
ordinary and two-turn damage, all nine changed accuracy thresholds, Steel Cut /
Fighting Strength effectiveness, Cut versus Ghosts and AI move choice.
The threshold tests exercise the existing engine RNG and do not certify source
byte-RNG/ability/item modifier parity. All 255 moves are additionally inspected
through production-linked Johto accessors; the twelve corrected rows match,
and all 43,792 teaching combinations still match. The known-gap inventory now
rejects all seventeen repaired field differences if they reappear; injected
regressions verify that gate.

Validation: both Emerald and Johto ROMs build with unused/deprecated
warnings treated as errors. All 42 source checks and all 12 emulator ARM probes
pass, including native/Johto/isolated-balance profile selection. All six
move-balance groups pass in both separate ROM profiles, as do all six unchanged
AI timing benchmarks. All 12 demo checks pass. Production-linked inspection
confirms the twelve corrected rows, 255 PP rows and dispatch bindings, 43,792
teaching combinations and rejection of all seventeen injected old differences.
The full native run passed 5,354 groups, with nine existing expected failures and
586 TODOs. Its sole new failure was the accuracy test counting Fly's charging
animation as a hit. The assertion now also requires second-turn damage; all six
balance groups pass when rerun in both profiles. No gameplay or frame-budget
failure remains from that run. CI repeats the full suite on the final commit.
The production campaign passes all 101 checks and 19 warps, including all three
starter paths, Sprout Tower, Falkner, the badge/reward/Togepi sequence and
Save/Continue.

Johto ROM SHA-256:
`af3fd547a7c6a3f52ab5499427aa4eaddfc05303c4bf85429a0364707625e0d0`.
The local audit identifies this as a working tree based on `3fdd1511`; the
unchanged clean-source guard remains enabled for CI's exact-commit report.

## Source Hidden Power

The pinned default build uses fixed power 70 and `UNKNOWN_T` in the move table.
The port maps that listed type to `TYPE_MYSTERY` and imports the six-DV dynamic
selection in `GetDynamicMoveType`, shared by battle damage, AI and optional
dynamic display. The source type constants and routines are pinned fixtures;
`build_hidden_power.py --check` validates them and all generated outputs.

The adapter reads each source DV as `IV >> 1`, matching the existing trainer
conversion `IV = DV * 2`. No saved IVs, identifier widths, native stat formulas
or save layouts are rewritten. Source parity bits pack HP, Attack, Defense,
Speed, SpAttack and SpDefense in that order. Fighting through Dragon receive
four patterns each, Dark receives three and Fairy receives the all-odd-DV
pattern. This is the default source distribution, not the native sixteen-type
formula extended by one type.

Source Anticipation reads Hidden Power's listed unknown type rather than its
calculated attack type. The optional display helper now reads party abilities
and items outside battle without accessing an absent battle allocation. The
baseline's static-type display setting is unchanged. Native Expansion retains
its own configured Hidden Power rules and wider engine capacities.

Eight groups in each profile cover fixed data, all 4,096 relevant IV parity
combinations, all 64 DV patterns in 128 real battle cases, Fairy effectiveness,
Ghost immunity, AI choice, Anticipation and display without battle allocation.
The known-gap gate no longer accepts the old Hidden Power power/type values.
General damage rounding/modifiers, accuracy RNG, broader abilities/AI, DV/stat
generation, source animation and the Hidden Power guru/UI remain separate work.

Validation with GCC 13.2.1: both production ROMs build; all 43 source checks,
12 ARM profile/header probes and 12 demo checks pass. All eight Hidden Power
groups pass in both ROM profiles. The source profile also passes the six prior
move-balance groups and all six unchanged AI timing benchmarks. The full native
suite passes 5,363 groups, with nine existing expected failures and 586 TODOs
(5,958 total); no unexpected failures. Production-linked inspection confirms
all 255 move dispatch bindings and PP rows, no mismatch in 43,792 teaching
combinations, and nine remaining differing move rows. Both injected old Hidden
Power field values are rejected by the known-gap gate.

The same production Johto ROM passes all 101 campaign checks and 19 warp
traversals, including three starters, Sprout Tower, Falkner, badge/reward/Togepi
flow and Save/Continue. The campaign still uses the documented demo opening;
this does not certify the source startup or the rest of the unfinished game.

Johto ROM SHA-256:
`7f29127eebbe6fdc0405db2cec44c7ff0d3f7cbe0f05d9b1cd5fd8853913d371`.

## Source Pain Split

The default source lists Pain Split as Ghost, with zero power, 20 PP and
perfect accuracy. `B_POLISHED_PAIN_SPLIT` selects the source row and behavior
for Johto and the isolated source-move test profile. Stable move/save IDs and
Expansion's optional content remain available.

The pinned effect uses the HP difference: subtract its rounded-up half from
the healthier battler first, then restore its rounded-down half to the other,
capped at that battler's maximum HP. The existing GBA integer-average result
is equivalent and retained; a source branch corrects the HP update order when
the target is healthier. Both active battle HP and party HP are checked.

Source Pain Split bypasses Substitute and Protect. This follows the pinned
script literally: `checkhit` records a failure flag, but this effect does not
read that flag or run `failuretext` before redistributing HP. Its own explicit
Fly/Dig guard still fails even if No Guard or Lock-On bypasses the earlier hit
check. The imported bypass flags and shared semi-invulnerability helper expose
these target rules to the engine and AI. Normal/Ghost damage immunities do not
block HP sharing; Prankster's Dark immunity remains enforced separately.

`build_pain_split.py --check` verifies the original command interpreter, effect
script, effect routine and data. Its independent difference-based reference
checks all 65,025 positive byte-sized HP pairs and generates 22 boundary/cap
fixtures. Tests exercise those fixtures with either side using the move, exact
HP update order, party synchronization and one-PP consumption. Additional
groups cover Substitute, Protect, Fly/Dig with No Guard or Lock-On, type
immunities, accuracy/evasion, Prankster and Counter/Mirror Coat.

Native move animation/dialogue (including the hidden-target failure message),
general accuracy RNG and ability/item interaction timing, source smart-AI
scoring and original UI remain separately audited work. Optional Expansion-only
gimmicks retain their native adapters. This suite certifies the listed core
redistribution and target rules, not every possible interaction.

Validation with GCC 13.2.1: both production ROMs build. All 44 source checks,
12 ARM profile/header probes and 12 demo checks pass. All ten Pain Split groups
pass in both native and isolated source profiles, including 44 boundary cases
with either side using the move. The source profile also passes all eight
Hidden Power groups, six prior move-balance groups and six unchanged AI timing
benchmarks. Production-linked inspection confirms Pain Split's Ghost type and
both bypass flags, all 255 dispatch/PP rows and 43,792 teaching combinations
without a compatibility mismatch. Eight move rows still differ; there are no
new gaps. The known-gap gate rejects all three simulated old Pain Split field
values.

The full native run passes 5,373 groups, with nine existing expected failures
and 585 TODOs (5,967 total), and no unexpected failures. The old Pain Split
placeholder TODO has been replaced with the ten implemented test groups.

The same production Johto ROM passes all 101 campaign checks and 19 warp
traversals, including all three starter paths, Sprout Tower, Falkner, the
badge/reward/Togepi sequence and Save/Continue. The documented demo opening
remains in use; the authentic source startup is still separate work.

Johto ROM SHA-256:
`8326d112000c1acd11972bb535b4f7973f39c448eb0eb31a80325bfcbbd2ca2c`.

## Source Sonic Boom and Dragon Rage

Pinned default-source rows encode static damage directly as power 20/40.
Their effect script runs `constantdamage` and `resettypematchup` without normal
damage calculation, variation or critical-hit commands. The command reads the
move power directly; type immunities still fail, while nonzero effectiveness
is reset to neutral. Source summary and move-menu routines print these values.
The generator checks these pinned routines and imports the damage constants.

The target retains `EFFECT_FIXED_HP_DAMAGE`, runtime power 1 and an explicit
20/40 `argument.fixedDamage` payload. This preserves Expansion's sentinel-based
runtime behavior and its shared battle/AI fixed-damage calculation.
`GetMoveDisplayPower` supplies source numbers in five presentation paths while
battle/AI power accessors retain their original meaning. Both profile choices
are compile-time selected, and stable IDs/save storage remain unchanged.

The new test groups exercise the data adapter; levels 5/50/100, stat extremes,
STAB and neutral/resisted/super-effective matchups; Ghost/Fairy immunity;
Technician, Choice Specs, Multiscale and Light Screen; Protect; Substitute;
HP immediately below/at/above the KO threshold; AI damage selection and immune
target avoidance; and Fly/Dig with and without No Guard. The production audit
accepts only the explicit effect/sentinel/payload combination and source display
value. It keeps the raw power in its report instead of hiding that storage
representation. The two old power mismatches are removed from the known-gap
inventory.

A separate test boot links unchanged production objects and opens the real
summary with Sonic Boom, Dragon Rage, Absorb and Water Gun. Controller input
selects each move; cropped rendered power glyphs must match ordinary 20/40
references in Johto, and retain native dashes in the native profile. Eight
screenshots and profile/ROM hashes are retained. This fixture is not included
in either shipping ROM. Other UI call sites are wired to the tested accessor,
but do not yet have their own controller replay.

Source accuracy RNG, general item/ability interaction timing, smart-AI scoring,
original animations and source UI art/layout remain separate gaps. These
checks certify the listed adapter and fixed-HP scenarios, not full battle parity.

Validation with GCC 13.2.1: both production ROMs build; all 45 source/asset
checks, 12 ARM profile/header probes and 12 demo checks pass. All nine new test
groups pass in both native and isolated source profiles. The source profile
also passes eight Hidden Power groups, ten Pain Split groups, six move-balance
groups and six unchanged AI timing benchmarks. The live summary comparison
passes for both profiles, with eight captured move selections.

Production-linked inspection confirms both fixed-HP adapters, all 255 PP/PP Up
and dispatch rows, and 43,792 teaching combinations with no compatibility
mismatch. Six basic move-data rows still differ, with 115 documented gap entries
and no new findings. The gate rejects all eight injected sentinel, effect,
payload and display regressions across the two moves. Machine identities and
lesson coverage are unchanged by this suite.

The full native run passes 5,382 groups, with nine existing expected failures
and 585 TODOs (5,976 total), and no unexpected failures.

The same production Johto ROM passes all 101 campaign checks and 19 warp
traversals, including three starter paths, Sprout Tower, Falkner, the badge/
reward/Togepi sequence, resident schedules and Save/Continue. The documented
demo opening remains; authentic source startup is still separate work.

Johto ROM SHA-256:
`4dc414e11889acb3ccac2bc4b4cb37561afd4fecc6a3c94dbd1a3d3f9b70a146`.

## Next priorities

1. Continue the source field-rule audit. Headbutt remains dormant; other field
   moves and ordinary encounters still need their source gates and modifiers.
2. Complete the remaining evolution options above, broader item definitions/
   effects and source TM/tutor inventories. Complete breeding/hatching behavior
   beyond base data and inherited moves.
3. Import remaining move data and implement a source battle profile, including
   trainer creation and AI. Validate source probabilities and rounding directly.
4. Import Pokemon graphics and complete the associated sprite metadata.
5. Establish the real source startup and rerun the existing campaign from its
   authentic initial state; continue correcting field/UI/audio gaps and the
   remaining campaign with the same source-based completion criteria.

Expansion's extra content and capacities remain available for later hacking.
Unused optional content is not a parity defect; using it in place of Polished
Crystal data or behavior in the playable baseline is.
