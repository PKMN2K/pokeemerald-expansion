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
move rows. The subsequent Counter/Mirror Coat accuracy suite resolves two more,
leaving four basic-data rows. The subsequent Dragon Dance/Sleep Talk listed-
accuracy suite resolves two more. The subsequent typeless-move suite imports
the Metronome and Struggle type rows, completing the currently audited basic
move fields (power with sentinel adapters, type, accuracy, PP/PP Up and category).
Secondary-effect probabilities, move effects, flags, candidate pools, battle
rules and source presentation are not certified by that data audit.
Machine identity/lesson gaps remain as recorded in the original report.

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

## Source Counter and Mirror Coat accuracy

The pinned default rows encode accuracy -1 for Counter and Mirror Coat. The
shared source hit-check routine returns for EFFECT_COUNTER before the ordinary
accuracy/evasion calculation; the source summary and field move menus print
three dashes for these rows. Target accuracy 0 is the explicit storage adapter.
`B_POLISHED_RETALIATION_ACCURACY` selects it only for the Johto/source-test
profiles, retaining native 100 accuracy and the existing shared reflect-damage
runtime. There is no per-call lookup or new runtime branch.

The deterministic importer pins and verifies the source counter command, move
rows, hit check and presentation contract. Its manifest limits completion to the
accuracy correction and ordinary singles regression scenarios. Full source
retaliation parity, including multi-hit/Parental Bond, saturation, damage-state
bookkeeping, unusual target/protection interactions and AI scoring, is still open.
Source move animations, UI art/layout and general damage/item/ability rules also
remain separate work. Doubles and Expansion-only mechanics keep native adapters.

Seven test groups cover source/native data identity, forced miss rolls under
accuracy/evasion/item modifiers, double damage on either side across neutral,
resisted and super-effective matchups, category/turn requirements, type immunity,
Substitute and stale damage. The source cases must succeed where the native
accuracy roll is forced to fail. A separate controller fixture links production
objects and compares rendered accuracy with Swift/Water Gun references in both
profiles; fixture parties never enter the shipping ROMs.

Validation with GCC 13.2.1: both production ROMs build; all 46 source/asset
checks, 12 ARM header/profile probes and 12 demo checks pass. All seven new
groups pass in both native and isolated source profiles. The source profile
also passes six move-balance, eight Hidden Power, ten Pain Split and nine
fixed-damage groups, plus all six unchanged AI timing benchmarks. Both summary
profiles pass the rendered comparison, with eight move selections captured.

The production audit checks all 255 move PP/PP Up and dispatch rows and all
43,792 teaching combinations, with no compatibility mismatch. Four basic-data
rows and 113 documented gap entries remain, with no new findings. Reintroducing
either old 100-accuracy value is rejected by the known-gap gate. Machine identity
and lesson-coverage gaps are unchanged.

The full native run passes 5,389 groups, with nine existing expected failures
and 585 TODOs (5,983 total), and no unexpected failures. The same production
Johto ROM passes all 101 campaign checks and 19 directed warp traversals,
including all three starter paths, Sprout Tower, Falkner, the badge/Roost/Togepi
rewards, resident schedules and Save/Continue. This remains the documented demo
campaign; the authentic source opening is separate work.

Johto ROM SHA-256:
`a71c38d5e376d0f6756f4bb9da8bdaed4c4ec6ec8449bbe51f4b23dddbb24d84`.

## Source Dragon Dance and Sleep Talk listed accuracy

The pinned default move rows list accuracy 100 for Dragon Dance and Sleep Talk.
Their source effect scripts contain no checkhit command: Dragon Dance performs
its two stat increases directly, while Sleep Talk loads the chosen move's data
and restarts execution. The source summary and field move menus print the
numeric table value. The existing B_POLISHED_MOVE_BALANCE profile now selects
imported 100 constants for Johto and the isolated source test ROM; native
Expansion retains zero accuracy and its dash presentation.

The unchanged GBA stat-change handler skips accuracy checks for the user.
Sleep Talk dispatches its called move before hit resolution and uses that move's
hit rules. An AI adapter preserves the existing accuracy-bypass estimate for
both parent effects, instead of treating their listed 100 as a hit probability.
Target IDs, save fields and capacities are unchanged.
The importer verifies the pinned move rows, effect scripts, Sleep Talk command
and source numeric-display path. The production-linked summary fixture verifies
both changed moves with controller input against numeric/dash references in
both profiles. It does not add parties or debug code to the shipping ROMs.

Nine test groups cover the data and these ordinary singles scenarios: Dragon
Dance under accuracy drops/evasion boosts/Bright Powder, partial/full stat caps,
and opposing Protect/Fly/Dig; Sleep Talk calling Dragon Dance, zero-PP attacks,
ordinary hit/miss checks and Swift's bypass, protection/type immunity, and awake
or no-eligible-move failures; and AI estimates under extreme accuracy/evasion
stages. The AI test reproduced a 33-versus-101 estimate before the adapter; it
requires the bypass sentinel for both parents while retaining reduced accuracy
for an ordinary damaging move. This certifies the listed-accuracy correction and
the stated regression cases. The complete Sleep Talk selection pool, disabled/
encored move handling, source RNG/sleep timing/calling bookkeeping, general stat
and item/ability rules, source animations/UI and smart AI remain open.

Validation with GCC 13.2.1: both production ROMs build; all 47 source/asset
checks, 12 ARM probes and 12 demo checks pass. All nine new groups pass in both
native and source profiles. Earlier move-balance, Hidden Power, Pain Split,
fixed-damage and retaliation groups pass, along with all six unchanged AI
frame-budget checks. The AI regression fails before the correction and passes
afterward; the native shipping ROM is byte-identical across that AI-only change.
The final rendered summary comparison passes in both profiles, with eight
controller-selected move screenshots.

The production audit checks all 255 move PP/PP Up and dispatch rows and all
43,792 teaching combinations with no compatibility mismatch. Only the Metronome
and Struggle type rows remain as basic-data differences. There are 111 documented
gap entries and no new findings; restoring either old zero-accuracy row is
rejected by the gate. Machine identity and lesson-coverage gaps remain open.

The final full native run passes 5,398 groups, with nine existing expected
failures and 585 TODOs (5,992 total), and no unexpected failures. The same final
Johto ROM passes all 101 campaign checks and 19 directed warp traversals,
including three starter paths, Sprout Tower, Falkner, badge/Roost/Togepi rewards,
resident schedules and Save/Continue. This remains the documented demo campaign;
the authentic source opening is separate work.

Johto ROM SHA-256:
`f06de64f7a2afbd94f8bf8f336edc2fd18f412de3bebfef7fc7ea7dfb1a567f0`.

## Source Metronome and Struggle type entries

The pinned default build gives both Metronome and Struggle `UNKNOWN_T`, whose
source label is `???`. The source's optional FAITHFUL Metronome row remains
Normal and is deliberately not selected. The importer explicitly maps this
source type to Expansion's `TYPE_MYSTERY` through the existing compile-time
move-balance profile; native Expansion retains its Normal table entries.
Move IDs, save fields, type-table capacity and optional engine content are
unchanged. Source Metronome's 20 PP was already supplied by the PP adapter.

Struggle's source STAB handler returns before both STAB and matchup calculation.
The existing GBA runtime already treats in-battle Struggle as typeless and skips
both. Metronome's source handler loads the selected move's data and restarts
execution; the parent's unknown type does not replace the called move's type.
This suite imports the table identities and verifies those narrow contracts.
It does not certify the complete implementations of either move.

The source generator verifies the move rows, source type label, early Struggle
return, and Metronome dispatch against hash-pinned fixtures. Eight native/source
test groups cover table identity/fallback, Struggle neutral damage against Ghost,
Rock, Steel and ordinary types, absence of Normal STAB/item boosts, native
Normalize/Pixilate regression behavior, Protect without recoil, and Metronome's
called-move STAB/effectiveness, Ghost immunity, protection and Water Absorb.
The production-linked controller fixture opens the real summary and selects
Metronome, Struggle, Hidden Power and Swift. Johto's changed icons must match
Hidden Power's mystery icon and differ from Swift's Normal icon; native icons
must all remain Normal. This is a separate fixture, absent from shipping ROMs.
Original source UI artwork/layout and battle animations remain open.

The audit also identifies further work explicitly: Metronome still selects
from the native pool/RNG instead of the source roster, exception list and
already-known-move exclusions. Source Struggle recoil floors quarter maximum
HP (minimum one), while native modern recoil rounds. Recoil, damage rounding,
PP-exhaustion selection, move bookkeeping, Conversion/Conversion 2, general
ability/item/weather rules and AI scoring remain separate source audits.
Tests of optional native abilities do not assert those abilities belong to the
source roster. No data match or passing regression closes these broader gaps.

Validation with Arm GNU GCC 13.2.1: both production ROM builds, all 48 source
checks, 12 ARM layout probes, 12 demo checks and both rendered summary profiles
pass. All eight new test groups pass in both native and source profiles. The
source profile also passes all six AI frame budgets and the earlier move-balance
(6), Hidden Power (8), Pain Split (10), fixed-damage (9), retaliation (7), and
self-move accuracy (9) groups. Existing native Metronome regressions pass all
10 groups; Struggle passes five with its one pre-existing TODO. The full native
suite remains part of CI; it was not rerun locally for this table-only change.

The production-linked audit reports zero differences in its audited basic
fields across 255 source moves, 255 matching PP/PP Up records, valid dispatch
bindings, and 43,792 matching teaching combinations. Its 109 machine identity/
lesson entries remain, with no new findings. Reintroducing either removed
Normal-type discrepancy is rejected by the known-gap gate. These results do
not certify secondary-effect chances or full move behavior. The audit, icon
comparison and demo checks all use the same production Johto ROM.

The same Johto ROM passes all 101 campaign checks and 19 directed warp
traversals, including all three starter branches, seven Sprout Tower battles,
Falkner, the badge/Roost/Togepi rewards, resident schedules and Save/reset/
Continue. These are checks of the documented demo campaign; the authentic
source opening is still a separate migration task.

Johto ROM SHA-256:
`b51696127e0c5b1b8a93cc4cf0a4c25fbee72f4c45320ca5243ffd94b5fb55f7`.

## Source Metronome selection and Struggle recoil suite

The default-source Metronome handler draws a byte, rejects its exception table
and each of the user's four current battle moves, then reloads the chosen move
and restarts execution. This suite imports that draw domain, rejection rules
and identity mapping under `B_POLISHED_MOVE_BALANCE`. All 256 source values are
mapped explicitly: 12 are excluded and 244 remain before known-slot rejection.
The default-source alias at 249 is Brick Break, not the old Rock Smash constant
name; Fresh Snack, Fury Strikes and HealingLight use their dedicated target IDs.
Known-slot checks include zero PP and transformed moves. Expansion's optional
move tables, native Metronome selection and stable save IDs are retained.

The pinned recoil command branches to Struggle before its Rock Head/Magic Guard
checks and calls GetQuarterMaxHP/FloorBC. The imported amount is therefore
`max(1, floor(maximum HP / 4))`. The source profile now bypasses native modern
rounding specifically for Struggle; other users of the recoil handler retain
their configured behavior. Focused battles check HP residues, both sides,
abilities, target HP/knockout, substitute damage and self-fainting. Existing
typeless tests retain coverage of Protect without recoil and called-move types.

The generator verifies these contracts against hash-pinned assembly. Unit tests
sweep the complete pool, uniqueness, exclusions, aliases, all known slots and
out-of-range inputs. The independent production-linked fixture derives expected
identities from the source assembly rather than the generated C table and
checks 256 draws plus 976 eligible-move/known-slot cases. Battle tests exercise
actual dispatch, parent PP, source custom moves and transformed move slots.

The target still uses the GBA PRNG. Conditional uniform selection over the
source candidates is preserved by byte-domain rejection sampling; this does
not reproduce the source hardware/link random stream or global RNG call timing.
Each called move retains its separately documented effect/flag/probability
gaps. General damage and AI rules, broader move-calling bookkeeping,
PP-exhaustion selection, source UI/art/animations and optional Expansion-only
battle formats/gimmicks remain outside this suite. The previous type-suite
entry's unported candidate-pool and recoil-rounding gaps are superseded here.


Validation with Arm GNU GCC 13.2.1: both production ROM builds, all 49 source
checks, 12 ARM probes, 12 demo checks and all 12 new native/source test groups
pass. The production-linked adapter reports zero mismatches across 256 draw
identities and 976 known-slot checks. Earlier source move-balance (6), Hidden
Power (8), Pain Split (10), fixed-damage (9), retaliation (7), self-move accuracy
(9), typeless (8) and AI frame-budget (6) groups also pass.
The full native regression run passes 5,418 tests with the same nine expected
failures and 585 existing TODOs (6,012 total); there are no unexpected failures.

The same production Johto ROM retains zero audited basic move-field differences
across 255 source moves, matching PP/PP Up records, valid dispatch bindings and
43,792 matching teaching combinations. The 109 documented machine identity/
lesson gaps are unchanged, with no new findings. These checks do not establish
complete secondary-effect or move-behavior parity.

The same Johto ROM passes all 101 campaign checks and 19 directed warp
traversals: all three starter paths, seven Sprout Tower victories, Falkner,
the badge/Roost/Togepi rewards, resident schedules and Save/reset/Continue.
This validates the documented demo campaign; the authentic source opening
remains an independent migration task.

Johto ROM SHA-256:
`99020af9ac3f6ad682a05b3c5f290b7119ce45c3a1625c6b2e918952b675fd59`.

## Source secondary stat-drop suite

The pinned default move table has 18 damaging moves using the six target
stat-drop effects. Each corresponding effect script applies damage, checks
effect chance, then calls loweroppstathit with one bare stat ID. The pinned
stat-change routine decodes that zero amount nibble as one stage. The source
chance command compares BattleRandomRange(100) against the listed chance,
doubles it for Serene Grace, and suppresses it for Sheer Force. Target gates
include fainting, Substitute, Shield Dust and Covert Cloak.

These 18 chances and stage amounts now have explicit compile-time source
adapters under B_POLISHED_MOVE_BALANCE. Octazooka previously retained native
50% despite the default source specifying 30%; its source chance is now 30%,
or 60% with Serene Grace. Acid's Special Defense and Crunch's Defense payloads
also have source guards against native generation-dependent alternatives.
No target IDs or save layouts change, and native Expansion data remains intact.

The deterministic generator verifies hash-pinned move/effect/stat assembly and
emits the imported constants and provenance manifest. The production-linked
audit checks the exact additional-effect count, stat-minus dispatch, chance,
stat payload and target/phase flags for these 18 moves; other secondary families
are not covered by this new audit. Eight native/source test groups exercise
all 18 payloads on both sides, the 15 probabilistic moves with/without Serene
Grace, Shield Dust/Covert Cloak/Sheer Force/Clear Body, Mold Breaker ability
bypass, intact/broken substitutes, misses, Protect, Water Absorb, target
knockouts, the stat floor and PP consumption.

The retained engine probability framework supplies the tested percentage
thresholds. Its random stream and global RNG call ordering are not source
imports. This suite also does not establish complete source stat-change,
ability/item, damage, accuracy or AI behavior. Source animation/UI art and
all other secondary families remain separate audits. Optional Expansion-only
mechanics remain available through the native framework.

Validation with Arm GNU GCC 13.2.1: both ROM builds, all eight new test groups
in native/source profiles, 50 source/asset checks, 12 ARM probes and 12 demo
checks pass. Earlier move-balance (6), Hidden Power (8), Pain Split (10),
fixed-damage (9), retaliation (7), self-move accuracy (9), typeless (8),
Metronome/Struggle (12) and AI frame-budget (6) groups all pass in the source
profile. The final production audit matches all 18 secondary records, all 255
audited basic move/PP rows and 43,792 teaching combinations. Its 109 existing
machine identity/lesson gaps are unchanged, with no new findings.
The full native run passes 5,426 tests, with the same nine expected failures
and 585 existing TODOs (6,020 total), and no unexpected failures.

The same Johto ROM passes all 101 campaign checks and 19 directed warp
traversals, including all starter branches, Sprout Tower, Falkner, the
badge/Roost/Togepi rewards, resident schedules and Save/reset/Continue.
The authentic source startup remains separate from this demo-campaign gate.

Johto ROM SHA-256:
`3b710e351738bcc821913c3c5da015f107a734d91cc3d12b61965e2343f69e55`.

## Source Sleep Talk selection and last-move history

The source call chain is explicit: `SleepTalk` announces the parent and spends
PP, `BattleCommand_sleeptalk` immediately calls `ClearLastMove`, and `ResetTurn`
marks the called move with charging value 1. `DisplayUsedMoveText` skips both
last-move updates for that marker. Encore/Disable read the cleared last-counter
record and fail. Native Expansion instead records Sleep Talk and permits Disable
to attach to it. The source profile now clears `gLastMoves` at PP deduction and
move end, including an awake/no-candidate failure or a failed called attack.
Disable rejects this empty history before scanning move slots, so an empty
slot cannot become a false match. Ordinary following moves
restore their own history. Native profiles and the engine's separate effective-
move record are preserved.

The source fixture generator verifies the command, shared Disable/Encore reader,
charging marker and announcement code, with exact pinned Git blob hashes. It
also derives static eligibility for all 254 ordinary source entries from the
five excluded effects (six moves). Struggle is never a legal source known move;
the source loop does not implement the extra ban claimed by its comment, so
retaining the native defensive ban is explicitly outside that comparison.

The ordinary candidate pool remains the matching native implementation. Disable
excludes its move; zero PP and Choice locks do not prevent calling it. The source
also discards Encore during candidate selection, but clears the history needed
to Encore Sleep Talk. Artificial pre-encored states are not claimed as ported.
The production-linked audit now rejects any static eligibility mismatch.

Remaining scope: exact source RNG and slot ordering, sleep duration, interrupted
turn/history semantics and other move-calling consumers, complete Encore/Disable
timing, individual called effects, smart AI, source presentation and optional
Expansion interactions. A static eligibility match is not effect-level parity.

Both native Emerald and Johto production ROMs build. All nine focused test
groups pass in both profiles, including 27 battle cases per profile and the
254-entry static check. All earlier source move suites and six AI timing checks
pass, together with 51 source checks and 12 ARM probes. The full native run
passes 5,435 tests, with the same nine expected failures and 585 existing TODOs
(6,029 total), and no unexpected failures.

The production-linked audit matches all 254 Sleep Talk eligibility entries,
all 255 basic move/PP records, all 18 target stat-drop secondary records and all
43,792 teaching combinations. The same 109 explicitly recorded machine/lesson
gap entries remain; there are no new or changed findings. The same final Johto
ROM passes all 12 demo checks, 101 campaign checks and 19 directed warp
traversals, including all starter branches, Sprout Tower, Falkner, the
badge/Roost/Togepi rewards, resident schedules and Save/reset/Continue.
The authentic source startup remains separate from this demo-campaign gate.

Johto ROM SHA-256:
`c0c4af6d0c3db46708ca1c7774119d047bc8e26531f83562d9a3c34e4ce634fc`.

## Source Disable and Encore duration

The pinned shared command initializes Disable at 5 and Encore at 4. Source
`PerformMove` calls `TickDisableAfterMove` after `DoTurn`, including a failed
action. That tick only changes 5 to 4 and skips a fainted battler. Both active
counters then decrement at end of turn; Encore also ends when its move's PP
reaches zero. Switching clears both counters, and Mental Herb clears the active
restriction while preserving the source's separate Choice bookkeeping.

The source profile now imports these initial counters and ticks a newly applied
Disable after the action owner finishes. Native Expansion previously used 4
for Disable regardless of action order, losing one affected opportunity when
the target had already acted. Its Encore initialization also subtracted a tick
for a target still to act; that adjustment is now confined to native profiles.
No counter field, stable ID or save layout is changed.

The fixture generator verifies the command, complete action/timer call chain,
end-turn handlers and exits against exact source blobs, and records expected
end-turn traces for both action orders. Eight battle groups cover both sides,
expiration messages and resumed selection, no active refresh, rejection with
missing history or zero PP, Encore's early PP exit, switch-out/return, Mental
Herb clearing/consumption and counters after full paralysis.

Mental Herb tests use the already-selected matching move. The source writes the
pending move before curing Encore; preserving that choice when it originally
differed, plus Unnerve, is covered by the following commitment suite. Full called-move
history/eligibility, disobedience, move replacement, Cursed Body application
gates, other restriction interactions, global action/end-turn ordering, AI and
presentation remain separate audits. Expansion-only formats and gimmicks keep
the native framework and are not certified as source behavior.

Validation with Arm GNU GCC 13.2.1: both production ROMs build. All eight
focused groups (30 battle cases per profile), every earlier source move suite
and six AI timing checks pass, together with 52 source checks and 12 ARM
compiler probes. The full native run passes 5,443 tests, with the same nine
expected failures and 585 existing TODOs (6,037 total), and no unexpected
failures.

The production-linked audit matches all 255 basic move/PP records, 18 target
stat-drop records, 254 Sleep Talk eligibility entries and 43,792 teaching
combinations. Its 109 explicitly recorded machine/lesson gaps are unchanged;
there are no new or changed findings. The same final Johto ROM passes all
12 demo checks, 101 campaign checks and 19 directed warp traversals, including
all starter branches, Sprout Tower, Falkner, the badge/Roost/Togepi rewards,
resident schedules and Save/reset/Continue. The authentic source startup
remains separate from this demo-campaign gate.

Johto ROM SHA-256:
`63e849ba3887279713370940d3e2030cd07213536f0ff23144a918e28989e22f`.

## Source Encore commitment through Mental Herb

Pinned source Encore assigns `BATTLE_VARS_MOVE_OPP` before calling the Mental
Herb handler. `home/battle_vars.asm` maps that variable to the opponent's pending
move. The cure clears the Encore nibble while preserving Choice bookkeeping;
it never restores the pending move. `DoTurn` later calls `UpdateMoveData`, which
finds that move's current known slot for `BattleConsumePP`. Thus a slower target
still executes the Encore-selected move after its herb cures the lock. The
native engine instead resumed the original selection in this situation.

The source profile now copies the validated Encore slot into the target's
pending ordinary move action before item resolution. This retains correct PP
and normal target resolution, and ordinary selection replaces it on the next
turn. Already-acted targets, non-move actions and active gimmicks are not
rewritten. Native profiles retain their own behavior. IDs, saved/volatile
layouts, capacities and the existing active-Encore path remain unchanged.

`build_encore_commitment.py` verifies the entire pinned call chain with two
additional exact-blob fixtures. It also checks source `GetUserItemAfterUnnerve`
against all 22 edible berries: Mental Herb is not suppressed. The matching
native item behavior is retained. The generator is enforced in CI and the
production move/machine audit's source-input checks.

Seven focused groups cover 56 battle cases per profile: both sides; all four
remembered slots; Tackle and Splash; correct PP and herb consumption; next-turn
free choice; already-acted targets; active Encore without a herb; no history,
exhausted PP and failed Sleep Talk rejection; an action stopped by paralysis;
switch/reentry; and Encore, Disable and Attract cures with/without Unnerve.
Both pending-choice groups failed against the original runtime before the fix.
All seven groups now pass in source and native profiles.

Scope remains ordinary singles. Full called-move history/eligibility,
disobedience, charging/recharge, priority/order edge cases, Mental Herb cure
ordering with simultaneous conditions, complete item/ability suppression,
source presentation and Expansion-only formats/gimmicks are separate audits.
No claim of complete Encore or Mental Herb parity is made.

Validation with Arm GNU GCC 13.2.1: both production ROMs build. All seven
focused groups (56 cases per profile), the 94 earlier source move groups and
six AI timing checks pass, together with 53 source checks and 12 ARM compiler
probes. The full native suite passes 5,450 tests, with the same nine expected
failures and 585 existing TODOs (6,044 total), and no unexpected failures.

The production-linked audit matches all 255 basic move/PP records, 18 target
stat-drop records, 254 Sleep Talk eligibility entries and 43,792 teaching
combinations. Its 109 recorded machine/lesson gaps remain unchanged, with no
new findings. The same final Johto ROM passes all 12 demo checks, 101 campaign
checks and 19 directed warp traversals, including all starter branches, Sprout
Tower, Falkner, the badge/Roost/Togepi rewards, resident schedules and
Save/reset/Continue. Authentic source startup remains separate from this
demo-campaign gate.

Johto ROM SHA-256:
`795cb088ae9b985fafde0ccd8ed5bef30ad1fdfdc6abcf3956ab531640f53bba`.

## Source Cursed Body eligibility and called-move history

Source `CursedBodyAbility` checks Future Sight, rolls 3/10, and calls the shared
`BattleCommand_disable`. The command rejects cleared history, Struggle, an
already-active Disable, unknown moves and zero remaining PP before showing
the ability. It starts the existing source Disable counter at five and checks
Mental Herb. The ordinary post-action tick makes that four, followed by the
normal end-turn countdown. Native Cursed Body previously bypassed the source
history/PP validation by directly disabling the selected move.

The source profile now resolves Cursed Body through the live recorded move and
known slot, requiring remaining PP. Sleep Talk and Metronome clear both source
last-move records before calling a move, and the called move's text skips
restoring them. The existing history adapter now clears Metronome as well as
Sleep Talk at PP deduction and move end. Ordinary subsequent moves restore
their own history. Native Cursed Body behavior and active gimmick selection/
duration are retained; IDs, save/volatile layouts and capacities do not change.

The fixture generator verifies that Cursed Body can run when its holder is
knocked out, that Mold Breaker does not suppress it, and that Future Sight
cannot activate it. Two new exact-blob fixtures cover the Future Sight user
predicate and Mold Breaker exception list. The other command, ability, history,
PP and counter fixtures are already pinned. CI and the production audit now
enforce this generated contract.

Eleven focused groups cover 82 cases per profile: both sides; all four known
slots; one/two PP; contact and noncontact attacks; statistical 30% activation;
miss/protection/type-immunity gates; intact defender substitutes; attacker substitutes;
Mold Breaker; holder knockout; active Disable; Mental Herb retained after a
rejected attempt and consumed after a valid one; expiration and restored use;
switch/reentry; Future Sight; Sleep Talk/Metronome history; and subsequent
Disable/Encore after Metronome hits, misses or protection. Three direct Cursed
Body groups reproduced the original runtime's eligibility failures. All eleven
groups now pass in native and source profiles.

Scope remains ordinary singles. Exact source RNG and call order,
substitute-breaking hits, all multi-hit, recoil/self-faint, charging/recharge,
disobedience, reflected moves, simultaneous
cure ordering, other history consumers and original presentation are separate
audits. Expansion-only callers, formats and gimmicks are not certified as source
behavior. This closes the selected ordinary Cursed Body gate/history gap, not
all Cursed Body or called-move behavior.

Validation with Arm GNU GCC 13.2.1: both production ROMs build. All eleven
focused groups (82 cases per profile), all 101 earlier source move groups and
six AI timing checks pass, together with 54 source checks and 12 ARM compiler
probes. The full native suite passes 5,461 tests, with the same nine expected
failures and 585 existing TODOs (6,055 total), and no unexpected failures.

The production-linked audit matches all 255 basic move/PP records, 18 target
stat-drop records, 254 Sleep Talk eligibility entries and 43,792 teaching
combinations. Its 109 recorded machine/lesson gaps remain unchanged, with no
new findings. The same final Johto ROM passes all 12 demo checks, 101 campaign
checks and 19 directed warp traversals, including starter branches, Sprout
Tower, Falkner, the badge/Roost/Togepi rewards, resident schedules and
Save/reset/Continue. Authentic source startup remains separate from this
demo-campaign gate.

Johto ROM SHA-256:
`c958387e6754c237ca81296f0f1050ccb4760dee5a349c251fcb82bf50d1119b`.

## Source Cursed Body after Substitute breaks

The pinned `NormalHit` effect applies damage before post-hit effects.
`SelfInflictDamageToSubstitute` clears `SUBSTATUS_SUBSTITUTE` when damage equals
or exceeds the remaining Substitute HP. `BattleCommand_posthiteffects` then
uses `CheckSubstituteOpp`, which reads that current flag rather than the
separate record that a Substitute absorbed this hit. Consequently Cursed Body
can activate on the breaking hit. Expansion's direct-HP-damage check prevented
this source behavior.

The source-profile adapter recognizes positive damage against an exhausted
Substitute before the native delayed flag cleanup. It keeps the direct HP hit
path, misses/failures, intact Substitute protection, existing recorded-move/PP
gate and Mental Herb handling. Sound and Infiltrator bypasses retain their
existing working path. Native profiles, active gimmicks, stable IDs, state/save
layouts and engine capacities are unchanged.

The existing generator now verifies the pinned apply-damage, flag-clear and
post-hit chain. Two additional exact-blob fixtures supply Substitute creation
and the sound-move list. Its generated manifest records the extended scope,
and the existing CI generator and battle prefix enforce it.

Seven new groups cover 30 cases per profile: both sides,
Substitute HP at 49/50/51 against 50 damage, contact/noncontact breaks, last PP,
Mental Herb, Sleep Talk/Metronome history, sound/Infiltrator bypasses, later
miss/protection/status actions, and native Z-move behavior. The threshold and
Mental Herb groups failed against the original runtime's missing activation.

This supersedes the preceding suite's exclusion of ordinary damaging
Substitute-breaking hits. Other post-hit abilities still have native Substitute
gates; this is not a general post-hit import. All multi-hit, recoil/self-faint,
global RNG/order, simultaneous item/ability cases, original presentation and
optional Expansion formats remain independent audits.

Validation with Arm GNU GCC 13.2.1: both production ROMs build. All seven new
groups pass in source/native profiles, together with all 112 earlier source
move groups, six AI timing checks, 54 source checks and 12 ARM compiler probes.
The full native suite passes 5,468 tests, with nine unchanged expected failures
and 585 existing TODOs (6,062 total), and no unexpected failures.

The production-linked audit matches all 255 basic move/PP records, 18 target
stat-drop records, 254 Sleep Talk eligibility entries and 43,792 teaching
combinations. The 109 recorded machine/lesson gaps are unchanged, with no new
or changed findings.

The same final Johto ROM passes all 12 demo checks, 101 campaign checks and
19 directed warp traversals, including all starter branches, Sprout Tower,
Falkner, the badge/Roost/Togepi rewards, resident schedules and Save/reset/
Continue. The authentic source opening remains separate from this demo gate.

Johto ROM SHA-256:
`4f5cfa5b3905c020d6fc1350e5dcb9ff41ae5312dc0548ead64ed2f02c3029eb`.

## Source Protect/Endure successive-use rules

The pinned Protect and Endure commands share `ProtectChance`. After each prior
success, it requires another successful 1-in-3 roll, up to six rolls: the
success denominators are 1, 3, 9, 27, 81, 243 and 729. The stored count saturates
at six, so later attempts stay at 1-in-729. Expansion previously capped the
odds at 1-in-27 and continued increasing the stored count.

The source profile now uses the imported denominators and saturates the
existing counter for ordinary Protect/Endure. One tagged weighted roll preserves
the aggregate probability; it does not reproduce the source RNG stream or its
sequence of ternary draws. Native profiles, optional protection moves and
active gimmicks retain their existing paths. No identifiers, save/state layouts
or engine capacities change.

`build_protect_chain.py` verifies exact pinned Protect/Endure fixtures, the
shared loop, cap, failure reset, selected-other-move reset, switch reset and
opponent-first gate. CI and the production audit enforce its generated header
and manifest. Nine battle groups cover 62 cases per profile: both sides and
moves, first-use protection/survival and PP, every probability stage, alternating
Protect/Endure, count saturation, failure, ordinary moves, switching/reentry,
acting after the opponent and optional Detect's retained native odds. The
saturation group reproduced the old runtime's source mismatch before the fix.

This closes the ordinary direct-use probability/counter scope. Interrupted or
called actions, disobedience, item/switch action ordering, global priority and
AI, all protection interactions, exact RNG and source presentation remain
separate audits. Optional Expansion formats and gimmicks are not certified as
source behavior.

Validation with Arm GNU GCC 13.2.1: both production ROMs build. All nine
focused groups (62 cases per profile), all 119 earlier source move groups and
six AI timing checks pass, along with 55 source checks and 12 ARM compiler
probes. The full native suite passes 5,477 tests, with nine unchanged expected
failures and 585 existing TODOs (6,071 total), and no unexpected failures.

The production-linked audit matches all 255 basic move/PP records, 18 target
stat-drop records, 254 Sleep Talk eligibility entries and 43,792 teaching
combinations. The 109 recorded machine/lesson gaps are unchanged, with no new
or changed findings.

The same final Johto ROM passes all 12 demo checks, 101 campaign checks and
19 directed warp traversals, including all starter branches, Sprout Tower,
Falkner, the badge/Roost/Togepi rewards, resident schedules and Save/reset/
Continue. The authentic source opening remains separate from this demo gate.

Johto ROM SHA-256:
`432b133d2a8fbbf7525e9cf68688739ce0f51ca2d471a75f973ad20fa3e1ac2c`.

## Source Belly Drum eligibility and HP cost

The pinned Belly Drum command first requires current HP strictly greater than
`max(1, floor(maxHP / 2))`. It then requests a forced Attack increase through
the source stat engine, which reverses the direction for effective Contrary.
If no stat change is possible, it fails without spending HP. Otherwise it
charges that half-maximum cost. The encoded sixteen-stage request reaches
+6 normally or -6 with Contrary from every starting stage.

Expansion's native gate always checks the upper Attack limit: Contrary at +6
fails, while Contrary at -6 can spend HP without changing Attack. The source
profile now supplies the effective ability to the existing limit comparator,
so Contrary instead checks the lower limit. The native max-stat operation and
HP calculation already produce the source final stage and cost, and remain in
use. Native profiles, optional effect users and active gimmicks retain their
existing gate. IDs, state/save layouts and engine capacities are unchanged.

`build_belly_drum.py` validates the pinned command, forced-stat dispatch,
Contrary reversal, source stage bounds, no-change failure, minimum/floored HP
cost and ability lookup after Neutralizing Gas. One new exact-blob fixture
supplies the command; the other fixtures are already pinned. Its generated
manifest records the source contract and stage cases, and is checked by CI and
the production audit.

Eight battle groups cover 84 cases per profile: both sides, ordinary and
Contrary users at -6/-4/0/+4/+6, even/odd HP and half-HP boundaries, one-to-three
maximum HP, other-stat preservation, repeated attempts after healing,
switch/reentry, suppressed Contrary and native Z-move healing/limits. The
stage-limit group reproduced the old runtime's source mismatch before the fix.

This closes the selected ordinary eligibility/final-stage/HP-cost scope.
Original Contrary success/failure text and animations, HP-versus-stat/item-event
ordering, complete held-item/ability interactions, called/reflected moves,
disobedience, global stat/damage/AI rules and exact source RNG remain separate
audits. Optional Expansion formats and gimmicks are not certified as source
behavior.

Validation with Arm GNU GCC 13.2.1: both production ROMs build. All eight
focused groups (84 cases per profile), all 128 earlier source move groups and
six AI timing checks pass, along with 56 source checks and 12 ARM compiler
probes. The full native suite passes 5,485 tests, with nine unchanged expected
failures and 585 existing TODOs (6,079 total), and no unexpected failures.

The production-linked audit matches all 255 basic move/PP records, 18 target
stat-drop records, 254 Sleep Talk eligibility entries and 43,792 teaching
combinations. The 109 recorded machine/lesson gaps are unchanged, with no new
or changed findings.

The same final Johto ROM passes all 12 demo checks, 101 campaign checks and
19 directed warp traversals, including all starter branches, Sprout Tower,
Falkner, the badge/Roost/Togepi rewards, resident schedules and Save/reset/
Continue. The authentic source opening remains separate from this demo gate.

Johto ROM SHA-256:
`e30d830793b012f7290cfeba6dc3de9e567455a209a99eeac8ef56928d1c5a39`.

## Source Rapid Spin cleanup under Sheer Force

The pinned Rapid Spin script treats its Speed increase and cleanup separately.
Sheer Force marks the secondary effect as failed, so Speed does not rise, but
the later `clearhazards` command still removes the user's Leech Seed, binding
move, and every layer of Spikes/Toxic Spikes on the user's side. Move failure
ends the script before cleanup; post-hit processing also aborts the remaining
sequence if the user faints. An intact or broken target Substitute does not
block the separate cleanup command. The source end-effect gate continues to
suppress Life Orb and Shell Bell.

Expansion's Sheer Force skip previously bypassed the cleanup along with other
move-end effects. The source profile now runs the existing cleanup script once
for successful ordinary singles Rapid Spin damage by a living user, then resumes
at the native skip destination. This keeps the Speed boost and other suppressed
end effects disabled. Native profiles, other moves, double battles and active
gimmicks retain their existing paths. No new state, identifiers, save-layout
changes or capacity limits are introduced.

`build_rapid_spin.py` checks the pinned default move row, effect-script order,
secondary-effect failure, cleanup operations, hit/faint gates and end-item
suppression. One new exact-blob fixture supplies the cleanup command; existing
fixtures provide the rest. CI and the production audit check the generated
manifest and the dedicated battle prefix.

Eleven groups cover 66 cases per profile: both sides, ordinary and Sheer Force
users, individual minimum/maximum hazard layers, the untouched opposite side,
combined binding/Leech Seed/stacked hazards, miss/protection/Ghost immunity/full
paralysis, intact/broken substitutes, target knockout, user contact knockout,
Life Orb/Shell Bell suppression, Neutralizing Gas, repeated cleanup through the
last PP and a following move, optional Mortal Spin, Terastal and double battles.
The hazard-layer group reproduced the old runtime's missing source cleanup.

Scope remains this bounded ordinary cleanup interaction. Native cleanup
message/animation order is retained. Complete contact/faint/switch/item/ability
scheduling, called/reflected/multi-hit moves, full Sheer Force damage/AI/RNG
rules and hazard/trapping mechanics remain separate audits. Expansion-only
hazards, formats and gimmicks are not certified as source behavior.

Validation with Arm GNU GCC 13.2.1: both production ROMs build. All eleven
focused groups (66 cases per profile), all 136 earlier source move groups and
six AI timing checks pass, along with 57 source checks and 12 ARM compiler
probes. The full native suite passes 5,496 tests, with nine unchanged expected
failures and 585 existing TODOs (6,090 total), and no unexpected failures.

The production-linked audit matches all 255 basic move/PP records, 18 target
stat-drop records, 254 Sleep Talk eligibility entries and 43,792 teaching
combinations. The 109 recorded machine/lesson gaps are unchanged, with no new
or changed findings.

The same final Johto ROM passes all 12 demo checks, 101 campaign checks and
19 directed warp traversals, including all starter branches, Sprout Tower,
Falkner, the badge/Roost/Togepi rewards, resident schedules and Save/reset/
Continue. The authentic source opening remains separate from this demo gate.

Johto ROM SHA-256:
`9fd96dc56f65867f12d0a58f07fff7b264e80e4c8d0df1fd57eb704e007805b2`.

## Source Perish Song target gates and countdown

The pinned `perish_song.asm` starts the user's counter independently of the
opponent, never refreshes an existing counter, and calls `checkpriority` only
for an opponent without a counter. That command checks positive-priority Armor
Tail first. Its Prankster branch checks Dark typing and returns before the
Soundproof branch; other users check Soundproof with Mold Breaker respected.
The effect script has no general hit check or Fly/Dig hidden-target gate.

Expansion previously cancelled the entire Prankster move against Armor Tail,
blocked a non-Dark Soundproof opponent even under Prankster, and applied its
general hiding check to Fly/Dig targets. The source profile now
resolves these gates per opponent, leaves the user's counter independent, and
bypasses Fly/Dig hiding while retaining the separate ability/type immunities.
Native profiles, doubles, active gimmicks and dynamically retyped Perish Song
retain their existing paths. No new counters, IDs, save layouts or capacity
limits are introduced.

The native countdown representation is retained: it starts at three and prints
before decrement, whereas the source starts at four and decrements before
printing. Both display 3, 2, 1, 0 and faint on the fourth end turn including use.
`build_perish_song.py` verifies the exact pinned command, effect-script command
list, priority and immunity branch order, counter non-refresh and end-turn
countdown. One new exact-blob fixture supplies the move command; existing
fixtures supply the other routines. CI and the production audit check the
manifest and dedicated battle prefix.

Nine groups cover 56 cases per profile: both sides; the ordinary/Prankster/Mold
Breaker versus ordinary/Soundproof/Armor Tail matrix; hidden Dark targets;
each countdown turn through fainting; repeat failure through the last PP;
switch/recast without refreshing the user; Protect, Substitute, Fly and Dig;
a Soundproof user; and native Terastal/double-battle controls. The ability
matrix reproduced the original failure before the runtime correction. The
hidden-target control separately reproduced the inherited hiding-check gap.

Scope remains these ordinary singles gates and countdown controls. Native
animation/message scheduling is retained: the source updates the user counter
before displaying opponent immunity. Complete end-turn faint ordering, Baton
Pass, mid-turn ability changes, called moves, AI choice, dynamically retyped
moves and optional formats/gimmicks remain separate audits.

Validation with Arm GNU GCC 13.2.1: both production ROMs build. All nine
focused groups (56 cases per profile), all 147 earlier source move groups and
six AI timing checks pass, along with 58 source checks and 12 ARM compiler
probes. The full native suite passes 5,505 tests, with nine unchanged expected
failures and 585 existing TODOs (6,099 total), and no unexpected failures.

The production-linked audit matches all 255 basic move/PP records, 18 target
stat-drop records, 254 Sleep Talk eligibility entries and 43,792 teaching
combinations. The 109 recorded machine/lesson gaps are unchanged, with no new
or changed findings.

The same final Johto ROM passes all 12 demo checks, 101 campaign checks and
19 directed warp traversals, including all starter branches, Sprout Tower,
Falkner, the badge/Roost/Togepi rewards, resident schedules and Save/reset/
Continue. The authentic source opening remains separate from this demo gate.

Johto ROM SHA-256:
`da3f7b0c2b73cc801dd8798f8058567e011aef128dae882902cd6b3b94d1486d`.

## Source Foresight gates and identification accuracy

The pinned Foresight command rejects Fly/Dig targets after the normal hit
check, so No Guard and Lock-On cannot bypass its effect-level guard. Repeating
identification fails; Substitute does not block it. The source type table
removes only Normal/Fighting versus Ghost immunity after identification.

Source identification ignores only positive target evasion and keeps negative
evasion and the user's accuracy changes. Expansion previously discarded all
evasion stages. The source profile now preserves those reductions and uses
the source's exact stage fractions for attacks against an identified opponent.
The combined stage is capped at -6..6 before applying (3+n)/3 or 3/(3-n).
Native percentage approximations are retained outside this scope.

The source's listed Foresight accuracy of -1 is stored as the byte 255. Despite
the table comment describing an accuracy bypass, this pinned effect calls
`checkhit`, copies that byte unchanged, and applies stat/ability/item modifiers.
For example, with no other modifiers a -6 accuracy stage produces floor(255 *
3 / 9) = 85 percent, and -5 produces 95 percent. Wonder Skin replaces the base
with 50 before stage modifiers. The port retains the public zero sentinel for
listed data, but uses the raw source byte in this execution path instead of
unconditionally bypassing accuracy. No Guard and Mold Breaker retain their
selected visible-target interactions.

These corrections are scoped to ordinary singles with no active gimmick.
Native profiles, doubles, active gimmicks and the optional Miracle Eye flag
keep their native paths. No identifiers, save structures, counters or engine
capacities change. `build_foresight.py` verifies the exact source command,
effect-script order, raw byte copying, immunity marker, hidden-target and
reflection gates, stage arithmetic, Wonder Skin and multiply/divide behavior.
Battle execution uses a separate source accuracy path to keep the existing
native AI estimation cost intact. The six timing checks retain their original
ceilings, including 24 frames for the smart Steven multi-battle case. Full
source AI accuracy estimates remain a separate gap.

Five new exact-blob fixtures supply the move, reflection, bypass list, type
table and math helper. CI and the production audit check the generated header,
manifest and dedicated battle prefix.

Ten groups cover 90 parameter combinations per profile, with weighted
probability trials: both sides; Fly/Dig with ordinary, either-side No Guard and
Lock-On; raw accuracy and Wonder Skin at selected severe/neutral stages; all
even evasion stages from -6 to +6 with neutral/reduced user accuracy; Ghost
versus unrelated immunities; Substitute/Protect; PP, repeated use and switch/
reentry; visible No Guard/Mold Breaker; and optional Miracle Eye, Terastal and
doubles. The initial tests reproduced the three discrepancies; the raw-byte
cases also exposed the native table's rounded -5 ratio before it was corrected.

Scope remains these gates and stage calculations. Source failure messages,
animations and complete reflected/called-move ordering remain separate.
Affection, other accuracy item/ability/weather modifiers, global accuracy/RNG,
Baton Pass and AI accuracy estimates/choices remain separate audits. Other
moves with a source -1 accuracy byte are not certified by this suite.

Validation with Arm GNU GCC 13.2.1: both production ROMs build. All ten
focused groups (90 parameter combinations per profile, including weighted
probability trials), all 156 earlier source move groups and six AI timing
checks pass, along with 59 source checks and 12 ARM compiler probes. The full
native suite passes 5,515 tests, with nine unchanged expected failures and 585
existing TODOs (6,109 total), and no unexpected failures. The final source
execution/AI separation was rerun through all source prefixes and the full
native suite without relaxing the AI timing ceilings.

The production-linked audit matches all 255 basic move/PP records, 18 target
stat-drop records, 254 Sleep Talk eligibility entries and 43,792 teaching
combinations. The 109 recorded machine/lesson gaps are unchanged, with no new
or changed findings.

The same final Johto ROM passes all 12 demo checks, 101 campaign checks and
19 directed warp traversals, including all starter branches, Sprout Tower,
Falkner, the badge/Roost/Togepi rewards, resident schedules and Save/reset/
Continue. The authentic source opening remains separate from this demo gate.

Johto ROM SHA-256:
`17f56e947ac4b0066565195baf945a067ea2b315ea5a7e01a82870a15dc9ee2b`.

## Source Mean Look application and reflection gates

The pinned Mean Look effect script has no general hit check. Its trap bypasses
Substitute and Protect, but the effect explicitly rejects Fly/Dig targets even
with No Guard or Lock-On. Ghost targets remain immune and repeat trapping fails.
The separate Magic Bounce command respects Substitute unless the original
attacker has Infiltrator; Mold Breaker prevents reflection. A reflected Mean
Look still applies through the original user's Substitute and cannot bounce
again. Protect does not block this reflection because Mean Look never performs
the general hit check that marks protection in the source.

Expansion previously blocked direct/reflected trap application through
Substitute, allowed No Guard/Lock-On to trap hidden targets, and reflected
through a defender's Substitute. Move-specific execution gates now reproduce
these source cases in ordinary singles without active gimmicks. Native
profiles, optional trapping moves, doubles and active gimmicks retain their
native gates. The native target-side trap and trapper fields adapt the source's
user-side CANT_RUN bit without new state, identifiers or save-layout changes.
Ordinary switching by the trapper releases the target.

`build_mean_look.py` verifies the pinned effect command order, absence of a hit
check, hidden/repeat/Ghost gates, reflection ordering and both switch-reset
routines. One exact-blob move fixture supplements the existing source files.
CI and the production move/machine audit verify the generated manifest and
run the dedicated battle prefix.

Nine groups cover 60 parameter cases per profile: both sides; Substitute,
Protect and ordinary application; Fly/Dig with ordinary, either-side No Guard
and Lock-On; the Magic Bounce/Substitute/Infiltrator/Mold Breaker matrix;
reflected application without recursion; Ghost immunity, repeat failure and
PP consumption; trapper switch release; and native Spider Web, Terastal and
double-battle controls. Four groups reproduced the inherited discrepancies
before the implementation changed.

This suite covers application and the selected controls, not the entire
trapping lifecycle. Baton Pass, Run Away/Shed Shell switching, fleeing, forced
switches, complete called-move/ability/priority ordering, accuracy modifiers,
source failure text/animations and AI estimates/choices remain separate audits.

Validation with Arm GNU GCC 13.2.1: both production ROMs build. All nine
focused groups (60 cases per profile), all 166 earlier source move groups and
six AI timing checks pass, along with 60 source checks and 12 ARM compiler
probes. The full native suite passes 5,524 tests, with nine unchanged expected
failures and 585 existing TODOs (6,118 total), and no unexpected failures.
The AI timing ceilings remain unchanged.

The production-linked audit matches all 255 basic move/PP records, 18 target
stat-drop records, 254 Sleep Talk eligibility entries and 43,792 teaching
combinations. The 109 recorded machine/lesson gaps are unchanged, with no new
or changed findings.

The same final Johto ROM passes all 12 demo checks, 101 campaign checks and
19 directed warp traversals, including all starter branches, Sprout Tower,
Falkner, the badge/Roost/Togepi rewards, resident schedules and Save/reset/
Continue. The authentic source opening remains separate from this demo gate.

Johto ROM SHA-256:
`9ca4fc8a6cc95de330fbdbb1e488bcb81f74867348c6e834967d99d553f222ea`.

## Source Run Away voluntary switching

In the pinned default release (`FAITHFUL` undefined), `UserCanSwitch` checks
Run Away before trapping abilities, wrapping and the opponent's CANT_RUN flag.
The player uses this rule directly. `AI_UserCanSwitch` first checks for another
living party member, then uses the same rule. `GetTrueUserAbility` respects
Neutralizing Gas. The source's ability description only mentions wild escape;
the executable default-build switch rule is authoritative here.

Expansion previously allowed Run Away to flee from wild battles but still
blocked voluntary switching through these traps. The source profile now
permits ordinary singles switches through Mean Look, Wrap, Arena Trap, Magnet
Pull and Shadow Tag. The shared move/field and ability trap checks serve the
player's selection path; a matching AI eligibility gate lets the existing
switching logic consider a legal switch. This does not replace source AI
strategy or certify its estimates. No identifiers, save fields or capacities
change, and the trap state remains intact until normal switch cleanup. Wrap
still deals residual damage while the Pokemon stays in battle.

The exemption respects ability suppression, including the native optional
Gastro Acid control. Native profiles, doubles and active gimmicks retain their
native paths. Optional Ingrain, Fairy Lock, Sky Drop, Commander and Battle Arena
restrictions remain active. Optional moves sharing the permanent-trap field
receive the same exemption in scoped singles; their complete behavior is not
certified as source content.

`build_run_away.py` verifies the default-build branch, ordered switch gates,
player/AI call sites, ability suppression and trapping-ability identities from
existing exact-blob fixtures. CI and the production audit verify the generated
manifest and run the dedicated battle prefix. Eight groups cover 72 parameter
cases per profile: both sides; positive and neutral-ability eligibility;
ordinary switches through each selected trap; residual damage and retained
trap state; Neutralizing Gas/Gastro Acid; either side's Terastal; optional field
restrictions and doubles; and actual AI switching at the end of a Perish Song
countdown. Live eligibility and AI tests reproduced inherited failures before
the correction. Negative controls explicitly use Keen Eye because forcing
ABILITY_NONE can leave a species' default ability active in the test harness.

Wild fleeing, forced switching, the full source trapping-ability rules,
Shed Shell/item suppression, Ghost exemptions and other non-Run Away switching
rules remain separate audits. Full source AI choices and estimates remain open.

Validation with Arm GNU GCC 13.2.1: both production ROMs build. All eight
focused groups (72 cases per profile), all 175 earlier source move groups and
six AI timing checks pass, along with 61 source checks and 12 ARM compiler
probes. The full native suite passes 5,532 tests, with nine unchanged expected
failures and 585 existing TODOs (6,126 total), and no unexpected failures.
The AI timing ceilings remain unchanged.

The production-linked audit matches all 255 basic move/PP records, 18 target
stat-drop records, 254 Sleep Talk eligibility entries and 43,792 teaching
combinations. The 109 recorded machine/lesson gaps are unchanged, with no new
or changed findings.

The same final Johto ROM passes all 12 demo checks, 101 campaign checks and
19 directed warp traversals, including all starter branches, Sprout Tower,
Falkner, the badge/Roost/Togepi rewards, resident schedules and Save/reset/
Continue. The authentic source opening remains separate from this demo gate.

Johto ROM SHA-256:
`7a0d077f8190c84f066401101bdd60847166a41a9726ae665bc5610ce14d989f`.

## Source mutual trapping abilities

The pinned `CheckIfTrappedByAbility` checks trapping in both directions. If the
opponent can trap the user, it reverses the turn context and checks whether the
user can also trap the opponent. If both checks succeed, neither Pokemon is
ability-trapped. The wrapper preserves the result flags when restoring turn
context. Ghost targets are immune, Magnet Pull requires Steel typing, Arena
Trap requires grounding, and Shadow Tag applies to other targets.

Expansion only exempted matching Shadow Tag holders. The source profile now
cancels all mutually eligible trapping abilities in ordinary singles without
active gimmicks, while preserving one-way traps. This also removes the native
blanket same-Shadow-Tag exemption in this scope: a Ghost Shadow Tag holder can
trap a non-Ghost Shadow Tag holder because the reverse trap does not apply.
The shared eligibility query serves player selection and the existing AI
switch gate; no AI strategy or estimates are replaced.

Eligibility uses current types, abilities after suppression and grounding.
The existing Run Away switch exemption remains earlier in the path. Cancelled
ability traps do not clear Mean Look or Wrap, and no counters, identifiers,
save fields or capacities change. Native profiles, doubles and active gimmicks
retain their native path. The native grounding helper retains optional field
extensions; selected Flying, Levitate, Air Balloon and Iron Ball cases match
the source's ordinary grounding checks.

`build_trapping_abilities.py` verifies the pinned wrapper, reversed-turn helper,
Ghost/type/grounding predicates, suppression and switch-gate ordering from
existing exact-blob fixtures. It generates 400 independent expected source
and native-control vectors over three trapping abilities plus neutral Keen
Eye, and Normal, Steel, Flying, Ghost and Steel/Flying examples. CI and the
production audit verify both the manifest and generated test header.

Eight groups cover 439 cases per profile: the 400-case matrix; Neutralizing
Gas; selected grounding/item cases on both sides; eligibility after optional
Gastro Acid suppression; retained Mean Look/Wrap effects; native Terastal and
doubles; and actual AI switching when an imminent Perish Song faint prompts a
legal switch. Four groups reproduced the inherited failures before the fix.
Neutral controls use explicit Keen Eye rather than ABILITY_NONE.

This closes the selected ability-trap eligibility and mutual-cancellation
scope. Complete wild fleeing/Teleport, source AI choices/estimates, forced
switching, full switch timing, Shed Shell/item suppression, Unnerve and all
other grounding modifiers remain separate audits. Optional content remains
available without certifying it as source behavior.

Validation with Arm GNU GCC 13.2.1: both production ROMs build. All eight
focused groups (439 cases per profile), all 183 earlier source battle groups
and six AI timing checks pass, along with 62 source checks and 12 ARM compiler
probes. The full native suite passes 5,540 tests, with nine unchanged expected
failures and 585 existing TODOs (6,134 total), and no unexpected failures.
The AI timing ceilings remain unchanged.

The production-linked audit matches all 255 basic move/PP records, 18 target
stat-drop records, 254 Sleep Talk eligibility entries and 43,792 teaching
combinations. The 109 recorded machine/lesson gaps are unchanged, with no new
or changed findings.

The same final Johto ROM passes all 12 demo checks, 101 campaign checks and
19 directed warp traversals, including all starter branches, Sprout Tower,
Falkner, the badge/Roost/Togepi rewards, resident schedules and Save/reset/
Continue. The authentic source opening remains separate from this demo gate.

Johto ROM SHA-256:
`dd7856d217d652f23cff2da7d97ef24f2ec54abefd9ab291d2565f54fba78737`.


## Source binding countdown and residual damage

The pinned `BattleCommand_traptarget` applies a uniformly chosen count of four
or five, or seven with Grip Claw. `HandleWrap` decrements before checking for
release: the last countdown turn prints release without dealing damage.
Expansion instead treated those counts as damaging turns and released the
Pokemon one turn later. The source profile now adapts Wrap, Fire Spin and
Whirlpool to the source timing in ordinary singles without active gimmicks.
It stores one fewer native damage tick, preserving existing fields, cleanup,
identifiers, save layouts and capacities.

The source returns before decrementing or releasing when Magic Guard prevents
residual damage or a Substitute is present. Creating a new Substitute clears
an existing bind in both engines; the pause applies when Infiltrator binds a
Pokemon already behind a Substitute. The new path preserves that pause and
resumes on the turn the protection disappears, including when only release
remains. Fly and Dig do not prevent damage or pause the countdown.

Residual damage is one eighth of maximum HP, or one sixth with Binding Band,
rounded down with a minimum of one. The source reads the opponent's current
item each turn, so gaining or losing Binding Band changes the next tick.
Unnerve does not suppress this non-berry item. The source profile now follows
that live item rule. Native profiles retain their cached item behavior.

`build_binding_moves.py` verifies the ordered source gates, duration selection,
Grip Claw parameter, Substitute cleanup/Infiltrator interaction, live item
lookup and HP-fraction helpers using existing exact-blob fixtures. CI and the
production audit check its generated manifest. Fourteen battle groups cover
108 cases per profile: all three moves, both sides, both random durations and
Grip Claw; paused and resumed damage; the release boundary; Binding Band gained
and lost with and without Unnerve; repeated binds; Fly/Dig; normal Substitute
creation/blocking; and native controls for optional binding moves, doubles and
Terastal. Countdown, damage-pause and item-change tests reproduced inherited
failures before the correction.

This closes the selected countdown and residual-damage scope. Initial hit and
secondary-effect ordering, all immunity interactions, broader switching/Baton
Pass cleanup and source AI estimates remain separate audits. Native messages
and animations remain presentation gaps, including source suppression of the
residual animation during Fly/Dig. Optional item suppression and Gastro Acid
remain engine extensions. Counters already created retain their stored values
if an optional gimmick starts later; no retroactive conversion is introduced.

Validation with Arm GNU GCC 13.2.1: both production ROMs build. All fourteen
focused groups (108 cases per profile), all 191 earlier source battle groups
and six AI timing checks pass, along with 63 source checks and 12 ARM compiler
probes. The full native suite passes 5,554 tests, with nine unchanged expected
failures and 585 existing TODOs (6,148 total), and no unexpected failures.
The AI timing ceilings remain unchanged.

The production-linked audit matches all 255 basic move/PP records, 18 target
stat-drop records, 254 Sleep Talk eligibility entries and 43,792 teaching
combinations. The 109 recorded machine/lesson gaps are unchanged, with no new
or changed findings.

The same final Johto ROM passes all 12 demo checks, 101 campaign checks and
19 directed warp traversals, including all starter branches, Sprout Tower,
Falkner, the badge/Roost/Togepi rewards, resident schedules and Save/reset/
Continue. The authentic source opening remains separate from this demo gate.

Johto ROM SHA-256:
`6617659f5337e391e876961d19030979862689656dd73e295db997d6d8c3b473`.


## Source Teleport in ordinary wild battles

The pinned `BattleCommand_teleport` distinguishes wild escape from trainer
switching. In ordinary wild battles it ends the encounter on either side,
without checking levels or requiring another living party Pokemon. Expansion
instead used its trainer-style switch path for the player under the default
modern preset, so Teleport switched to a reserve or failed without one. The
source profile now routes ordinary wild singles to the existing flee path.

The source checks unsuppressed Run Away first, then the opponent's Mean Look
flag, then source trapping-ability eligibility. Wrap does not prevent Teleport.
Smoke Ball and Shed Shell do not override these Teleport checks. Neutralizing
Gas suppresses Run Away, mutually eligible trapping abilities cancel, and a
Mean Look already in effect still blocks escape after a later Ghost-type
change. Both sides now use that ordered eligibility check. Existing move
cancelers, party updates and native teleport outcome handling remain in place.

Trainer Teleport still switches through source move/ability traps when a
living replacement exists, without passing Baton Pass state. Optional legacy
trainer generation presets retain their native selection. Doubles, link and
special encounter formats, active gimmicks and optional Ingrain/Fairy Lock/
Commander/Sky Drop restrictions retain native execution. Ordinary recordings
use the same source rule. No identifiers, save fields or capacities change.

`build_teleport.py` verifies the ordered wild gates, excluded item/Wrap checks,
trainer distinction, source suppression/mutual trapping and special-type
boundary. The new Teleport fixture is pinned to its exact Git blob alongside
existing source fixtures. The generator also imports the original two-line
move description into the source profile; native descriptions remain intact.
CI and the production audit verify the generated manifest and description.

Thirteen groups cover 102 cases per profile: level and reserve variations;
both native wild generation presets; Mean Look/Wrap with Run Away, Smoke Ball
and Shed Shell; type-sensitive ability traps and mutual cancellation;
Neutralizing Gas; an existing trap after a live Ghost-type change; sleep,
paralysis and Taunt; living/fainted/missing trainer reserves; trainer traps;
and native controls for doubles, player Terastal and optional field effects.
They also check the live move-description string. The initial wild player
cases reproduced the inherited failure before the correction.

Wild opponents use the actual random-move controller in this test harness.
Each focused wild opponent therefore has one available move, with the player
providing setup and follow-up actions. Continued combat after failed escape
is checked through a subsequent player action. Recorded-test teardown itself
sets a teleport outcome, so outcome values alone are not used to prove that a
player successfully escaped.

This closes ordinary wild Teleport execution, not all source escape rules.
The source's TRAP/FORCEITEM/NEVER_SHINY/LEGENDARY encounter prohibition still
needs complete encounter-type mapping. Native special flags and the optional
no-running guard are preserved. Source AI choices, general fleeing, Pursuit
and full switch/party persistence timing, source flee presentation and other
special cases remain separate. Trick-or-Treat only supplies the optional live
type change used by one regression; its whole behavior is not certified.

Validation with Arm GNU GCC 13.2.1: both production ROMs build. All thirteen
focused groups (102 cases per profile), all 205 earlier source battle groups
and six AI timing checks pass, along with 64 source checks and 12 ARM compiler
probes. The full native suite passes 5,567 tests, with nine unchanged expected
failures and 585 existing TODOs (6,161 total), and no unexpected failures.
The AI timing ceilings remain unchanged.

The production-linked audit matches all 255 basic move/PP records, 18 target
stat-drop records, 254 Sleep Talk eligibility entries and 43,792 teaching
combinations. The 109 recorded machine/lesson gaps are unchanged, with no new
or changed findings.

The same final Johto ROM passes all 12 demo checks, 101 campaign checks and
19 directed warp traversals, including all starter branches, Sprout Tower,
Falkner, the badge/Roost/Togepi rewards, resident schedules and Save/reset/
Continue. The authentic source opening remains separate from this demo gate.

Johto ROM SHA-256:
`c7030ccab1ce66811797d6d473f42d1823e12f962c48a73754b11d507310426d`.


## Source Shed Shell and Ghost AI switch exemptions

The pinned `AI_UserCanSwitch` first requires another living party Pokemon,
then uses the same `UserCanSwitch` rule as player selection. Shed Shell is
checked before trapping abilities, Wrap and Mean Look. The default-build
Run Away exemption and a live Ghost-type check also precede those traps.
Unnerve only suppresses edible berries, so it does not disable Shed Shell.

Expansion's AI eligibility gate directly rejected binding and escape-prevention
state and trapping abilities. Unlike the normal player path, it did not honor
Shed Shell, and it rejected Ghost Pokemon still affected by binding moves.
The source profile now applies those two exemptions before the AI trap checks
in ordinary singles without active gimmicks. Existing switch strategy,
candidate selection and the living-reserve requirement remain in place.
This does not add these exemptions to general wild-flee predicates.

The gate reads current battle types and the current effective held item.
Gaining or losing Shed Shell through Trick, adding Ghost typing after Mean
Look, and removing Ghost typing while bound update the next eligibility check.
The item is not consumed and trap state is not cleared merely by considering
a switch. Native profiles, doubles, active gimmicks and optional Ingrain,
Fairy Lock, Sky Drop, Commander and Battle Arena restrictions retain the
native AI path. The previous Run Away adapter remains in place. No identifiers,
save fields or capacities change.

`build_switch_exemptions.py` verifies the shared player/AI call sites, gate
order, live type lookup, Shed Shell held-effect identity and the Unnerve berry
exclusion from existing exact-blob source fixtures. CI and the production
audit check its generated manifest.

Nine actual AI battle groups cover 39 cases per profile. They test Shed Shell
against Mean Look, Wrap, Arena Trap, Magnet Pull and Shadow Tag, with Smoke
Ball and no-item controls; two Ghost species under Fire Spin and Whirlpool;
item/type gains and losses; Unnerve and Neutralizing Gas; optional Klutz and
Embargo suppression; player Terastal and Fairy Lock controls; and absent or
fainted reserves. An imminent Perish Song faint prompts the existing AI to
switch. The tests distinguish a chosen switch from a replacement after
fainting and check the departing Pokemon's saved HP and held item. Seven
groups reproduced the inherited blocked-switch failures before the fix.

This closes the selected AI eligibility gap. Full source AI strategy, scoring
and estimates, player eligibility under every optional item-suppression case,
general fleeing, forced switching, Pursuit and full switch timing remain
separate audits. Optional moves sharing the permanent-trap field receive the
same scoped exemption without certifying those moves as source content.
Klutz, Embargo, Trick-or-Treat and Soak are selected native extension controls.

Validation: both production ROM builds, all 65 source checks and 12 ARM
compiler probes passed. The source profile passed 227 battle groups plus six
AI timing groups without changing timing ceilings. The full native suite
passed 5,576 groups (nine expected failures and 585 TODOs). After replacing a
deprecated test-only party alias, all nine new groups passed again in both
profiles with CI's deprecated/unused warning errors enabled. The production
ELF audit matched all 255 move/PP records, 18 secondary stat drops, 254
Sleep Talk eligibility entries and 43,792 teaching combinations; its 109
known gap entries had no new or changed entries. The same final Johto ROM
passed all 12 demo checks and 101 campaign checks, including 19 source warps,
Falkner's gym and the post-battle rewards. These results certify this scoped
eligibility change, not full battle or campaign parity.

Validated production ROM SHA-256:
`0c787965ebf8c403aae58c94433f09fcbff1701e04a2c5f93388673f1587c24f`.

## Source Roar protection and hidden-target hit gates

The pinned Roar script calls `checkhit` before priority, reflection and forced
switching. That hit check tests Protect before No Guard, tests either side's
No Guard before Fly/Dig, and tests Lock-On only after Fly/Dig. Its perfect-hit
exception follows those gates. Substitute does not block Roar.

Expansion's ordinary Roar bypassed Protect and let Lock-On bypass Fly/Dig.
The source profile now blocks Roar with Protect, including before Magic Bounce,
and rejects a hidden Fly/Dig target unless either side has effective No Guard.
The override applies only to Roar execution in singles without active gimmicks.
Native AI hit estimates, move/species/item identifiers, save fields, capacities,
party selection and the existing forced-switch machinery are unchanged.
Whirlwind is absent from the pinned source and retains its native behavior,
as do doubles, active gimmicks and optional protective effects.

`build_roar_hit_rules.py` verifies the source effect sequence and gate ordering,
Protect, Substitute and No Guard checks, and selected existing forced-switch
rules. The original Roar effect is added as an exact-blob fixture from the
existing source pin; no source version or variant changes. CI and the
production move/machine audit verify the generated manifest.

Nine emulator battle groups cover 63 cases per profile: both sides' Protect,
Substitute, Fly/Dig, No Guard and Lock-On interactions; reduced accuracy and
raised evasion; living/fainted/absent trainer replacements; Suction Cups and
Soundproof with Mold Breaker; Protect before Magic Bounce; optional Whirlwind,
Terastal on either side and doubles; and ordinary wild level comparisons with
and without a player reserve. Failed wild Roar is followed by a damaging move
to establish that combat continues, rather than relying only on the recorded
test teardown's outcome. The first two groups reproduced both inherited
hit-gate failures before the runtime correction.

This is a scoped hit-gate correction. Source special-encounter prohibitions,
the roaming opponent's level exemption, full reflected-move execution,
Prankster/Soundproof ordering (covered in the later Roar priority suite),
turn priority, deferred switch timing, candidate selection, ability activation
order, AI and source presentation remain separate audits. Ordinary wild level
and trainer reserve checks are regression coverage of matching native paths. The subsequent
Magic Bounce suite verifies that Roar reflects through Substitute as a source
sound exception and imports the general Substitute reflection gate.

Validation: both production ROM builds, all 66 source checks and 12 ARM
compiler probes passed. The source profile passed 236 battle groups plus six
AI timing groups without changing timing ceilings. The full native suite
passed 5,585 groups, including all nine new Roar groups (63 cases), with nine
expected failures and 585 existing TODOs; there were no unexpected failures.
The production ELF audit retained zero basic move-data differences and zero
teaching compatibility mismatches, with 109 known gaps and no new or changed
entries. The same final Johto ROM passed all 12 demo checks and 101 campaign
checks, including all 19 source warps, Falkner and the post-battle rewards.
These checks validate the scoped hit-gate correction, not full Roar, battle
or campaign parity.

Validated production ROM SHA-256:
`203964fd1a5fa2a42710a7341a3e0732831799fa32f68fd83b2074bfe116861a`.

## Source Magic Bounce Substitute eligibility

The pinned reflection command checks Protect failure and Fly/Dig before
Substitute. Ordinary moves cannot reflect through Substitute unless the
original user has Infiltrator, the move is one of the six explicit bypass
entries, or it is one of the ten source sound moves. Roar is a sound move:
its existing reflection through Substitute is correct, as the new battle
checks establish. Mold Breaker suppression and the no-recursion gate remain.

The earlier adapter covered only Mean Look. The source profile now applies
this reflection gate to all 255 mapped source moves in ordinary singles
without active gimmicks. Native reflection-pool flags still decide whether a
move is a reflection candidate; the following inventory suite certifies that static pool.
Source membership uses the pinned PP registry independently of whether the
PP override is enabled. The generated exception table preserves source move
identity mappings, including Disarm Voice. Optional Expansion moves, Magic
Coat, doubles and active gimmicks retain native behavior. No identifiers,
capacities or save fields change, and AI estimates remain untouched.

`build_magic_bounce.py` verifies the pinned reflection/Substitute command
ordering and imports the six explicit exceptions and ten sound entries. All
source inputs already have exact-blob fixtures at the existing source pin.
CI and the production move/machine audit verify both the generated table and
its 255-row identity manifest. The shared rule replaces the previous
Mean Look-specific reflection check.

Nine battle groups cover 78 cases per profile: both sides' Toxic, Thunder
Wave and Will-O-Wisp with/without Substitute and with Infiltrator/Mold Breaker;
Leer, Charm and Scary Face; all six explicit bypass entries; Growl, Screech,
Sing, Supersonic and Roar; Protect/Fly/Dig with No Guard; no recursive bounce;
reflection after breaking a live Substitute; and optional Spider Web, Magic
Coat, either-side Terastal and doubles. The status and stat-drop groups fail
on the unchanged parent runtime and pass with the adapter. Reflection
activation for Encore is checked even when its returned effect has no legal
last move; this is not certification of reflected Encore execution.

Static reflection-pool coverage is recorded below. Reflected-effect execution/accuracy, nested
ability-triggered moves, timing/priority/ability ordering and source
presentation remain separate audits. The existing native protect, hidden,
Mold Breaker and recursion paths are regression controls, not newly imported
implementations. Other Roar special-encounter and forced-switch gaps remain.

Validation: both production ROM builds, all 67 source checks and all 12 ARM
compiler probes passed. All 245 source battle groups and six AI timing groups
passed without relaxing timing ceilings. The full native suite passed 5,594
groups with nine expected failures and 585 existing TODOs; there were no
unexpected failures. All nine new groups (78 cases per profile) passed in
both source and native profiles, with CI's deprecated/unused warning errors
enabled for the new code and tests.

The production audit retains zero basic move-data differences and zero
teaching compatibility mismatches; its 109 known gap entries have no new or
changed findings. The same final Johto ROM passed all 12 demo checks and 101
campaign checks, including 19 directed warps, Falkner, the badge/Roost/Togepi
rewards and Save/reset/Continue. These results certify this scoped reflection
gate, not full Magic Bounce, battle or campaign parity.

Validated production ROM SHA-256:
`09f1a862fc2ca9960eea56613473323f336c602b2666109e80ef8b99e948a7df`.

## Source Magic Bounce reflection candidate inventory

The pinned default move table, 152 effect constants and indexed script
pointers now define an independent reflection inventory for all 255 mapped
moves. Exactly 31 moves select scripts containing `bounceback`; 224 do not.
The source Fly label aliases Dig and is resolved explicitly. The two newly imported pointer
and constant fixtures are verified against their GitHub blobs at the existing
source pin. PoisonPowder correctly maps to the target Poison Powder alias.

The existing compiled reflection accessor matches that source inventory, so
this suite retains the runtime. A generated test table checks all 255 flags,
and the production-linked move/machine audit now reports the same comparison.
Two new battle groups exercise all 31 candidates from either side, with and
without Mold Breaker (124 cases), plus six self/field/nonreflecting sound
controls from either side (12 cases). These checks run under both native and
source profiles through the existing Magic Bounce CI prefix.

This completes static reflection-candidate coverage and ordinary activation
coverage. Returned accuracy is audited in the following suite; other returned
effects, conditional pre-reflection gates, nested ability execution, timing,
AI and presentation remain separate work. Generation presets are not the specification: the source-derived
expectations fail if future flags drift. No production data, runtime, saved
identifiers, optional mechanics or table capacities change.

Validation: both production builds, all 67 source checks and all 12 ARM
compiler probes passed. The source profile passed 248 source-rule groups and
six AI timing groups without changing timing ceilings. The full native suite
passed 5,597 groups, with nine expected failures and 585 existing TODOs; no
unexpected failures occurred. The new inventory check and both new battle
groups passed in both profiles with deprecated/unused warning errors enabled.

The production-linked audit matches all 255 reflection flags (31 candidates),
retains zero audited move-data differences and zero teaching compatibility
mismatches, and records the same 109 known gaps with no new or changed entries.
The final Johto ROM passed all 12 demo checks and 101 campaign checks,
including 19 directed warps, Falkner, rewards and Save/reset/Continue.
Both production ROM hashes are unchanged from the previous suite.

Validated Johto ROM SHA-256:
`09f1a862fc2ca9960eea56613473323f336c602b2666109e80ef8b99e948a7df`.

## Source Magic Bounce returned-move accuracy

The pinned `bounceback` command sets `SUBSTATUS_IN_ABILITY` around `ResetTurn`;
`BattleCommand_checkhit` immediately returns on that flag before its accuracy
roll. The port previously rolled accuracy again for the reflected move. The
source profile now skips that returned roll for mapped source moves in singles
without active gimmicks, including status effects and the separate stat-change
accuracy path. The source builder verifies this command ordering against the
existing exact-blob fixtures.

An existing battle-only padding bit records whether reflection originated from
Magic Bounce or Magic Coat. It is assigned for each reflection and cleared on
reflection completion and ordinary action cleanup. Magic Coat, Expansion-only
moves, doubles, active gimmicks and native profiles retain their own accuracy
behavior. Source membership uses the pinned PP registry independently of the
PP override. The adjustment is in execution, outside the AI accuracy estimator;
no save layout, target identifier or capacity changes.

Ten battle groups cover 56 cases per profile: both sides' Toxic, Thunder Wave,
Will-O-Wisp and Sing; seven stat drops; Supersonic, Leech Seed and Foresight;
raised evasion and Bright Powder; original-user PP consumption and unchanged
reflector PP; ordinary accuracy after reflection; type/Insomnia immunities;
no reflection and Mold Breaker controls; optional Grass Whistle, Magic Coat,
either-side Terastal and doubles; and Magic Bounce precedence over Magic Coat.
The status, stat-drop and confusion groups reproduced missed returned moves
on the unchanged parent runtime.

This certifies the returned accuracy-roll bypass, not full Magic Bounce.
Original pre-reflection hit-check RNG consumption, source random-stream timing,
non-accuracy hit gates, full reflected effects, nested execution, priority,
AI and presentation remain separate audits. Existing gates and immune effects
are regression controls; this suite does not import all of those rules.

Validation: both production ROM builds, all 67 source checks and all 12 ARM
compiler probes passed. The full native suite passed 5,607 groups with nine
expected failures and 585 existing TODOs; there were no unexpected failures.
The source profile passed 258 source-rule groups and six AI timing groups
without relaxing timing ceilings. All ten new groups (56 cases per profile)
passed with deprecated/unused warning errors enabled. A separate ARM compile
probe confirms the battle-state structure remains 1,192 bytes.

The production-linked audit retains zero audited move-data differences,
255 matching reflection flags and zero teaching compatibility mismatches;
its 109 known gaps have no new or changed entries. The final Johto ROM passed
all 12 demo checks and 101 campaign checks, including 19 directed warps,
Falkner, badge/Roost/Togepi rewards and Save/reset/Continue.

Validated Johto ROM SHA-256:
`601fb85597bf9794ae9538d07e4cdc661c0a24a89c1eda2c6d570d8c083dbe97`.

## Source Roar Prankster and Soundproof ordering

The pinned priority table gives Roar priority -6; Prankster raises it to -5.
`checkpriority` checks positive-priority Armor Tail separately, then its
Prankster branch checks Dark typing and returns before Soundproof. A failed
Roar ends its effect script before `bounceback`. Thus Prankster Roar is blocked
by Dark targets despite its negative priority, while non-Dark Soundproof
targets do not block it. Ordinary users still respect Soundproof; Mold Breaker
still suppresses it and Suction Cups.

The previous native path required positive priority for Dark immunity, checked
Soundproof first and processed reflection before that immunity decision. The
source profile now checks Prankster/Dark immunity before reflection and bypasses
the Soundproof decision for non-Dark targets of Prankster Roar. Both adaptations
are in move execution, outside shared AI estimates. Scope is Normal-type Roar
in singles without active gimmicks. Dynamically retyped Roar, Whirlwind, doubles
and active gimmicks retain the tested native paths. No move IDs, save fields,
party capacities or move priority values change.

The new priority-table fixture is verified against its exact GitHub blob at
the existing source pin. `build_roar_hit_rules.py` verifies that table, the
Prankster increment, ordered immunity branches and the failure exit before
reflection. The existing CI Roar prefix and source/production audit include
these checks without adding a second source pin.

Ten battle groups cover 90 cases per profile: both sides; ordinary, Prankster
and Mold Breaker users against ordinary, Soundproof, Suction Cups and Armor
Tail targets with and without Dark typing; Dark immunity before Magic Bounce;
the actual reflector ability on a returned Roar; Fly/Dig reached with No Guard;
Protect and Substitute; ordinary wild continuation/escape outcomes; and native
Whirlwind, either-side Terastal, Electrify and doubles controls. The unchanged
parent failed the Dark-target, Soundproof and pre-reflection regression groups.

This closes these Roar immunity decisions and their ordering before reflection.
Source special-encounter bans, the roaming opponent's level exemption, broader
returned effects, deferred switch timing, candidate choice, full turn ordering,
AI and presentation remain separate audits. Other moves using `checkpriority`
and broader combinations with optional protective effects remain unaudited.

Validation: both production builds, all 67 source checks and all 12 ARM
compiler probes passed. The full native suite passed 5,617 groups, with nine
expected failures and 585 existing TODOs; no unexpected failures occurred.
The source profile passed 268 source-rule groups and six AI timing groups
without changing timing ceilings. All ten new groups (90 cases per profile)
passed with deprecated/unused warning errors enabled.

The production-linked audit retains zero audited move-data differences,
255 matching reflection flags and zero teaching compatibility mismatches;
its 109 known gaps have no new or changed entries. The same final Johto ROM
passed all 12 demo checks and 101 campaign checks, including 19 directed warps,
Falkner, badge/Roost/Togepi rewards and Save/reset/Continue.

Validated Johto ROM SHA-256:
`fb45d6078e956b443d0849c22e11955eccffc4640b3adef16f84b6670d659ab3`.

## Source Roar roaming level exemption

Source `roar.asm` compares the user and target levels, then exempts only the
opponent in `BATTLETYPE_ROAMING`. The player never receives that exemption.
The existing engine rejected every lower-level user. The source profile now
allows the roaming opponent's Roar through the final level gate in singles
without active gimmicks. Earlier hit/ability gates remain in force; a reflected
Roar uses its actual post-reflection user. Ordinary wild battles, trainer
switches, optional Whirlwind, doubles and active gimmicks retain their paths.
No move IDs, save layout, roster capacities or shared AI estimates change.

The pinned fixture check now validates the complete level/side/roaming branch
and the preceding side-dependent level swap. `ROAMING_BATTLE_TEST` adds the
real roamer flag to scripted wild battle tests; it does not replace the live
Roar command or claim flee-AI coverage. Six groups cover 56 cases per profile:
both sides and three level relations with/without a reserve; ability gates
including Mold Breaker and Prankster; Protect/Substitute; either-side Terastal
and Whirlwind controls; and both directions of Magic Bounce at unequal levels.
Failed escape cases execute a later damaging move to prove battle continuation.
That check exposed stale `targetsDone` state in the native reflection return:
the reflector's following attack played its animation but dealt no damage.
`MoveEndClearBits` now clears the reflector's target bookkeeping before
restoring the original attacker, in both profiles. Eight of these cases
cover both sides, failed Roar and successful Growl, and Magic Bounce/Magic Coat.
The unchanged runtime failed both the roaming-level and reflection-continuation
regressions. This cleanup does not change which moves can be reflected.

This closes the roaming opponent's Roar level exemption. Source special
encounter prohibitions, roaming encounter selection, flee AI, post-battle
roaming persistence, deferred switching, presentation and broader optional
interactions remain separate audits.

Validation: both production builds, all 67 source checks and all 12 ARM
compiler probes passed. The full native suite passed 5,623 groups, with nine
expected failures and 585 existing TODOs; no unexpected failures occurred.
The source profile passed 274 source-rule groups and six AI timing groups
without changing timing ceilings. All six new groups (56 cases per profile)
passed with deprecated/unused warning errors enabled.

The production-linked audit retains zero audited move-data differences,
255 matching reflection flags and zero teaching compatibility mismatches;
its 109 known gaps have no new or changed entries. The same final Johto ROM
passed all 12 demo checks and 101 campaign checks, including 19 directed warps,
Falkner, badge/Roost/Togepi rewards and Save/reset/Continue.

Validated Johto ROM SHA-256:
`f7b7e88a93cc10d1f3d133bc4d18246db957257843420bf890a8afdfd892b2f2`.

## Source sound-status Prankster ordering

The pinned `checkpriority` command checks positive-priority Armor Tail, then
branches on the user's actual ability. Its Prankster branch checks Dark typing
and returns before Soundproof. The source scripts for Growl, Sing, Supersonic
and Screech invoke that command before `bounceback`. Those four moves have
base priority zero, so Prankster raises them to +1; Armor Tail still blocks
them. This extends the earlier Roar execution correction to those four moves.

The existing native path blocked Prankster sound moves with Soundproof and
reflected them before applying Dark immunity. A generated five-entry table
now selects the source sound status moves with `checkhit`, `checkpriority`
and `bounceback` in that order, including the already-covered Roar. It is
derived from the pinned move category/type, sound list and effect scripts;
Perish Song and Heal Bell are excluded because their scripts use separate
handlers. The existing source priority-table fixture confirms the four new
moves use default priority zero. Existing fixture checks retain the exact
Prankster/Soundproof branch and failure-before-reflection evidence.

The runtime adapter remains in execution, outside shared AI estimates. Scope
is Normal-type source sound status moves in singles without active gimmicks.
Native Armor Tail and Mold Breaker decisions remain in use. No identifiers,
move priorities, save fields or engine capacities change.

Eight new battle groups cover 130 cases per profile: both sides, ordinary/
Prankster/Mold Breaker users, Dark and non-Dark Soundproof targets, Armor Tail,
Dark immunity before Magic Bounce, the actual reflector ability, Protect and
Substitute, optional Noble Roar, either-side Terastal, doubles and Ion Deluge.
The unchanged parent failed the Soundproof, pre-reflection Dark and Substitute
regression groups. The hidden Fly/Dig reflection gate was inspected and found
already implemented and covered by existing tests; it needed no code change.

This closes these four sound moves' Prankster Dark/Soundproof decisions and
pre-reflection ordering. Other source moves' priority decisions, broader
returned effects, conditional gates, full turn ordering, AI and presentation
remain separate audits. Dynamically retyped moves and optional formats retain
the tested native paths. Mixed Magic Coat interactions with these source
priority gates remain unaudited. CI includes the new source-test prefix.

Validation: both production builds, all 67 source checks and all 12 ARM
compiler probes passed. The full native suite passed 5,631 groups, with nine
expected failures and 585 existing TODOs; no unexpected failures occurred.
The source profile passed 282 source-rule groups and six AI timing groups
without changing timing ceilings. All eight new groups (130 cases per profile)
passed with deprecated/unused warning errors enabled.

The production-linked audit retains zero audited move-data differences,
255 matching reflection flags and zero teaching compatibility mismatches;
its 109 known gaps have no new or changed entries. The same final Johto ROM
passed all 12 demo checks and 101 campaign checks, including 19 directed warps,
Falkner, badge/Roost/Togepi rewards and Save/reset/Continue.

Validated Johto ROM SHA-256:
`e37d7b9a443dddb024277d40ec52d280035d657017860d915a03596a728ba420`.

## Source pre-reflection Prankster Dark immunity

Of the pinned source's 31 reflection candidates, 29 invoke `checkpriority`
before `bounceback`. The user's actual Prankster ability triggers Dark immunity,
and the later `failuretext` exits before reflection. Spikes and Toxic Spikes
omit this check. The preceding sound-status suite covered five candidates;
this suite extends the pre-reflection decision to the remaining 24 moves.

The generator derives eligibility and source types from the pinned move rows,
effect mapping and scripts, and checks the failure exit. A source-profile
execution adapter now applies Dark immunity before Magic Bounce for those
moves in singles, with matching source types and no active gimmicks. The new
24-move scope requires the target's actual Magic Bounce ability; optional
Magic Coat alone retains its existing ordering. The previous five sound
adapters keep their behavior. Shared AI estimates, identifiers, priorities,
save layout and Expansion capacities are unchanged.

Nine battle groups cover 184 cases per profile: all 24 moves on both sides
against Dark and non-Dark targets; PP consumption; returned status, stat,
seed and trapping effects; the reflector's actual ability; both source hazard
exceptions; and optional Noble Roar, Magic Coat, doubles, either-side Terastal
and dynamically retyped controls. Existing all-candidate ordinary-user and
Mold Breaker tests remain in the source suite. Two new groups failed against
the unchanged runtime and pass with this correction.

This closes pre-reflection Prankster Dark immunity for the 29 source candidates
within the stated scope. Full reflected-effect parity, other conditional gates,
pre-reflection accuracy RNG, type-immunity ordering, broader priority rules,
AI and presentation remain separate audits. Magic Coat combinations with the
five earlier sound adapters remain unaudited.

Validation: both production builds, all 67 source checks and all 12 ARM
compiler probes passed. The full native suite passed 5,640 groups, with nine
expected failures and 585 existing TODOs; no unexpected failures occurred.
The source profile passed 291 source-rule groups and six AI timing groups
without changing timing ceilings. All nine new groups (184 cases per profile)
passed with deprecated/unused warning errors enabled.

The production-linked audit retains zero audited move-data differences,
255 matching reflection flags and zero teaching compatibility mismatches;
its 109 known gaps have no new or changed entries. The same final Johto ROM
passed all 12 demo checks and 101 campaign checks, including 19 directed warps,
Falkner, badge/Roost/Togepi rewards and Save/reset/Continue.

Validated Johto ROM SHA-256:
`12aaf1ef1ee3bc58b4c2f7e463146c083b238ed696975d0594ac89ecc816c843`.

## Source Corrosion powder-immunity exception

The pinned `checkpowder` handler returns early for PoisonPowder or Toxic when
the actual attacker has Corrosion. Of the four source powder moves, only
PoisonPowder takes that branch. It skips the matchup checks for Safety Goggles,
either Grass type and Overcoat. Native Expansion retained those powder
immunities even with Corrosion, so the source profile now skips that blocker
for Poison-type PoisonPowder during execution in singles without active gimmicks.

The adapter uses the actual, suppression-aware attacker ability and leaves
shared AI estimates and the later status handlers unchanged. Magic Coat
returns, doubles, active gimmicks and dynamically retyped moves retain their
tested native behavior. No identifiers, save fields or capacities change.

The original powder list and Safeguard command are now authenticated fixtures.
The new verifier checks the exact Corrosion branch, powder matchup gates,
status-stage continuation and Safeguard exit. Source powder immunity records
a failed matchup but does not itself end the move; an otherwise eligible
Magic Bounce target still reflects it. That native reflection result already
matched, and the new tests retain it. Reflected moves use Magic Bounce's
actual ability, not the original user's Corrosion.

Ten battle groups cover 121 cases per profile: all four powders and all three
powder barriers on both sides, ordinary/Mold Breaker/Corrosion users, PP,
Protect, Safeguard, existing status, misses, both reflection directions,
ability suppression, Magic Coat, doubles, either-side Terastal and Electrify.
The unchanged parent failed the Corrosion bypass and ability-comparison groups;
all ten pass after the correction. CI includes the new verifier and test prefix.

This closes the powder-immunity stage only. Source Corrosion's later status-
immunity and Substitute bypasses, Toxic, Toxic Orb and damaging poison effects
remain separate audits, as do general status rules, the source random stream,
full returned-effect ordering, AI and presentation.

Validation: both production builds, all 68 source checks and all 12 ARM
compiler probes passed. The final full native suite passed 5,650 groups, with
nine expected failures and 585 existing TODOs; no unexpected failures occurred.
The final source profile passed 301 source-rule groups and six AI timing groups
without changing timing ceilings. All ten new groups (121 cases per profile)
passed with deprecated/unused warning errors enabled.

The production-linked audit retains zero audited move-data differences,
255 matching reflection flags and zero teaching compatibility mismatches;
its 109 known gaps have no new or changed entries. The same final Johto ROM
passed all 12 demo checks and 101 campaign checks, including 19 directed warps,
Falkner, badge/Roost/Togepi rewards and Save/reset/Continue.

Validated Johto ROM SHA-256:
`bd344bc31a03d1d6172bdec6741e86a339909c40ba90599fb10b62c2fa19aa97`.

## Source Corrosion Substitute bypass

The pinned source selects only PoisonPowder and Toxic for the direct-user
Corrosion branch in `CanStatusTarget`. That branch jumps past the status-stage
Substitute check. The earlier `checkhit` Substitute gate rejects Swagger and
trapping effects, so it does not stop these two moves. Both can poison the
owner of a Substitute without damaging or removing it. Protect, Safeguard,
misses and existing major status still prevent the tested applications.

Native execution blocked this interaction in two places: its move canceler
and `trynonvolatilestatus` handler. One execution-only eligibility helper now
bypasses both Substitute checks for Poison-type PoisonPowder/Toxic with the
actual, suppression-aware Corrosion user, in singles without active gimmicks.
The shared Substitute/AI helpers and reflection eligibility remain unchanged.
Magic Coat returns, doubles, active gimmicks and dynamically retyped moves
retain their tested native paths. No identifiers, save fields or capacities
change.

The distinction from reflection is deliberate: Corrosion does not bypass
Magic Bounce's separate Substitute gate. A protected Magic Bounce target
therefore receives poison without reflecting. A reflected move uses the
reflector's actual ability and cannot borrow the original user's Corrosion
to reach that user's Substitute. The native ability checks later in status
application remain outside this suite.

The Toxic command is now an authenticated fixture. A new source verifier
checks both effect scripts, the early Substitute gate, the exact Corrosion
branch, existing-status checks, Safeguard exit and separate reflection gate.
Eleven groups cover 98 cases per profile: both moves and sides; ordinary,
Corrosion, Infiltrator and Mold Breaker users; Poison/Steel targets; PP and
unchanged Substitute HP; Protect, Safeguard, existing status and misses;
both reflection directions; suppression; and optional move/format controls.
Three groups failed against the unchanged parent before the correction.
CI includes the new verifier and source-test prefix.

This closes the two moves' Corrosion Substitute bypass within the stated
scope. Corrosion's status-stage ability/item immunity bypasses, Toxic Orb,
damaging poison effects and their healing lifecycle remain separate audits,
as do general status rules, full RNG, AI decisions and presentation.

Validation: both production builds, all 69 source checks and all 12 ARM
compiler probes passed. The final full native suite passed 5,661 groups, with
nine expected failures and 585 existing TODOs; no unexpected failures occurred.
The final source profile passed 312 source-rule groups and six AI timing groups
without changing timing ceilings. All eleven new groups (98 cases per profile)
passed with deprecated/unused warning errors enabled.

The production-linked audit retains zero audited move-data differences,
255 matching reflection flags and zero teaching compatibility mismatches;
its 109 known gaps have no new or changed entries. The same final Johto ROM
passed all 12 demo checks and 101 campaign checks, including 19 directed warps,
Falkner, badge/Roost/Togepi rewards and Save/reset/Continue.

Validated Johto ROM SHA-256:
`2518fd04f160ddf198f581490338232f4637876f32eb52071d80545c7cef428f`.

## Source Corrosion status abilities and healing order

The pinned source's direct-user Corrosion branch for PoisonPowder and Toxic
also skips Immunity, Pastel Veil and Leaf Guard prevention. It applies and
displays poison, then runs post-status healing. Immunity and Pastel Veil cure
that poison before the next battler acts; Leaf Guard does not cure existing
status, so its owner stays poisoned even in sun.

Healing order matters: the source checks held status-healing items before
status-healing abilities. A Pecha or Lum Berry is consumed first, leaving
nothing for Immunity or Pastel Veil to cure. Native Expansion instead rejected
the initial status and, if allowed through, would run the ability cure before
the berry. The source profile now bypasses those three ability-prevention
checks during actual Toxic/PoisonPowder execution and offers the two healing
berry effects before the normal move-end immunity phase.

The adapter shares the earlier Corrosion status eligibility helper: actual,
suppression-aware Corrosion; Poison-type Toxic/PoisonPowder; singles; no active
gimmick; and no optional Magic Coat return. It does not change shared AI/status
estimates, identifiers, save fields or Expansion capacities. Protect,
Safeguard, accuracy and existing major status still gate application. Optional
ability barriers, other status moves and damaging poison effects keep their
native paths. The source defines HELD_PREVENT_POISON but assigns it to no live
item, so no speculative item mapping was introduced.

The new verifier authenticates the prevention branch, sun check, status
application, post-status order, healing ability table and routines, and Pecha/
Lum source item records. Twelve groups cover 198 cases per profile, including
both moves and sides, ordinary and Corrosion users, visible poison then cure,
PP, no residual damage after healing, berry consumption before ability healing,
Substitute, prevention gates, suppression and optional-mechanic controls.
Three groups failed against the unchanged parent. CI runs the new source
verifier and battle prefix.

This closes the scoped status-ability bypass and immediate healing sequence.
Toxic Orb, broader damaging-poison/hazard behavior, general status/Synchronize
rules, full RNG, AI decisions and source presentation remain separate audits.

Validation: both production builds, all 70 source checks and all 12 ARM
compiler probes passed. The final full native suite passed 5,673 groups, with
nine expected failures and 585 existing TODOs; no unexpected failures occurred.
The final source profile passed 324 source-rule groups and six AI timing groups
without changing timing ceilings. All twelve new groups (198 cases per profile)
passed with deprecated/unused warning errors enabled.

The production-linked audit retains zero audited move-data differences,
255 matching reflection flags and zero teaching compatibility mismatches;
its 109 known gaps have no new or changed entries. The same final Johto ROM
passed all 12 demo checks and 101 campaign checks, including 19 directed warps,
Falkner, badge/Roost/Togepi rewards and Save/reset/Continue.

Validated Johto ROM SHA-256:
`d598bdc4ceca3d16b991ba9534465239425aa5bcd7f75228d3d9fc9141e9d845`.

## Source status orbs and Safeguard

The pinned `HandleStatusOrbs` passes mode 2 into `CanStatusTarget`. That mode
explicitly skips Safeguard, Substitute and Mold Breaker checks. It still uses
the living holder's type, actual ability and existing status. Toxic Orb checks
the holder's own Corrosion for the Poison/Steel exception; an opposing
Corrosion user cannot supply it, and Corrosion does not bypass Fire immunity
to Flame Orb.

The port's ordinary status eligibility routine blocked these held items behind
Safeguard. The two orb handlers now use an explicit orb eligibility entry point
which requests the source Safeguard exception in singles without an active
gimmick on either battler. Both entry points share the same private status
checker, and ordinary move/ability/AI queries always retain the normal gate.
All other type, ability, existing-status and item-suppression checks remain.
No identifiers, save fields or Expansion capacities change.

The source runs status orbs after that turn's poison/burn damage and does not
consume the held item. The existing native activation timing and item retention
already match these cases. Tests verify no residual damage on the activation
turn, then damage on the following turn while the orb remains held. Damage
rounding and the complete end-turn sequence remain separate audits.

The verifier authenticates both orb item records, exact activation dispatch,
mode-2 Safeguard/Substitute/Mold Breaker gates, actual-holder Corrosion and
placement after poison/burn damage. Thirteen groups cover 136 cases per
profile: both orbs and sides, Safeguard controls, type and ability protections,
Corrosion ownership and suppression, existing status, Protect/Substitute,
item retention, first residual timing, doubles, either active Terastal,
optional item suppression and ordinary status-move controls. Five groups
failed against the unchanged parent. CI includes the new verifier and prefix.

This closes the scoped activation gates, item retention and first residual
turn. Complete item definitions/art, broader orb/status/Synchronize behavior,
end-turn ordering, damage rounding and presentation remain separate audits.
Doubles, active gimmicks and optional mechanics retain their native paths.

Validation: both production builds, all 71 source checks and all 12 ARM
compiler probes passed. The final full native suite passed 5,686 groups, with
nine expected failures and 585 existing TODOs; no unexpected failures occurred.
The final source profile passed 337 source-rule groups and six AI timing groups
without changing timing ceilings. All thirteen new groups (136 cases per profile)
passed with deprecated/unused warning errors enabled.

The production-linked audit retains zero audited move-data differences,
255 matching reflection flags and zero teaching compatibility mismatches;
its 109 known gaps have no new or changed entries. The same final Johto ROM
passed all 12 demo checks and 101 campaign checks, including 19 directed warps,
Falkner, badge/Roost/Togepi rewards and Save/reset/Continue.

Validated Johto ROM SHA-256:
`fea02657406b2e896f22378f364c5e59debff897ab273a1584ced8728b9eb30b`.

## Source damaging poison and Corrosion

The pinned source uses `EFFECT_POISON_HIT` for Poison Sting, Gunk Shot,
Poison Jab and Sludge Bomb. Their scripts apply damage, roll the secondary
chance and call `CanPoisonTarget` in direct-user mode. The Corrosion shortcut
accepts only `EFFECT_POISON` and `EFFECT_TOXIC`, so damaging poison moves still
check Poison/Steel immunity and the ordinary status protections.

Expansion allowed Corrosion to bypass those type checks for damaging moves.
The source profile now uses an authenticated, generated four-move membership
table in the additional-effect handler. During eligible poison effects, the
ordinary status query receives no Corrosion exception. The actual attacker
ability and shared AI/status estimates remain unchanged. Toxic and PoisonPowder
keep their already audited Corrosion behavior.

The adapter is limited to these four moves with Poison battle type in singles
without an active gimmick on either battler. Optional damaging poison moves,
dynamically retyped moves, doubles and active gimmicks retain native behavior.
No identifiers, save fields or Expansion capacities change.

Eleven groups cover 174 cases per profile: all four moves and both sides,
ordinary and Corrosion users, damage and PP, susceptible and Poison/Steel
victims, secondary-effect success/failure, Immunity, Pastel Veil, Leaf Guard
with and without sun, Protect, Safeguard, Substitute, existing status, misses,
ability suppression, poison-status controls and optional-mechanic boundaries.
The Corrosion/Poison-type group fails against the unchanged parent. The new
source verifier and battle prefix are wired into CI.

This closes the scoped damaging-poison Corrosion/type distinction and selected
protection gates. The source's 30% secondary records are authenticated; the
battle tests force both outcomes and do not certify the full source RNG.
General status/hazard behavior, damage rounding, complete post-hit ordering,
AI decisions and source presentation remain separate audits.

Validation: both production builds, all 72 source checks and all 12 ARM
compiler probes passed. The final full native suite passed 5,697 groups, with
nine expected failures and 585 existing TODOs; no unexpected failures occurred.
The final source profile passed 348 source-rule groups and six AI timing groups
without changing timing ceilings. All eleven new groups (174 cases per profile)
passed with deprecated/unused warning errors enabled.

The production-linked audit retains zero audited move-data differences,
255 matching reflection flags and zero teaching compatibility mismatches;
its 109 known gaps have no new or changed entries. The same final Johto ROM
passed all 12 demo checks and 101 campaign checks, including 19 directed warps,
Falkner, badge/Roost/Togepi rewards and Save/reset/Continue.

Validated Johto ROM SHA-256:
`fea2f96d56b51c6fca90cb6da69c04b469c510a86322b630603b58c5ec503089`.

## Source Heavy Boots and Toxic Spikes absorption

The pinned hazard entry routine checks airborne state, then the effective
held item, before dispatching either Spikes or Toxic Spikes. Heavy Boots
return before the Poison-type absorption branch. A grounded Poison-type
holder therefore leaves Toxic Spikes in place, including both layers. The
same gate applies to ordinary and forced entry.

Expansion absorbed Toxic Spikes before consulting Heavy-Duty Boots. The
source profile now checks effective boots before that absorption branch in
the switch-in execution path. It preserves the item, skips absorption and
advances the existing hazard counter normally. Shared hazard/AI queries,
identifiers, save fields and Expansion capacities remain unchanged. The
adapter is limited to singles without an active gimmick on either battler.

The new verifier authenticates the Heavy Boots held effect, common ordinary/
forced entry path, ordering before both hazard handlers and grounded Poison
absorption. An additional fixture from the same source pin authenticates the
two-layer Toxic Spikes cap. The source item maps to the existing target
`ITEM_HEAVY_DUTY_BOOTS` held effect; full source name, price, description, art
and inventory migration remain separate work.

Eleven battle groups cover 108 cases per profile: both sides and layers,
boots and no-item controls, later susceptible entrants, preserved layer counts,
stacked Spikes/Toxic Spikes in either order, ordinary and Steel-type targets,
Flying and Levitate, effective-item suppression, Knock Off and reentry,
Roar/Whirlwind, doubles and either active Terastal. Six groups failed against
the unchanged parent. CI includes the new source verifier and battle prefix.

This closes the scoped boots/absorption ordering gap. Full source hazard
grounding, forced-switch Mold Breaker interactions, status immunity, damage
rounding, entry-item ordering, AI decisions and presentation remain separate
audits. Doubles and active gimmicks retain native absorption; optional item
suppression/removal continues through the existing engine paths.

Validation: both production builds, all 73 source checks and all 12 ARM
compiler probes passed. The final full native suite passed 5,708 groups, with
nine expected failures and 585 existing TODOs; no unexpected failures occurred.
The final source profile passed 359 source-rule groups and six AI timing groups
without changing timing ceilings. All eleven new groups (108 cases per profile)
passed with deprecated/unused warning errors enabled.

The production-linked audit retains zero audited move-data differences,
255 matching reflection flags and zero teaching compatibility mismatches;
its 109 known gaps have no new or changed entries. The same final Johto ROM
passed all 12 demo checks and 101 campaign checks, including 19 directed warps,
Falkner, badge/Roost/Togepi rewards and Save/reset/Continue.

Validated Johto ROM SHA-256:
`060f80e5d4f806b68a0ac4de6a3ad12bf9d1c869fc8d348c405266c5afe7593d`.

## Source Synchronize and Substitute after direct status moves

The pinned `SynchronizeAbility` switches perspective to the actual holder,
then returns poison, bad poison, paralysis or burn through the ordinary status
commands. Those commands use `StatusTargetVerbose` mode zero, including
`CheckSubstituteOpp`. The original attacker's Substitute therefore blocks the
returned status. The reflector has Synchronize; it does not inherit the
original attacker's Infiltrator or Corrosion. Ability processing also skips
move-based sound exceptions in the source Substitute check.

Expansion's Synchronize eligibility query did not check Substitute. The source
profile now adds this gate at actual Synchronize activation for the six direct
source status moves: PoisonPowder, Stun Spore, Thunder Wave, Toxic, Glare and
Will-O-Wisp. A generated table authenticates their identities and source types.
The adapter requires their original battle type, singles, no active gimmick,
and no optional Magic Coat return. It uses the existing failed-activation
popup while keeping the substitute intact and preventing status application.
Shared status/AI queries, identifiers, save fields and capacities are unchanged.

Synchronize is checked before the holder's healing berry in the source. The
existing native sequence already matches the tested cases: the holder can
heal after the return attempt, while an original attacker protected by
Substitute keeps its own Lum Berry. Ordinary returns without Substitute still
apply, including bad poison from Toxic.

Eleven groups cover 178 cases per profile: all six moves and both sides,
with/without Substitute, PP and substitute HP, actual reflector ability,
original Infiltrator/Corrosion/Mold Breaker, substitutes on both sides,
both berry owners, ability suppression, doubles, either active Terastal,
retyped moves and out-of-scope status/damaging effects. Six groups failed
against the unchanged parent. CI includes the source verifier and battle prefix.

This closes the scoped Substitute return gate for direct source status moves.
Synchronize after damaging effects, contact abilities and other status sources,
full ordering, failure text, source animations, RNG and AI decisions remain
separate audits. Optional moves, retyped moves, Magic Coat returns, doubles and
active gimmicks retain native paths.

Validation: both production builds, all 74 source checks and all 12 ARM
compiler probes passed. The final full native suite passed 5,719 groups, with
nine expected failures and 585 existing TODOs; no unexpected failures occurred.
The final source profile passed 370 source-rule groups and six AI timing groups
without changing timing ceilings. All eleven new groups (178 cases per profile)
passed with deprecated/unused warning errors enabled.

The production-linked audit retains zero audited move-data differences,
255 matching reflection flags and zero teaching compatibility mismatches;
its 109 known gaps have no new or changed entries. The same final Johto ROM
passed all 12 demo checks and 101 campaign checks, including 19 directed warps,
Falkner, badge/Roost/Togepi rewards and Save/reset/Continue.

Validated Johto ROM SHA-256:
`4d0e075c94e2e39f544b021ac6305004310b48bb20568294602535d7edce18f3`.

## Source Synchronize and Substitute after damaging moves

The pinned secondary burn, poison and paralysis commands apply their status
through `StatusTarget` and `PostStatusWithSynchronize`. The return therefore
reuses the same direct-status Substitute check as the previous suite. A source
move-script audit identifies 21 damaging moves in this path; Tri Attack has
both burn and paralysis outcomes, while freeze does not trigger Synchronize.
The new authenticated Tri Attack fixture pins its three-way dispatch.

The source profile extends the existing activation gate with generated move,
original type and status membership. A Substitute protecting the original
attacker now blocks the return while leaving Synchronize's activation visible.
The actual reflector ability governs the return, so the original attacker's
Infiltrator, Corrosion or Mold Breaker cannot bypass its own Substitute.
The holder's healing berry still resolves after the return attempt; a protected
attacker retains its own berry.

The activation helper now receives the existing status-trigger context.
Poison Touch during the same source move remains a separate ability effect,
even for Poison Jab where both sources produce poison. Shared eligibility/AI
queries, save fields, identifiers and optional capacities are unchanged.
The adapter keeps its singles, original battle type and no-active-gimmick gates.

Twelve groups cover 234 cases per profile: every source move, both sides and
Substitute/no-Substitute controls, both reflectable Tri Attack outcomes and
freeze, original attacker abilities, outgoing Substitute/Infiltrator, berry
ownership, suppression, failed secondary rolls, misses, Protect, doubles,
either active Terastal, dynamic retyping and Poison Touch. The all-move group
failed on the unchanged runtime before the adapter was added. The previous
direct-status suite now uses three non-source damaging moves for its optional
move boundary. CI includes the new source verifier and battle prefix.

This closes the scoped Substitute return gate for source damaging status
moves. Full Synchronize behavior for contact abilities and other status sources,
secondary RNG, damage/recoil and status ordering, failure text, source animation
and AI remain separate audits. Optional moves, retyping, Magic Coat, doubles
and active gimmicks retain their existing paths.

An adjacent freeze gap surfaced during validation: applied freeze initializes
`freezeTurns` to two, while the action check thaws at two, even when the random
thaw roll fails. The freeze boundary test lets the holder act first to isolate
Synchronize. Source freeze duration/thaw handling needs its own correction and
regression suite; this change does not claim to resolve it.

Validation: both production builds, all 75 source checks and all 12 ARM
compiler probes passed. The final full native suite passed 5,731 groups, with
nine expected failures and 585 existing TODOs; no unexpected failures occurred.
The final source profile passed 382 source-rule groups and six AI timing groups
without changing timing ceilings. All twelve new groups (234 cases per profile)
passed with deprecated/unused warning errors enabled.

The production-linked audit retains zero audited move-data differences,
255 matching reflection flags and zero teaching compatibility mismatches;
its 109 known gaps have no new or changed entries. The same final Johto ROM
passed all 12 demo checks and 101 campaign checks, including 19 directed warps,
Falkner, badge/Roost/Togepi rewards and Save/reset/Continue.

Validated Johto ROM SHA-256:
`912dd2b46b1a19ab8625850ddcdb44a376d1ed14cace240e9e881c7c2e979f11`.

## Source natural freeze and thaw duration

The pinned frozen-action block checks Sacred Fire, Scald and Flare Blitz for
self-thaw, then compares one `BattleRandom` byte against `1 + (20 percent)`.
The source `percent` macro expands to integer multiplication by 255 divided by
100, so rolls 0 through 51 thaw and 52 through 255 do not: 52/256, rather than
a generic 20% or 25% preset. Failed checks prevent the action. There is no
freeze duration counter or forced-thaw limit in this complete source block.

The inherited engine initializes newly applied `freezeTurns` to two and also
uses two as its forced-thaw limit. A freshly frozen battler therefore thawed
on its next attempted action even when its random thaw roll failed. In the
source profile's ordinary singles scope, the natural action gate now uses
the authenticated byte threshold and ignores the native duration counter.
It neither advances that counter nor lets existing counter contents force a
thaw. Existing status clearing and party updates still run on a successful
thaw. No new status-origin metadata, save fields or identifiers are introduced.

The scope checks both actual opposing battlers, including when the frozen
battler chooses a self-targeted move. Doubles and either active gimmick keep
native natural-thaw behavior. This is a status rule independent of the move
that caused freezing: optional freezing moves in ordinary source-profile
singles use the same natural check. Their other effects remain unaudited.
Basic source self-thaw moves, optional Flame Wheel and berry cures retain
their existing paths.

Eleven battle groups cover 128 deterministic cases and two probability cases.
They include Ice Punch, Ice Beam, Blizzard and Tri Attack on both sides,
fresh freeze before and after the holder acts, PP preservation, successful
and failed rolls, five successive blocked actions, byte-boundary checks under
two generation presets, switching, Lum/Aspear cures, refreezing after a consumed
berry, self-thaw moves, doubles, either Terastal and optional freezing attacks.
The source probability cases enumerate all 256 byte values on each side.
Seven groups failed on the unchanged runtime; the four boundary/control
groups passed. CI includes the new authenticated verifier and battle prefix.

This resolves the natural-thaw portion of the previously recorded freeze gap.
The source RNG stream and full action-cancellation order are not ported here.
In particular, self-thaw versus confusion and other blockers, freeze versus
flinch/recharge/disobedience, incoming thaw attacks, freeze eligibility,
optional ability/item interactions and source presentation remain separate
audits. Native-profile battles retain their existing implementation.

Validation: both production builds, all 76 source checks and all 12 ARM
compiler probes passed. The final full native suite passed 5,742 groups, with
nine expected failures and 585 existing TODOs; no unexpected failures occurred.
The final source profile passed 393 source-rule groups and six AI timing groups
without changing timing ceilings. All eleven new groups passed with
deprecated/unused warning errors enabled:
128 deterministic cases and two probability cases. The source probability
checks exercised all 256 byte values on each side.

The production-linked audit retains zero audited move-data differences,
255 matching reflection flags and zero teaching compatibility mismatches;
its 109 known gaps have no new or changed entries. The same final Johto ROM
passed all 12 demo checks and 101 campaign checks, including 19 directed warps,
Falkner, badge/Roost/Togepi rewards and Save/reset/Continue.

Validated Johto ROM SHA-256:
`f18792c635a0887cd592bbc9b10e0f3204822bb49a77ac40fc2278da6324c384`.

## Source self-thaw before confusion, infatuation and Disable

The pinned frozen-action block recognizes Sacred Fire, Scald and Flare Blitz
by move identity. It calls `DefrostUser` before the source checks confusion,
infatuation and Disable. Thus a later blocker can prevent an attack, but the
battler remains thawed and keeps the move's PP. Recharge and effective flinch
occur earlier and prevent the self-thaw stage. The source move-identity check
has no battle-type restriction and does not use the natural-thaw random roll.

The engine previously deferred self-thaw until after these action blockers.
For the three authenticated source moves, ordinary source-profile singles now
clear freeze immediately before the Disable canceler, after flinch. The thaw
script uses the existing generic thaw message and status/party update. Execution
then resumes at the same canceler, so Disable still blocks its selected move.
Cleared freeze also prevents a second thaw at the original later stage.
No canceler IDs, save fields, move capacities or shared AI queries change.

The shared freeze scope checks both actual opposing battlers. Doubles and
either active gimmick keep native ordering. Optional self-thaw moves and
Frostbite also retain their existing paths. Normalize does not remove the
source move-identity exemption. Ordinary Fire and Water attacks receive no
new thaw exemption.

Twelve battle groups cover 128 cases per profile: all three moves and both
sides, successful and self-hitting confusion, acting and blocked infatuation,
Disable applied after legal move selection, PP, thaw/volatile animation order,
flinch, Inner Focus, Steadfast, combined flinch/confusion, optional thaw moves,
doubles, either Terastal, Normalize, non-thawing attacks, Frostbite and party
status persistence after switching. Five groups failed on the unchanged runtime;
seven control groups passed. CI includes the new source verifier and prefix.

This closes the self-thaw relationship to Disable, confusion and infatuation.
It does not reorder those blockers relative to each other or port the entire
source action sequence. Obedience, PP validation, Truant and other earlier
native cancelers, natural freeze versus flinch, source confusion probability
and damage, incoming thaw attacks, freeze eligibility, optional combinations
and original presentation remain separate audits.

Validation: both production builds, all 77 source checks and all 12 ARM
compiler probes passed. The final full native suite passed 5,754 groups, with
nine expected failures and 585 existing TODOs; no unexpected failures occurred.
The final source profile passed 405 source-rule groups and six AI timing groups
without changing timing ceilings. All twelve new groups (128 cases per profile)
passed with deprecated/unused warning errors enabled.

The production-linked audit retains zero audited move-data differences,
255 matching reflection flags and zero teaching compatibility mismatches;
its 109 known gaps have no new or changed entries. The same final Johto ROM
passed all 12 demo checks and 101 campaign checks, including 19 directed warps,
Falkner, badge/Roost/Togepi rewards and Save/reset/Continue.

Validated Johto ROM SHA-256:
`ad6559ba11ecd5dfb6fe10298bb9e85a9848dc0cba5afdc3df4c236e21884945`.

## Source move-effect flinch eligibility

Pinned source: Polished Crystal v3.2.3 at
`3fa43192379df5c3e7b09a08e4d5d79af4f02f42`, default release variant.
`build_flinch_hit.py` authenticates all thirteen default-source FlinchHit/Stomp
identities, their effect-script mappings and the complete `flinchtarget` gate.
After damage and the secondary-effect roll, this gate rejects Substitute,
sleep and freeze before checking turn order and effect failure. The status is
checked when the effect executes, before the target's own wake/thaw check.

The GBA execution adapter now rejects the mapped move-effect flinch against
sleeping or frozen targets in source-profile singles without either active
gimmick. Consequently a natural thaw, Sacred Fire/Scald/Flare Blitz self-thaw,
waking on the current action or Sleep Talk can proceed without a move-effect
flinch. A rejected flinch does not activate Steadfast. The adapter does not
change status timers, cancellation order, move/save IDs or table capacities.

This distinction matters for the preceding self-thaw suite: its two flinch
controls used Bite against an already frozen target, an inherited native
eligibility assumption. They now use Tackle with King's Rock to test an actual
source-eligible flinch. The authenticated item/Stench post-hit path calls
`FlinchTarget` directly and bypasses the move-effect sleep/freeze gate; the new
adapter does not impose that gate on it. Natural freeze versus effective
item/Stench flinch and sleep versus effective flinch remain separate ordering
gaps. No natural-freeze cancellation reorder is included here.

Twelve new battle groups cover 308 cases per profile: every mapped move on both
sides, successful/failed natural thaw, all three source self-thaw moves, waking
on the current action, Sleep Talk while still asleep, healthy targets, Inner
Focus, Shield Dust, Steadfast, other nonvolatile statuses, a subsequent healthy
turn, optional Fake Out, doubles, either Terastal, King's Rock, Razor Fang and
Stench. Five groups failed before the runtime change; seven control groups
passed. CI includes the source verifier and new battle prefix. The existing
self-thaw suite retains twelve groups and 128 cases with corrected flinch
setup.

This closes sleep/freeze eligibility for the thirteen source move effects.
It does not certify complete flinch eligibility, secondary-effect probability,
recharge clearing, Mold Breaker/ability suppression, item/Stench behavior,
AI, original presentation or the full source action sequence. Optional moves,
other statuses, doubles and either active gimmick keep their existing paths.

Validation: both production builds, all 78 source checks and all 12 ARM
compiler probes passed. The final full native suite passed 5,766 groups, with
nine expected failures and 585 existing TODOs; no unexpected failures occurred.
The final source profile passed 417 source-rule groups and six AI timing groups
without changing timing ceilings. All twelve new groups (308 cases per profile)
passed with deprecated/unused warning errors enabled. An initial native run
reached its 20-minute local outer limit after 5,535 passes with no reported
test failures; the complete rerun used eight workers and a 40-minute outer
limit. The battle tests and AI frame ceilings were unchanged.

The production-linked audit retains zero audited move-data differences,
255 matching reflection flags and zero teaching compatibility mismatches;
its 109 known gaps have no new or changed entries. The same final Johto ROM
passed all 12 demo checks and 101 campaign checks, including 19 directed warps,
Falkner, badge/Roost/Togepi rewards and Save/reset/Continue.

Validated Johto ROM SHA-256:
`727bc7fffabf6c54967aa0271c8b70a3a2afe6a42e5d0d7ff5508b224d052019`.

## Source effective flinch before frozen actions

Pinned source: Polished Crystal v3.2.3 at
`3fa43192379df5c3e7b09a08e4d5d79af4f02f42`, default release variant.
`build_frozen_flinch.py` authenticates the flinch/freeze ordering, the direct
item/Stench flinch path, the true-user ability query and Steadfast's Speed
selection. Effective flinch calls `CantMove` and `EndTurn` before the source
reaches either natural thaw or its three self-thaw moves. Thus the frozen
battler retains status and PP, while active Steadfast still raises Speed.

The source-profile frozen-battler branch now invokes the existing flinch
canceler before the sleep/freeze gate. This prevents both successful and failed
natural-thaw outcomes from replacing the source flinch response, and prevents
flinched attempts from reaching the freeze timer or random roll. The existing
flinch script handles cancellation and Steadfast once; a subsequent non-flinched
attempt still checks its own thaw roll or self-thaw move.

The source also rechecks `GetTrueUserAbility` when the target acts. This query
accounts for Neutralizing Gas but does not apply the previous attacker's Mold
Breaker bypass. The adapter uses the acting battler's active ability, clearing
a pending flinch if Inner Focus protects it. Clearing the flag also prevents
the later native flinch stage from blocking a naturally thawed or self-thawing
battler. Neutralizing Gas can still suppress Inner Focus and Steadfast.

Fourteen new battle groups cover 172 cases per profile: King's Rock, Razor Fang
and Stench on both sides; both natural-thaw outcomes; failed flinch rolls;
Steadfast and repeated flinches; the next non-flinched attempt; Inner Focus with
and without Mold Breaker; all three source self-thaw moves; Neutralizing Gas;
Shield Dust; confusion/infatuation; party status across switching; optional
Fake Out; doubles; either Terastal with a self-targeting move; and sleeping
target controls. Nine groups failed before the runtime change; five controls
passed. CI includes the new source verifier and battle prefix.

This closes effective-flinch precedence for frozen battlers in ordinary
source-profile singles without either active gimmick. Any effective optional
flinch producer in that scope inherits the frozen-status rule, while its own
move effect remains outside this parity claim. Doubles, either active gimmick
and sleeping targets keep their existing ordering. Healthy-battler action
ordering, recharge clearing, earlier native cancelers, full item/Stench
eligibility and probabilities, other suppression combinations, incoming thaw,
source RNG stream, AI and presentation remain separate audits. Stable move,
item, species and save identifiers and the engine's capacities are preserved.

Validation: both production builds, all 79 source checks and all 12 ARM
compiler probes passed. The final full native suite passed 5,780 groups, with
nine expected failures and 585 existing TODOs; no unexpected failures occurred.
The final source profile passed 431 source-rule groups and six AI timing groups
without changing timing ceilings. All fourteen new groups (172 cases per profile)
passed with deprecated/unused warning errors enabled.

The production-linked audit retains zero audited move-data differences,
255 matching reflection flags and zero teaching compatibility mismatches;
its 109 known gaps have no new or changed entries. The same final Johto ROM
passed all 12 demo checks and 101 campaign checks, including 19 directed warps,
Falkner, badge/Roost/Togepi rewards and Save/reset/Continue.

Validated Johto ROM SHA-256:
`4bf349bdf43f9ca9fa6644023ee148e22336b09107b0141509420d7500cf0410`.

## Source effective flinch before sleeping actions

Pinned source: Polished Crystal v3.2.3 at
`3fa43192379df5c3e7b09a08e4d5d79af4f02f42`, default release variant.
`build_sleeping_flinch.py` authenticates the complete source sleep-action block
and its position after effective flinch. It also verifies the preceding suite's
flinch block, active ability query and separate item/Stench path. Effective
flinch ends the action before either ordinary or Early Bird sleep decrement,
waking or Sleep Talk execution. Sleep status and move PP remain unchanged;
active Steadfast still raises Speed even when sleep would prevent acting.

The existing early-flinch adapter now handles sleeping as well as frozen
battlers in source-profile singles without either active gimmick. It rechecks
the acting battler's active Inner Focus after suppression, without the previous
attacker's Mold Breaker bypass. Inner Focus discards the pending flag and lets
ordinary sleep progression run; Neutralizing Gas can still suppress it or
Steadfast. A subsequent non-flinched attempt resumes the sleep check, while
repeated flinches preserve the timer. No saved fields, identifiers, capacities
or non-flinched sleep processing change.

Thirteen new battle groups cover 116 cases per profile: both sides, all three
item/Stench producers, waking and non-waking counters, Steadfast, Early Bird,
Sleep Talk and called-move PP, repeated flinches, the next non-flinched attempt,
failed flinch rolls, Inner Focus with and without Mold Breaker, Neutralizing
Gas, Shield Dust, optional Fake Out, doubles, either Terastal with a self-targeted
move, and party status after switching. Eight groups failed on the unchanged
runtime; five controls passed. The preceding frozen-flinch and move-effect
flinch suites' item/Stench and optional Fake Out sleep controls now expect the
newly imported ordering.
CI includes the authenticated source verifier and the new battle prefix.

This closes effective-flinch precedence for sleeping battlers in the stated
scope. It supersedes the preceding suite's recorded sleep-order gap. Sleep
application and duration, Rest, sleep clause, complete Sleep Talk selection,
full item/Stench eligibility and probability, healthy-battler ordering, recharge
clearing, earlier native cancelers, source RNG stream, AI and presentation
remain separate audits. Optional flinch producers inherit this general status
rule without certifying their own effects. Native-profile battles, doubles and
either active gimmick retain their existing order.

Validation: both production builds, all 80 source checks and all 12 ARM
compiler probes passed. The full native suite passed 5,793 groups, with nine
expected failures and 585 existing TODOs; no unexpected failures occurred.
The source profile passed 444 source-rule groups and six AI timing groups
without changing timing ceilings. All thirteen new groups (116 cases per
profile) passed with deprecated/unused warning errors enabled. A final native
focused run also passed the strengthened Sleep Talk animation assertions.

The production-linked audit retains zero audited move-data differences,
255 matching reflection flags and zero teaching compatibility mismatches;
its 109 known gaps have no new or changed entries. The same final Johto ROM
passed all 12 demo checks and 101 campaign checks, including 19 directed warps,
Falkner, badge/Roost/Togepi rewards and Save/reset/Continue.

Validated Johto ROM SHA-256:
`5da09c4390999936ba792b142a4ff6151aa651032b59bc5042b07b1110814b8a`.

## Source action-time Inner Focus recheck

Pinned source: Polished Crystal v3.2.3 at
`3fa43192379df5c3e7b09a08e4d5d79af4f02f42`, default release variant.
`build_flinch_focus.py` authenticates the source flinch block and true-user
ability query through the existing source verifiers. The block clears the
pending flag and checks active Inner Focus before `CantMove`, without any
nonvolatile-status prerequisite. The previous attacker's Mold Breaker does
not bypass this acting-battler check; Neutralizing Gas can still suppress it.

The flinch canceler now performs that recheck for any acting battler in ordinary
source-profile singles without either active gimmick. Protected flinch is
cleared before `CancelMultiTurnMoves`, so a charged move is not cancelled by
the rejected flinch. Later action blockers still apply. The earlier sleep/freeze
dispatch delegates to the same handler and continues its existing status checks
when Inner Focus rejects the flinch. Unprotected flinches retain cancellation
and Steadfast. No save fields, IDs, capacities or probabilities change.

Fifteen battle groups cover 94 cases per profile: all thirteen source flinch
moves on both sides, King's Rock and Razor Fang, successful/failed effect rolls,
ordinary Inner Focus protection, Steadfast, Neutralizing Gas and protection
after it switches out, burn/poison/paralysis, confusion/infatuation, charged
Solar Beam, the following turn, optional Fake Out, doubles and either Terastal
with a self-targeted move. Optional Gastro Acid and Ability Shield retain their
existing engine behavior as regression controls. Nine groups failed before the
runtime change; six controls passed. CI runs the authenticated source verifier
and the new battle prefix.

This completes the active Inner Focus recheck beyond the previously adapted
sleeping/frozen cases. It does not move healthy-battler flinch ahead of earlier
native cancelers, change flinch application/eligibility or recharge clearing,
port probabilities or the RNG stream, or certify full action order, AI or
presentation. Optional flinch producers inherit this general action check;
their own effects and optional item/ability parity remain separate audits.
Native-profile battles, doubles and either active gimmick keep native behavior.

Validation: both production builds, all 81 source checks and all 12 ARM
compiler probes passed. The full native suite passed 5,808 groups, with nine
expected failures and 585 existing TODOs; no unexpected failures occurred.
The source profile passed 459 source-rule groups and six AI timing groups
without changing timing ceilings. All fifteen new groups (94 cases per profile)
passed with deprecated/unused warning errors enabled.

The production-linked audit retains zero audited move-data differences,
255 matching reflection flags and zero teaching compatibility mismatches;
its 109 known gaps have no new or changed entries. The same final Johto ROM
passed all 12 demo checks and 101 campaign checks, including 19 directed warps,
Falkner, badge/Roost/Togepi rewards and Save/reset/Continue.

Validated Johto ROM SHA-256:
`0063daf63557f4b677b55af5ce4c7d14ad84695255f6a5c0792358c42587f048`.

## Source move-effect flinch clears recharge

Pinned source: Polished Crystal v3.2.3 at
`3fa43192379df5c3e7b09a08e4d5d79af4f02f42`, default release variant.
`build_flinch_recharge.py` authenticates all thirteen FlinchHit/Stomp mappings
and gates, `FlinchTarget`, `EndRechargeOpp`, the secondary-effect checks,
Hyper Beam/Giga Impact recharge scripts, both sides' locked move selection,
and the existing action-time Inner Focus rule.

For the thirteen mapped moves in ordinary source-profile singles without
either active gimmick, an eligible flinch now clears pending recharge before
the target's action check. It sets the pending flinch even if Inner Focus is
active: the source checks that ability only when the target acts. Unprotected
targets flinch instead of displaying recharge, and Steadfast activates. Active
Inner Focus permits the already selected move to proceed through later checks;
a repeated Hyper Beam or Giga Impact consumes PP and starts recharge again.
The engine's selected move slot, action and PP handlers are reused. No IDs,
save fields, capacities or probabilities change.

Fourteen battle groups cover 176 cases per profile: all thirteen moves against
both source recharge moves on both sides, Inner Focus, Mold Breaker, all four
move slots, Steadfast, Neutralizing Gas, failed secondary rolls and misses,
Shield Dust/Covert Cloak/Sheer Force, intact and broken Substitute, sleep/freeze,
targets that already acted, and subsequent actions. Doubles and either Terastal
are native controls. Item/Stench and optional Dragon Rush controls explicitly
retain their separate paths. Six groups fail with the parent runtime; eight
controls pass. CI includes the source verifier and battle prefix.

This closes move-effect recharge clearing for the mapped source moves. The
separate direct item/Stench path, including after-action behavior, remains open.
General recharge application, fainted targets, broader flinch eligibility,
probabilities/RNG stream, full cancellation order, AI and original presentation
remain separate audits. Optional recharge producers inherit this pending-state
adapter without certification of their effects. Optional flinch producers,
native-profile battles, doubles and either active gimmick retain native behavior.

Validation: both production builds, all 82 source checks and all 12 ARM
compiler probes passed. The full native suite passed 5,822 groups, with nine
expected failures and 585 existing TODOs; no unexpected failures occurred.
The source profile passed 473 source-rule groups and six AI timing groups
without changing timing ceilings. All fourteen new groups (176 cases per profile)
passed with deprecated/unused warning errors enabled.

The production-linked audit retains zero audited move-data differences,
255 matching reflection flags and zero teaching compatibility mismatches;
its 109 known gaps have no new or changed entries. The same final Johto ROM
passed all 12 demo checks and 101 campaign checks, including 19 directed warps,
Falkner, badge/Roost/Togepi rewards and Save/reset/Continue.

Validated Johto ROM SHA-256:
`0bd7cb6aaaa681a21cc1869f3d87376118f34281983500867e70c567029b4590`.

## Source item and Stench flinch clears recharge

Pinned source: Polished Crystal v3.2.3 at
`3fa43192379df5c3e7b09a08e4d5d79af4f02f42`, default release variant.
`build_proc_recharge.py` authenticates the held-item/Stench selection, complete
`.flinch_up` helper, both flinch-item attributes and end-turn flag clearing,
with the prior recharge and active Inner Focus verifiers as dependencies.
The direct helper calls `FlinchTarget` without the move-effect sleep/freeze or
acted-turn gates. Active Inner Focus is checked later when the target acts.

Successful existing King's Rock, Razor Fang and Stench procs now clear pending
recharge in ordinary source-profile singles without either active gimmick.
An explicit transient origin flag marks only those two producer call sites;
optional Fling and intrinsic move effects retain their separate paths. The
adapter runs after inherited effect eligibility gates, sets flinch and clears
recharge even if the target already acted. Pending actions retain the existing
Inner Focus, Steadfast, sleep/freeze, move-slot and PP behavior. After an already
completed action, the target can choose again next turn without a stale flinch.
No save fields, stable IDs, capacities or probabilities change.

Sixteen new groups cover 272 cases per profile: both items and Stench, both
source recharge moves on both sides, before/after-action timing, Inner Focus,
Steadfast, Mold Breaker, Neutralizing Gas, all four move slots, frozen/asleep
targets, missed hits and failed rolls, no-damage moves, all thirteen intrinsic
source flinch moves, Substitute, Shield Dust, Covert Cloak, Sheer Force,
optional Fling, doubles and either Terastal. Eight groups fail with the parent
runtime and eight controls pass. The previous move-effect suite's item/Stench
control is updated to assert this source adapter. CI runs the new verifier
and battle prefix.

This supersedes the pending item/Stench recharge-clearing gap in the previous
suite for successful inherited procs. It does not certify full proc eligibility
or chance/RNG behavior. Covert Cloak, broken Substitute, Sheer Force, attacker
fainting to Rocky Helmet, multi-hit attacks and optional interactions still
need their own source audits. General recharge application, full cancellation
order, AI and original presentation remain open. Native-profile battles,
doubles and either active gimmick retain native behavior. Optional recharge
and damage moves inherit the adapter without certification of their effects.

Validation: both production builds, all 83 source checks and all 12 ARM
compiler probes passed. The full native suite passed 5,838 groups, with nine
expected failures and 585 existing TODOs; no unexpected failures occurred.
The source profile passed 489 source-rule groups and six AI timing groups
without changing timing ceilings. All sixteen new groups (272 cases per profile)
passed with deprecated/unused warning errors enabled.

The production-linked audit retains zero audited move-data differences,
255 matching reflection flags and zero teaching compatibility mismatches;
its 109 known gaps have no new or changed entries. The same final Johto ROM
passed all 12 demo checks and 101 campaign checks, including 19 directed warps,
Falkner, badge/Roost/Togepi rewards and Save/reset/Continue.

Validated Johto ROM SHA-256:
`4e7d7599376605d29833c6c20828beebfaddc05fcea3b939dbc198eb9599959d`.

## Source flinch proc blockers: Covert Cloak and Sheer Force

Pinned source: Polished Crystal v3.2.3 at
`3fa43192379df5c3e7b09a08e4d5d79af4f02f42`, default release variant.
`build_proc_blockers.py` authenticates the complete direct flinch helper via
the previous verifiers, its post-hit entry gates, the separate end-move Sheer
Force check, Covert Cloak's source attribute and the Thunderbolt/Ice Beam/
Flamethrower scripts. Source `effectchance` rejects Covert Cloak and Sheer
Force; the later direct held-item/Stench flinch helper only checks Shield Dust
after Mold Breaker, fainted targets and intrinsic flinch-command exclusion.

The explicit held-item/Stench origin marker now selects those source blocker
rules in ordinary source-profile singles without either active gimmick.
Covert Cloak and Sheer Force no longer reject these direct flinch procs.
Shield Dust still blocks them and is recorded through the existing handler;
Mold Breaker can bypass it. Active Inner Focus remains an action-time recheck.
The same source scope helper is reused for the prior recharge adapter, so the
newly eligible procs also clear recharge before or after the target acts.
Intrinsic move effects and optional Fling retain their own checks. No IDs,
save fields, capacities, probabilities or move damage calculations change.

Fourteen new groups cover 242 cases per profile: both flinch items and Stench,
Covert Cloak, Sheer Force with three intrinsic-status moves and a no-item
control, Shield Dust, Mold Breaker, Inner Focus, Steadfast, both source recharge
moves, both turn orders, failed rolls/misses, all thirteen intrinsic source
flinch moves, other secondary effects, intact/broken Substitute, optional Fling,
doubles and either Terastal. Six groups fail against the parent runtime and
eight controls pass. The previous proc-recharge blocker control now asserts
these source rules. CI includes the new verifier and battle prefix.

This supersedes the Covert Cloak and Sheer Force proc-blocker gaps recorded in
the previous suite. Full proc eligibility is still not certified: broken
Substitute, attacker fainting to Rocky Helmet, multi-hit attacks and other
item/ability combinations remain separate work. Chance/RNG behavior, general
recharge application, complete action order, AI and original presentation
remain open. Native-profile battles, doubles and either active gimmick retain
native behavior. Optional damage/recharge moves inherit the scoped adapter
without certification of their own effects.

Validation: both production builds, all 84 source checks and all 12 ARM
compiler probes passed. The full native suite passed 5,852 groups, with nine
expected failures and 585 existing TODOs; no unexpected failures occurred.
The source profile passed 503 source-rule groups and six AI timing groups
without changing timing ceilings. All fourteen new groups (242 cases per profile)
passed with deprecated/unused warning errors enabled.

The production-linked audit retains zero audited move-data differences,
255 matching reflection flags and zero teaching compatibility mismatches;
its 109 known gaps have no new or changed entries. The same final Johto ROM
passed all 12 demo checks and 101 campaign checks, including 19 directed warps,
Falkner, badge/Roost/Togepi rewards and Save/reset/Continue.

Validated Johto ROM SHA-256:
`bcd844ea7b977bb57facbd1ce58aca6e4e4ecaafab2a9837a6716dfaacbac016`.

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
