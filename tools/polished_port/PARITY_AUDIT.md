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
| Species/form identities | Native numeric IDs do not equal source IDs or form values. Thirty source forms previously lacked target IDs. | **Registered:** all 391 identities now resolve. Thirty forms have distinct appended IDs 1573..1602, with base/form-table round trips. Their original artwork, metadata and evolution rules are still pending. |
| Custom move identities | Fury Strikes, HealingLight and Fresh Snack do not exist as separate Expansion moves. | **Added:** distinct ordinary move IDs and generated source names, descriptions and basic data. Native battle adapters implement source multi-hit probability and healing fractions/rounding. Original battle animations and Fresh Snack's field use remain open. |
| Other move data and effects | `src/data/moves_info.h` still supplies most move definitions; battle configuration selects many latest-generation rules. | Import all source move data/flags and audit effect implementations, damage/stat rules, status, weather and RNG. A matching move name is insufficient. |
| Species stats and related data | Native generation presets previously supplied these values. | **Imported:** all 391 base-data mappings provide default-source stats, types, three ability slots, growth, catch/EXP/EV data, gender, egg groups/cycles and initial happiness. Gameplay uses `GetSpeciesBaseData`; additional Expansion species retain native data. Held items, Pokedex/presentation data, ability effects and battle/creation rules remain open. |
| Evolution conditions, breeding and teaching | Native evolution tables, egg moves, hatch-step timing, breeding rules, TM/tutor compatibility and item mappings remain. Source egg groups and initial cycle counts are imported. | Import source conditions, egg/TM/tutor data and mechanics; validate all related item-to-move mappings. Importing an evolution move does not import its evolution condition. Existing Roost/Flash reward tests prove only those selected paths. |
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

The new forms deliberately retain documented temporary native art, Pokedex
metadata and evolution rules. Source held-item rows and teaching lists are
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

## Next priorities

1. Import evolution conditions, egg moves, teaching compatibility and held-item
   data/probabilities. Complete breeding/hatching behavior beyond base data.
2. Import remaining move data and implement a source battle profile, including
   trainer creation and AI. Validate source probabilities and rounding directly.
3. Import Pokemon graphics and complete the associated sprite metadata.
4. Establish the real source startup and rerun the existing campaign from its
   authentic initial state; continue correcting field/UI/audio gaps and the
   remaining campaign with the same source-based completion criteria.

Expansion's extra content and capacities remain available for later hacking.
Unused optional content is not a parity defect; using it in place of Polished
Crystal data or behavior in the playable baseline is.
