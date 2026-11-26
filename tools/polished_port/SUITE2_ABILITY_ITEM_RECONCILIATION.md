# Suite 2 ability and item reconciliation

Pinned source: Polished Crystal v3.2.3 default non-FAITHFUL,
`3fa43192379df5c3e7b09a08e4d5d79af4f02f42`. This ledger retains every
Suite 2 mechanic, including explicitly enabled source options. It does not certify
complete parity from inventory or test counts. Each identity below is mapped to
its reviewed branch family and the concrete evidence or remaining work.

There are 154 real abilities plus `NO_ABILITY`, and 89 held-effect constants. Exactly
76 named effects occur in the authenticated default item attribute table. Literal
zero means no held-effect class; several such items still have hardcoded battle or
friendship behavior, reconciled separately below. Unmapped constants are
distinguished from reachable default items. The item table includes every mapped
source item label.

## Canonical execution evidence

**Suite 2 implementation and local acceptance are complete, with the documented
Light Metal nontermination deviation.** The canonical source/native battle suites,
shared Kee/Cud control, all six source AI controls, both production builds and all
17 production integration checks passed. There are zero unexpected battle failures.
The next planned work is Suite 3; the strict global parity flags remain unchanged.

`SUITE2_VALIDATION.json` is the canonical repository acceptance record. It binds
these results to the runtime/test snapshot, log hashes and production ROM hashes.
The associated GitHub Actions run will verify the commit containing that record;
its immutable head SHA and job conclusions remain separate from the local results.

The unchanged runtime/test snapshot is
`ba8c3fe373110b51a1021aad77df43aacabb21422d5be774f82b3d4492beef95`.
Canonical reports and logs are in
`/workspace/scratch/daf7712eb9ba/validation/full-closure/`:
`battle-suite.json`, `battle-suite.md`, `attack-suite.json`, `native.log`,
`source.log`, `source-cud-kee.log` and `ai.log`.

| Canonical run | Result | Time | Interpretation |
| --- | --- | ---: | --- |
| Full native catalog | 6,746 passed; 585 existing TODOs; 9 expected failures; 7,340 total | 614.02 s | Passed with zero unexpected failures. Existing TODO/expected-failure groups are disclosed, not silently counted as passes. |
| Source `Polished` catalog | 1,524/1,524 passed | 393.79 s | Every current source-prefixed group passed. |
| Shared source Kee/Cud Chew control | 1/1 passed | 3.79 s | Executes the reviewed generic Kee retry through the source Cud dispatcher. |
| Source AI controls | 6/6 passed | 2.22 s | Separate AI performance/regression controls passed. |

### Named ability/item evidence in the canonical runs

Every group in the following files has a PASS line in both canonical `source.log`
and `native.log`. Test paths are relative to `test/battle`. The shared generic
Kee/Cud group is recorded separately in `source-cud-kee.log` and also passes in the
full native run. Counts identify the evidence; they do not replace the source
branch reconciliation.

| Test file or group | Source | Native | Distinguishing branch evidence |
| --- | ---: | ---: | --- |
| `ability/polished_endturn_abilities.c` | 13/13 | 13/13 | Shed Skin 77/256, full/floored weather HP, hiding, Gas, Hydration and Moody cap/candidate masks. |
| `ability/polished_absorption.c` | 11/11 | 11/11 | Status exemption, Ground precheck, full HP/caps, frozen Flash Fire legacy presets and Bulletproof suppression. |
| `hold_effect/polished_survival_items.c` | 12/12 | 12/12 | Band 30/256 with 29/30 cutoffs, failed Band no Sturdy fallback, Sash priority and multihit rechecks. |
| `polished_persistence.c` | 13/13 | 13/13 | Acquisition, consumption, restoration and temporary native Dynamax expiration/switch seams. |
| `ability/polished_hit_stat_abilities.c` | 15/15 | 15/15 | Dolls, Mirror Herb, Sheer Force, Berserk/berry timing, Anger Point criticaltext, immediate Liechi, lethal/pending-loss Enigma and repeated Future Sight impact. |
| `ability/polished_departure_abilities.c` | 2/2 | 2/2 | Natural Cure effective ability, Gas, original party slot and gained/lost Skill Swap. |
| `ability/polished_cud_chew.c` | 5/5 | 5/5 | Ineffective HP/status/critical/stat retries, successful retry, Persim confusion and one-shot expiration. |
| `ability/polished_entry_closure.c` | 8/8 | 8/8 | Trace blacklist/retry/NONE rejection, Magma Armor entry/acquisition/Gas, current-item Frisk and selective Screen Cleaner. |
| `polished_affection.c` | 18/18 | 18/18 | Both-side thresholds and true-party/Future Sight identity, all four independent effects and default/native boundaries. |
| `polished_damage_closure.c` | 9/9 | 9/9 | Guts, typed pinch thresholds, Rivalry, Sand Force, Pads/Tough Claws, Steely Spirit, Galvanize, Adaptability and actual-action Analytic. |
| `item_effect/polished_medicine_closure.c` | 6/6 | 6/6 | Live active/bench status/PP, confusion versus attraction, eligibility, PP Ups and Transform. |
| `polished_item_branch_closure.c` | 12/12 | 12/12 | Sun healing order, Gluttony, Starf, Policy actual-change/dolls, sound/Perish Song and reactive category berries. |
| `hold_effect/polished_weakness_policy.c` | 8/8 | 8/8 | Source 162 early/7 late/86 absent dispatch and the eight timing distinctions below. |
| `hold_effect/polished_leppa.c` | 7/7 | 7/7 | Packed PP Up arithmetic, active/party/Transform identity and held/forced callers. |
| `polished_self_hit_items.c` | 4/4 | 4/4 | Source confusion/disobedience/crash item deferral and native/gimmick controls. |
| `polished_cleanup_closure.c` | 4/4 | 4/4 | Terminal loss, full-party boxed capture and restoration boundaries. |
| `polished_switch_closure.c` | 15/15 | 15/15 | Switching, faint/entry order, Regenerator/Pursuit and trap lifetime. |
| `status1/polished_status_gates.c` | 9/9 | 9/9 | Source status immunity/order, Infiltrator/dolls, legacy presets and native controls. |
| `Cud Chew will activate Kee Berry effect again on the next turn` | 1/1 | 1/1 | Forced DefendHit retry on a harmless turn, with total Defense +2. |

### Source authentication and reproducibility

The final authenticated-source guard/mutation suite passed **73/73**, including
**144 new Weakness Policy mutations**. Reproducibility passed **108 commands**
(77 builders and 31 tests); all **12 ARM header probes** passed. These are separate
validation layers from the canonical battle execution above. Source token checks
prove provenance and drift detection; runtime and production-calculator tests
prove the scenarios they assert.

Historical evidence is retained in the earlier focused logs: v8 hitstats was 14/15
in each profile, v8 item closure was 8/12 source and 11/12 native, and later focused
v11/v12 corrections became green. Those old deficiencies are superseded by the
canonical results above. The v11 Weakness Policy control used unmapped Rock Smash;
its v12 replacement with mapped Low Kick changed only the fixture. The prior
71-test guard checkpoint in `action-guards-v9.log` and 65-group checkpoint from
root session 27290 remain historical; 73/73 is the final guard result. The aborted
v7 continuation attempt is not validation evidence.

**Executed** means the named canonical run passed. **Bound** identifies a reviewed
source branch connected to an explicit runtime or production-calculator input;
its named catalog is included in the canonical execution. A generic native test
is not automatically source evidence. The separately reviewed and executed shared
Kee/Cud control is identified above. No local Suite 2 execution gate remains open.
Canonical results supersede the v8 fixture failures. Suite 3 context work and the
published commit's GitHub Actions verification retain their separate scope.

### Production build and input evidence

Both production builds passed: source **36.08 s**, native **38.76 s**. The final
production-linked Shift/wild probes also passed **nine compiled cases and 13 real
input branches per profile**. Reports are in
`/workspace/scratch/daf7712eb9ba/validation/production-final/` under
`shift-source-v2/shift-switch-validation.json` and
`shift-native-v2/shift-switch-validation.json`, with matching `shift-source-v2.log`
and `shift-native-v2.log`. Each report asserts the actual compiled rule constant
and records the production ROM/map hashes.

These probes observe Natural Cure's outgoing party cure and participant reset,
source/native entry and hazard order, accepted/declined/cancelled Shift, absent or
fainted reserves, both faint-trap lifetimes and wild terminal loss. The terminal
loss observer records the player at zero HP and the foe at 999 HP before cleanup.
Setup uses normal trainer Dynamax/Tera sentinels and the Rain Badge; after battle
initialization the driver only reads state and supplies genuine key input, without
patching gameplay RAM or controller decisions. The detailed branch mapping is in
`SUITE2_SWITCHING_FAINTING_RECONCILIATION.md`.

All **17/17 production integration checks** passed, recorded in
`/workspace/scratch/daf7712eb9ba/validation/production-final/results.json` and
summarized with hashes in `SUITE2_VALIDATION.json`. The full New Bark v3 run returned
zero in **446.74 s** and recorded **102 checks and 19 warps** in
`production-final/new-bark-v3/validation.json`. Its source ROM and ELF remained
unchanged; the ROM hash matches the source production build and Shift report.
This completes the local campaign gate while preserving the separately owned
Suite 3 context and presentation scope.

## Default, source-option and native boundaries

Every identity row below includes a boundary disposition. **B** means a reachable
default battle branch, with source and runtime binding in its named family. **C**
means a reachable context/reward branch retained by its original Suite 3 owner.
**Z** means the absent ability/effect class, not a claim that every literal-zero
item is inert. **U** means no named default item uses the source held constant.
**B+C** retains both obligations. **B-negative+C** records a meaningful absence of
a native battle effect as well as its context effect.

Source battle adapters use their explicit `UsesPolishedBattleRules`, action,
damage, accuracy or lifecycle predicates. Eligible mapped source moves in singles
use the pinned rules. Native-profile, excluded format, active-gimmick and optional
move cases retain the corresponding native path where that predicate excludes
them; a shared unchanged helper remains shared. This is a per-operation boundary,
not a claim that one global gate wraps every native ability. Explicit generation
controls distinguish frozen Flash Fire, Moody's five-stat pool, Weak Armor +2,
Berserk/Sheer Force, Illuminate, source critical odds and raw survival rolls.
Consumption/acquisition backups are the intentional exception to a momentary
gimmick gate: bookkeeping continues through temporary Dynamax so later source
restoration neither duplicates a consumed item nor erases a native acquisition.

Three source options remain unset by default. Affection (`0x2B1`) uses its four
independent battle branches described below and passed all 18 groups in both
profiles. Classic categories (`0x2B2`) make category-reading abilities/items follow
the source type category before battle type conversion; its category/damage and
Counter/Mirror Coat and optional-gimmick controls passed the canonical source and
native runs. Traded-as-OT (`0x2B3`) changes obedience eligibility, not any ability/item
identity; its both-side and optional-gimmick controls also passed both profiles.
Their menu/setup/presentation work remains Suite 3. These options are included in
Suite 2 mechanics even though the default is off.

## Ability branch families

The identity table gives a primary family; cross-cutting suppression, copying,
stat history and party persistence are also covered by the relevant rows here.
All named source catalogs below are represented in the canonical passing run.
Weakness Policy's phase relative to Justified, Weak Armor and Mirror Herb has its
own eight passing groups; the section below governs that interaction rather than
relying on isolated component tests. Earlier fixture deficiencies are resolved.

| Family | Source and production binding | Distinguishing evidence and disposition |
| --- | --- | --- |
| A00 None | `NO_ABILITY`, effective-ability gate | Negative controls throughout the source catalogs. |
| A01 Status prevention/cure | `StatusHealAbilities`, `HealStatusAbility`, Own Tempo/Oblivious; status-set and ability-cure helpers | Contact/Synchronize/Corrosion matrices; `polished_status_gates.c` 9/9 and entry 8/8 passed both canonical profiles. Magma Armor entry/acquisition/Gas, Berserk Gene Own Tempo/Gas, source Infiltrator/doll order and legacy-preset immunity are distinguished. |
| A02 Contact status/reflection | `RunContactAbilities`, `_AfflictStatusAbility`, `SynchronizeAbility`; contact eligibility/status pipeline | `polished_contact_status.c` and matrix, Corrosion/status/Substitute and Synchronize/Substitute catalogs distinguish every source direct attack, hit/KO/doll, Poison Touch vs defender contact, Pads, Gas/Mold, immunity, type, Shield Dust, chance and cure order. Bound. |
| A03 Contact volatiles | Cursed Body/Cute Charm/Perish Body/Tangling Hair handlers | `polished_contact_volatile.c` and matrix distinguish chance, KO, doll, gender, already-active and suppression. The shared true-party gender helper and Rivalry Transform matrix passed both canonical profiles. |
| A04 Stat responses/copies | Intimidate, Download, `RunStatIncreaseAbilities`, `RunHitAbilities`, `ResolveOpponentBerserk`; queues and AbilityBattleEffects | `polished_stat_items.c`, `polished_mirror_ledger.c`, `polished_entry_items.c`: actual increments/caps, Contrary, no feedback, White Herb/Pack phase and living Moxie. Hitstats 15/15 and Weakness Policy 8/8 passed both canonical profiles, including repeated Future Sight criticaltext and early/late Justified/Weak Armor/Mirror Herb order. Shared Guard Spec/Illuminate corrections also passed. |
| A05 Absorption/nullification | `CheckNullificationAbilities`, `CheckTypeMatchup`, `RunEnemyNullificationAbilities`; CanAbilityAbsorbMove/hit/grounding | Absorption 11/11; Soundproof through sound/Roar/Perish Song and Heal Bell exception; Levitate through trapping/hazards. Source Mold Breaker 50-entry comparison found Bulletproof exclusion, now tested. `polished_stat_command_order.c` damaging misses before Armor Tail/Soundproof, Sucker Punch preconditions and actual called-move priority all passed both canonical profiles. |
| A06 Survival | falseswipe/applydamage; GetPolishedSurvivalDamage/GetAdjustedDamage | Survival 12/12: both sides, full/injured, Endure/False Swipe, Band/Sash/Sturdy order, raw bytes, old presets, later-hit rolls and native doubles. Optional affection 18/18 both profiles includes exact cutoffs, distributions, precedence/failed-roll fallback and per-hit controls. Native untagged Band statistics are not source proof. |
| A07 Endturn | WeatherRecoveryAbility, EndturnAbilityTableA/B, Moody/Shed Skin/Hydration; residual scheduler | Endturn 13/13 plus lifecycle toxic/Poison Heal and stat-item Speed Boost/Moody phase tests passed. Solar Power/Dry Skin positive sun HP-event order and suppression are distinguished in the canonical item closure 12/12 in both profiles. |
| A08 Items/history | Pickup/Harvest/Cud Chew/RegainItemByAbility and theft/consume; history/backups/BS_ConsumeBerry | `polished_item_recovery.c`, suite 2 item moves, persistence 13/13, Cud 5/5 plus reused Kee Cud 1/1. Old active-opponent history, berry-only, sun/Gas, no overwrite, forced/removed history, Unburden, trainer/wild, Gluttony and every source reconsume handler are bound. Both-side confusion retry/expiration and forced DefendHit executed in the canonical runs. |
| A09 Entry information/copy | Trace/Imposter/Anticipation/Forewarn/Frisk/Screen Cleaner; entry/copy dispatcher | `polished_entry_information.c` distinguishes listed powers, Counter120, zero-power rejection, ties and optional fallback; suite 2 copy handles Transform. Entry 8/8 passed both profiles. All three `polished_command_closure.c` Skill Swap controls (Flash Fire/Unburden clearing, Imposter exclusion, Cloud Nine exclusion) passed both canonical profiles. |
| A10 Weather | WeatherAbility and holder/opponent weather queries | Core tests: finite duration, all 4 setters, Hail vs Snow, same-weather no refresh, optional controls. Accuracy/damage/endturn explicitly cover Cloud Nine/Gas/Umbrella positions. Bound. |
| A11 Offensive damage | ApplyDamageAbilities and type/contact/list/category/status branches; source damage calculator | `polished_damage.c` explicitly enables Huge Power/Hustle/Technician, Solar Power, Sheer Force, Reckless, Iron Fist/Launcher/Sharpness and Pixilate. Damage closure 9/9 passed both canonical profiles: Guts status/category/Gas, pinch, Rivalry, Sand Force, Pads/Tough Claws, Steely Spirit, Galvanize, Adaptability and actual-action Analytic. Rivalry original-party gender after Transform also passed its separate command-identity control. |
| A12 Defensive damage | defensive/after-type tables and stage readers; production source calculator | `polished_damage.c` explicitly enables Marvel Scale status/Psystrike, Fur Coat category, Thick Fat typed hit, Multiscale full/injured, Dry Skin Fire, Filter/Solid Rock effectiveness and Unaware stage signs. Bound calculation proof; no redundant named tests required. |
| A13 Speed/order | ApplySpeedAbilities/GetSpeed and quick-roll priority; total-Speed/order helpers | Core sweep covers raw/stages/status/weather/Swift Swim/Chlorophyll/Sand Rush/Slush Rush/Quick Feet/Unburden/items/saturation; actual odd-rounding order and recovery lifetime. Quick Draw overrides slow-order items only at matching priority in accuracy/order tests. Bound. |
| A14 Accuracy | ApplyAccuracyAbilities/checkhit; GetPolishedTotalAccuracy/miss order | Accuracy catalog covers all 255 rows, both stage signs, per-modifier flooring, ability/item/weather ordering, No Guard/Lock-On/hiding, suppression and Umbrella. Affection evasion/bypasses/Blunder Policy, checkhit-before-blocker, Sucker Punch and shared stat/reflection suppression controls all passed canonical execution. |
| A15 Criticals | critical/high-crit table/chance table; critical stage/roll | Core source odds/native presets, Super Luck/item stages, all 9 moves, armor/Mold, Leek species, zero-power and gimmick controls; damage calculator Sniper. Optional affection independent multiplier, Transform and original Future Sight party identity passed both canonical profiles. |
| A16 Secondary/flinch | effectchance/selfeffectchance/Stench/Inner Focus/Steadfast | Secondary-stat-drop catalog and flinch/recharge/sleep/freeze families distinguish Serene Grace, Shield Dust, Sheer Force, chance, multi-hit and cancellation. Bound. |
| A17 Trap/priority/substitute/bounce | trap queries/GetPriority/CheckSubstituteOpp/Magic Bounce reset | Trapping vectors/live tests; Roar/sound/Perish Song priority; Magic Bounce/reflection/history; Scrappy/Infiltrator immunity and doll/hazard controls. Bound. |
| A18 Departure/faint/indirect HP | RunSwitchAbilities/RunFaintAbilities/drain/recoil helpers | Natural Cure 2/2, switch 15/15 including surviving Regenerator vs Pursuit KO, and self-hit items 4/4 passed both canonical profiles. Action/attack/lifecycle/Struggle tests distinguish Damp, Ooze, Rock Head/Magic Guard exceptions, Aftermath and faint order. Source Enigma revival before actual faint and the victim's queued action passed in both canonical hitstats runs. |
| A19 Cross-cutting action/identity | Pressure PP, Early Bird duration, multi-hit/Parental, Gorilla choice, weight, stored/suppressed ability | Action-boundary/foundations/multihit/choice/dynamic/copy catalogs. Active Future Sight keeps its effective ability and Parental Bond repeat; an external original user has no ability. Corrected hit-2 reentry passed both canonical profiles. Light Metal weight-one source loop is documented nonterminating input, not emulated or a passing case. Table/blacklist equality is only supporting provenance. |
| A20 Postbattle rewards | RunPostBattleAbilities, authenticated overworld caller | Reachable original Suite 3 owner; Honey Gather, postbattle Pickup and Natural Cure party scan. Not dead code; caller details below. |

## Item branch families

| Family | Source/production binding | Evidence and remaining work |
| --- | --- | --- |
| I00 None | literal zero held attributes | Potion/Poke Ball identities still exercise theft/Trick/persistence. Light Ball, Thick Club, Leek, Lucky Punch, Soothe Bell and Armor Suit have separate ID-based bindings below; they are not inert. |
| I01 Berries | HandleUserHealingItems, held status/confusion/stat/crit helpers, Enigma; threshold/force-consume/Cud dispatcher | `move_effect/polished_suite2_items.c`, status reflection, full/capped no-effect, Cud, stat/copy and persistence passed. Gluttony and Starf masks/probability are executed source evidence. Source Cud handlers are bound for HP/status/confusion/stat/Lansat, Leppa and forced DefendHit through the separately executed Kee control. Enigma lethal/pending-loss and reactive category/doll controls passed both canonical profiles. |
| I02 Leppa | PreparePPRestore/GetZeroPPMove/GetNonfullPPMove/LeppaRestorePP | Packed PP Up/active/party/Transform/caller7/7 passed both canonical profiles. Raw source encoded-PP addition can wrap; ordinary restore clears PP Up flags. Forced reconsumption compares the encoded byte, preserving the source full-slot selection quirk. |
| I03 Passive/orbs | HandleLeftovers and source residual phases | Lifecycle and status-orb catalogs; wrong-type Black Sludge bypasses Magic Guard. Native Leftovers/Sludge fixtures are support, not alone current source execution. |
| I04 Damage/locks | damage passes, source data, Choice/Metronome histories | Production damage/copy/current-species/category/type/order/compression controls and Choice/move-history catalogs passed canonical execution; damage closure 9/9 passed both profiles. |
| I05 Speed/accuracy/priority | GetSpeed/checkhit/equal-priority quick rolls | Core/accuracy/copy catalogs distinguish Quick Powder Transform, item flooring, Zoom after switch, Claw/Custap/Unnerve/Draw/Lagging order. `polished_escape.c` explicitly covers all seven training items' absent source Speed penalty and an actual Macho Brace action-order reversal. Bound. |
| I06 Crit/flinch/survival | critical/item flinch/applydamage | Core crit/Leek, full flinch/recharge catalogs and survival 12/12. Band test seam checks existing attacker multiHitOn only, native RNG unaffected. |
| I07 Contact/immunity items | CheckContactMove/type/grounding/secondary gates | Contact matrices, absorption11/11, balloon/persistence, Goggles weather/powder, Cloak/status and Clear Amulet/stat controls. Pads/Tough Claws damage control passed both canonical profiles. |
| I08 Reactive/HP items | source move-specific supereffectivetext, posthiteffects, CheckThroatSpray, miss state, drain/endmove | **Weakness Policy phase implemented and canonically verified:** 8/8 each, binding 162 early, 7 late, 86 absent source identities; Justified/Weak Armor/Mirror Herb order; no capped early retry; living zero-actual-damage activation; and early dead-user versus late attacker-KO behavior. Item closure 12/12 passed both. Action/attack/rapid-spin bind Jaboca contact KO, Shell Bell versus faint/recoil/Sheer Force and Big Root; failed-sound/cap/Perish Song and affection Blunder Policy also passed. |
| I09 Switch/entry | deferred flags, SpikesDamage, HandleRoomService, reserve selection | Deferred/entry/status/switch catalogs distinguish reserve/alive/consumption/Pursuit/hazards/random selection. All three direct Room Service groups passed both canonical profiles: speed snapshot after activation, capped nonconsumption, and off/expiry. Entry-only coverage is not used to substitute for these controls. |
| I10 Herbs/Gene/state | stat/copy history, charge skip, source turn-start Gene | Mirror ledger/stat-items/Gene/action-boundary/multihit/contact catalogs bind actual cap/direction, stale history, copies/cures, delayed acquisition, PP, Loaded Dice and Destiny Knot. Weakness Policy's early/late contribution to the Mirror Herb ledger is verified by the eight canonical I08 phase controls; isolated copy tests are not used as a substitute. |
| I11 Timers/weather | prolongation bytes and effective weather | Core/lifecycle/accuracy/damage/endturn catalogs. Source Umbrella helper behavior is distinguished despite no default mapped item. |
| I12 Encounters/rewards | Cleanse Tag, money/EV item effects | Context/rewards retain their original owner. The seven EV items also have a Suite 2 negative branch: source Speed has no native training-item penalty, explicitly bound through I05. Pay Day's in-battle effect remains Suite 2; payout/rewards are not silently certified. |
| I13 Unmapped preventive effects | HELD_PREVENT_* constants with no default item | Genuinely unmapped default inputs, retained in inventory. |

## Local acceptance and explicit semantic exception

All local Suite 2 acceptance gates passed: canonical source/native battle execution,
shared Kee/Cud, AI, authenticated-source guards, reproducibility, ARM header probes,
both production builds and all 17 production integration checks, including the
full New Bark campaign run. `SUITE2_VALIDATION.json` records the exact commands,
results, snapshot and artifact hashes. The GitHub Actions workflow
`.github/workflows/build.yml` will verify the commit containing that record;
its run provides the immutable published-head evidence.

One semantic exception remains explicit: source Light Metal can halve an
authenticated weight of one to zero, after which Low Kick's source table scan does
not terminate. The target deliberately retains a bounded native fallback for this
modified-ability input. It is not emulated, not a passing literal-parity case, and
must not disappear behind an inventory count or suite result.

There is no remaining unimplemented terminating ability/item branch identified by
this finite review. Context/reward work retains the explicit Suite 3 dispositions
below. The completed local acceptance does not certify those separate contexts;
`complete_battle_behavior_parity` remains false in the canonical validation record.

## Weakness Policy source phase — implemented and canonically verified

Authenticated `data/moves/effects.asm` places `supereffectivetext` in three distinct
groups covering all 255 default source move identities:

| Source command placement | Move identities | Required Weakness Policy behavior |
| --- | ---: | --- |
| Before post-faint and post-hit effects | 162 | Resolve the policy at this early command, before Justified/Weak Armor and their immediate Mirror Herb checks. A skipped capped policy must not be retried by a generic later item pass after another effect reopens a stat. |
| After post-hit effects in the multi-hit script | 7 | Resolve only at the command's final ordinary strike, after the preceding post-hit effects. Preserve the source early termination when the attacker has fainted before this late command. |
| Command absent | 86 | No source Weakness Policy activation from that move script; a generic native item pass must not supply the missing command. |

`BattleCommand_supereffectivetext` gates the source policy by effectiveness, the
living holder, its current item and whether at least one requested stat can
actually change. It does not require a positive actual HP-loss counter. A living
holder already at one HP can therefore activate after a super-effective False
Swipe deals zero actual damage. The older raw damage/cap tests did not establish
this zero-loss case; the dedicated v12 False Swipe/native-Tera control now does.

The Mirror Herb ledger is part of the observable phase contract. An early policy
may contribute its Attack and Special Attack changes before Justified or Weak
Armor invokes the immediate copy check; a late multi-hit policy occurs after
those hit abilities. Isolated ability/item controls cannot prove that combined
order. The implemented dispatch distinguishes an early capped policy remaining
unused after a later stat drop reopens eligibility, and a late policy remaining
unused when the attacker's post-hit KO ends the move before `supereffectivetext`.

The source phase table is implemented at the ordinary and Future Sight early
script checkpoints and the separate late path. The generic later item pass cannot
retry an early policy or supply a command absent from the source script. Source
eligibility does not require positive actual damage; only the late path requires
the attacker to remain alive. The existing certain self-item script preserves
through-Substitute stat changes and native/gimmick paths retain their own behavior.

`test/battle/hold_effect/polished_weakness_policy.c` has eight distinguishing
groups, all passing in `weakness-policy-v12-1.log` and
`weakness-policy-v12-0.log`:

| Group | Phase behavior proved |
| --- | --- |
| Ordinary hit abilities and Mirror Herb | Policy precedes Justified/Weak Armor and their immediate copy checkpoint. |
| Final ordinary multihit | Double Kick resolves its policy after the final hit-ability checkpoint. |
| Capped stat reopened by a secondary effect | No late retry; the Contrary control retains the source actual-change decision. |
| False Swipe with zero actual HP loss | A living super-effective zero-loss hit can activate the source policy; native and active Tera retain their boundaries. |
| Parental Bond first strike | The policy runs before that strike's hit ability and does not wait for or retry on the second strike. |
| Delayed Future Sight | The impact checkpoint applies source caps using current battle state. |
| Explosion after user faint | The early policy checkpoint still runs when the attacker is already at zero HP. |
| Late multihit after contact KO | The late policy checkpoint does not run after post-hit contact damage faints the attacker. |

The sole v11 failure in this new catalog was an ordinary control using unmapped
Rock Smash. It was changed to mapped Low Kick for v12; there was no corresponding
runtime change. The v11 adjacent item 12, hitstats 15 and Corrosion 10 groups already
passed both profiles. All eight phase groups now also pass both canonical full
runs, alongside those adjacent catalogs. Both production builds, the Shift/wild
probes and the full campaign integration gate also passed; their canonical record
is `SUITE2_VALIDATION.json`.
The final authenticated guard result is 73/73, including 144 new Weakness Policy
mutation cases.

## Source Anger Point phase correction

`engine/battle/effect_commands.asm:BattleCommand_criticaltext` resets the critical
flag, checks that the opponent survived, obtains its effective ability after Mold
Breaker, and calls `AngerPointAbility` before returning. Source burn scripts place
this command before `postfainteffects`, `burntarget` and `posthiteffects`. The source
ability calls `CheckMirrorHerb` immediately after raising Attack. This is a real
phase requirement: resolving Anger Point in the target's late MOVE_END ability
phase would let source burn-command thaw and post-hit Rage happen first.

The corrected target calls `BS_PolishedCriticalAngerPoint` after the critical text's
existing wait in ordinary-hit and Future Sight scripts. `TryPolishedCriticalAngerPoint`
checks source hit eligibility, the effective ability, a living target, a landed
critical and current damage, including an intact doll. It invokes the existing
`BattleScript_AbilityStatChange` and immediate source Mirror Herb handler. The old
MOVE_END Anger Point branch now applies only outside source hit rules. The new
both-side critical Flamethrower fixture asserts Anger Point before thaw; native
already uses that visible order. It and the source doll/Gas/Mold/Mirror Herb matrix
passed canonical execution. The separate Rage-cap ordering and breaking-doll
Rage controls also passed both canonical profiles.
If immediate Liechi consumption has already capped Attack, the source still
checks Mirror Herb. The new no-change path uses the existing resumable Mirror Herb
script without an Anger Point popup; it must not leave a pending copy continuation.

## Immediate attacking-damage items

`core.asm:DealDamageToOpponent` calls `SubtractHPFromUser`, whose `SubtractHP`
falls into `HandleUserHealingItems`, before `criticaltext`. That helper first
rejects a fainted user, cures held primary status/confusion, then checks pending
item loss before ordinary HP/stat berries. Consequently Liechi may consume and
raise Attack before Anger Point caps it. A source-aware criticaltext hook alone
would be insufficient if berries remained in late target MOVE_END phases.

The source Berserk branch instead uses `SubtractHPFromUser_SkipItems`, then
`CheckEnigmaBerry`, then reevaluates its half-HP crossing and resolves single-hit
Berserk before ordinary healing. `ResolveOpponentBerserk_CheckMultihit` defers
ordinary loops and Parental Bond to their existing end-of-loop resolution. Four
new canceler checkpoints mirror these orders after actual HP damage. The ordinary
pass is limited to the eighteen mapped source HP/status/confusion/stat/critical
items; native optional items, moves, formats and active gimmicks retain their
original phase. Shared item dispatch retains Unnerve and pending-loss gates.

Future Sight's source effect also runs `applydamage` and `criticaltext` at impact.
`UsesPolishedHitCheckpoint` includes that authenticated delayed impact alongside
ordinary source actions. This narrow helper is used by the new healing/Berserk/
Anger Point checkpoints and their corresponding pending/late gates; ordinary
action history and faint scheduling still use their original eligibility. The
delayed-hit Liechi control uses Focus Energy plus Scope Lens to guarantee the
critical hit and distinguishes early source consumption from native capped
nonconsumption on both sides; it passed canonical execution. Active Future Sight does repeat under
Parental Bond: `posthiteffects.check_parental_bond` gets the true active ability,
rejects an ordinary ongoing multihit loop, then sets `SUBSTATUS_IN_ABILITY` and
loops backward to `checkhit`. Its second pass clears that bit and resolves Berserk.
`home/battle.asm:GetTrueUserAbility` returns no ability only for an external delayed
user, so a replacement's Parental Bond is never borrowed.

The v8 delayed Berserk fixture incorrectly expected a single hit. With its initial
501/1000 HP, hit 1 crosses half and Sitrus heals 250; hit 2 still leaves HP above
half and correctly cancels Berserk. The corrected four-parameter fixture asserts
both HP changes, the intervening heal, unchanged Special Attack for Parental Bond,
and the single-hit Pressure control's ordinary Berserk. The source loop must also
execute `criticaltext` on hit 2. `MoveEndMultihitMove` now chooses
`BattleScript_PolishedFutureAttackHit` for an authenticated source impact; that
label starts before `futuresighttargetfailure`, after the initial impact message.
The native path keeps its existing scheduling-script reentry. The source-only
second critical message distinguishes this routing correction. The corrected
group passed in both canonical profiles, following the v11 hitstats 15/15 rerun. These changes
do not alter action/history/faint scheduling or add another Parental Bond
eligibility rule.

`CheckEnigmaBerry` is a separate source call. Its ordered checks are effective
damage, held Enigma after Unnerve, and less than full HP. It contains neither an
alive check nor the ordinary pending-item-loss check; transitive `GetUserItem` and
`RestoreHP` do not add either one. Thus it can restore a lethal target from zero
before criticaltext and actual faint processing. The target now permits that
specific immediate Enigma activation and blocks a later post-hit retry for the
same source hit. It does not clear the source's previously recorded first-faint
reservation: later handlers must simply reject faint processing while HP is
positive. The new both-side fixture asserts restored active/party HP, no Moxie or
Destiny Bond consequence, and execution of the target's already selected Splash;
active Tera/native controls preserve lethal faint behavior.

The pending-loss fixture covers both Knock Off and Bug Bite. It distinguishes
Enigma consumption before criticaltext from later removal/forced eating. Attacker
Unnerve blocks the victim's held Enigma trigger but does not block that attacker's
later forced Bug Bite consumption. Both this pending-loss group and the lethal
revival/pending-action group passed in each canonical profile.

## Optional source affection binding

This is a Suite 2 battle mechanic even though its option is disabled by default.
`FLAG_POLISHED_AFFECTION` reserves the previously unused shared ID `0x2B1` without
renumbering existing flags. Its unset state preserves the default. Source mechanics
use `GetPolishedAffectionLevel`; native heart bonuses remain excluded in eligible
source battles, so they cannot stack with the source option.

| Source anchor | Runtime binding | Distinguishing evidence in `polished_affection.c` |
| --- | --- | --- |
| `effect_commands.asm:CheckAffection` and `home/battle.asm:TrueUserPartyAttr` | `GetPolishedAffectionLevel`, original Future Sight party slot when external | Every threshold at 179/180, 219/220, 254/255 and zero; both parties despite changed active friendship; option off; Link/Tower exclusions; actual Transform and external Future Sight with opposite original/replacement friendships; native/doubles/Tera controls. |
| `BattleCommand_critical` after `CriticalHitChances` | `CalcCritChanceStageForRules` excludes native hearts; `IsCriticalHit` doubles the source numerator at level 3 | Both sides and stages 0/1/2, with flag off/on and friendship 254/255, use `RNG_CRITICAL_HIT` to distinguish doubling from adding stages. |
| `BattleCommand_checkhit`, after perfect-accuracy exceptions and before ordinary accuracy | `DoesMoveMissTarget`, `RNG_POLISHED_AFFECTION_EVASION` | Exact 1/10 evasion; enabled level 3 only; passing evasion still performs 50% Zap Cannon accuracy; Swift/No Guard/Lock-On/rain Thunder/hail Blizzard/Poison Toxic/Minimize Stomp bypass; affection miss still activates Blunder Policy. |
| `endturn.asm:HandleAffectionSelfCure`, before `HandleFutureSight` | `HandleEndTurnAffection`, `RNG_POLISHED_AFFECTION_CURE` | Level 2 threshold, Gas independence, actual cure-before-Future-Sight order and exact 1/5 across all six primary statuses on both sides passed canonical execution. |
| `BattleCommand_applydamage`, after Endure and before Band/Sash/Sturdy | `GetPolishedSurvivalDamage`, `RNG_POLISHED_AFFECTION_ENDURE`; result script selector | Strict injured-HP cutoffs, option-off, Endure/False Swipe priority, affection preserving Sash, failed-roll Sash/Sturdy fallback, later-hit rerolls and distributions 2/20, 3/20, 4/20 passed canonical execution. Sash keeps losing distribution trials alive; successful trials assert retained item. |

The authenticated source guard separately checks ordered tokens for all
four effects, both-party identity and the endturn schedule. It is source provenance,
not a replacement for execution. No source ability suppression is applied to these
friendship effects. Source UI, affection text/animation fidelity and option menu
exposure retain the original Suite 3 presentation/context owner.

The v6c probability diagnostic was a fixture control-flow problem. A failing THEN
assertion exits before the runner clears `runThen`; if a parameter's last trial
fails that way, the next parameter can execute THEN during setup, after
`didRunRandomly` was reset and before its RNG call. The displayed trial name is
still the previous parameter's completed name. The corrected affection and Starf
fixtures use the relevant animation/message as their statistical SCENE predicate,
so expected losing trials exit before THEN. Endpoint checks remain for successful
trials; deterministic boundary/cap controls are retained. The harness is unchanged.

## Acquisition, reconsumption, loss and capture

Persistence 13 covers trainer/wild Trick; held/bagged wild Thief; original matching
versus different berries; removed/eaten/nonberry; Harvest refill; trainer/wild
Pickup; current/reserve slots; new caught party member; and temporary native
Dynamax consumption/acquisition while active, after expiration and after switching.
Backups now track consumption/acquisition throughout eligible source-enabled
singles, independent of a temporary gimmick. Runtime eligibility and final
restoration keep their own rule gates. Final 13/13 logs are after the reproduced
Dynamax duplication and erased-acquisition failures.

`ability/polished_item_recovery.c` covers old active-opponent history, party identity
after switch, holding-item rejection, wild-opponent and own-history rejection,
White Herb recovery, removed/force-eaten items creating no history, berry-only
Harvest, sun/Cloud Nine/Gas, earned Unburden, Harvest before Pickup/deferred
reconsumption, and two Pickup histories consumed once. Suite 2 copy tests keep Cud
history independent through Transform/Skill Swap. Cud no-effect/confusion and packed
Leppa passed canonical execution in both profiles; they are no longer missing
reconsumption work.

`polished_cleanup_closure.c` reaches real terminal loss/whiteout, six slots,
removed/eaten/nonberry cleanup and full-party boxed capture. Its terminal callback
and swap-prompt checks are distinct from a direct TryRestoreHeldItems probe.
Capture arithmetic and reward contexts remain with the existing Suite 3 owners.

## Proven postbattle reachability

The authenticated extraction omits a caller, but `RunPostBattleAbilities` is not
unreachable. Additional pinned `engine/overworld/scripting.asm` blob
`e279653d8b6473ffc69e363f603011c0a7aaea1f` (50,494 bytes) has
`Script_reloadmapafterbattle` call it after ExitBattle/restoration: loss enters
whiteout; nonloss trainers run it after Mom's purchase attempt; wild battles skip
it on flee and otherwise run it. Reverse party order, Natural Cure, postbattle
Pickup/Honey Gather and ReceiveBattleItem bag fallback remain original Suite 3
reward work. No dependency on Suite 2 restoration was found requiring that reward
routine to run before cleanup.

## Complete source ability identity mapping

The boundary codes are defined above; each row binds the identity to the concrete
default branch and optional/native controls of its family. Multiple families are
shown where a context or a distinct battle path would otherwise be obscured.

| Source identity | Reviewed family | Boundary |
| --- | --- | --- |
| `NO_ABILITY` | A00 | Z |
| `STENCH` | A16 | B |
| `DRIZZLE` | A10 | B |
| `SPEED_BOOST` | A07 | B |
| `BATTLE_ARMOR` | A15 | B |
| `STURDY` | A06 | B |
| `DAMP` | A18 | B |
| `LIMBER` | A01 | B |
| `SAND_VEIL` | A14 | B |
| `STATIC` | A02 | B |
| `VOLT_ABSORB` | A05 | B |
| `WATER_ABSORB` | A05 | B |
| `OBLIVIOUS` | A01 | B |
| `CLOUD_NINE` | A10 | B |
| `COMPOUND_EYES` | A14 | B |
| `INSOMNIA` | A01 | B |
| `IMMUNITY` | A01 | B |
| `FLASH_FIRE` | A05 | B |
| `SHIELD_DUST` | A16 | B |
| `OWN_TEMPO` | A01 | B |
| `SUCTION_CUPS` | A17 | B |
| `INTIMIDATE` | A04 | B |
| `SHADOW_TAG` | A17 | B |
| `LEVITATE` | A05 | B |
| `EFFECT_SPORE` | A02 | B |
| `SYNCHRONIZE` | A02 | B |
| `CLEAR_BODY` | A04 | B |
| `NATURAL_CURE` | A18, A20 | B+C |
| `LIGHTNING_ROD` | A05 | B |
| `SERENE_GRACE` | A16 | B |
| `SWIFT_SWIM` | A13 | B |
| `CHLOROPHYLL` | A13 | B |
| `ILLUMINATE` | A04, A14 | B |
| `TRACE` | A09 | B |
| `HUGE_POWER` | A11 | B |
| `POISON_POINT` | A02 | B |
| `INNER_FOCUS` | A16 | B |
| `MAGMA_ARMOR` | A01 | B |
| `WATER_VEIL` | A01 | B |
| `MAGNET_PULL` | A17 | B |
| `SOUNDPROOF` | A05 | B |
| `RAIN_DISH` | A07 | B |
| `SAND_STREAM` | A10 | B |
| `PRESSURE` | A19 | B |
| `THICK_FAT` | A12 | B |
| `EARLY_BIRD` | A19 | B |
| `FLAME_BODY` | A02 | B |
| `RUN_AWAY` | A17 | B |
| `KEEN_EYE` | A04, A14 | B |
| `HYPER_CUTTER` | A04 | B |
| `PICKUP` | A08, A20 | B+C |
| `HUSTLE` | A11, A14 | B |
| `CUTE_CHARM` | A03 | B |
| `STICKY_HOLD` | A08 | B |
| `SHED_SKIN` | A07 | B |
| `GUTS` | A11 | B |
| `MARVEL_SCALE` | A12 | B |
| `LIQUID_OOZE` | A18 | B |
| `OVERGROW` | A11 | B |
| `BLAZE` | A11 | B |
| `TORRENT` | A11 | B |
| `SWARM` | A11 | B |
| `ROCK_HEAD` | A18 | B |
| `DROUGHT` | A10 | B |
| `ARENA_TRAP` | A17 | B |
| `VITAL_SPIRIT` | A01 | B |
| `WHITE_SMOKE` | A04 | B |
| `SHELL_ARMOR` | A15 | B |
| `TANGLED_FEET` | A14 | B |
| `MOTOR_DRIVE` | A05 | B |
| `RIVALRY` | A11 | B |
| `STEADFAST` | A16 | B |
| `SNOW_CLOAK` | A14 | B |
| `GLUTTONY` | A08 | B |
| `ANGER_POINT` | A04 | B |
| `UNBURDEN` | A08 | B |
| `DRY_SKIN` | A05, A07, A12 | B |
| `DOWNLOAD` | A04 | B |
| `IRON_FIST` | A11 | B |
| `POISON_HEAL` | A07 | B |
| `ADAPTABILITY` | A11 | B |
| `SKILL_LINK` | A19 | B |
| `HYDRATION` | A07 | B |
| `SOLAR_POWER` | A07, A11 | B |
| `QUICK_FEET` | A13 | B |
| `SNIPER` | A15 | B |
| `MAGIC_GUARD` | A18 | B |
| `NO_GUARD` | A14 | B |
| `TECHNICIAN` | A11 | B |
| `LEAF_GUARD` | A01 | B |
| `MOLD_BREAKER` | A19 | B |
| `SUPER_LUCK` | A15 | B |
| `AFTERMATH` | A18 | B |
| `ANTICIPATION` | A09 | B |
| `FOREWARN` | A09 | B |
| `UNAWARE` | A12 | B |
| `TINTED_LENS` | A11 | B |
| `FILTER` | A12 | B |
| `SCRAPPY` | A17 | B |
| `ICE_BODY` | A07 | B |
| `SOLID_ROCK` | A12 | B |
| `SNOW_WARNING` | A10 | B |
| `HONEY_GATHER` | A20 | C |
| `FRISK` | A09 | B |
| `RECKLESS` | A11 | B |
| `PICKPOCKET` | A08 | B |
| `SHEER_FORCE` | A11 | B |
| `CONTRARY` | A04 | B |
| `UNNERVE` | A08 | B |
| `DEFIANT` | A04 | B |
| `CURSED_BODY` | A03 | B |
| `WEAK_ARMOR` | A04 | B |
| `LIGHT_METAL` | A19 | B; zero-weight exception |
| `MULTISCALE` | A12 | B |
| `HARVEST` | A08 | B |
| `MOODY` | A07 | B |
| `OVERCOAT` | A10 | B |
| `POISON_TOUCH` | A02 | B |
| `REGENERATOR` | A18 | B |
| `BIG_PECKS` | A04 | B |
| `SAND_RUSH` | A13 | B |
| `WONDER_SKIN` | A14 | B |
| `ANALYTIC` | A11 | B |
| `IMPOSTER` | A09 | B |
| `INFILTRATOR` | A17 | B |
| `MOXIE` | A04 | B |
| `JUSTIFIED` | A04 | B |
| `RATTLED` | A04 | B |
| `MAGIC_BOUNCE` | A17 | B |
| `SAP_SIPPER` | A05 | B |
| `PRANKSTER` | A17 | B |
| `SAND_FORCE` | A11 | B |
| `FUR_COAT` | A12 | B |
| `BULLETPROOF` | A05 | B |
| `COMPETITIVE` | A04 | B |
| `MEGA_LAUNCHER` | A11 | B |
| `TOUGH_CLAWS` | A11 | B |
| `PIXILATE` | A11 | B |
| `PARENTAL_BOND` | A04, A19 | B |
| `BERSERK` | A04 | B |
| `SLUSH_RUSH` | A13 | B |
| `GALVANIZE` | A11 | B |
| `CORROSION` | A02 | B |
| `TANGLING_HAIR` | A03 | B |
| `SCREEN_CLEANER` | A09 | B |
| `STEELY_SPIRIT` | A11 | B |
| `PERISH_BODY` | A03 | B |
| `GORILLA_TACTICS` | A19 | B |
| `NEUTRALIZING_GAS` | A19 | B |
| `PASTEL_VEIL` | A01 | B |
| `QUICK_DRAW` | A13 | B |
| `CUD_CHEW` | A08 | B |
| `SHARPNESS` | A11 | B |
| `ARMOR_TAIL` | A17 | B |
| `MINDS_EYE` | A14, A17 | B |


## Complete held-effect and default item identity mapping

Labels are copied from authenticated `data/items/attributes.asm`. A dash means no
named default attribute row; `HELD_NONE` is represented by literal0 instead. U rows
are not treated as source-default items merely because Emerald offers a native
item using a similar helper. Ring Target and Utility Umbrella have explicitly
reviewed source helper paths despite their absent default item rows.

| Source effect | Family | Default source item labels | Boundary |
| --- | --- | --- | --- |
| `HELD_NONE` | I00 | — | Z |
| `HELD_BERRY` | I01 | ORAN BERRY, SITRUS BERRY, FIGY BERRY, BERRY JUICE | B |
| `HELD_LEFTOVERS` | I03 | LEFTOVERS | B |
| `HELD_RESTORE_PP` | I02 | LEPPA BERRY | B |
| `HELD_CLEANSE_TAG` | I12 | CLEANSE TAG | C |
| `HELD_HEAL_STATUS` | I01 | CHERI BERRY, CHESTO BERRY, PECHA BERRY, RAWST BERRY, ASPEAR BERRY, LUM BERRY | B |
| `HELD_HEAL_CONFUSE` | I01 | PERSIM BERRY | B |
| `HELD_PREVENT_POISON` | I13 | — | U |
| `HELD_PREVENT_BURN` | I13 | — | U |
| `HELD_PREVENT_PARALYZE` | I13 | — | U |
| `HELD_PREVENT_SLEEP` | I13 | — | U |
| `HELD_PREVENT_FREEZE` | I13 | — | U |
| `HELD_PREVENT_CONFUSE` | I13 | — | U |
| `HELD_METAL_POWDER` | I04 | METAL POWDER | B |
| `HELD_QUICK_POWDER` | I05 | QUICK POWDER | B |
| `HELD_EVIOLITE` | I04 | EVIOLITE | B |
| `HELD_TYPE_BOOST` | I04 | SILK SCARF, BLACK BELT, SHARP BEAK, POISON BARB, SOFT SAND, HARD STONE, SILVERPOWDER, SPELL TAG, METAL COAT, CHARCOAL, MYSTIC WATER, MIRACLE SEED, MAGNET, TWISTEDSPOON, NEVERMELTICE, DRAGON FANG, BLACKGLASSES, PINK BOW | B |
| `HELD_CATEGORY_BOOST` | I04 | MUSCLE BAND, WISE GLASSES | B |
| `HELD_ACCURACY_BOOST` | I05 | WIDE LENS | B |
| `HELD_ESCAPE` | I09 | SMOKE BALL | B |
| `HELD_CRITICAL_UP` | I06 | SCOPE LENS, RAZOR CLAW | B |
| `HELD_FLINCH_UP` | I06 | KING'S ROCK, RAZOR FANG | B |
| `HELD_QUICK_CLAW` | I05 | QUICK CLAW | B |
| `HELD_AMULET_COIN` | I12 | AMULET COIN | C |
| `HELD_BRIGHTPOWDER` | I05 | BRIGHTPOWDER | B |
| `HELD_FOCUS_BAND` | I06 | FOCUS BAND | B |
| `HELD_EV_DOUBLE` | I12 | MACHO BRACE | B-negative+C |
| `HELD_EV_HP_UP` | I12 | POWER WEIGHT | B-negative+C |
| `HELD_EV_ATK_UP` | I12 | POWER BRACER | B-negative+C |
| `HELD_EV_DEF_UP` | I12 | POWER BELT | B-negative+C |
| `HELD_EV_SPE_UP` | I12 | POWER ANKLET | B-negative+C |
| `HELD_EV_SAT_UP` | I12 | POWER LENS | B-negative+C |
| `HELD_EV_SDF_UP` | I12 | POWER BAND | B-negative+C |
| `HELD_CHOICE` | I04 | CHOICE BAND, CHOICE SCARF, CHOICE SPECS | B |
| `HELD_SELF_PSN` | I03 | TOXIC ORB | B |
| `HELD_SELF_BRN` | I03 | FLAME ORB | B |
| `HELD_PROLONG_WRAP` | I08 | GRIP CLAW | B |
| `HELD_PROLONG_SCREENS` | I11 | LIGHT CLAY | B |
| `HELD_PROLONG_RAIN` | I11 | DAMP ROCK | B |
| `HELD_PROLONG_SUN` | I11 | HEAT ROCK | B |
| `HELD_PROLONG_SANDSTORM` | I11 | SMOOTH ROCK | B |
| `HELD_PROLONG_HAIL` | I11 | ICY ROCK | B |
| `HELD_AIR_BALLOON` | I07 | AIR BALLOON | B |
| `HELD_ASSAULT_VEST` | I04 | ASSAULT VEST | B |
| `HELD_BIG_ROOT` | I08 | BIG ROOT | B |
| `HELD_BINDING_BAND` | I08 | BINDING BAND | B |
| `HELD_DESTINY_KNOT` | I10 | DESTINY KNOT | B |
| `HELD_EXPERT_BELT` | I04 | EXPERT BELT | B |
| `HELD_FOCUS_SASH` | I06 | FOCUS SASH | B |
| `HELD_HEAVY_BOOTS` | I09 | HEAVY BOOTS | B |
| `HELD_LIFE_ORB` | I04 | LIFE ORB | B |
| `HELD_METRONOME` | I04 | METRONOME | B |
| `HELD_PROTECTIVE_PADS` | I07 | PROTECT PADS | B |
| `HELD_RING_TARGET` | I07 | — | U |
| `HELD_ROCKY_HELMET` | I07 | ROCKY HELMET | B |
| `HELD_ROOM_SERVICE` | I09 | ROOM SERVICE | B |
| `HELD_SAFETY_GOGGLES` | I07 | SAFE GOGGLES | B |
| `HELD_SHED_SHELL` | I09 | SHED SHELL | B |
| `HELD_SHELL_BELL` | I08 | SHELL BELL | B |
| `HELD_WEAKNESS_POLICY` | I08 | WEAK POLICY | B; phase canonically verified |
| `HELD_ZOOM_LENS` | I05 | ZOOM LENS | B |
| `HELD_MENTAL_HERB` | I10 | MENTAL HERB | B |
| `HELD_POWER_HERB` | I10 | POWER HERB | B |
| `HELD_WHITE_HERB` | I10 | WHITE HERB | B |
| `HELD_BLACK_SLUDGE` | I03 | BLACK SLUDGE | B |
| `HELD_RAISE_STAT` | I01 | LIECHI BERRY, GANLON BERRY, SALAC BERRY, PETAYA BERRY, APICOT BERRY, STARF BERRY | B |
| `HELD_RAISE_CRIT` | I01 | LANSAT BERRY | B |
| `HELD_OFFEND_HIT` | I08 | JABOCA BERRY, ROWAP BERRY | B |
| `HELD_DEFEND_HIT` | I08 | KEE BERRY, MARANGABERRY | B |
| `HELD_HIT_RAISE_ATK` | I08 | — | U |
| `HELD_HIT_RAISE_SAT` | I08 | — | U |
| `HELD_HIT_RAISE_SDF` | I08 | — | U |
| `HELD_SWITCH` | I09 | — | U |
| `HELD_SWITCH_TARGET` | I09 | RED CARD, EJECT BUTTON | B |
| `HELD_EJECT_PACK` | I09 | EJECT PACK | B |
| `HELD_LAGGING_TAIL` | I05 | LAGGING TAIL | B |
| `HELD_IRON_BALL` | I05 | IRON BALL | B |
| `HELD_ENIGMA_BERRY` | I01 | ENIGMA BERRY | B |
| `HELD_CUSTAP_BERRY` | I05 | CUSTAP BERRY | B |
| `HELD_THROAT_SPRAY` | I08 | THROAT SPRAY | B |
| `HELD_BLUNDER_POLICY` | I08 | BLUNDRPOLICY | B |
| `HELD_UTILITY_UMBRELLA` | I11 | — | U |
| `HELD_BERSERK_GENE` | I10 | BERSERK GENE | B |
| `HELD_NO_BUG_BITE` | I01 | POMEG BERRY, KELPSY BERRY, QUALOT BERRY, HONDEW BERRY, GREPA BERRY, TAMATO BERRY | B |
| `HELD_CLEAR_AMULET` | I07 | CLEAR AMULET | B |
| `HELD_MIRROR_HERB` | I10 | MIRROR HERB | B |
| `HELD_PUNCHING_GLOVE` | I04 | PUNCHINGLOVE | B |
| `HELD_COVERT_CLOAK` | I07 | COVERT CLOAK | B |
| `HELD_LOADED_DICE` | I10 | LOADED DICE | B |


## Hardcoded behavior among zero-held-effect items

The absence of a held-effect class does not remove these authenticated ID checks.
They are included in the identity reconciliation in addition to the 76 named
effect classes.

| Default source item identity | Source/runtime branch | Evidence and boundary |
| --- | --- | --- |
| LIGHT BALL, THICK CLUB | `effect_commands.asm` raw-stat item adjustment and `UserValidBattleItem`; `PolishedDamageBeforeRoll` | B. `polished_damage.c` explicitly tests Light Ball and Thick Club raw Attack doubling before compression, Pikachu forms, Alolan Marowak, and the special-move rejection for Thick Club. Native damage and optional-move selectors remain native. |
| LEEK, LUCKY PUNCH | `BattleCommand_critical`, `.ValidBattleItemTable`; critical held-item stage helper | B. `polished_battle_core.c` explicitly tests source Leek/Farfetch'd/Sirfetch'd eligibility and source stage probabilities. Lucky Punch's shared Chansey +2 branch matches the authenticated table and is exercised by `hold_effect/lucky_punch.c`; that file's old-generation expected probabilities are native supporting evidence, not a claim that its entire matrix passed source rules. Source critical-stage probability proof is shared with the core stage controls. |
| SOOTHE BELL | source friendship modification; medicine/X-item/friendship handlers | B+C. `polished_friendship.c` uses the held item in successful and ineffective real item actions; `test/polished_medicine_friendship.c` binds medicine friendship tiers. Overworld friendship contexts retain their original owner. |
| ARMOR SUIT | source form/item validity and `UserCanLoseItem`; source eligibility/removal/Transform helpers | B+C. Suite 2 item/copy/Armor Suit controls passed both canonical profiles and bind the battle item restriction and original identity. The historical invalid Transform fixture is corrected. Form setup/context is not certified by the held-effect table. |
| EXP.SHARE, LUCKY EGG | hardcoded source reward item IDs | C. Reward/experience allocation remains original Suite 3 work; neither item is an omitted Suite 2 attack modifier. |
| EVERSTONE and evolution/field/key/valuable/mail identities | evolution or nonbattle use/eligibility | C or ordinary held-item transfer/persistence as applicable. No additional default attack modifier was found in the authenticated battle source. Their context implementations retain the original owners. |

## Zero-held-effect used items

Balls, medicines, X items and field/key/evolution valuables have no held class.
Their Suite 2 battle-use boundary is item dispatch, target party/move slot,
no-effect/restriction gate and HP/status/PP/friendship result. `polished_x_items.c`
proves all six +2 boosts and later Mirror Herb noncopy; action/obedience/Protect
catalogs exercise item action selection. Held-effect coverage alone does not prove
every medicine/menu path. The six `item_effect/polished_medicine_closure.c`
groups now bind active/bench cures, confusion versus attraction, fainted/mismatched/
ineffective rejection, PP Up persistence and Transform, and all four PP slots.
All six groups passed in each canonical profile after the live-party assertion corrections.
They are execution evidence for those branches, not every possible menu sequence.
Existing `item_effect/polished_healing.c`, `test/polished_item_healing.c` and
`item_effect/polished_medicine_friendship.c` bind healing amounts, success-only
bitter-medicine penalties, active/bench synchronization and native controls.
`test/polished_medicine_friendship.c` binds all four bitter medicines to their
source friendship tiers; `move_effect/polished_return.c` checks the immediate
battle damage consequence. `polished_friendship.c` already exercises all six X
items, Guard Spec and Dire Hit through real successful/ineffective item actions,
including capped boosts and live/party friendship synchronization. These existing
distinguishing controls do not need duplicate new fixtures.
Source Persim follows `UseItemText` into `UseDisposableItem`; it is consumed and has
no reuse exception. Balls/capture formulas and reward/overworld-only effects keep
their original owners. Source battle-menu flags identify reachable bag items.

## Semantic review signoff — implementation and local acceptance complete

All 155 source ability identities, all 89 held-effect constants, the 76 named source
default held-effect classes and the ID-based zero-class exceptions have explicit
dispositions above. The previously open Weakness Policy phase is implemented and
verified by its eight dedicated groups in both canonical profiles, including the
Mirror Herb/Justified/Weak Armor interactions. The full source 1,524/1,524, full native
6,746 passed with 585 existing TODOs and 9 expected failures, shared source Kee/Cud 1/1
and source AI 6/6 all passed without unexpected failures on the recorded unchanged
runtime/test snapshot. Final source guards 73/73 also passed.

Source Enigma revival, active-versus-external Future Sight abilities, option-off
defaults, temporary-gimmick persistence and the nonterminating Light Metal input
remain explicit. Both production builds, all 13 Shift/wild input branches per
profile and all 17 integration checks, including the full New Bark campaign,
passed. `SUITE2_VALIDATION.json` records all local gates; the associated GitHub
Actions run will verify the containing commit. Earlier fixture checkpoints are
historical evidence only. Suite 3 is the next planned work, and strict global
parity flags remain unchanged.

Default labels with literal zero held effect:

POKE BALL, GREAT BALL, ULTRA BALL, MASTER BALL, SAFARI BALL, LEVEL BALL, LURE BALL, MOON BALL, FRIEND BALL, FAST BALL, HEAVY BALL, LOVE BALL, REPEAT BALL, TIMER BALL, NEST BALL, NET BALL, DIVE BALL, LUXURY BALL, HEAL BALL, QUICK BALL, DUSK BALL, DREAM BALL, PREMIER BALL, CHERISH BALL, POTION, SUPER POTION, HYPER POTION, MAX POTION, ANTIDOTE, BURN HEAL, PARALYZE HEAL, AWAKENING, ICE HEAL, FULL HEAL, FULL RESTORE, REVIVE, MAX REVIVE, ETHER, MAX ETHER, ELIXIR, MAX ELIXIR, HP UP, PROTEIN, IRON, CARBOS, CALCIUM, ZINC, RARE CANDY, PP UP, PP MAX, FRESH WATER, SODA POP, LEMONADE, MOOMOO MILK, RAGECANDYBAR, PEWTERCRUNCH, ENERGYPOWDER, ENERGY ROOT, HEAL POWDER, REVIVAL HERB, SACRED ASH, REPEL, SUPER REPEL, MAX REPEL, ESCAPE ROPE, POKE DOLL, MULCH, SWEET HONEY, X ATTACK, X DEFEND, X SPEED, X SP ATK, X SP DEF, X ACCURACY, DIRE HIT, GUARD SPEC., ABILITY CAP, ABILITYPATCH, LEAF STONE, FIRE STONE, WATER STONE, THUNDERSTONE, ICE STONE, MOON STONE, SUN STONE, DUSK STONE, SHINY STONE, EVERSTONE, EXP.SHARE, LUCKY EGG, SOOTHE BELL, LIGHT BALL, LEEK, THICK CLUB, LUCKY PUNCH, ARMOR SUIT, DRAGON SCALE, UPGRADE, DUBIOUS DISC, PROTECTOR, ELECTIRIZER, MAGMARIZER, OVAL STONE, ODD SOUVENIR, LINKING CORD, SILVER LEAF, GOLD LEAF, MINT LEAF, BOTTLE CAP, NUGGET, BIG NUGGET, TINYMUSHROOM, BIG MUSHROOM, BALMMUSHROOM, PEARL, BIG PEARL, PEARL STRING, STARDUST, STAR PIECE, BRICK PIECE, RARE BONE, SLOWPOKETAIL, HELIX FOSSIL, DOME FOSSIL, OLD AMBER, FLOWER MAIL, SURF MAIL, LITEBLUEMAIL, PORTRAITMAIL, LOVELY MAIL, EON MAIL, MORPH MAIL, BLUESKY MAIL, MUSIC MAIL, MIRAGE MAIL.
