# Whole battle parity suite

The current catalog contains 1,095 source unit/battle groups and six AI timing
groups. Run every registered group:

```sh
python3 tools/polished_port/scripts/run_battle_suite.py
```

Use `--native-regression` to also run the full native suite, `--check` for
inventory validation without emulation, or `--strict-completeness` to require
complete behavioral parity. The latter currently exits **2**, deliberately.
`--output`, `--jobs`, `--timeout` and `--driver` have the same meaning as in the
previous attack runner, which remains supported.

CI uploads `polished-battle-suite` with `battle-suite.md`, `battle-suite.json`,
the compatible `attack-suite.json`, exact test names, execution counts and logs.
A green regression run means its scenarios passed; it does not certify the
remaining battle engine. Failed execution, timeouts, missing summaries, skipped
source groups and expected source failures are rejected.

## Coverage and remaining work

The source archive contains **342 files authenticated back to the pinned Git
commit**. It includes every assembly file under the battle engine/data,
trainer/facility data, item/Pokemon engines, shared home routines, constants and
macros. Missing or edited blobs/trees fail authentication. The generated ledger
records **4,143 global lexical blocks**, their exact source hashes, local-label
and branch-site counts, **255 moves, 133 effects and 129 command names**, every
source ability/held-effect identifier and all registered source tests.

This is a conservative inventory, not an interpreter or control-flow proof.
It includes data labels, presentation routines and alternate conditional
branches. A block awaiting review is not necessarily a confirmed bug. External
field scripts and transitive dependencies still require end-to-end evidence.
The default non-FAITHFUL release remains the semantic authority.

| Domain | Remaining parity evidence |
| --- | --- |
| Setup | Trainer/wild creation, special opponents, battle types and saved state |
| Actions | Ordering, ties, obedience, PP, cancellation and RNG call order |
| Damage | Full formula, modifiers, types, criticals, accuracy, rounding and saturation |
| Moves | Every branch and interaction of all 133 effects |
| Status | Application/cure/reflection, volatile state and self-confusion damage |
| Abilities | Every trigger, suppression, copy and interaction branch |
| Items | Held/bag effects, consumption, removal, recovery and persistence |
| Field | Weather/screens, residuals, timers and complete end-turn ordering |
| Switching | Traps, forced/pivot switches, hazards and state transfer/reset |
| Fainting | Contact/recoil/residual order, simultaneous KOs and outcomes |
| Capture/escape | Ball arithmetic, restrictions, delivery, fleeing and roamers |
| Rewards | EXP, growth, money, happiness, learning and evolution |
| AI | Source choices, heuristics, items, switching and permitted information |
| Facility/link | Tower generation/restrictions and link-specific behavior |
| Presentation/RNG | Exact stream, messages, animations and observable sequence |

All fifteen domains remain open. Existing manifests and passing scenarios are
retained as scoped evidence; no names, inventory totals, native tests or AI
performance checks automatically close a source block/domain.

## Foundation corrections

Fifteen new parameterized battle groups verify source confusion self-hit odds
(85/256), full paralysis (64/256), half-Speed paralysis with Quick Feet's
exception, uniform direct-sleep counters 2–4, Ice physical Defense in Hail and
Rock special Defense in Sandstorm. Source rules are independent of native
generation presets in ordinary singles without either active gimmick.

Tests cover both sides, PP loss, counter decrement/waking, Early Bird, four
sleep moves, physical/special categories, Psystrike, Cloud Nine, Utility
Umbrella, odd Defense rounding, and native controls for doubles and either
Terastal participant. Effect Spore's separately ported 1–3 initial counters
and Rest's dedicated path remain separate. Optional attacks use these shared
status/weather rules inside the source battle profile. Exact RNG stream,
complete damage/action/event ordering remain open; the following suite adds scoped self-confusion damage evidence.

## Shared battle calculation corrections

Twenty-six additional groups cover source confusion arithmetic, Speed order,
critical chances, default affection and weather. The confusion reference includes
8,112 generated boundary vectors, all 169 stage pairs in the damage entry point,
and live cases on both sides. It checks simultaneous raw-stat compression,
numerator-stage rounding and the source saturation value; ability/item/burn
modifiers and survival clamps cannot enter the source self-hit calculation.
Berry consumption and full faint sequencing remain open.

The Speed matrix covers 25,480 combinations of raw stats, stages, paralysis,
abilities, items and weather, plus live action-order checks. Tests also cover
fainted raw Speed, the 65,535 cap, native badge-boost exclusion and Quick Powder
using the copied species after Transform. These checks do not establish all
priority, tie-breaking or AI decision rules.

Critical tests exercise source stages under native generation 3/6/9 settings,
all nine high-critical moves, armor/Mold Breaker, Leek species/form eligibility,
zero-power Return, both sides, and native doubles/Terastal controls. Ordinary
source singles suppress affection bonuses as required by the pinned default
options. Wild-battle tests keep the native affection path active as a control;
scripted singles otherwise disable it through their recorded-link flags.
Optional affection settings/thresholds and external Future Sight remain open.

Weather checks cover all four source weather abilities, both sides, older and
modern native settings, five/eight-turn expiry boundaries, Snow Warning's Hail,
Slush Rush immunity, Cloud Nine/Goggles, overworld replacement and same-weather
no-refresh. The suite does not certify complete weather/ability/residual event
order. Optional formats and gimmicks retain native behavior.

The archive additionally authenticates math internals, default options, high
critical move data and HRAM definitions. `battle_core.json` records the exact
source paths and limits. All fifteen domain-level obligations remain open.

## Shared accuracy and action ordering

Thirty-three additional groups cover the raw accuracy and priority records for
all 255 mapped moves. The 2,028 generated stage vectors drive 43,433 calls through
the runtime accuracy calculation, including the sun-specific Thunder/Hurricane
value. A separate 1,248-case matrix checks flooring between ability/item stages;
1,020 priority comparisons cover both sides and Prankster.

The source uses exact three-based accuracy ratios. Keen Eye, Illuminate and
Mind's Eye preserve negative evasion; Wonder Skin sets checked status accuracy
to 50 before stages. Weather accuracy checks use the target's Utility Umbrella.
The pinned `50 percent + 1` expression evaluates to **128**, because this source
macro still uses 255; the suite deliberately preserves that executable value.
Raw 255 is scaled unless the source effect explicitly bypasses the hit check.
Zoom Lens recognizes a prior switch, and Lock-On cannot bypass Fly/Dig.

Quick Draw/Claw and Custap activate only within equal priority brackets. Unnerve
blocks Custap, Quick Draw overrides its holder's Lagging Tail, and pure Speed
comparisons ignore move-only quick/lagging flags. Tests include odd HP thresholds,
Gluttony, both sides, suppression, immunity/protection, native profiles, optional
moves, doubles and either active Terastal. Shared quick-item rules also apply
to optional attacks in ordinary source singles; unmapped move accuracy/priority
definitions retain their prior rules.

These scenarios do not certify full damage, action, cancellation, residual or
faint sequencing. Lock-On lifetime, external Future Sight, exact RNG stream,
source AI choices, capture/rewards and facility/link behavior remain open. The
fifteen domain-level obligations and strict completion failure are retained.

Regenerate the ledger after adding tests:

```sh
python3 tools/polished_port/scripts/build_battle_foundations.py
python3 tools/polished_port/scripts/build_battle_core.py
python3 tools/polished_port/scripts/build_accuracy_order.py
python3 tools/polished_port/scripts/build_damage_pipeline.py
python3 tools/polished_port/scripts/build_battle_suite.py
python3 tools/polished_port/scripts/test_battle_suite.py
```

To reproduce the authenticated archive, supply a checkout containing the exact
pinned commit to `build_battle_suite.py --import-source /path/to/source`.
Import reads immutable Git objects, not checkout files. CI needs no source
network fetch. The negative tests reject deleted/corrupt source objects and
false completion claims even when regression execution is marked passed.

## Shared damage pipeline

The shared damage adapter now routes 168 source attacks through the pinned
`damagestats`, `damagecalc`, `stab` and `damagevariation` arithmetic. It reduces
raw Attack/Defense together, applies stat stages to the numerator, preserves
ordered integer ability/item/burn/critical operations and the source saturation
boundaries, and combines STAB/type/weather before rolling damage. Reflection,
fixed-damage and other special effect paths retain their existing paths.

Twenty-six groups add 10,816 generated arithmetic vectors, exercised from both
sides in both attack categories (43,264 runtime comparisons), plus live damage
rolls, screens, Facade, Acrobatics, Hex, species items and native-extension
controls. The before/after check reproduced four failures on the parent runtime.
Source punching, slicing, launcher, Sheer Force and Reckless memberships are
derived from the pinned lists and scripts. In particular, Reckless does not
boost this release's separate Flare Blitz effect.

Conditional boosts preserve their script position: Acrobatics, Avalanche, Hex,
Venoshock, Facade and Rollout act before the roll; Pursuit, Knock Off and the
Fly/Dig/Minimize multipliers act afterward. Metal Powder uses the original
Ditto identity even after Transform, boosts physical Defense by 1.5, and leaves
special Defense unchanged. Critical damage uses 1.5 (2.25 with Sniper) before
Defense/50 division, independently of the native generation preset. The AI
numeric interface consumes this same calculation without applying the native
post-roll modifiers a second time.

Native AI estimates reuse the unchanged final item/ability modifier across
their damage rolls. The cache is reset for every new calculation; a 64-roll
comparison checks fresh versus reused results while changing screens,
abilities, items and type effectiveness, including resist-berry bookkeeping.
The cache occupies previously unused context bits and changes no saved data.

This is scoped arithmetic evidence, not certification of every attack branch
or the remaining battle engine. The dynamic and delayed-damage suite below
adds the three previously deferred attacks. Source stat creation, type-chart and
immunity edge cases, item removal/consumption, all action/end-turn/faint ordering,
AI decisions, capture/rewards, facilities/link and exact RNG/presentation
remain open. Optional moves, damage abilities/items, terrain, doubles and
gimmicks retain native paths. See `manifests/damage_pipeline.json`.

Validation: all 835 source groups and six AI timing groups passed. The complete
native suite passed 6,057 groups with nine existing expected failures and 585
existing TODOs, with no unexpected failures. The 26 added groups pass in both
profiles, and the original AI frame limits remain unchanged. Both production
ROM builds, all 96 source checks and twelve ARM compiler probes passed.

The production-linked move/machine audit found zero differences in the 255
mapped move records and zero mismatches across 43,792 teaching-compatibility
checks. All 109 previously documented machine/lesson gaps remain unchanged.
Strict completion still deliberately exits 2: fifteen domains, 4,065 lexical
blocks and 133 effect families retain behavioral verification obligations.

The Johto campaign passed 101 checks and nineteen directed warps, including
Falkner, badge/Roost/Togepi rewards and Save/reset/Continue, on ROM SHA-256
`02e314dc00ef53246bf5950777278ecba847becd43b30c011fb5a81b662e0cfd`.
After the final Me First, Dive and Glaive Rush compatibility guards, the ROM was
rebuilt and all twelve demo checks passed again; those guards also pass the
final source and native battle suites. Final Johto ROM SHA-256:
`9e578a3cbdd4d7743197dcf2eb380fa61cae77d14c7cbfbdd1eb25c2399c518a`.

## Dynamic powers and delayed attacks

The shared source calculation now includes all 168 mapped attacks whose effect
scripts call `damagecalc`. The three previously deferred moves have dedicated
adapters; this is not a claim that every branch of those attacks is certified.

Low Kick reads the pinned body-data row for all 391 mapped species/forms,
including cosmetic aliases and the current transformed species. It halves that
weight after Light Metal suppression, then chooses the first strictly lower
threshold in the source's descending table. The unusual thresholds 2000, 1000,
50, 25, 10 and 0 are intentional source behavior, not modern weight tiers.
Unmapped species, Float Stone, Autotomize and Heavy Metal keep native arithmetic.
Light Metal on weight-one Gastly/Haunter produces zero: the source table scan
can never terminate. This adapter deliberately retains native arithmetic for
that ability-copy boundary instead of reproducing the hang; it is an explicit
source deviation.
Gyro Ball uses effective Speed, halves both words while the user's exceeds 255,
omits the native plus-one and clamps to 1–150; a zero user Speed produces one.

Future Sight retains target-indexed native storage while resolving source
singles in user Speed order, honoring Trick Room and fresh random Speed ties.
The comparison is skipped when no delayed attack is pending; exact RNG stream
parity remains open. Impact resets the skipped hit-check loop state,
checks Protect and Fly/Dig/No Guard, and bypasses Substitute. Protect and Endure
survive until a pending source impact. Switched-out or fainted original users
supply their party stats/types, with no attacking stages, ability or held-item
boost; the active replacement's accuracy stage is ignored. The source damage
and critical paths are used once per external hit. Active users retain current
stats/items, but Analytic and Metronome never boost Future Sight. Impact does
not overwrite selected-move history, change consecutive-use counters, advance
Rampage, clear Rollout or activate Destiny Bond. Doubles, gimmicks and native
profiles retain their separate paths.

Twenty-eight groups include all 65,535 positive word-weight values, 391 mapped
weights, 1,560 both-side/Light Metal damage comparisons plus four zero-weight
guards, 529 Gyro Ball boundary vectors, 968 effective-Speed comparisons and
338 external/active accuracy-stage combinations. Live tests cover switching, STAB, held boosts, active stat changes,
critical probabilities, Dark immunity, protection, Substitute, semi-invulnerable
targets, repeated scheduling, both-side impact order, Trick Room, tied-Speed
probabilities, history and native controls.
The first five groups all failed on the parent runtime. Expanded testing exposed
additional Substitute, stale hit-check loop, protection-lifetime and delayed
move bookkeeping gaps before the fixes.

The authenticated archive now has 336 source files and 4,067 lexical blocks.
All fifteen battle domains and 133 effect families still retain obligations.
In particular, global residual/faint/replacement ordering (including simultaneous
lethal delayed attacks), complete ability/item trigger ordering, type and
suppression edges, source AI decisions, capture/rewards, facility/link rules and
exact RNG/presentation are still open. The strict completion gate must continue
to reject full parity; a passing regression suite cannot close those obligations.

Validation: all 863 source groups and six AI timing groups passed after the
final guards, with the original AI frame limits unchanged. The full native
run passed 6,084 groups with nine existing expected failures and 585 existing
TODOs, with no unexpected failures; the final 28-group native rerun also
passed after the Trick Room, Speed-tie and zero-weight guards. Both final
production ROM builds, all 97 source checks, twelve ARM compiler probes and
twelve live demo checks passed.

The final production-linked move/machine audit found zero differences in all
255 mapped move records and zero mismatches across 43,792 compatibility checks.
All 109 documented machine/lesson gaps are unchanged. Strict completeness
continues to exit 2 for the fifteen domains, 4,067 lexical blocks and 133 effect
families that still need behavioral evidence.

The campaign passed 101 checks and nineteen directed warps, including Falkner,
badge/Roost/Togepi rewards and Save/reset/Continue, on Johto ROM SHA-256
`ac3d5192d2d8e51b56586471040930780da85cc253fd071daa8cdd7a00418172`.
After the final guards, both ROMs were rebuilt and the twelve demo checks and
production-linked audit passed again. Final Johto ROM SHA-256:
`4d075879c5bab3e9ef6698e237e0f6cafc83dce3a87bdc0c107decdee15ebea0`.

## Capture, rewards and registered trainer construction

Twenty-six runtime groups cover source ball rounding and conditions, persistent
Heavy Ball rates, critical-capture thresholds, Safari counters and eligibility,
EXP distribution, EV bonuses/caps, Transform, caught-item persistence and owned
capture display. Five host tests authenticate the source constants, trainer
class defaults, party-wide custom flags and all 21 registered team bindings.
A production-linked probe verifies all 43 trainer Pokemon against independent
source attributes and stat calculations. AI decision parity remains open.

The ownership adapter uses reserved Pokedex bytes without moving later save
fields. A second production probe delivers seven Pokemon through the actual
party/box APIs and checks regional distinctions, cosmetic aliases and legacy
migration through two Flash save/reset/load cycles (72 assertions). The ordinary
campaign additionally checks captured/traded ownership through real-input
Save/reset/Continue. Neither fixture certifies the entire capture UI, all
acquisition paths or every save consumer. The field Exp. Share accessor avoids
consulting battle storage outside a battle.

Capture preflight/tutorial paths, complete Safari flow, run/roaming, all
post-battle persistence and happiness, unregistered trainers and source AI
remain open along with the other domain obligations.

Validation: all 889 source groups and six AI timing groups passed, with the
original frame limits unchanged. Earlier source sweeps did not complete: the
evolution fixtures assumed native EXP and one reached an unhandled full-moveset
prompt, timing out before later tests also timed out on that runner. The
Poliwhirl fixture now awards enough EXP to evolve without an unrelated move
prompt, while Caterpie explicitly expects source Bug Bite before its evolution
move. All six evolution groups passed in both profiles before the full source
rerun. Real-input emulator probes also passed evolution completion and B-button
cancellation in both profiles. No gameplay changes or timing-limit increases
were needed.

The native sweep passed 6,110 groups and
exposed one incorrect expectation in the new EXP control: it mixed generation-3
splitting with the native build's current participation rules. The control now
uses generation 9, and all 26 focused native groups passed on the final code.
The six updated evolution controls also passed in the native profile.
The nine existing expected failures and 585 existing TODOs are unchanged; no
other unexpected native failures occurred. This is full-sweep plus focused-rerun
evidence, not a claim of a second full native sweep after the fixture corrections.

Both final production ROM builds, 100 source checks (with focused rechecks after
the test/manifest updates), twelve ARM probes and twelve final demo checks
passed. Production-linked checks verified all 21 registered trainers/43 Pokemon
and 72 capture-storage assertions over two Flash save/reset/load cycles. The
move/machine audit reports zero differences in 255 move records and zero
mismatches in 43,792 compatibility checks; all 109 known machine/lesson gaps
remain unchanged.

The campaign passed 102 checks and nineteen directed warps, including all
starter branches, Falkner, badge/Roost/Togepi rewards and captured/traded source
ownership through Save/reset/Continue. Campaign ROM SHA-256:
`26775c5aa2813c2b880b34d34c4e89d6ea4f2dfc6a405a56e7e1af418986f98a`.
After the defensive field Exp. Share guard, both ROMs were rebuilt; the twelve
demo checks, trainer probe, two-reset storage probe and move/machine audit all
passed again. Final Johto ROM SHA-256:
`9c1c9d9aa4346568fe12228762c1259aef6998f3e20c4cba205667a462d2f3c1`.

The strict completion gate still exits 2 for fifteen domains, 4,136 lexical
blocks and 133 effect families awaiting complete behavioral evidence.

## Source escape and roaming flee decisions

Twenty-seven additional runtime groups and four host tests cover authenticated
`CheckRunSpeed`, `EnemyCanFlee`, `CheckLockedIn` and roaming status persistence.
The independent byte-ALU reference supplies 5,184 vectors, including word
truncation, zero divisors, quotient overflow, inclusive random-byte equality
and the 255-to-zero attempt-counter wrap. Run uses effective Speed; choosing
Fight resets the counter, while switching preserves it. The source does not
halve Speed for Macho Brace or Power items, including ordinary action ordering.
Active Run Away precedes Smoke Ball, then Ghost typing; move traps precede
trapping abilities. Neutralizing Gas still suppresses Run Away.

The real wild opponent controller now selects source roaming actions. Sleep,
freeze and locked move commitments prevent flee selection; traps and status
are checked again when the flee action executes. A faster roamer gives the
player an action before fleeing, so newly applied trapping/status can stop it.
Enemy Smoke Ball does not bypass source traps, and waking an initially sleeping
roamer does not invent a flee decision later in the same turn. Eligible roaming
opponents grant the player's source free-escape shortcut. Doubles, active
gimmicks, special battle formats and additional native restrictions retain
their scoped native paths.

The first four valid regression groups failed on the parent runtime: ability
versus item precedence, effective paralysis Speed, Ghost escape under older
native settings, and training-item Speed. Full parity remains OPEN. Source
special battle-type bindings, trainer forfeiture, replacement-menu escape,
complete Safari flow, roamer creation/routes/encounters and complete battle-end
outcomes still require evidence. Exact RNG stream and source presentation are
not certified by these deterministic scenario checks. Expansion's full HP and
save capacity is retained.

All 916 source groups and six AI timing groups passed with unchanged frame
budgets. Final review added an Enigma e-Reader control that failed because
roamer ordering consulted the player's eligibility rather than the fleeing
opponent's. After correcting that guard, all 27 escape groups passed again in
both source and native profiles. This is full-source plus focused final-rerun
evidence; the current suite does not claim a new full native sweep.

All 102 source reproducibility/check commands and twelve ARM compiler probes
passed. The production Flash probe passed twelve assertions across one
save/reset/load cycle: source toxic poison became ordinary poison and 300 HP
survived without truncation. The probe exercises the shipping save APIs; it
does not certify the full roamer encounter or battle-end lifecycle. Both ROM
builds, twelve demo checks, 21 registered trainer teams/43 Pokemon and the
72-assertion capture-storage probe also passed. The production-linked audit
found zero differences in all 255 mapped moves and zero mismatches across
43,792 compatibility checks; the 109 known machine/lesson gaps are unchanged.
The strict completeness gate still exits 2 for fifteen domains, 4,136 lexical
blocks and 133 effect families awaiting complete behavioral evidence.

The campaign passed 102 checks and nineteen directed warps, including all
starter/rival branches, Sprout Tower, Falkner, badge/Roost/Togepi rewards and
Save/reset/Continue, on Johto ROM SHA-256
`1a1abc0a2e1374980f9d53a0d10be10465ce9151c831c051e97ef359c152f8f8`.
After the optional Enigma action-order guard, both production ROMs were rebuilt.
The twelve demo checks, twelve roamer Flash assertions, 21 trainer/43 Pokemon
probe, 72 capture-storage assertions and production-linked move/machine audit
all passed again. Final Johto ROM SHA-256:
`422447102b0ac75aba0e89336e588219d3c27660246e0a63cd9743355ff0549a`.

## Source battle friendship and level-one growth

This suite imports all 23 happiness event rows from the pinned default source,
including the 100/160 tier boundaries, signed penalties and additive raw
Soothe Bell/Luxury Ball bonuses. Bonuses also apply when the source base gain
is zero; negative changes receive no extras, and values saturate at 0/255.
The shared event adapter binds level-up/home gains, teaching/vitamin requests,
fainting and imported boss battles for mapped player Pokemon. Live battler
friendship is synchronized so later moves use the updated party value.

Battle X Items, Guard Spec and Dire Hit now award source happiness only after
successful effects. Repeated/capped failures cannot reuse a previous stat-change
result. The strong-foe penalty uses the source 30-level boundary and 160-point
tier; a player faint does not lose happiness when the opposing active Pokemon
has already fainted. Imported boss eligibility is derived from the source
class list for all 21 registered trainers: Falkner qualifies, Sprout Elder Li
does not. Eggs and fainted party members do not receive boss gains. Optional
unregistered trainers, opponent controllers, extra species, doubles, facilities,
link battles and active gimmicks keep their scoped native paths.

Source species-data mode creates mapped level-one Pokemon with zero EXP. Party
and boxed level resolution, battle EXP bars and the summary screen share the
correct threshold. Levels 2–100 match all four source growth curves. Existing
saved EXP values and target/save layouts are retained; unmapped species and
the native species profile keep the native level-one threshold.

Twenty new regression groups include 23,552 independent byte/carry reference
cases and 400 growth thresholds, tier/home/item boundaries, live successful
and failed item use, faint/replacement and simultaneous lethal recoil,
multi-level EXP rewards, party synchronization and optional-format controls.
The four initial regression cases failed on the parent runtime before fixes.
The production Flash probe checks zero-EXP level-one records and friendship
values through save/reset/load; its fixture does not certify all event flows.

This is a scoped suite. Walking cadence, vitamin/EV-berry/Rare Candy item
dispatch, grooming and other field services, exact location provenance,
X-item boost magnitude, global faint/residual/replacement ordering, full
battle-end persistence, presentation and RNG stream remain open. Importing the
23-row table does not certify every event's gameplay binding. All fifteen
battle domains and 133 effect families remain open; the enlarged authenticated
inventory contains 342 files and 4,143 lexical blocks.

Validation: all 936 source groups and six AI timing groups passed. The full
native regression sweep passed 6,158 groups, with its nine existing expected
failures and 585 existing TODOs; there were no unexpected failures. All twenty
new groups also passed in both focused profiles. All 104 source reproducibility
commands and twelve ARM compiler probes passed. Both production ROMs built.

The Johto ROM passed twelve demo checks, 21 trainer teams/43 Pokemon, the new
30-assertion friendship/growth Flash probe, 72 capture-storage assertions and
twelve roamer-storage assertions. The production move/machine audit retains
zero audited move-data differences and zero mismatches in 43,792 compatibility
checks, with its 109 known machine/lesson gaps unchanged. The campaign passed
102 checks and nineteen directed warps, including all starter/rival branches,
Sprout Tower, Falkner, badge/Roost/Togepi rewards and Save/reset/Continue.
Strict completeness correctly exits
2 for fifteen open domains, 4,143 lexical blocks and 133 effect families.

Validated Johto ROM SHA-256:
`84e3ac873fc3dce29e507ca0327fd183b79bae90bd54da582a174975c39769a4`.

## Three-suite completion plan

See [BATTLE_PARITY_PLAN.md](BATTLE_PARITY_PLAN.md) for the complete three-suite
assignment and completion contracts. Suite 1 implements the shared end-turn,
entry and replacement runtime. Suite 2 owns action-specific behavior, including
complete switching and faint chains; Suite 3 owns context and final closure.
All fifteen domains retain a primary completion owner and remain OPEN.

The lifecycle implementation adds 41 source/native regression groups. The
authenticated phase/checkpoint manifest and three-suite ownership checks run
in CI. Regression review corrected the Baton Pass trap direction: the source
stores CANT_RUN on the trapping user, whereas Emerald stores escapePrevention
on the target. Baton Pass releases the user's target and inherits an opponent's
trap. Toxic Spikes now synchronizes on ordinary entry before the entrant's
status berry; forced entry skips reflection. Controls cover an opposing
Substitute, later voluntary entry, both replacements and Mold Breaker hazards.

Validation: all 977 source groups and six AI timing groups passed. The full
native sweep passed 6,197 groups with nine existing expected failures and 585
existing TODOs. Its only two failures were incorrect expectations in the new
Trick Room/Room Service scenes: native residual/entry sorting remains
fast-to-slow under Trick Room. After correcting only those test expectations,
all 35 lifecycle groups passed in a focused native rerun; all six entry-status
groups had passed in the full native sweep. All 41 added groups therefore have
passing source and native evidence. This is a full native sweep plus focused
test corrections, not a second full native sweep; runtime code was unchanged.

All 106 source reproducibility checks and twelve ARM compiler probes passed.
Both production ROMs built. The Johto ROM passed twelve demo checks, 21 trainer
teams/43 Pokemon, 72 capture-storage, twelve roamer-storage and thirty
friendship/growth Flash assertions. The production-linked move/machine audit
found zero audited move-data differences and zero mismatches in 43,792
compatibility checks; all 109 previously recorded machine/lesson gaps remain.

The complete campaign passed 102 checks and nineteen directed warps, including
all starter/rival branches, Sprout Tower, Falkner, badge/Roost/Togepi rewards and
Save/reset/Continue. Its initial wrapper reached a ten-minute execution limit;
the successful rerun used a thirty-minute limit without changing the ROM.
Strict completeness still exits 2 for fifteen domains, 4,143 lexical blocks
and 133 effect families. Suite 1 is implemented and regression-validated;
domain closure remains pending the later suites.

Validated Johto ROM SHA-256:
`cb255c31a399a03ef468b51ee8d1a9cbf25302e83b4c42a25f72a0bc686ecec0`. Passing these scoped scenarios
does not certify every branch in any whole battle domain.
