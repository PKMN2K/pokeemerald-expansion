# Suite 2 move-command reconciliation

Pinned default non-FAITHFUL source: `3fa43192379df5c3e7b09a08e4d5d79af4f02f42`.

Move-specific Suite 2 source/runtime reconciliation, including assigned fixes. Suite 2 implementation and local acceptance are closed with the documented nontermination deviation in SUITE2_VALIDATION.json, SUITE2_CLOSURE_REVIEW.md and BATTLE_PARITY_PLAN.md; this document does not substitute test counts or name mappings for branch evidence.

This review covers all 129 used commands and all 133 used effect scripts, with all 255 source move identities retained in the family table. A command-level row describes its source branches and transitive dependencies, then gives actual runtime definitions and existing execution fixtures. Shared action/damage/status and switching/faint rows contain concrete semantic adjudication, production bindings and distinguishing fixture evidence; their owners remain visible for maintenance. The JSON companion contains the complete ordered scripts, authenticated blob identifiers, runtime anchors and all matched test titles. Neither the inventory totals nor a generic Expansion pass establishes parity.

## Current review disposition

Suite 2 implementation and local acceptance are closed with the documented reachable Light Metal nontermination deviation. [SUITE2_VALIDATION.json](SUITE2_VALIDATION.json) records the source/native full catalogs, both production builds and all 17 integration probes. The branch interpretations, live bindings and distinguishing controls in this review support that disposition; counts alone do not establish parity.

The final Weakness Policy correction passed independent static review, all eight focused groups in each profile, both full catalogs and local production acceptance. Production-linked Shift/wild-faint inputs passed 13 branches per profile. The New Bark campaign passed 102 checks and all 19 warp events in 446.74 seconds using unchanged source ROM/ELF files. The associated Actions run records the immutable head SHA and job conclusions for the containing commit.

The 133 `complete_behavior_parity` flags and authored strict code gate remain unchanged. They describe the conservative whole-battle/context audit universe, including the original Suite 3 obligations and the documented deviation; they are not a list of unreviewed Suite 2 move commands. The next planned implementation suite is Suite 3.

## Full execution evidence

All four canonical `validation/full-closure` runner profiles completed successfully on the same runtime/test snapshot. The native TODO and expected-failure categories are reported separately.

| Profile | Passed | Registered total | Other registered outcomes | Seconds |
| --- | ---: | ---: | --- | ---: |
| Native full catalog | 6,746 | 7,340 | 585 TODO; 9 expected failures | 614.02 |
| Polished source catalog | 1,524 | 1,524 | None | 393.79 |
| Shared Cud Chew / Kee Berry under source rules | 1 | 1 | None | 3.79 |
| Source AI thinking-time checks | 6 | 6 | None | 2.22 |

Runtime/test snapshot SHA-256: `ba8c3fe373110b51a1021aad77df43aacabb21422d5be774f82b3d4492beef95`, verified unchanged by the root runner. The report's whole tracked-worktree hash is a different scope and is preserved separately in the JSON evidence.

Canonical reports are `validation/full-closure/battle-suite.json` and `attack-suite.json`; logs are `native.log`, `source.log`, `source-cud-kee.log` and `ai.log` in that directory. The JSON companion embeds their exact run commands, return codes, durations, counts, report/log SHA-256 hashes and the matched passing titles for each review group. All eight Weakness Policy groups passed again in both full catalogs. This completes the current catalog execution gate; it does not promote native TODOs or original Suite 3 AI-decision/presentation work to completed source behavior.

Production-linked input probes and production build/ROM evidence passed in [SUITE2_VALIDATION.json](SUITE2_VALIDATION.json). Associated Actions records verify the containing commit. The conservative whole-context completion flags and documented Light Metal nontermination deviation remain explicit.

## Reviewed findings

| Finding | Source behavior | Current disposition |
| --- | --- | --- |
| Weakness Policy phase | 59 early families/162 moves run before postfaint/posthit; two ordinary loop families/seven moves run after posthit on the final strike; PB second hit returns | Implemented and independently reviewed; eight/eight source and eight/eight native groups passed focused v12; current full execution passed; local production/input acceptance passed; associated Actions records verify the containing commit |
| Pay Day bank | Either actor; five level additions; modulo 2^24; current Substitute helper honors Infiltrator | Four focused groups passed both source and native |
| Sleeping Rollout | Return before lock/counter and Curl multiplier; preserve prior state | Live damage and calculator cases passed source/native. Native Sleep Talk also ends without a lock; the old distinguishing-lock expectation was rejected |
| Capped Minimize; legacy Focus Energy | Minimize bit still sets at Evasion cap; Focus Energy always supplies the source two-stage flag | Both command regressions passed focused v5 source/native |
| Bug Bite; Thief/Knock Off | Bug Bite consumes through intact doll; item removal uses current doll/Infiltrator; pending loss precedes berry healing | First seven command-edge groups passed focused v5 source/native |
| Sketch/Infiltrator; Attract/Foresight rejection | Sketch uses the helper bypass; Attract hidden/gender/existing checks precede Oblivious; Foresight repeat fails independent of native preset | Sketch passed source/native v8; Attract/Foresight passed v8/v9; full catalog passed |
| Party identity | Armor Suit matches party Armored form; Attract/Cute Charm/Rivalry use original party gender after Transform | Source/native v9 identity and Armor Suit controls passed |
| Direct Room Service | Snapshot source Speed order after Trick Room is enabled; consume only after successful drop | All three source/native direct order/cap/toggle/expiry groups passed v8 |
| Brick Break | Ignore screens for damage, then clear both counters after damage even on target KO | Source/native command-closure groups passed v8/v9, including optional breaker controls |

## Finite execution obligations

The ten finite move-specific evidence rows have the following actual repository fixtures, executed successfully in both profiles by v8/v9. They are resolved source/runtime branches. The current full-catalog rerun passed independently of those focused results.

| Reviewed branch | New actual fixture(s) | Status |
| --- | --- | --- |
| `growth` | `test/battle/move_effect/polished_command_closure.c:4` — Polished command closure Growth uses effective user weather and independent stat caps | Source/native focused v8/v9 passed; full catalog passed |
| `haze` | `test/battle/move_effect/polished_command_closure.c:38` — Polished command closure Haze resets all seven stages on both sides; `test/battle/move_effect/polished_command_closure.c:62` — Polished command closure Haze preserves Focus Energy Minimize and Defense Curl | Source/native focused v8/v9 passed; full catalog passed |
| `curl` | `test/battle/move_effect/polished_command_closure.c:62` — Polished command closure Haze preserves Focus Energy Minimize and Defense Curl; `test/battle/move_effect/polished_command_closure.c:88` — Polished command closure capped Defense Curl still boosts Rollout | Source/native focused v8/v9 passed; full catalog passed |
| `sucker_punch` | `test/battle/move_effect/polished_command_closure.c:119` — Polished command closure Sucker Punch checks attack status earlier action and item selection | Source/native focused v8/v9 passed; full catalog passed |
| `transform` | `test/battle/move_effect/polished_command_closure.c:146` — Polished command closure Transform rejects hidden targets with No Guard under the oldest preset | Source/native focused v8/v9 passed; full catalog passed |
| `skill_swap` | `test/battle/move_effect/polished_command_closure.c:167` — Polished command closure Skill Swap clears both earned Flash Fire and Unburden states; `test/battle/move_effect/polished_command_closure.c:196` — Polished command closure Skill Swap skips Cloud Nine reentry after the swap announcement; `test/battle/move_effect/polished_command_closure.c:217` — Polished command closure Skill Swap does not activate a newly acquired Imposter | Source/native focused v8/v9 passed; full catalog passed |
| `rampage` | `test/battle/move_effect/polished_command_closure.c:245` — Polished command closure rampage preserves existing confusion on its final action; `test/battle/move_effect/polished_command_closure.c:266` — Polished command closure rampage confuses after a final protected action under an old preset; `test/battle/move_effect/polished_command_closure.c:286` — Polished command closure a final rampage contact knockout never confuses the fainted user; `test/battle/move_effect/polished_command_closure.c:318` — Polished command closure Future Sight cannot finish a running rampage | Source/native focused v8/v9 passed; full catalog passed |
| `substitute` | `test/battle/move_effect/polished_command_closure.c:339` — Polished command closure Substitute honors minimum equal-cost and repeated creation boundaries | Source/native focused v8/v9 passed; full catalog passed |
| `screens` | `test/battle/move_effect/polished_command_closure.c:364` — Polished command closure screen durations and failed refresh stay independent with Light Clay; `test/battle/move_effect/polished_command_closure.c:394` — Polished command closure Brick Break removes both screens only after a successful hit; `test/battle/move_effect/polished_command_closure.c:431` — Polished command closure Brick Break clears screens after HP damage including a lethal hit | Source/native focused v8/v9 passed; full catalog passed |
| `safeguard` | `test/battle/move_effect/polished_command_closure.c:469` — Polished command closure Safeguard rejects refresh and expires after exactly five turns | Source/native focused v8/v9 passed; full catalog passed |

The current `Polished command` batch has 54 groups, including the finite closure, edge, identity, posthit and primary-stat/miss-routing matrices. Sketch and Armor Suit use their existing `Polished Sketch` and `Polished Suite 2 Armor Suit` prefixes. Exact tagged-RNG trials, per-parameter cases and live assertions are in the fixtures; a group count is only a runner inventory.

## Completed local acceptance

[SUITE2_VALIDATION.json](SUITE2_VALIDATION.json) is the canonical acceptance record and retains the exact commands, log hashes, production artifact hashes and runtime/test snapshot.

| Local gate | Actual result |
| --- | --- |
| Full native/source catalogs and shared controls | Passed; exact counts and durations are above |
| Native production build | Passed in 38.76 seconds |
| Source production build | Passed in 36.08 seconds |
| Production/integration checks | All 17 passed |
| Source/native Shift and wild-faint inputs | Nine compiled fixture cases and 13 real-input branches per profile; 26 branches total |
| New Bark campaign | 102 checks and 19 warp events passed in 446.74 seconds; source ROM/ELF hashes unchanged |

The production-linked input cases cover Switch/Predict acceptance, decline, cancellation/reoffer and wild faint acceptance, decline, trapped-victim and mandatory-selector behavior. Their exact source/runtime interpretation remains in `SUITE2_SWITCHING_FAINTING_RECONCILIATION.md`. The campaign driver now identifies the actual script-owned trainer Shift question and declines it with B; its shared real-input Bite selector retains the level-13 and exact moveset assertions when Totodile reaches that level during Finch. Party, outcome, reward and Save/Continue assertions were retained.

The associated Actions run for the commit containing the canonical validation record supplies immutable head-SHA and job-conclusion verification. Local production acceptance is complete. Original Suite 3 setup/AI-choice/link/facility/capture-rewards/presentation boundaries and the authored strict completeness code gate remain unchanged.

One reachable semantic deviation remains: source Low Kick after Light Metal reduces a weight-one target to zero and does not terminate; the adapter deliberately retains bounded native arithmetic. This is neither a default-inventory exclusion nor successful literal parity. The final Weakness Policy correction passed all eight source/native focused groups, both full catalogs and local production acceptance.

Focused logs cited above include `validation/weakness-policy-v12-{0,1}.log`, `corrosion-powder-v11-{0,1}.log`, `hitstats-v11-{0,1}.log`, `item-closure-v11-{0,1}.log`, and `validation/commands-v9-{0,1}.log`, `incoming-thaw-v9-{0,1}.log`, `magic-bounce-v9-{0,1}.log`, the v9 Armor Suit/hitstats logs, `sketch-v8-{0,1}.log`, and the v10 status/reactive-item logs in the active validation workspace. Canonical reproducible result identifiers are maintained by the parent acceptance record.

## Weakness Policy source phase inventory

The complete 133-effect classification, source command positions, all 255 source-to-target bindings and native effect/script mappings are embedded in the JSON companion. This is authenticated placement and current-hook reachability evidence; independent static review and all eight source/native focused v12 phase groups passed; current full execution passed; local production acceptance passed; the associated Actions run records verification of the containing commit.

| Source phase | Effect families | Mapped source moves |
| --- | ---: | ---: |
| early_before_postfaint_posthit | 59 | 162 |
| late_after_posthit_before_endloop | 2 | 7 |
| absent | 72 | 86 |

The only late families are EFFECT_DOUBLE_HIT (Double Kick and Bonemerang) and EFFECT_MULTI_HIT (Double Slap, Pin Missile, Rock Blast, Icicle Spear and Polished Fury Strikes). Every other source-present script is early. Supereffectivetext returns on PB second hit, returns before the final ordinary loop strike, requires super-effective effectiveness and a surviving target (not a positive HP write), and consumes only if either self-stat attempt actually changes a stage. It has no Substitute rejection.

Target EffectHit and the Future Sight impact both reach resultmessage before secondary effects and move-end hit abilities. Struggle reaches the same path through its native wrapper. Effectiveness string suppression on PB first strike is not an item-eligibility predicate. Early-class source attempts must not retry at the late generic item pass if both stats were capped at the original checkpoint.

### Complete early-hook reachability proof

The JSON companion retains every individual early move and all literal or conditional native effect/script bindings under `weakness_policy_phase_inventory.rows`; its `native_checkpoint_coverage` object also lists all 160 direct bindings. The following control-flow proof covers all 162 early moves, not only representative fixtures.

| Bound early moves | Production entry and continuation | Required checkpoint |
| --- | --- | --- |
| 160 individually listed moves | `include/move.h:GetMoveEffect` reads `gMovesInfo`; `GetMoveBattleScript` directly returns the effect table script. Every native effect variant for these moves binds `BattleScript_EffectHit`. | Its `resultmessage` and `waitmessage`, before `setadditionaleffects` / `moveendall` |
| Struggle | `EFFECT_RECOIL` binds `EffectHit`; the alternative `EFFECT_STRUGGLE` wrapper increments its game statistic and unconditionally jumps to `BattleScript_EffectHit`. | The same ordinary hit checkpoint |
| Future Sight | The scheduling script has no damage or resultmessage. Actual impact runs `BattleScript_PolishedFutureAttackHit` and `BattleScript_DoFutureAttackResult`; `MoveEndMultihitMove` explicitly uses that impact label for active Parental Bond reentry. | Impact resultmessage/waitmessage; source eligibility uses `UsesPolishedHitCheckpoint` |

| Conditional continuation | Why it cannot bypass a successful early checkpoint |
| --- | --- |
| Fly, Dig, Solar Beam and Power Herb | `CancelerCharging` calls returning charging/Herb scripts. A charge-only action jumps to MoveEnd without damage; a continuation, immediate sun or Herb execution resumes the existing attackcanceler and then EffectHit. |
| Explosion | `CancelerExplosion` calls `BattleScript_Explosion`, which sets attacker HP zero and returns. A successful hit still reaches EffectHit. Target Policy therefore cannot require a living attacker. |
| Magnitude and source move callers | Magnitude announcement returns to attackcanceler. `CancelerPPDeduction` and `Cmd_jumptocalledmove` select `GetMoveBattleScript` for the actual called move, so the same mapped hit entry is used. |
| Pursuit interception | `Cmd_jumpifnopursuitswitchdmg` and `SetTargetToNextPursuiter` schedule the ordinary UseMove action; `HandleAction_UseMove` selects the mapped EffectHit script. There is no separate Pursuit damage-result script. |
| Drain, recoil, Rollout, recharge and pivot effects | Their damage entry remains EffectHit; draining/recoil/additional/pivot processing occurs after the implemented checkpoint. The successful attackcanceler does not replace their primary hit script. |
| Failure and optional paths | `ShouldSkipToMoveEnd` skips avoided attacks, and initial interruption/failure/charge-only exits have no qualifying damage. Other resultmessage commands belong to failure, status, self-confusion or multihit-summary scripts. They are not alternate successful early mapped damage paths. Native optional moves, doubles and active gimmicks retain their existing rule eligibility. |

For late ordinary loops, the two hit sites must do nothing: the final per-hit target-item checkpoint after hit abilities remains authoritative. For early source families, the late generic target-item pass must never retry, even if an earlier capped attempt retained the Policy and a subsequent secondary effect created room. For absent source scripts, neither source checkpoint may activate Policy. Parental Bond second hit remains excluded at both source phases.

The source transitive item lookup (`GetOpponentItemAfterUnnerve` -> current held item) only suppresses edible berries under Unnerve and does not inspect pending item loss. A reserved Knock Off/Thief loss therefore cannot suppress the early Policy attempt. Current real target HP is the source faint guard. These are eligibility requirements preserved by the current hook, separate from its proven control-flow reachability.

## Review groups

### shared_action

**Owner:** root shared pipeline review. **Status:** source/runtime semantically reconciled; current full source/native catalog execution passed; local production/input acceptance passed; associated Actions records verify the containing commit

**Commands:** `checkobedience`, `doturn`, `hastarget`, `usedmovetext`, `failuretext`, `endmove`, `clearmissdamage`

CheckTurn resolves recharge, flinch, sleep/Early Bird, natural or named-move thaw, confusion, attraction, Disable and paralysis before obedience and PP. CantMove clears the source bond/charge/Rollout/rampage state at its specified failure boundary. IncreaseMetronomeCount runs after that check and before obedience, with called and delayed execution excluded. Obedience uses the player/foreign low-16-bit identity, badges and selectable traded-as-OT option; alternate execution updates the selected identity again. doturn suppresses repeated PP for called/locked continuations, and hastarget owns the direct Pressure cost while permitting initial Fly/Dig/Solar Beam after an opposing faint. failuretext carries the exact failure kind into Blunder Policy; manual move-body messages are separate. endmove is an interpreter sentinel, not a blanket state-reset command.

**Authenticated source anchors:** `engine/battle/effect_commands.asm:725 BattleCommand_checkobedience`; `engine/battle/effect_commands.asm:1052 BattleCommand_doturn`; `engine/battle/effect_commands.asm:1082 BattleCommand_hastarget`; `engine/battle/effect_commands.asm:1039 BattleCommand_usedmovetext`; `engine/battle/effect_commands.asm:2615 BattleCommand_failuretext`; `engine/battle/effect_commands.asm:138 DoMove.ReadMoveEffectCommand (endmove sentinel)`; `engine/battle/effect_commands.asm:4123 BattleCommand_clearmissdamage`

**Actual runtime definitions:** `src/battle_move_resolution.c:3772 DoAttackCanceler`; `src/battle_move_resolution.c:291 CancelerObedience`; `src/battle_move_resolution.c:379 CancelerPowerPoints`; `src/battle_move_resolution.c:1145 CancelerPPDeduction`; `src/battle_move_resolution.c:6977 GetRecordedMove`; `src/battle_main.c:4781 SetActionsAndBattlersTurnOrder`; `src/battle_move_resolution.c:278 PolishedIncreaseMetronomeCount`; `src/battle_util.c:6200 GetPolishedObedience`; `src/battle_move_resolution.c:172 CancelerAsleepOrFrozen`; `src/battle_move_resolution.c:538 CancelerConfused`; `src/battle_move_resolution.c:621 CancelerInfatuation`; `src/battle_move_resolution.c:606 CancelerParalyzed`; `src/battle_move_resolution.c:7000 SetSameMoveTurnValues`; `src/battle_move_resolution.c:2418 PolishedAccuracyFailure`

**Reviewed transitive anchors:** `engine/battle/effect_commands.asm:186 CheckTurn`; `engine/battle/effect_commands.asm:454 CantMove`; `engine/battle/effect_commands.asm:504 IncreaseMetronomeCount`; `engine/battle/effect_commands.asm:593 ResetTurn`; `engine/battle/effect_commands.asm:6160 ClearLastMove`; `engine/battle/effect_commands.asm:627 CheckAffection`; `engine/battle/effect_commands.asm:2782 GetFailureResultText`

**Branch adjudication:**

| Source branch | Production binding | Distinguishing evidence and disposition |
| --- | --- | --- |
| CheckTurn action blockers, source byte probabilities, sleep duration and thaw-before-later-cancel ordering | `DoAttackCanceler / CancelerAsleepOrFrozen / CancelerConfused / CancelerInfatuation / CancelerParalyzed` | Actual source fixtures check exact 85/256 confusion, 64/256 paralysis, 52/256 freeze, uniform sleep counters, Early Bird, PP on cancellation, repeated freeze and source/native optional scopes. Evidence: `test/battle/polished_battle_foundations.c:4` — Polished foundations confusion uses the source byte probability independently of generation; `test/battle/polished_battle_foundations.c:26` — Polished foundations confusion decrements and consumes PP only when the attack executes; `test/battle/polished_battle_foundations.c:50` — Polished foundations paralysis keeps a one in four stop chance under Champions settings; remaining exact fixture titles are listed in the JSON row |
| Obedience player eligibility, foreign-ID width, source option, alternatives and inherited called state | `GetPolishedObedience / CancelerObedience / CancelerCallSubmove` | Source/native fixtures cover both sides, badges, 16-bit foreign identity, link/facility guards, alternate moves and held-history updates. The established-gimmick fixture was corrected after v8 and passed focused v9 in both profiles; this is execution, not option metadata. Evidence: `test/battle/polished_move_history.c:4` — Polished move history Metronome records the disobedient alternate action; `test/battle/polished_move_history.c:187` — Polished move history Magic Bounce preserves source charging through a later own action; `test/battle/polished_obedience.c:7` — Polished obedience uses Johto badge thresholds including unchanged intermediate badges; remaining exact fixture titles are listed in the JSON row |
| PP and selected/last/counter move identity across misses, callers, charging and departures | `CancelerPPDeduction / GetRecordedMove / PolishedIncreaseMetronomeCount / SetSameMoveTurnValues` | Distinguishing tests cover Pressure on direct and called attacks, first charge versus continuation, history after interruption, Metronome/Sleep Talk and reflected reentry, native gimmick phase and enemy Baton Pass inheritance. Source and native counts have independent storage. Evidence: `test/battle/polished_action_boundaries.c:249` — Polished charging moves pay PP once including Pressure with continuation or Power Herb; `test/battle/polished_action_boundaries.c:277` — Polished Solar Beam sunlight skips charging without consuming Power Herb; `test/battle/polished_action_boundaries.c:300` — Polished charging interruption releases Fly Dig and Solar Beam without a second PP cost; remaining exact fixture titles are listed in the JSON row |
| Failure message kind decides Blunder Policy; seven manual bodies omit the common failure helper | `PolishedManualMissMessage / PolishedAccuracyFailure / MoveEndSprayLeppaBlunder` | v8 source execution passed the manual seven-move matrix, Trick/Pain Split, Sing versus priority and reflected holder ownership. Numeric accuracy misses, Protect, Substitute and immunity are distinct cached outcomes. Evidence: `test/battle/move_effect/polished_stat_command_order.c:61` — Polished command stat accuracy misses precede every source defensive stat guard; `test/battle/move_effect/polished_stat_command_order.c:199` — Polished command stat misses precede Soundproof Armor Tail and Prankster Dark nullification; `test/battle/move_effect/polished_stat_command_order.c:231` — Polished command reflected original miss gives Blunder Policy only to the reflector; remaining exact fixture titles are listed in the JSON row |

**Actual test catalog entries:**

- `test/battle/move_effect/polished_stat_command_order.c:296` — Polished command manual miss text does not consume Blunder Policy (source catalog).
- `test/battle/move_effect/polished_stat_command_order.c:359` — Polished command Trick and Pain Split retain distinct manual and priority miss paths (source catalog).
- `test/battle/polished_action_boundaries.c:6` — Polished forgetting a slot clears only its active player locks, including bench learning (source catalog).
- `test/battle/polished_action_boundaries.c:35` — Polished rampage called by Sleep Talk never locks the sleeping user (source catalog).
- `test/battle/polished_action_boundaries.c:50` — Polished Rollout called by Sleep Talk never advances the sleeping user lock (source catalog).
- `test/battle/polished_action_boundaries.c:73` — Polished rampage clears its lock when the middle turn is protected (source catalog).
- `test/battle/polished_action_boundaries.c:92` — Polished rampage confuses on final-turn flinch despite an older native preset (source catalog).
- `test/battle/polished_action_boundaries.c:115` — Polished Explosion releases Unnerve berries before attack damage (source catalog).
- `test/battle/polished_action_boundaries.c:134` — Polished Explosion clears status before fainting and respects Damp prevention (source catalog).
- `test/battle/polished_action_boundaries.c:150` — Polished Rest uses the opponent Umbrella for Leaf Guard independent of generation (source catalog).
- 102 additional matched entries are listed explicitly in `SUITE2_MOVE_RECONCILIATION.json`; they are not promoted to complete branch coverage.

**Remaining / scope:** The selectable traded-as-OT flag and either established gimmick controls are implemented; their corrected focused source/native v9 run passed; the later full current-candidate run also passed. Source pre-action Metronome count is separate from native post-success maintenance, including Baton Pass, reflection reentry and gimmick transitions. Exact shared RNG stream/call-count equivalence remains the original Suite 3 responsibility, not an exemption from these Suite 2 state transitions.

### shared_accuracy

**Owner:** root shared pipeline review. **Status:** source/runtime semantically reconciled; current full source/native catalog execution passed; local production/input acceptance passed; associated Actions records verify the containing commit

**Commands:** `checkhit`, `checkpriority`, `checkpowder`, `bounceback`, `thunderaccuracy`

checkhit first applies Protect, source Swagger early Substitute, guaranteed-move/No Guard/Poison Toxic/weather/hiding/Lock-On rules, then source affection and exact staged byte accuracy. Intermediate stage, ability and held-item floors remain ordered. The authenticated checkhit-before-checkpriority scripts preserve an existing failure through Armor Tail, Prankster-Dark or Soundproof. Sucker Punch checks its selected-action precondition first; Counter/Mirror Coat do not use checkpriority. checkpowder is a separate earlier branch: Toxic/Thunder Wave may change a successful hit to type immunity without replacing an existing miss, while Sing/Soundproof and powder Overcoat may overwrite it. Magic Bounce ResetTurn preserves the original failure kind and skips a new checkhit roll; its child still reruns checkpowder and priority. Manual confusion/Attract/Foresight/Leech Seed/Trick miss messages and Pain Split ignoring ordinary miss flags are preserved independently.

**Authenticated source anchors:** `engine/battle/effect_commands.asm:1948 BattleCommand_checkhit`; `engine/battle/effect_commands.asm:2332 BattleCommand_checkpriority`; `engine/battle/effect_commands.asm:1837 BattleCommand_checkpowder`; `engine/battle/move_effects/magic_bounce.asm:1 BattleCommand_bounceback`; `engine/battle/move_effects/thunder.asm:1 BattleCommand_thunderaccuracy`

**Actual runtime definitions:** `src/battle_move_resolution.c:2973 CancelerAccuracyCheck`; `src/battle_move_resolution.c:1523 CancelerMoveEffectFailureTarget`; `src/battle_move_resolution.c:4689 MoveEndBouncedMove`; `src/battle_util.c:10274 BlocksPrankster`; `src/battle_move_resolution.c:2324 UsesPolishedPreEffectAccuracy`; `src/battle_move_resolution.c:6504 UsesPolishedPrimaryStatAccuracy`; `src/battle_move_resolution.c:2369 PolishedPreEffectTypeImmunity`; `src/battle_move_resolution.c:2409 PolishedPriorityWouldBlock`; `src/battle_move_resolution.c:2418 PolishedAccuracyFailure`; `src/battle_util.c:11752 DoesMoveMissTarget`; `src/battle_util.c:11552 GetPolishedTotalAccuracy`

**Reviewed transitive anchors:** `engine/battle/effect_commands.asm:593 ResetTurn`; `engine/battle/abilities.asm:863 CheckNullificationAbilities`; `engine/battle/effect_commands.asm:2782 GetFailureResultText`; `engine/battle/effect_commands.asm:6235 CheckSubstituteOpp`; `engine/battle/effect_commands.asm:627 CheckAffection`

**Branch adjudication:**

| Source branch | Production binding | Distinguishing evidence and disposition |
| --- | --- | --- |
| All 255 raw accuracies, combined stages, integer floors and weather/ability/item exceptions | `GetPolishedTotalAccuracy / DoesMoveMissTarget / source accuracy helpers` | Production-function vectors cover every source row; live fourth/fifth-stage cutoffs, target Umbrella, Cloud Nine, rain/hail, accuracy abilities, item multiplication and suppression establish runtime application. The inventory count alone is not the evidence. Evidence: `test/battle/polished_accuracy_order.c:7` — Polished accuracy preserves lowered evasion for Keen Eye Illuminate and Minds Eye; `test/battle/polished_accuracy_order.c:24` — Polished accuracy Illuminate ignores positive evasion under old native presets; `test/battle/polished_accuracy_order.c:37` — Polished accuracy sun uses the pinned raw Thunder and Hurricane byte; remaining exact fixture titles are listed in the JSON row |
| Primary stat and blocked damaging scripts check accuracy before caps/defensive rejection | `TARGET_FAILURE_POLISHED_ACCURACY / StatChangeCanAnyChange / CancelerPriorityBlock` | Matrices distinguish all nine primary moves, caps, Guard Spec/abilities/items/doll, Armor Tail/Soundproof and called Quick Attack. Native controls and the Sucker Punch precondition prove the phase is source-scoped. Evidence: `test/battle/move_effect/polished_stat_command_order.c:16` — Polished command stat accuracy precedes caps for every primary target stat script; `test/battle/move_effect/polished_stat_command_order.c:41` — Polished command stat accuracy keeps the exact ninety percent capped Growl cutoff; `test/battle/move_effect/polished_stat_command_order.c:61` — Polished command stat accuracy misses precede every source defensive stat guard; remaining exact fixture titles are listed in the JSON row |
| Reflection carries original hit/miss/immunity and attributes Policy to the current reflector | `MoveEndBouncedMove / polishedAccuracyState / PolishedAccuracyFailure` | Actual scenes and final statuses distinguish retained original misses, original guaranteed hits without rerolls, cached-state reset, Toxic versus Steel, Thunder Wave versus Ground, current returned immunity, Magic Coat and converted-type native fallback. Evidence: `test/battle/ability/polished_magic_bounce_accuracy.c:4` — Polished Magic Bounce accuracy preserves original status misses through reflection; `test/battle/ability/polished_magic_bounce_accuracy.c:32` — Polished Magic Bounce accuracy preserves original stat misses through reflection; `test/battle/ability/polished_magic_bounce_accuracy.c:59` — Polished Magic Bounce accuracy preserves confusion seed and source Foresight misses; remaining exact fixture titles are listed in the JSON row |
| Sing nullification differs from the other sound moves; affection remains independent | `TARGET_FAILURE_BOUNCE source checkpowder gate / BlocksPrankster / DoesMoveMissTarget` | Source Sing reruns Soundproof before priority even on a reflected miss; ordinary Screech preserves its accuracy miss through priority. Affection level-three evasion is a separate 1/10 roll with bypass and Policy controls. Evidence: `test/battle/move_effect/polished_sound_priority.c:14` — Polished sound priority applies Prankster Dark and Soundproof gates; `test/battle/move_effect/polished_sound_priority.c:41` — Polished sound priority retains positive-priority Armor Tail and Mold Breaker; `test/battle/move_effect/polished_sound_priority.c:64` — Polished sound priority rejects Dark targets before Magic Bounce; remaining exact fixture titles are listed in the JSON row |

**Actual test catalog entries:**

- `test/battle/ability/polished_magic_bounce_accuracy.c:4` — Polished Magic Bounce accuracy preserves original status misses through reflection (source catalog).
- `test/battle/ability/polished_magic_bounce_accuracy.c:32` — Polished Magic Bounce accuracy preserves original stat misses through reflection (source catalog).
- `test/battle/ability/polished_magic_bounce_accuracy.c:59` — Polished Magic Bounce accuracy preserves confusion seed and source Foresight misses (source catalog).
- `test/battle/ability/polished_magic_bounce_accuracy.c:87` — Polished Magic Bounce accuracy reuses an original status hit without a reflected roll (source catalog).
- `test/battle/ability/polished_magic_bounce_accuracy.c:111` — Polished Magic Bounce accuracy restores ordinary rolls after reflection (source catalog).
- `test/battle/ability/polished_magic_bounce_accuracy.c:132` — Polished Magic Bounce accuracy retains returned status immunities (source catalog).
- `test/battle/ability/polished_magic_bounce_accuracy.c:159` — Polished Magic Bounce accuracy retains original type immunity through reflection (source catalog).
- `test/battle/ability/polished_magic_bounce_accuracy.c:194` — Polished Magic Bounce accuracy keeps ordinary and Mold Breaker move rolls (source catalog).
- `test/battle/ability/polished_magic_bounce_accuracy.c:214` — Polished Magic Bounce accuracy leaves optional moves Magic Coat and gimmicks native (source catalog).
- `test/battle/ability/polished_magic_bounce_accuracy.c:237` — Polished Magic Bounce accuracy leaves optional doubles native (source catalog).
- 111 additional matched entries are listed explicitly in `SUITE2_MOVE_RECONCILIATION.json`; they are not promoted to complete branch coverage.

**Remaining / scope:** All finite new miss-routing matrices passed source v8; native Magic Bounce and sound-priority also passed. v8 exposed the late stat Substitute check and one probabilistic-fixture setup error, both corrected before v9. Dynamically retyped optional Ion Deluge, Magic Coat, optional moves, doubles and either gimmick retain explicit native controls. No unblocked ordinary damaging multihit action is cached by the narrow blocker path.

### shared_damage

**Owner:** root shared pipeline review. **Status:** source/runtime semantically reconciled; current full source/native catalog execution passed; local production/input acceptance passed; associated Actions records verify the containing commit

**Commands:** `critical`, `damagestats`, `damagecalc`, `stab`, `damagevariation`, `constantdamage`, `conditionalboost`, `doubleflyingdamage`, `doubleminimizedamage`, `doubleundergrounddamage`, `falseswipe`, `getmagnitude`, `gyroball`, `happinesspower`, `hiddenpower`, `lowkick`, `resettypematchup`

The source compresses raw attack/defense to bytes after item/weather/screen modifications, then applies stage, offensive/defensive ability, burn, Flash Fire, critical and held-item multipliers in the authenticated order before division and +2. Combined STAB/type/weather, Parental Bond quarter damage, Tinted Lens/Filter/Expert Belt, conditional multipliers, 85–100% roll and post-roll doubles/Knock Off retain their integer boundaries. Critical probability is 1/24, 3/24, 12/24 or guaranteed, with source Focus Energy and selectable affection. Dynamic Return/Hidden Power/Magnitude/Low Kick/Gyro Ball and constant/retaliation damage use their respective source commands rather than ordinary modern damage. The selectable classic-category flag reads the source table type before current battle conversion. Light Metal on a source weight-one target is reachable nontermination: the target deliberately uses bounded native arithmetic for that input; this is a documented deviation, not an exclusion or passing parity result.

**Authenticated source anchors:** `engine/battle/effect_commands.asm:1172 BattleCommand_critical`; `engine/battle/effect_commands.asm:3894 BattleCommand_damagestats`; `engine/battle/effect_commands.asm:4248 BattleCommand_damagecalc`; `engine/battle/effect_commands.asm:1458 BattleCommand_stab`; `engine/battle/effect_commands.asm:1894 BattleCommand_damagevariation`; `engine/battle/effect_commands.asm:4511 BattleCommand_constantdamage`; `engine/battle/effect_commands.asm:6303 BattleCommand_conditionalboost`; `engine/battle/effect_commands.asm:6349 BattleCommand_doubleflyingdamage`; `engine/battle/effect_commands.asm:6445 BattleCommand_doubleminimizedamage`; `engine/battle/effect_commands.asm:6355 BattleCommand_doubleundergrounddamage`; `engine/battle/move_effects/false_swipe.asm:1 BattleCommand_falseswipe`; `engine/battle/move_effects/magnitude.asm:1 BattleCommand_getmagnitude`; `engine/battle/move_effects/gyro_ball.asm:1 BattleCommand_gyroball`; `engine/battle/move_effects/return.asm:1 BattleCommand_happinesspower`; `engine/battle/move_effects/hidden_power.asm:1 BattleCommand_hiddenpower`; `engine/battle/move_effects/low_kick.asm:1 BattleCommand_lowkick`; `engine/battle/effect_commands.asm:1870 BattleCommand_resettypematchup`

**Actual runtime definitions:** `src/battle_util.c:8965 CalculateMoveDamageVars`; `src/polished_damage.c:236 PolishedDamageBeforeRoll`; `src/polished_damage.c:365 PolishedDamageAfterRoll`; `src/battle_util.c:8436 DoFixedDamageMoveCalc`; `src/battle_util.c:8705 CalcCritChanceStage`; `src/battle_util.c:8766 IsCriticalHit`; `src/battle_util.c:8872 GetAdjustedDamage`; `src/polished_damage.c:65 UsesPolishedDamageRules`; `src/polished_damage.c:24 PolishedLowKickPower`; `src/polished_damage.c:19 PolishedBattleWeight`; `include/polished_damage.h:14 PolishedDamageScale`; `src/battle_util.c:9878 GetBattleMoveCategory`

**Branch adjudication:**

| Source branch | Production binding | Distinguishing evidence and disposition |
| --- | --- | --- |
| Raw stat compression, staged numerator, additive/clamped damage, integer multiplier sequence | `PolishedDamageBeforeRoll / PolishedDamageScale / PolishedDamageAfterRoll` | Source production calculator vectors and live exact-damage controls include screens, critical stages, weather, burn, each source offensive/defensive ability, item fractions, suppression, Protective Pads and Parental Bond. Evidence: `test/battle/polished_damage.c:7` — Polished damage matches all source stat compression stage and critical vectors; `test/battle/polished_damage.c:34` — Polished damage applies offensive ability burn critical and held boosts before adding two; `test/battle/polished_damage.c:55` — Polished damage rolls after STAB type and weather on both sides; remaining exact fixture titles are listed in the JSON row |
| Critical base and stage thresholds, forced/no-crit cases, original identity and affection | `CalcCritChanceStageForRules / IsCriticalHit / GetPolishedAffectionLevel` | Tagged probability tests distinguish source 24-way thresholds, old native presets, armor/suppression, Focus Energy, original/external Future Sight identity and affection numerator changes. Evidence: `test/battle/move_effect/polished_command_edges.c:135` — Polished command Focus Energy keeps two source stages under the native generation two preset; `test/battle/polished_affection.c:89` — Polished affection doubles the critical numerator independently at stages zero one and two; `test/battle/polished_battle_core.c:116` — Polished core default affection keeps high friendship critical odds at the source base rate; remaining exact fixture titles are listed in the JSON row |
| Return, Hidden Power, Magnitude, Low Kick/Gyro Ball, fixed damage and 16-bit retaliation | `CalcMoveBasePower / PolishedLowKickPower / DoFixedDamageMoveCalc` | Source-wide friendship and DV vectors, exact weight/division bands, probability scripts, living-HP clipping and production 32767/32768/65534 saturation probes are separately identified. Artificial >32K HP probe inputs are not claimed as ordinary legal battles. Evidence: `test/battle/move_effect/polished_fixed_damage.c:4` — Polished fixed damage ignores level, stats, STAB and nonzero type multipliers; `test/battle/move_effect/polished_fixed_damage.c:31` — Polished fixed damage is unchanged by Technician, Choice Specs, Multiscale and Light Screen; `test/battle/move_effect/polished_fixed_damage.c:50` — Polished fixed damage preserves Ghost and Fairy immunities; remaining exact fixture titles are listed in the JSON row |
| Selectable pre-split categories and post-hit survival decisions | `GetBattleMoveCategory / GetAdjustedDamage / source survival helpers` | Live category-dependent damage and Counter/Mirror Coat assertions supplement type-table provenance; Endure/False Swipe, affection, Focus items and Sturdy each have ordered and multihit controls. Evidence: `test/battle/hold_effect/polished_survival_items.c:4` — Polished survival Focus Band uses its raw thirty byte outcomes; `test/battle/hold_effect/polished_survival_items.c:20` — Polished survival Focus Band checks byte twenty nine versus thirty; `test/battle/hold_effect/polished_survival_items.c:36` — Polished survival a failed Focus Band roll cannot fall back to Sturdy; remaining exact fixture titles are listed in the JSON row |

**Actual test catalog entries:**

- `test/battle/move_effect/polished_attack_suite.c:41` — Polished attack suite unblocked source attacks actually damage either target (source catalog).
- `test/battle/polished_affection.c:89` — Polished affection doubles the critical numerator independently at stages zero one and two (source catalog).
- `test/battle/polished_category_option.c:7` — Polished category option uses source table types before battle type conversion (source catalog).
- `test/battle/polished_category_option.c:48` — Polished category option routes actual damage into Counter or Mirror Coat (source catalog).
- `test/battle/polished_category_option.c:79` — Polished category option leaves optional moves and either active gimmick native (source catalog).
- `test/battle/polished_damage_closure.c:4` — Polished damage closure Guts includes statused physical attacks and excludes special or suppressed boosts (source catalog).
- `test/battle/polished_damage_closure.c:38` — Polished damage closure Analytic follows actual prior actions including priority switching and cancellation (source catalog).
- `test/battle/polished_damage_closure.c:73` — Polished damage closure pinch abilities check exact HP type and suppression (source catalog).
- `test/battle/polished_damage_closure.c:111` — Polished damage closure Rivalry preserves both genderless branches and exact fractions (source catalog).
- `test/battle/polished_damage_closure.c:134` — Polished damage closure Sand Force checks weather type Gas and Cloud Nine but not Umbrella (source catalog).
- 102 additional matched entries are listed explicitly in `SUITE2_MOVE_RECONCILIATION.json`; they are not promoted to complete branch coverage.

**Remaining / scope:** The finite offensive-ability closure matrix passed source/native v8. The corrected source category/gimmick fixture and its arithmetic/catalog dependencies passed in the current consolidated execution. Preserve the single explicit weight-one/Light Metal nontermination deviation. Optional terrains, native-only damage items/forms/moves/gimmicks keep the adapter fallback; they are not source inventory failures.

### shared_stats

**Owner:** root shared pipeline review. **Status:** reviewed; Weakness Policy focused and full source/native execution passed; local production/input acceptance passed; associated Actions records verify the containing commit

**Commands:** `forceraisestat`, `forcelowerstat`, `loweroppstat`, `loweroppstathit`, `lowerstat`, `raiseoppstat`, `raisestathit`

FarChangeStat distinguishes self/target, forced/CANMISS and secondary calls. A primary target accuracy failure precedes the current Substitute helper, Mist from actual Guard Spec, effective stat abilities, Clear Amulet and caps. Swagger retains its earlier doll guard. Secondary effects retain their independent effect chance/SubHit restrictions, then use the same source guards; queued MOVE_NONE is not a reason to fall back to native Illuminate/Mist rules. Contrary reverses and Simple adjusts the requested amount before actual capped mutation; pending Mirror Herb records actual increments and discards consumed or failed checkpoints. Capped forced changes return to the next command, allowing Minimize/Curl to set their own bits.

**Authenticated source anchors:** `engine/battle/effect_commands.asm:5273 BattleCommand_forceraisestat`; `engine/battle/effect_commands.asm:5280 BattleCommand_forcelowerstat`; `engine/battle/effect_commands.asm:5328 BattleCommand_loweroppstat`; `engine/battle/effect_commands.asm:5344 BattleCommand_loweroppstathit`; `engine/battle/effect_commands.asm:5265 BattleCommand_lowerstat`; `engine/battle/effect_commands.asm:5320 BattleCommand_raiseoppstat`; `engine/battle/effect_commands.asm:5288 BattleCommand_raisestathit`

**Actual runtime definitions:** `src/battle_move_resolution.c:6915 DoStatChange`; `src/battle_stat_change.c:939 AdjustStatStage`; `src/battle_stat_change.c:517 IncreaseStat`; `src/battle_stat_change.c:457 DecreaseStat`; `src/battle_stat_change.c:1161 SetAdditionalEffectsOnStatChange`; `src/battle_move_resolution.c:6581 StatChangeCanAnyChange`; `src/battle_stat_change.c:436 UsesPolishedStatCommandRules`; `src/battle_stat_change.c:444 CanDecreaseStat`; `src/battle_stat_change.c:717 IsMistProtected`; `src/battle_stat_change.c:844 IsAbilityBlocked`; `src/battle_stat_change.c:977 AbilityPreventsSpecificStatDrop`

**Reviewed transitive anchors:** `engine/battle/stats.asm:1 FarChangeStat`; `engine/battle/stats.asm:314 DoRaiseStat`; `engine/battle/stats.asm:311 DoLowerStat`; `engine/battle/effect_commands.asm:6235 CheckSubstituteOpp`

**Branch adjudication:**

| Source branch | Production binding | Distinguishing evidence and disposition |
| --- | --- | --- |
| Primary accuracy before cap, current doll, Guard Spec and effective defensive abilities/items | `StatChangeCanAnyChange / IsMistProtected / CanDecreaseStat` | The nine-move matrix and actual Guard Spec item distinguish source timing, Infiltrator, Gas, Illuminate legacy presets and Clear Amulet precedence. The secondary Octazooka extension validates queued guard context; optional Tickle/Kinesis and gimmicks remain native. Evidence: `test/battle/move_effect/polished_stat_command_order.c:16` — Polished command stat accuracy precedes caps for every primary target stat script; `test/battle/move_effect/polished_stat_command_order.c:41` — Polished command stat accuracy keeps the exact ninety percent capped Growl cutoff; `test/battle/move_effect/polished_stat_command_order.c:61` — Polished command stat accuracy misses precede every source defensive stat guard; remaining exact fixture titles are listed in the JSON row |
| All stat directions, partial caps, Contrary/Simple and continuation after a failed stat step | `AdjustStatStage / IncreaseStat / DecreaseStat / SetAdditionalEffectsOnStatChange` | Shared seven-stat production/live tests connect to the move-specific Growth, Haze, Minimize and capped-Curl command continuations. These are actual state assertions, not merely successful move animations. Evidence: `test/battle/move_effect/polished_attack_suite.c:68` — Polished attack suite source stat changes and limits work on either side; `test/battle/move_effect/polished_command_closure.c:4` — Polished command closure Growth uses effective user weather and independent stat caps; `test/battle/move_effect/polished_command_closure.c:38` — Polished command closure Haze resets all seven stages on both sides; remaining exact fixture titles are listed in the JSON row |
| Reactive stat checkpoints copy actual increments once, then clear or preserve the correct queue | `StatChanged / TryStatChange / source Mirror Herb checkpoint helpers` | Fixtures cover capped copies, acquiring/losing the herb, Simple/Contrary, accuracy/evasion, suppression, no feedback, nested Defiant/Competitive, between-hit checks and faint/replacement cleanup. Evidence: `test/battle/ability/polished_hit_stat_abilities.c:4` — Polished hit stats Justified Rattled and Weak Armor activate on the exact Substitute break; `test/battle/ability/polished_hit_stat_abilities.c:36` — Polished hit stats breaking Substitute preserves Gas suppression but ignores Mold Breaker; `test/battle/ability/polished_hit_stat_abilities.c:63` — Polished hit stats Anger Point and its Mirror Herb copy include intact and broken Substitutes; remaining exact fixture titles are listed in the JSON row |

**Actual test catalog entries:**

- `test/battle/move_effect/polished_command_closure.c:4` — Polished command closure Growth uses effective user weather and independent stat caps (source catalog).
- `test/battle/move_effect/polished_command_closure.c:38` — Polished command closure Haze resets all seven stages on both sides (source catalog).
- `test/battle/move_effect/polished_command_closure.c:62` — Polished command closure Haze preserves Focus Energy Minimize and Defense Curl (source catalog).
- `test/battle/move_effect/polished_command_closure.c:88` — Polished command closure capped Defense Curl still boosts Rollout (source catalog).
- `test/battle/move_effect/polished_command_edges.c:4` — Polished command Attract rejects Fly and Dig even when either battler has No Guard (source catalog).
- `test/battle/move_effect/polished_command_edges.c:33` — Polished command Attract checks gender before announcing Oblivious (source catalog).
- `test/battle/move_effect/polished_command_edges.c:58` — Polished command Attract retains an existing infatuation when its repeat fails (source catalog).
- `test/battle/move_effect/polished_command_edges.c:77` — Polished command Foresight rejects a repeat under the native generation three preset (source catalog).
- `test/battle/move_effect/polished_command_edges.c:101` — Polished command Minimize sets its volatile even when Evasion is already capped (source catalog).
- `test/battle/move_effect/polished_command_edges.c:135` — Polished command Focus Energy keeps two source stages under the native generation two preset (source catalog).
- 60 additional matched entries are listed explicitly in `SUITE2_MOVE_RECONCILIATION.json`; they are not promoted to complete branch coverage.

**Remaining / scope:** v8 identified and v9 candidate fixes address precisely the late doll gate, queued secondary Illuminate scope and Rage forced-self script. Earlier mirror-ledger/secondary/stat-item catalogs remain relevant execution evidence, not generic ownership handoffs. All 54 command groups passed source/native v9. The broader source/native status-gate followup passed v10 after replacing unmapped optional Bind with source Wrap in its fixture. The later full current-candidate rerun also passed and is recorded separately from those focused results. The finite Weakness Policy phase correction passed all eight source/native focused v12 groups; current full-catalog execution also passed. The original late-only source runtime was wrong for 59 early supereffectivetext families (162 mapped moves), changing Justified/Weak Armor immediate Mirror Herb totals. GetPolishedWeaknessPolicyPhase and the two BS_PolishedSuperEffectiveItem hooks now use the authenticated early/late/absent classification; IsOnTargetHitActivation suppresses every early/absent late retry, including an initially capped holder whose stat later drops. The two ordinary multihit families (seven moves) remain after hit effects on the final strike, with the source attacker-faint termination guard; PB second hit returns. Future Sight scheduling is excluded, while its actual impact uses UsesPolishedHitCheckpoint. Holder life, current item, self-stat caps/Contrary, Substitute bypass and zero-damage False Swipe admission match the source. Independent bounded static review passed, followed by eight/eight source and eight/eight native groups in weakness-policy-v12-{1,0}.log. These are the distinguishing phase fixtures, not a promotion of prior conditional-item tests. The v11 ordinary contact fixture used unmapped optional Rock Smash; replacing it with mapped Low Kick corrected that expectation without a runtime change. Full-catalog execution passed; local production gates passed; associated Actions records verify the containing commit.

### shared_status

**Owner:** root shared pipeline review. **Status:** source/runtime semantically reconciled; current full source/native catalog execution passed; local production/input acceptance passed; associated Actions records verify the containing commit

**Commands:** `burn`, `burntarget`, `confuse`, `confusetarget`, `freezetarget`, `paralyze`, `paralyzetarget`, `poison`, `poisontarget`, `sleep`, `toxic`, `tristatuschance`, `effectchance`, `selfeffectchance`, `flinchtarget`

Primary source status scripts perform their explicit Safeguard and target guards in script order; CanStatusTarget then resolves current type, held prevention, Substitute, effective immunity/Leaf Guard, existing status and source duration. Corrosion bypass is confined to the authenticated Toxic/Poison Powder and self-orb paths. Secondary chance retains source byte overflow and separates successful chance from successful application through SubHit, Sheer Force, Shield Dust/Covert Cloak, suppression and living-target gates. Confusion has its own Own Tempo/already-confused/doll/manual-failure path and counter; flinch can clear recharge even when its volatile is blocked. Burntarget thaws an already frozen target in seven exact scripts unless Sheer Force, before hit abilities; Tri Attack jumps to BurnTarget and deliberately omits defrosting. A later living-target current-Fire thaw occurs after hit abilities and before Air Balloon/Rage.

**Authenticated source anchors:** `engine/battle/effect_commands.asm:5412 BattleCommand_burn`; `engine/battle/effect_commands.asm:5178 BattleCommand_burntarget`; `engine/battle/effect_commands.asm:5979 BattleCommand_confuse`; `engine/battle/effect_commands.asm:5955 BattleCommand_confusetarget`; `engine/battle/effect_commands.asm:5210 BattleCommand_freezetarget`; `engine/battle/effect_commands.asm:5408 BattleCommand_paralyze`; `engine/battle/effect_commands.asm:5204 BattleCommand_paralyzetarget`; `engine/battle/effect_commands.asm:5402 BattleCommand_poison`; `engine/battle/effect_commands.asm:5198 BattleCommand_poisontarget`; `engine/battle/effect_commands.asm:4852 BattleCommand_sleep`; `engine/battle/move_effects/toxic.asm:1 BattleCommand_toxic`; `engine/battle/move_effects/tri_attack.asm:1 BattleCommand_tristatuschance`; `engine/battle/effect_commands.asm:2380 BattleCommand_effectchance`; `engine/battle/effect_commands.asm:2400 BattleCommand_selfeffectchance`; `engine/battle/effect_commands.asm:5693 BattleCommand_flinchtarget`

**Actual runtime definitions:** `src/battle_util.c:5794 CanSetNonVolatileStatusInternal`; `src/battle_set_effect.c:39 HandleSetEffectNonVolatile`; `src/battle_set_effect.c:75 HandleSetEffectConfusion`; `src/battle_set_effect.c:99 HandleSetEffectFlinch`; `src/battle_move_resolution.c:4121 TryPolishedIncomingThaw`; `src/battle_move_resolution.c:4161 MoveEndPolishedPostHit`; `src/battle_script_commands.c:1756 TrySynchronizeActivation`

**Reviewed transitive anchors:** `engine/battle/effect_commands.asm:6235 CheckSubstituteOpp`; `engine/battle/effect_commands.asm:5235 DefrostOpponent`; `engine/battle/effect_commands.asm:4848 PostStatus`

**Branch adjudication:**

| Source branch | Production binding | Distinguishing evidence and disposition |
| --- | --- | --- |
| Primary status guard precedence and downstream Synchronize/cure order | `CanSetNonVolatileStatusInternal / HandleSetEffectNonVolatile / TrySynchronizeActivation` | Actual source cases distinguish current types, Safeguard/doll and Infiltrator/Gas, Corrosion move identity, immunity, Synchronize before berries, source forced-entry restrictions and effective ability acquisition/suppression. Evidence: `test/battle/ability/polished_absorption.c:4` — Polished absorption Sap Sipper skips source Grass status moves; `test/battle/ability/polished_absorption.c:24` — Polished absorption Volt Absorb and Motor Drive skip source Thunder Wave; `test/battle/ability/polished_absorption.c:40` — Polished absorption Lightning Rod blocks damage and status despite older native presets; remaining exact fixture titles are listed in the JSON row |
| Secondary probability versus application; confusion, flinch and freeze action state | `CalcSecondaryEffectChance / HandleSetEffectConfusion / HandleSetEffectFlinch / CancelerAsleepOrFrozen` | Tagged trials and post-state checks cover exact source probabilities/overflow, blockers/suppression, broken doll SubHit, chance without effect, duration/counter state and flinch clearing recharge. Evidence: `test/battle/move_effect/polished_secondary_stat_drops.c:5` — Polished secondary stat drops lower exactly the selected target stat by one on both sides; `test/battle/move_effect/polished_secondary_stat_drops.c:34` — Polished secondary stat drops use source probabilities and Serene Grace doubling; `test/battle/move_effect/polished_secondary_stat_drops.c:62` — Polished secondary stat drops respect Shield Dust Covert Cloak Sheer Force and Clear Body; remaining exact fixture titles are listed in the JSON row |
| Incoming burn-command thaw versus posthit current-Fire thaw, Tri Attack and native scope | `TryPolishedIncomingThaw / MoveEndPolishedPostFaint / MoveEndPolishedPostHit / MoveEndDefrost` | Eleven source/native groups cover exact seven scripts, Normalize, effective Sheer Force, secondary blockers, intact/broken doll/Infiltrator, miss/immunity/interruption, user/target KO, phase order and optional controls. The current full rerun passed after the documented fixture corrections. Evidence: `test/battle/ability/polished_hit_stat_abilities.c:134` — Polished hit stats Anger Point resolves during critical text before the burn command thaws; `test/battle/move_effect/polished_post_hit.c:4` — Polished command posthit Rage follows Anger Point and cannot boost after its critical cap; `test/battle/move_effect/polished_post_hit.c:30` — Polished command posthit runs Justified then Air Balloon then Rage before later hit items; remaining exact fixture titles are listed in the JSON row |

**Actual test catalog entries:**

- `test/battle/ability/polished_contact_volatile.c:6` — Polished volatile contact abilities use the exact Substitute break boundary (source catalog).
- `test/battle/ability/polished_contact_volatile.c:34` — Polished volatile contact Cute Charm uses source three-in-ten activation under every preset (source catalog).
- `test/battle/ability/polished_contact_volatile.c:53` — Polished volatile contact Tangling Hair cannot lower stats after its holder faints (source catalog).
- `test/battle/ability/polished_contact_volatile.c:71` — Polished volatile contact Tangling Hair respects the attacking Pokemon's Substitute (source catalog).
- `test/battle/ability/polished_contact_volatile.c:90` — Polished volatile contact Perish Body starts the attacker's count even when its holder faints (source catalog).
- `test/battle/ability/polished_contact_volatile_matrix.c:7` — Polished volatile contact classification covers all 124 direct attacks for all three abilities (source catalog).
- `test/battle/ability/polished_contact_volatile_matrix.c:30` — Polished volatile contact Protective Pads Punching Glove and optional Long Reach prevent activation (source catalog).
- `test/battle/ability/polished_contact_volatile_matrix.c:48` — Polished volatile contact rejects misses Protect noncontact damage and status moves (source catalog).
- `test/battle/ability/polished_contact_volatile_matrix.c:68` — Polished volatile contact Mold Breaker does not suppress these abilities but Neutralizing Gas does (source catalog).
- `test/battle/ability/polished_contact_volatile_matrix.c:88` — Polished volatile contact attacker Substitute blocks Tangling Hair but not Cute Charm or Perish Body (source catalog).
- 193 additional matched entries are listed explicitly in `SUITE2_MOVE_RECONCILIATION.json`; they are not promoted to complete branch coverage.

**Remaining / scope:** The source incoming-thaw candidate had v8 fixture errors (raw zero caused natural thaw; Tri Attack RNG expects an index; fresh native freeze obscured the gimmick control); these were corrected before v9 without altering the established thaw hooks; all eleven groups then passed both profiles. The shared stat doll regression is recorded under shared_stats. Source prevention HELD codes with no corresponding default item attribute do not supply a live default item case, but shared routine semantics are retained.

### shared_damage_faint

**Owner:** root shared pipeline review. **Status:** reviewed; Weakness Policy focused and full source/native execution passed; local production/input acceptance passed; associated Actions records verify the containing commit

**Commands:** `applydamage`, `postfainteffects`, `posthiteffects`

applydamage clips assigned damage to real HP or doll HP and resolves survival before writing HP. Immediate source ordinary healing/status/stat items and Enigma run at their source checkpoints; Enigma can revive zero HP before postfaint and ignores ordinary pending-loss suppression. Single-hit or final-hit Berserk uses its required pre/post-healing state. criticaltext activates Anger Point and checks Mirror Herb even after an earlier berry capped Attack; supereffectivetext can activate Weakness Policy. postfainteffects orders move recoil/drain and Destiny Bond or faint abilities; posthiteffects checks current doll and failure state, then hit abilities, current-Fire thaw, Air Balloon and Rage, followed by source reactive items and deferred switching. A command return does not terminate the remaining script, explaining Bug Bite after intact-doll posthit rejection.

**Authenticated source anchors:** `engine/battle/effect_commands.asm:2656 BattleCommand_applydamage`; `engine/battle/effect_commands.asm:3198 BattleCommand_postfainteffects`; `engine/battle/effect_commands.asm:3238 BattleCommand_posthiteffects`

**Actual runtime definitions:** `src/battle_move_resolution.c:3482 TryMoveDamageUpdate`; `src/battle_move_resolution.c:4043 MoveEndRage`; `src/battle_move_resolution.c:5607 MoveEndPolishedSwitchItems`; `src/battle_move_resolution.c:5760 MoveEndPolishedDeferredSwitch`; `src/battle_move_resolution.c:5000 MoveEndPolishedPostFaint`; `src/battle_move_resolution.c:4161 MoveEndPolishedPostHit`; `src/battle_move_resolution.c:4121 TryPolishedIncomingThaw`; `src/battle_util.c:3307 UsesPolishedHitCheckpoint`; `src/battle_move_resolution.c:3636 CancelerPolishedDamageHealing`; `src/battle_move_resolution.c:3679 CancelerPolishedDamageEnigma`; `src/battle_move_resolution.c:3691 CancelerPolishedDamageBerserk`; `src/battle_script_commands.c:1312 BS_PolishedCriticalAngerPoint`; `src/battle_hold_effects.c:20 GetPolishedWeaknessPolicyPhase`; `src/battle_hold_effects.c:30 IsPolishedWeaknessPolicyActivation`; `src/battle_script_commands.c:1320 BS_PolishedSuperEffectiveItem`

**Reviewed transitive anchors:** `engine/battle/effect_commands.asm:6235 CheckSubstituteOpp`; `home/battle.asm:609 HasOpponentFainted`; `home/battle.asm:601 HasUserFainted`; `engine/battle/core.asm:1892 HandleUserHealingItems`; `engine/battle/move_effects/knock_off.asm:38 CheckPendingItemLoss`; `engine/battle/effect_commands.asm:5235 DefrostOpponent`

**Branch adjudication:**

| Source branch | Production binding | Distinguishing evidence and disposition |
| --- | --- | --- |
| Before-faint HP updates, survival, berries and Enigma revival | `TryMoveDamageUpdate / source immediate-damage cancelers / GetAdjustedDamage` | Source/native assertions observe exact HP/item state and queued pending action, including lethal Enigma, reserved Knock Off/Bug Bite loss, ordinary healing before criticaltext and per-hit survival. Evidence: `test/battle/ability/polished_hit_stat_abilities.c:99` — Polished hit stats immediate Liechi consumption precedes critical text and Anger Point caps; `test/battle/ability/polished_hit_stat_abilities.c:157` — Polished hit stats lethal Enigma restores its holder before faint effects and its pending action; `test/battle/ability/polished_hit_stat_abilities.c:211` — Polished hit stats Future Sight consumes Liechi before critical text and Anger Point; remaining exact fixture titles are listed in the JSON row |
| Critical and super-effective text commands have mechanical effects | `BS_PolishedCriticalAngerPoint / TryWeaknessPolicy / source certain self-stat scripts` | Anger Point/Mirror Herb and Weakness Policy are Suite 2 mechanics. Only exact wording/sprite/animation timing is Suite 3. Intact/broken dolls and actual stat changes have distinct controls. The finite Weakness Policy phase correction passed all eight source/native focused v12 groups; current full-catalog execution also passed. The original late-only source runtime was wrong for 59 early supereffectivetext families (162 mapped moves), changing Justified/Weak Armor immediate Mirror Herb totals. GetPolishedWeaknessPolicyPhase and the two BS_PolishedSuperEffectiveItem hooks now use the authenticated early/late/absent classification; IsOnTargetHitActivation suppresses every early/absent late retry, including an initially capped holder whose stat later drops. The two ordinary multihit families (seven moves) remain after hit effects on the final strike, with the source attacker-faint termination guard; PB second hit returns. Future Sight scheduling is excluded, while its actual impact uses UsesPolishedHitCheckpoint. Holder life, current item, self-stat caps/Contrary, Substitute bypass and zero-damage False Swipe admission match the source. Independent bounded static review passed, followed by eight/eight source and eight/eight native groups in weakness-policy-v12-{1,0}.log. These are the distinguishing phase fixtures, not a promotion of prior conditional-item tests. The v11 ordinary contact fixture used unmapped optional Rock Smash; replacing it with mapped Low Kick corrected that expectation without a runtime change. Full-catalog execution passed; local production gates passed; associated Actions records verify the containing commit. Evidence: `test/battle/ability/polished_hit_stat_abilities.c:63` — Polished hit stats Anger Point and its Mirror Herb copy include intact and broken Substitutes; `test/battle/ability/polished_hit_stat_abilities.c:99` — Polished hit stats immediate Liechi consumption precedes critical text and Anger Point caps; `test/battle/ability/polished_hit_stat_abilities.c:134` — Polished hit stats Anger Point resolves during critical text before the burn command thaws; remaining exact fixture titles are listed in the JSON row |
| Recoil, Ooze, Destiny Bond, Moxie/Aftermath, hit items and deferred faint order | `MoveEndPolishedPostFaint / MoveEndPolishedPostHit / deferred switch executor` | Existing scenes and state assertions establish lethal ordering, first-faint overrides, Magic Guard/contact gates, reservations before items, canceled reservations and no duplicate Pursuit. Production-linked trainer Shift and wild-faint terminal input cases passed in both profiles; original link/facility context obligations retain their Suite 3 owner. Evidence: `test/battle/move_effect/polished_post_hit.c:4` — Polished command posthit Rage follows Anger Point and cannot boost after its critical cap; `test/battle/move_effect/polished_post_hit.c:30` — Polished command posthit runs Justified then Air Balloon then Rage before later hit items; `test/battle/move_effect/polished_post_hit.c:61` — Polished command posthit Rage uses the current Substitute after the breaking hit; remaining exact fixture titles are listed in the JSON row |
| Weakness Policy exact early/late/absent command phases, with no retry after an early capped attempt | `GetPolishedWeaknessPolicyPhase / BS_PolishedSuperEffectiveItem / ItemBattleEffects / TryWeaknessPolicy` | Eight current groups assert early Justified/Weak Armor plus Mirror Herb, late ordinary multihit, first PB strike, capped secondary/no retry, zero-loss False Swipe, delayed Future Sight, early Explosion attacker HP0 and late contact-user KO. Runtime/table implementation and independent static review are complete; all eight groups passed source/native focused v12. The subsequent full run independently passed in both source and native profiles. Evidence: `test/battle/hold_effect/polished_weakness_policy.c:4` — Polished Weakness Policy precedes ordinary hit abilities and their Mirror Herb checkpoint; `test/battle/hold_effect/polished_weakness_policy.c:37` — Polished Weakness Policy follows the final ordinary multihit ability checkpoint; `test/battle/hold_effect/polished_weakness_policy.c:67` — Polished Weakness Policy cannot retry after a secondary reopens a capped stat; remaining exact fixture titles are listed in the JSON row |

**Actual test catalog entries:**

- `test/battle/ability/polished_hit_stat_abilities.c:4` — Polished hit stats Justified Rattled and Weak Armor activate on the exact Substitute break (source catalog).
- `test/battle/ability/polished_hit_stat_abilities.c:36` — Polished hit stats breaking Substitute preserves Gas suppression but ignores Mold Breaker (source catalog).
- `test/battle/ability/polished_hit_stat_abilities.c:63` — Polished hit stats Anger Point and its Mirror Herb copy include intact and broken Substitutes (source catalog).
- `test/battle/ability/polished_hit_stat_abilities.c:99` — Polished hit stats immediate Liechi consumption precedes critical text and Anger Point caps (source catalog).
- `test/battle/ability/polished_hit_stat_abilities.c:134` — Polished hit stats Anger Point resolves during critical text before the burn command thaws (source catalog).
- `test/battle/ability/polished_hit_stat_abilities.c:157` — Polished hit stats lethal Enigma restores its holder before faint effects and its pending action (source catalog).
- `test/battle/ability/polished_hit_stat_abilities.c:211` — Polished hit stats Future Sight consumes Liechi before critical text and Anger Point (source catalog).
- `test/battle/ability/polished_hit_stat_abilities.c:238` — Polished hit stats Future Sight Parental Bond repeats and cancels Berserk after intermediate healing (source catalog).
- `test/battle/ability/polished_hit_stat_abilities.c:283` — Polished hit stats immediate Enigma precedes reserved Knock Off and Bug Bite loss (source catalog).
- `test/battle/ability/polished_hit_stat_abilities.c:321` — Polished hit stats optional doubles retain native Anger Point Substitute rejection (source catalog).
- 141 additional matched entries are listed explicitly in `SUITE2_MOVE_RECONCILIATION.json`; they are not promoted to complete branch coverage.

**Remaining / scope:** The active Future Sight/Parental Bond hit-2 criticaltext reentry, source self-item Policy through Substitute and source Rage after a breaking doll are implemented corrections with passing focused source/native v9 (hitstats/Rage) and v10 (Policy) evidence; full-catalog execution also passed. Production input validation passed all 26 source/native Switch/Predict and wild faint branches in the switching ledger; ordinary effect/faint consequences are no longer left as a generic owner handoff. The finite Weakness Policy phase correction passed all eight source/native focused v12 groups; current full-catalog execution also passed. The original late-only source runtime was wrong for 59 early supereffectivetext families (162 mapped moves), changing Justified/Weak Armor immediate Mirror Herb totals. GetPolishedWeaknessPolicyPhase and the two BS_PolishedSuperEffectiveItem hooks now use the authenticated early/late/absent classification; IsOnTargetHitActivation suppresses every early/absent late retry, including an initially capped holder whose stat later drops. The two ordinary multihit families (seven moves) remain after hit effects on the final strike, with the source attacker-faint termination guard; PB second hit returns. Future Sight scheduling is excluded, while its actual impact uses UsesPolishedHitCheckpoint. Holder life, current item, self-stat caps/Contrary, Substitute bypass and zero-damage False Swipe admission match the source. Independent bounded static review passed, followed by eight/eight source and eight/eight native groups in weakness-policy-v12-{1,0}.log. These are the distinguishing phase fixtures, not a promotion of prior conditional-item tests. The v11 ordinary contact fixture used unmapped optional Rock Smash; replacing it with mapped Low Kick corrected that expectation without a runtime change. Full-catalog execution passed; local production gates passed; associated Actions records verify the containing commit.

### switching

**Owner:** switch/faint review. **Status:** source/runtime semantically reconciled; current full source/native catalog execution passed; local production/input acceptance passed; associated Actions records verify the containing commit

**Commands:** `arenatrap`, `batonpass`, `pursuit`, `roar`, `switchout`, `teleport`, `spikes`, `toxicspikes`

ForceDeferredSwitch retains the first reservation, checks living user/reserves before consumption and preserves exact source switch flags. Pursuit interception precedes departure abilities, and its KO aborts the departure pass. Normal departure clears source state, whereas Baton Pass retains only its specified stage/volatile/history bits. Source Roar alone carries the exact forced-target Mold Breaker hazard context; item switches do not. Simultaneous faint replacements are ordered by departed Speed, all hazard/status/item work completes before both entry abilities, and repeated entry KOs continue the same replacement batch. Faint clearing preserves the victim Mean Look trap until actual replacement. The runtime-selectable Switch/Predict offer and wild faint prompt use their actual production input paths.

**Authenticated source anchors:** `engine/battle/move_effects/mean_look.asm:1 BattleCommand_arenatrap`; `engine/battle/move_effects/baton_pass.asm:1 BattleCommand_batonpass`; `engine/battle/move_effects/pursuit.asm:1 BattleCommand_pursuit`; `engine/battle/move_effects/roar.asm:1 BattleCommand_roar`; `engine/battle/effect_commands.asm:5615 BattleCommand_switchout`; `engine/battle/move_effects/teleport.asm:1 BattleCommand_teleport`; `engine/battle/move_effects/spikes.asm:1 BattleCommand_spikes`; `engine/battle/move_effects/spikes.asm:21 BattleCommand_toxicspikes`

**Actual runtime definitions:** `src/battle_move_resolution.c:5760 MoveEndPolishedDeferredSwitch`; `src/battle_script_commands.c:5788 Cmd_forcerandomswitch`; `src/battle_main.c:3000 SwitchInClearSetData`; `src/battle_main.c:3134 FaintClearSetData`; `src/battle_script_commands.c:3469 CanBattlerSwitch`; `src/battle_move_resolution.c:35 PolishedReserveSwitch`; `src/battle_end_turn.c:176 PolishedOrderReplacements`; `src/battle_script_commands.c:3843 BS_PolishedShiftSwitchPrompt`; `src/battle_script_commands.c:3854 BS_PolishedShiftSwitchEntry`

**Actual test catalog entries:**

- `test/battle/ability/polished_departure_abilities.c:4` — Polished departure Natural Cure uses the effective ability and the departing party slot (source catalog).
- `test/battle/ability/polished_departure_abilities.c:28` — Polished departure Natural Cure follows Skill Swap acquisition and loss (source catalog).
- `test/battle/polished_cleanup_closure.c:20` — Polished cleanup actual loss restores every removed party berry before blackout (source catalog).
- `test/battle/polished_cleanup_closure.c:65` — Polished cleanup actual wild loss distinguishes removed berries eaten berries and nonberries (source catalog).
- `test/battle/polished_cleanup_closure.c:99` — Polished cleanup full party capture preserves current boxed items and existing backups (source catalog).
- `test/battle/polished_cleanup_closure.c:148` — Polished cleanup full party capture keeps the optional native swap prompt (source catalog).
- `test/battle/polished_switch_closure.c:7` — Polished switch closure wild Red Card switches the player before the pending enemy action (source catalog).
- `test/battle/polished_switch_closure.c:35` — Polished switch closure wild Red Card retains itself without a living player reserve (source catalog).
- `test/battle/polished_switch_closure.c:57` — Polished switch closure optional wild Red Card attacks retain native encounter termination (source catalog).
- `test/battle/polished_switch_closure.c:80` — Polished switch closure trainer Red Card uniformly selects each living reserve (source catalog).
- 102 additional matched entries are listed explicitly in `SUITE2_MOVE_RECONCILIATION.json`; they are not promoted to complete branch coverage.

**Remaining / scope:** Branch-by-branch source/runtime details are in SUITE2_SWITCHING_FAINTING_RECONCILIATION.md. The fifteen switch and four actual cleanup groups have source/native execution; the current consolidated rerun also passed. Production-linked real-input cases passed: Switch/Predict accept/decline/cancel-reoffer and wild accept/decline/trapped-victim/mandatory-selector branches, totaling 13 branches per profile in SUITE2_VALIDATION.json. Source option UI/default initialization, AI choice, link/facility and later rewards remain the original Suite 3 owners.

### presentation

**Owner:** Suite 2 mechanical hooks / Suite 3 visual presentation. **Status:** reviewed; Weakness Policy focused and full source/native execution passed; local production/input acceptance passed; associated Actions records verify the containing commit

**Commands:** `cleartext`, `criticaltext`, `lowersub`, `moveanim`, `moveanimnosub`, `raisesub`, `supereffectivetext`

cleartext/moveanim/moveanimnosub/lowersub/raisesub manage display state; temporary visible doll lowering does not delete its HP/volatile. criticaltext and supereffectivetext also have source mechanical hooks: criticaltext checks Anger Point and Mirror Herb, and supereffectivetext checks Weakness Policy after its life/type conditions. These mechanical hooks are reviewed in shared_damage_faint; their names are not a reason to classify them as cosmetic.

**Authenticated source anchors:** `home/battle.asm:825 BattleCommand_cleartext`; `engine/battle/effect_commands.asm:2888 BattleCommand_criticaltext`; `engine/battle/effect_commands.asm:2440 BattleCommand_lowersub`; `engine/battle/effect_commands.asm:2464 BattleCommand_moveanim`; `engine/battle/effect_commands.asm:2481 BattleCommand_moveanimnosub`; `engine/battle/effect_commands.asm:2598 BattleCommand_raisesub`; `engine/battle/effect_commands.asm:3008 BattleCommand_supereffectivetext`

**Actual runtime definitions:** `src/battle_util.c:1410 BattleScriptExecute`; `src/battle_script_commands.c:1126 Cmd_attackanimation`; `src/battle_script_commands.c:1285 Cmd_critmessage`; `src/battle_script_commands.c:1363 Cmd_resultmessage`; `src/battle_script_commands.c:1312 BS_PolishedCriticalAngerPoint`; `src/battle_hold_effects.c:294 TryWeaknessPolicy`

**Actual test catalog entries:**

- `test/battle/ability/polished_hit_stat_abilities.c:4` — Polished hit stats Justified Rattled and Weak Armor activate on the exact Substitute break (source catalog).
- `test/battle/ability/polished_hit_stat_abilities.c:36` — Polished hit stats breaking Substitute preserves Gas suppression but ignores Mold Breaker (source catalog).
- `test/battle/ability/polished_hit_stat_abilities.c:63` — Polished hit stats Anger Point and its Mirror Herb copy include intact and broken Substitutes (source catalog).
- `test/battle/ability/polished_hit_stat_abilities.c:99` — Polished hit stats immediate Liechi consumption precedes critical text and Anger Point caps (source catalog).
- `test/battle/ability/polished_hit_stat_abilities.c:134` — Polished hit stats Anger Point resolves during critical text before the burn command thaws (source catalog).
- `test/battle/ability/polished_hit_stat_abilities.c:157` — Polished hit stats lethal Enigma restores its holder before faint effects and its pending action (source catalog).
- `test/battle/ability/polished_hit_stat_abilities.c:211` — Polished hit stats Future Sight consumes Liechi before critical text and Anger Point (source catalog).
- `test/battle/ability/polished_hit_stat_abilities.c:238` — Polished hit stats Future Sight Parental Bond repeats and cancels Berserk after intermediate healing (source catalog).
- `test/battle/ability/polished_hit_stat_abilities.c:283` — Polished hit stats immediate Enigma precedes reserved Knock Off and Bug Bite loss (source catalog).
- `test/battle/ability/polished_hit_stat_abilities.c:321` — Polished hit stats optional doubles retain native Anger Point Substitute rejection (source catalog).
- 19 additional matched entries are listed explicitly in `SUITE2_MOVE_RECONCILIATION.json`; they are not promoted to complete branch coverage.

**Remaining / scope:** Mechanical hooks remain Suite 2 with explicit current-candidate fixtures. Exact text/animation/art, visible sprite timing and shared RNG stream remain the originally assigned Suite 3 work. The finite Weakness Policy phase correction passed all eight source/native focused v12 groups; current full-catalog execution also passed. The original late-only source runtime was wrong for 59 early supereffectivetext families (162 mapped moves), changing Justified/Weak Armor immediate Mirror Herb totals. GetPolishedWeaknessPolicyPhase and the two BS_PolishedSuperEffectiveItem hooks now use the authenticated early/late/absent classification; IsOnTargetHitActivation suppresses every early/absent late retry, including an initially capped holder whose stat later drops. The two ordinary multihit families (seven moves) remain after hit effects on the final strike, with the source attacker-faint termination guard; PB second hit returns. Future Sight scheduling is excluded, while its actual impact uses UsesPolishedHitCheckpoint. Holder life, current item, self-stat caps/Contrary, Substitute bypass and zero-damage False Swipe admission match the source. Independent bounded static review passed, followed by eight/eight source and eight/eight native groups in weakness-policy-v12-{1,0}.log. These are the distinguishing phase fixtures, not a promotion of prior conditional-item tests. The v11 ordinary contact fixture used unmapped optional Rock Smash; replacing it with mapped Low Kick corrected that expectation without a runtime change. Full-catalog execution passed; local production gates passed; associated Actions records verify the containing commit.

### multihit

**Owner:** move-command review with explicit shared dependencies. **Status:** reviewed/live scoped

**Commands:** `startloop`, `endloop`

Source fixed doubles ignore count modifiers; variable attacks select 2/3/4/5 using source probabilities, Skill Link before Loaded Dice. Each hit recalculates damage and contact state; initial cancellation, accuracy once, user/target KO and newly acquired sleep govern loop exit.

**Authenticated source anchors:** `engine/battle/effect_commands.asm:2955 BattleCommand_startloop`; `engine/battle/effect_commands.asm:5658 BattleCommand_endloop`

**Actual runtime definitions:** `src/battle_move_resolution.c:2790 SetRandomMultiHitCounter`; `src/battle_move_resolution.c:2806 CancelerMultihitMoves`; `src/battle_move_resolution.c:4777 MoveEndMultihitMove`; `src/battle_move_resolution.c:3772 DoAttackCanceler`

**Reviewed transitive anchors:** `engine/battle/effect_commands.asm:5640 EndMultihit`; `engine/battle/effect_commands.asm:1042 CheckUserIsCharging`

**Actual test catalog entries:**

- `test/battle/move_effect/polished_moves.c:15` — Polished Fury Strikes uses the source two-hit probability (source catalog).
- `test/battle/move_effect/polished_moves.c:30` — Polished Fury Strikes uses the source five-hit probability (source catalog).
- `test/battle/move_effect/polished_moves.c:45` — Polished Fury Strikes preserves Skill Link and Loaded Dice (source catalog).
- `test/battle/move_effect/polished_multihit.c:14` — Polished multihit existing sleep and flinch still prevent starting an attack (source catalog).
- `test/battle/move_effect/polished_multihit.c:36` — Polished multihit spends PP once and Pressure adds only one PP (source catalog).
- `test/battle/move_effect/polished_multihit.c:58` — Polished multihit miss and Protect abort before the first strike (source catalog).
- `test/battle/move_effect/polished_multihit.c:85` — Polished multihit accuracy is checked once for the entire sequence (source catalog).
- `test/battle/move_effect/polished_multihit.c:104` — Polished multihit processes intact and broken Substitute separately on each strike (source catalog).
- `test/battle/move_effect/polished_multihit.c:145` — Polished multihit target fainting stops further strikes and clears next-turn state (source catalog).
- `test/battle/move_effect/polished_multihit.c:173` — Polished multihit attacker fainting to Rocky Helmet ends the attack (source catalog).
- 23 additional matched entries are listed explicitly in `SUITE2_MOVE_RECONCILIATION.json`; they are not promoted to complete branch coverage.

**Remaining / scope:** Loop count/identity and stop conditions have actual source fixtures. Survival items, immediate healing, hit abilities and between-hit stat copies are now connected to shared_damage_faint, polished_survival_items.c, polished_hit_stat_abilities.c and polished_stat_items.c; they are not left as an unreviewed generic dependency. Active Future Sight Parental Bond hit-2 source reentry passed its corrected source/native v9 fixture and passed again in the current full catalog. Weakness Policy early Parental Bond versus final ordinary-loop timing is implemented and independently reviewed, with all eight source/native focused v12 groups passing and the current full execution also passed; local production acceptance passed; the seven late-family moves must retain their authenticated after-posthit checkpoint.

### charge

**Owner:** move-command review with explicit shared dependencies. **Status:** reviewed/live scoped

**Commands:** `charge`

Fly/Dig/Solar Beam first action, continuation, effective sunlight/Umbrella, Power Herb and CheckTurn cancellation preserve charge identity. Initial charge can proceed without a live opponent. Called ResetTurn makes continuation history/PP differ from direct moves.

**Authenticated source anchors:** `engine/battle/effect_commands.asm:5743 BattleCommand_charge`

**Actual runtime definitions:** `src/battle_move_resolution.c:684 CancelerCallSubmove`; `src/battle_move_resolution.c:1145 CancelerPPDeduction`; `src/battle_move_resolution.c:6977 GetRecordedMove`; `src/battle_script_commands.c:5741 Cmd_twoturnmoveschargestringandanimation`; `src/battle_util.c:1232 CancelMultiTurnMoves`

**Reviewed transitive anchors:** `engine/battle/effect_commands.asm:1042 CheckUserIsCharging`; `engine/battle/effect_commands.asm:593 ResetTurn`; `home/battle.asm:622 GetWeatherAfterUserUmbrella`; `engine/battle/effect_commands.asm:543 CheckPowerHerb`

**Actual test catalog entries:**

- `test/battle/polished_action_boundaries.c:249` — Polished charging moves pay PP once including Pressure with continuation or Power Herb (source catalog).
- `test/battle/polished_action_boundaries.c:277` — Polished Solar Beam sunlight skips charging without consuming Power Herb (source catalog).
- `test/battle/polished_action_boundaries.c:300` — Polished charging interruption releases Fly Dig and Solar Beam without a second PP cost (source catalog).
- `test/battle/polished_action_boundaries.c:323` — Polished charging called by Metronome preserves caller PP and cleared history against Pressure (source catalog).
- `test/battle/polished_action_boundaries.c:397` — Polished called charging history ends on the next selection or replacement (source catalog).
- `test/battle/polished_action_boundaries.c:424` — Polished called attacks spend only caller PP against Pressure (source catalog).
- `test/battle/polished_action_boundaries.c:488` — Polished charging starts against a fainted opponent and resumes only after replacement (source catalog).
- `test/battle/polished_move_history.c:187` — Polished move history Magic Bounce preserves source charging through a later own action (source catalog).

**Remaining / scope:** Exact sprite/text timing remains presentation owner. Direct and called Pressure paths, first-turn target faint and called-departure cleanup now have distinguishing execution tests.

### rampage

**Owner:** move-command review with explicit shared dependencies. **Status:** source/runtime semantically reconciled; current full source/native catalog execution passed; local production/input acceptance passed; associated Actions records verify the containing commit

**Commands:** `rampage`

BattleCommand_rampage returns while asleep before setting the flag or count. Awake initialization chooses one or two further actions; an existing rampage decrements its count. HandleRampage first rejects a faint user, any Future Sight impact and inactive rampage. At count zero it clears the lock and preserves existing confusion or effective Own Tempo; otherwise it creates a source two-to-five-action confusion timer. A nonfinal nonzero wAttackMissed clears rampage without confusion. CantMove separately checks the final remaining action before clearing the flag, so a final flinch can confuse even though the move body never ran.

**Authenticated source anchors:** `engine/battle/effect_commands.asm:5494 BattleCommand_rampage`

**Actual runtime definitions:** `src/battle_set_effect.c:459 HandleSetEffectThrash`; `src/battle_util.c:1232 CancelMultiTurnMoves`; `src/battle_move_resolution.c:6134 MoveEndRampage`; `src/polished_damage.c:11 IsPolishedFutureSightImpact`

**Reviewed transitive anchors:** `engine/battle/effect_commands.asm:5517 HandleRampage`; `engine/battle/effect_commands.asm:454 CantMove`; `engine/battle/move_effects/future_sight.asm:37 GetFutureSightUser`

**Branch adjudication:**

| Source branch | Production binding | Distinguishing evidence and disposition |
| --- | --- | --- |
| Sleep, initialization/repetition, nonfinal failed action and final interruption | `HandleSetEffectThrash / CancelMultiTurnMoves / MoveEndRampage` | Live source Sleep Talk rejects the lock; a middle Protect clears count, lock and identity; final flinch under GEN_3 still confuses except with Own Tempo. Native controls exercise two/three-action duration and final immunity/failure. Source remaining-count storage differs from native total-count storage but decrement-before-final evaluation has the same observable schedule. Evidence: `test/battle/move_effect_secondary/thrash.c:9` — Thrash lasts for 2 or 3 turns; `test/battle/move_effect_secondary/thrash.c:26` — Thrash confuses the user after it finishes; `test/battle/move_effect_secondary/thrash.c:46` — Thrash does not confuse the user if it is canceled on turn 1 of 3; remaining exact fixture titles are listed in the JSON row |
| Final count zero rejects existing confusion and a fainted user; failure reason does not precede finality | `MoveEndRampage final-count branch / confusion and living-user guards` | Both-side source/native fixtures preserve the exact prior confusion counter, require final Protect confusion under an older preset, and forbid confusion animation after final Rocky Helmet KO. Source checks zero count before wAttackMissed; target decrements to zero before IsBattlerUnaffectedByMove. Thus final numeric accuracy miss and type immunity share the proved branch rather than a distinct rejection predicate. Evidence: `test/battle/move_effect/polished_command_closure.c:245` — Polished command closure rampage preserves existing confusion on its final action; `test/battle/move_effect/polished_command_closure.c:266` — Polished command closure rampage confuses after a final protected action under an old preset; `test/battle/move_effect/polished_command_closure.c:286` — Polished command closure a final rampage contact knockout never confuses the fainted user |
| Any Future Sight impact cannot decrement or finish the current action lock | `IsPolishedFutureSightImpact early return in MoveEndRampage` | Both-side live fixtures assert active original user remains locked with one action and no confusion after impact; native instead completes. GetFutureSightUser returns nc for both active and external impacts, and the target common helper recognizes both. External-impact rampage is explicitly code-equivalent, not a separate live fixture assertion. Evidence: `test/battle/move_effect/polished_command_closure.c:318` — Polished command closure Future Sight cannot finish a running rampage |

**Actual test catalog entries:**

- `test/battle/move_effect/polished_command_closure.c:245` — Polished command closure rampage preserves existing confusion on its final action (source catalog).
- `test/battle/move_effect/polished_command_closure.c:266` — Polished command closure rampage confuses after a final protected action under an old preset (source catalog).
- `test/battle/move_effect/polished_command_closure.c:286` — Polished command closure a final rampage contact knockout never confuses the fainted user (source catalog).
- `test/battle/move_effect/polished_command_closure.c:318` — Polished command closure Future Sight cannot finish a running rampage (source catalog).
- `test/battle/move_effect_secondary/thrash.c:9` — Thrash lasts for 2 or 3 turns (native control only).
- `test/battle/move_effect_secondary/thrash.c:26` — Thrash confuses the user after it finishes (native control only).
- `test/battle/move_effect_secondary/thrash.c:46` — Thrash does not confuse the user if it is canceled on turn 1 of 3 (native control only).
- `test/battle/move_effect_secondary/thrash.c:60` — Thrash does not confuse the user if it is canceled on turn 2 of 3 (native control only).
- `test/battle/move_effect_secondary/thrash.c:74` — Thrash confuses the user if it is canceled on turn 3 of 3, Protect (native control only).
- `test/battle/move_effect_secondary/thrash.c:89` — Thrash confuses the user if it is canceled on turn 3 of 3, Immunity (native control only).
- 5 additional matched entries are listed explicitly in `SUITE2_MOVE_RECONCILIATION.json`; they are not promoted to complete branch coverage.

**Remaining / scope:** All seven named source rampage groups passed in the full source and native logs before the unrelated Corrosion fixture correction; the final current-candidate full run also passed. The four both-side command-closure groups settle the historical missing existing-confusion, final failed action, faint user and Future Sight checks. Their Future Sight case keeps the original user active. External replay uses the same source ret-nc and target IsPolishedFutureSightImpact guard, so that predicate is code-equivalent here rather than a separately claimed external-rampage fixture. Inactive-rampage return and numeric miss/type immunity versus Protect share directly compared source/target predicates; no separate source branch is inferred from a move name.

### rollout

**Owner:** move-command review with explicit shared dependencies. **Status:** fixed/focused source and native validation passed

**Commands:** `rolloutpower`

rolloutpower returns while asleep before touching lock, counter or Defense Curl multiplier. Awake hit starts/increments to five, miss clears the rollout bit, curled adds a doubling, and each multiplication saturates at FFFF.

**Authenticated source anchors:** `engine/battle/move_effects/rollout.asm:3 BattleCommand_rolloutpower`

**Actual runtime definitions:** `src/battle_move_resolution.c:7000 SetSameMoveTurnValues`; `src/polished_damage.c:236 PolishedDamageBeforeRoll`

**Reviewed transitive anchors:** `engine/battle/effect_commands.asm:5556 CheckRampageStatusAndGetRolloutCount`

**Actual test catalog entries:**

- `test/battle/move_effect/polished_command_edges.c:162` — Polished command sleeping Rollout skips the Defense Curl damage multiplier (source catalog).
- `test/battle/move_effect/rollout.c:6` — Rollout's power doubles after each hit and resest after the 5th hit (native control only).
- `test/battle/move_effect/rollout.c:34` — Rollout's base power resets if the chain has been broken (native control only).
- `test/battle/move_effect/rollout.c:59` — Rollout resets if original user is forced to switch out (native control only).
- `test/battle/polished_action_boundaries.c:50` — Polished Rollout called by Sleep Talk never advances the sleeping user lock (source catalog).
- `test/battle/polished_damage.c:290` — Polished damage Rollout multiplies the completed formula and caps before the roll (source catalog).

**Remaining / scope:** Both source and native live Sleep Talk finish without a lock; the earlier purported distinguishing lock expectation was rejected by the native run. The actual distinguishing source defect was Defense Curl amplification while asleep: the new live called Rollout matrix and production calculator vectors cover asleep × curled × old timer. Parent reports source/native focused command v5 plus Rollout passed.

### recharge

**Owner:** move-command review with explicit shared dependencies. **Status:** reviewed/live scoped

**Commands:** `rechargenextturn`

Successful source recharge effect sets the recharge bit; CheckTurn spends the next action without PP. Source flinch commands can clear recharge even where flinch itself is blocked; move identity/slot and native optional attacks retain separate rules.

**Authenticated source anchors:** `engine/battle/effect_commands.asm:6057 BattleCommand_rechargenextturn`

**Actual runtime definitions:** `src/battle_set_effect.c:382 HandleSetEffectRecharge`; `src/battle_move_resolution.c:3772 DoAttackCanceler`; `src/battle_set_effect.c:99 HandleSetEffectFlinch`

**Actual test catalog entries:**

- `test/battle/move_effect_secondary/polished_flinch_recharge.c:10` — Polished flinch recharge clears both source recharge moves after every source flinch move (source catalog).
- `test/battle/move_effect_secondary/polished_flinch_recharge.c:56` — Polished flinch recharge lets active Inner Focus repeat the locked move and recharge anew (source catalog).
- `test/battle/move_effect_secondary/polished_flinch_recharge.c:99` — Polished flinch recharge activates Steadfast instead of consuming the recharge turn (source catalog).
- `test/battle/move_effect_secondary/polished_flinch_recharge.c:143` — Polished flinch recharge reuses the selected move slot through Mold Breaker and active Inner Focus (source catalog).
- `test/battle/move_effect_secondary/polished_flinch_recharge.c:186` — Polished flinch recharge flinches through Inner Focus suppressed by Neutralizing Gas (source catalog).
- `test/battle/move_effect_secondary/polished_flinch_recharge.c:229` — Polished flinch recharge preserves recharge after misses or failed secondary rolls (source catalog).
- `test/battle/move_effect_secondary/polished_flinch_recharge.c:267` — Polished flinch recharge honors Shield Dust Covert Cloak and Sheer Force with a Mold Breaker control (source catalog).
- `test/battle/move_effect_secondary/polished_flinch_recharge.c:310` — Polished flinch recharge preserves recharge behind intact and broken Substitutes (source catalog).
- `test/battle/move_effect_secondary/polished_flinch_recharge.c:349` — Polished flinch recharge rejects move-effect flinch on sleeping and frozen recharge targets (source catalog).
- `test/battle/move_effect_secondary/polished_flinch_recharge.c:389` — Polished flinch recharge does not clear newly set recharge when the target already acted (source catalog).
- 34 additional matched entries are listed explicitly in `SUITE2_MOVE_RECONCILIATION.json`; they are not promoted to complete branch coverage.

**Remaining / scope:** The source flinch/recharge matrices assert PP, recharge bit and source/native move identity under blockers and suppression. Shared initial hit/faint outcomes are adjudicated under shared_action/shared_damage_faint; no independent recharge command gap remains.

### rage

**Owner:** move-command review with explicit shared dependencies. **Status:** source/runtime semantically reconciled; current full source/native catalog execution passed; local production/input acceptance passed; associated Actions records verify the containing commit

**Commands:** `rage`

Initial failure exits before rage; failed repeats retain an existing source rage bit. Selecting a different move or nonmove action clears it before a faster foe can hit. Source posthiteffects resolves hit abilities, current-Fire thaw and Air Balloon before Rage. It tests the current surviving doll, so a breaking doll permits the boost. The actual boost is a forced self stat change; the native deferred doll bit must not block it a second time.

**Authenticated source anchors:** `engine/battle/move_effects/rage.asm:1 BattleCommand_rage`

**Actual runtime definitions:** `src/battle_util.c:12283 SetOrClearRageVolatile`; `src/battle_main.c:4781 SetActionsAndBattlersTurnOrder`; `src/battle_move_resolution.c:4043 MoveEndRage`; `data/battle_scripts_1.s:3530 BattleScript_PolishedRageIsBuilding`

**Actual test catalog entries:**

- `test/battle/move_effect/polished_post_hit.c:4` — Polished command posthit Rage follows Anger Point and cannot boost after its critical cap (source catalog).
- `test/battle/move_effect/polished_post_hit.c:30` — Polished command posthit runs Justified then Air Balloon then Rage before later hit items (source catalog).
- `test/battle/move_effect/polished_post_hit.c:61` — Polished command posthit Rage uses the current Substitute after the breaking hit (source catalog).
- `test/battle/polished_move_history.c:150` — Polished move history Rage survives an interrupted repeat without starting on failure (source catalog).
- `test/battle/polished_move_history.c:171` — Polished move history using a player item clears Rage before the opponent attacks (source catalog).

**Remaining / scope:** v8 passed Anger Point cap ordering and Justified→Balloon→Rage; the breaking-doll fixture exposed the second shared-stat rejection. The source-only certain-self script fixes that confirmed branch; native script behavior remains unchanged, and all three source/native posthit groups passed v9. The current full-catalog execution passed and is recorded separately.

### counter

**Owner:** move-command review with explicit shared dependencies. **Status:** reviewed/live and boundary probe

**Commands:** `counter`

Counter/Mirror Coat require the appropriate opposing current-turn category/last strike and nonzero real incoming damage; type immunity still blocks. Doubles 16-bit damage with carry saturated FFFF, except Parental Bond reentry does not double again.

**Authenticated source anchors:** `engine/battle/move_effects/counter.asm:1 BattleCommand_counter`

**Actual runtime definitions:** `src/battle_util.c:8436 DoFixedDamageMoveCalc`; `src/battle_move_resolution.c:3482 TryMoveDamageUpdate`

**Actual test catalog entries:**

- `test/battle/move_effect/polished_retaliation.c:4` — Polished retaliation accuracy bypasses accuracy drops, evasion boosts and Bright Powder (source catalog).
- `test/battle/move_effect/polished_retaliation.c:39` — Polished retaliation accuracy retains double damage for either user side and nonzero matchups (source catalog).
- `test/battle/move_effect/polished_retaliation.c:78` — Polished retaliation accuracy still requires damage of the matching category this turn (source catalog).
- `test/battle/move_effect/polished_retaliation.c:95` — Polished retaliation accuracy preserves Ghost and Dark immunities (source catalog).
- `test/battle/move_effect/polished_retaliation.c:113` — Polished retaliation accuracy does not reflect damage absorbed by the user's Substitute (source catalog).
- `test/battle/move_effect/polished_retaliation.c:132` — Polished retaliation accuracy cannot reuse previous-turn damage (source catalog).
- `test/battle/polished_action_boundaries.c:209` — Polished retaliation uses only the last incoming strike (source catalog).
- `test/battle/polished_action_boundaries.c:230` — Polished retaliation Parental Bond repeats the same doubled amount (source catalog).
- `test/battle/polished_action_boundaries.c:458` — Polished retaliation saturates doubled source damage at the 16-bit boundary (source catalog).

**Remaining / scope:** Saturation uses a production calculator test at 32767/32768/65534; this is explicitly not a claim of legal >32K battle HP damage.

### protect

**Owner:** move-command review with explicit shared dependencies. **Status:** reviewed/live scoped

**Commands:** `protect`, `endure`

Shared source counter resets at selection of a different/nonmove action, not at interrupted execution; alternating Protect/Endure share capped 1/3^n chances. Last-action and unsuccessful checks fail/reset as in source; disobedient alternate moves reselect the chain.

**Authenticated source anchors:** `engine/battle/move_effects/protect.asm:1 BattleCommand_protect`; `engine/battle/move_effects/endure.asm:1 BattleCommand_endure`

**Actual runtime definitions:** `src/battle_util.c:12225 CanUseMoveConsecutively`; `src/battle_util.c:12259 TryResetConsecutiveUseCounter`; `src/battle_script_commands.c:5424 Cmd_setprotectlike`; `src/battle_main.c:4781 SetActionsAndBattlersTurnOrder`

**Actual test catalog entries:**

- `test/battle/move_effect/polished_protect_chain.c:4` — Polished Protect interruption preserves the source consecutive probability (source catalog).
- `test/battle/move_effect/polished_protect_chain.c:25` — Polished Protect counter resets when another selected move cannot execute (source catalog).
- `test/battle/move_effect/polished_protect_chain.c:42` — Polished Protect counter resets on a player item action (source catalog).
- `test/battle/move_effect/polished_protect_chain.c:60` — Polished Protect and Endure called by Sleep Talk reset at each selection (source catalog).
- `test/battle/move_effect/polished_protect_chain.c:93` — Polished Protect chain first use protects or endures on both sides (source catalog).
- `test/battle/move_effect/polished_protect_chain.c:124` — Polished Protect chain success probability follows every stage and remains capped (source catalog).
- `test/battle/move_effect/polished_protect_chain.c:156` — Polished Protect chain alternating Protect and Endure shares the probability counter (source catalog).
- `test/battle/move_effect/polished_protect_chain.c:186` — Polished Protect chain saturates the stored count without changing PP costs (source catalog).
- `test/battle/move_effect/polished_protect_chain.c:214` — Polished Protect chain failure resets the next attempt to guaranteed success (source catalog).
- `test/battle/move_effect/polished_protect_chain.c:246` — Polished Protect chain an ordinary selected move resets the next attempt (source catalog).
- 6 additional matched entries are listed explicitly in `SUITE2_MOVE_RECONCILIATION.json`; they are not promoted to complete branch coverage.

**Remaining / scope:** All capped odds, interruptions, called moves, item actions and disobedient alternate choices have source tests; optional Detect/doubles/either gimmick retain native controls.

### focus

**Owner:** move-command review with explicit shared dependencies. **Status:** fixed/focused source and native validation passed

**Commands:** `focusenergy`

Sets source FOCUS_ENERGY unless already set; critical command consumes it as exactly two stages. Reapplication fails without replacing the flag.

**Authenticated source anchors:** `engine/battle/move_effects/focus_energy.asm:1 BattleCommand_focusenergy`

**Actual runtime definitions:** `src/battle_script_commands.c:6355 Cmd_setfocusenergy`; `src/battle_util.c:8641 UsesPolishedCriticalRules`; `src/battle_util.c:8652 CalcCritChanceStageForRules`

**Actual test catalog entries:**

- `test/battle/move_effect/focus_energy.c:9` — Focus Energy increases the user's critical hit ratio by 1 stage (Gen 1-2) or 2 stages (Gen 3+) (native control only).
- `test/battle/move_effect/focus_energy.c:43` — Focus Energy multiplies crit chance by 4 with gen 1 crit chance (native control only).
- `test/battle/move_effect/polished_command_edges.c:135` — Polished command Focus Energy keeps two source stages under the native generation two preset (source catalog).
- `test/battle/polished_battle_core.c:127` — Polished core critical stages use source odds independently of native generation (source catalog).

**Remaining / scope:** Source Focus Energy now sets its two-stage flag independently of B_FOCUS_ENERGY_CRIT_RATIO; the legacy GEN_2/repeat fixture distinguishes source 1/2 from native 1/8 critical probability under B_CRIT_CHANCE=GEN_3. Parent v5 passed both profiles.

### curl

**Owner:** move-command review with explicit shared dependencies. **Status:** source/runtime reconciled; finite closure matrices passed source/native v8

**Commands:** `curl`

Unconditionally sets CURLED after ForceRaiseStat; a capped Defense raise does not terminate the script. Source Rollout multiplication reads this bit only while awake.

**Authenticated source anchors:** `engine/battle/move_effects/defense_curl.asm:1 BattleCommand_curl`

**Actual runtime definitions:** `src/battle_stat_change.c:1161 SetAdditionalEffectsOnStatChange`; `src/polished_damage.c:236 PolishedDamageBeforeRoll`

**Actual test catalog entries:**

- `test/battle/move_effect/polished_command_closure.c:62` — Polished command closure Haze preserves Focus Energy Minimize and Defense Curl (source catalog).
- `test/battle/move_effect/polished_command_closure.c:88` — Polished command closure capped Defense Curl still boosts Rollout (source catalog).
- `test/battle/move_effect/defense_curl.c:9` — Defense Curl raises Defense by 1 stage (native control only).
- `test/battle/move_effect/defense_curl.c:34` — Defense Curl doubles the power of Rollout even if stat couldn't be changed (native control only).
- `test/battle/move_effect/polished_command_edges.c:162` — Polished command sleeping Rollout skips the Defense Curl damage multiplier (source catalog).
- `test/battle/polished_damage.c:290` — Polished damage Rollout multiplies the completed formula and caps before the roll (source catalog).

**Remaining / scope:** Capped and uncapped Defense Curl produce the same enhanced Rollout damage; no-Curl control differs. These are executed source/native polished_command_closure.c assertions, not a name-only dispatch inference. The current full-catalog rerun passed; local production acceptance passed; the associated Actions run records verification of the containing commit.

### minimize

**Owner:** move-command review with explicit shared dependencies. **Status:** fixed/focused source and native validation passed

**Commands:** `minimize`

Unconditionally sets MINIMIZED after forced +2 Evasion. FarChangeStat prints cap rejection then returns normally, so Minimize still changes the volatile at the Evasion cap.

**Authenticated source anchors:** `engine/battle/move_effects/minimize.asm:1 BattleCommand_minimize`

**Actual runtime definitions:** `src/battle_stat_change.c:1161 SetAdditionalEffectsOnStatChange`; `src/polished_damage.c:365 PolishedDamageAfterRoll`

**Actual test catalog entries:**

- `test/battle/move_effect/minimize.c:13` — Minimize increases the user's Evasion by 2 stages (Gen5+) or 1 stage (Gen1-4) (native control only).
- `test/battle/move_effect/polished_command_edges.c:101` — Polished command Minimize sets its volatile even when Evasion is already capped (source catalog).
- `test/battle/move_flags/minimize_double_damage.c:4` — MinimizeDoubleDamage flag makes moves cause double damage to Minimized targets (native control only).

**Remaining / scope:** Source SetAdditionalEffectsOnStatChange sets the volatile even when Evasion is capped. The both-side Double Team to +6 then Minimize fixture asserts the bit and doubled source Stomp/Body Slam damage; native keeps its capped behavior. Parent v5 passed both profiles.

### growth

**Owner:** move-command review with explicit shared dependencies. **Status:** source/runtime reconciled; finite closure matrices passed source/native v8

**Commands:** `growth`

GetWeatherAfterUserUmbrella selects +2 Attack/SpAtk in effective sun, else +1 each. Two independent ForceRaiseStat calls preserve partial caps and Contrary/Simple handling.

**Authenticated source anchors:** `engine/battle/move_effects/growth.asm:1 BattleCommand_growth`

**Actual runtime definitions:** `src/battle_stat_change.c:939 AdjustStatStage`; `src/battle_move_resolution.c:6915 DoStatChange`

**Reviewed transitive anchors:** `home/battle.asm:622 GetWeatherAfterUserUmbrella`; `engine/battle/effect_commands.asm:5275 ForceRaiseStat`

**Actual test catalog entries:**

- `test/battle/move_effect/polished_command_closure.c:4` — Polished command closure Growth uses effective user weather and independent stat caps (source catalog).
- `test/battle/move_effect/growth.c:4` — Growth increases Attack and Sp. Atk by 1 stage or 2 stages under Sun (Gen 5+) (native control only).
- `test/battle/move_effect/polished_self_move_accuracy.c:38` — Polished self move accuracy retains Dragon Dance partial and full stat caps (source catalog).

**Remaining / scope:** Effective no-weather/sun/rain, user versus target Umbrella, Cloud Nine and independent Attack/SpAtk caps. These are executed source/native polished_command_closure.c assertions, not a name-only dispatch inference. The current full-catalog rerun passed; local production acceptance passed; the associated Actions run records verification of the containing commit.

### haze

**Owner:** move-command review with explicit shared dependencies. **Status:** source/runtime reconciled; finite closure matrices passed source/native v8

**Commands:** `resetstats`

ResetStats fills both seven-stat arrays with BASE_STAT_LEVEL; it does not clear unrelated volatile effects such as Focus Energy, Minimize or Defense Curl.

**Authenticated source anchors:** `engine/battle/effect_commands.asm:6071 BattleCommand_resetstats`

**Actual runtime definitions:** `src/battle_script_commands.c:5731 Cmd_normalisebuffs`; `src/battle_script_commands.c:5712 TryResetBattlerStatChanges`

**Actual test catalog entries:**

- `test/battle/move_effect/polished_command_closure.c:38` — Polished command closure Haze resets all seven stages on both sides (source catalog).
- `test/battle/move_effect/polished_command_closure.c:62` — Polished command closure Haze preserves Focus Energy Minimize and Defense Curl (source catalog).
- `test/battle/move_effect/haze.c:9` — Haze resets stat changes (native control only).
- `test/battle/move_effect_secondary/haze.c:9` — Freeze Frost restores stat changes when it was succesful (native control only).

**Remaining / scope:** All seven stages on both sides, with non-Haze control and preserved Focus Energy/Minimize/Defense Curl. These are executed source/native polished_command_closure.c assertions, not a name-only dispatch inference. The current full-catalog rerun passed; local production acceptance passed; the associated Actions run records verification of the containing commit.

### healing

**Owner:** move-command review with explicit shared dependencies. **Status:** reviewed/live scoped

**Commands:** `heal`, `healweather`, `roost`

Heal rejects full HP; ordinary recovery floors half with minimum one. Rest also rejects sleep/Insomnia/Vital Spirit/opponent-umbrella Leaf Guard, sets three ticks, heals before PostStatus. HealingLight uses floor(third)×2 in sun, quarter in adverse weather, half otherwise, after user Umbrella. Roost mutates current Flying types only on nonfull HP and restores at endturn.

**Authenticated source anchors:** `engine/battle/effect_commands.asm:6092 BattleCommand_heal`; `engine/battle/move_effects/healinglight.asm:1 BattleCommand_healweather`; `engine/battle/move_effects/roost.asm:1 BattleCommand_roost`

**Actual runtime definitions:** `src/battle_script_commands.c:5521 Cmd_tryhealhalfhealth`; `src/battle_script_commands.c:5613 Cmd_trysetrest`; `src/battle_script_commands.c:7459 Cmd_recoverbasedonsunlight`; `src/battle_script_commands.c:3165 Cmd_setroost`; `src/battle_util.c:10624 GetBattlerTypes`; `src/battle_script_commands.c:6436 Cmd_transformdataexecution`

**Reviewed transitive anchors:** `home/battle.asm:179 GetHalfMaxHP`; `home/battle.asm:175 GetQuarterMaxHP`; `home/battle.asm:143 GetThirdMaxHP`; `engine/battle/effect_commands.asm:4836 IsOpponentLeafGuardActive`; `engine/battle/effect_commands.asm:4848 PostStatus`

**Actual test catalog entries:**

- `test/battle/move_effect/polished_attack_suite.c:142` — Polished attack suite half HP recovery floors odd maximum HP (source catalog).
- `test/battle/move_effect/polished_attack_suite.c:252` — Polished attack suite recovery still fails at full HP and optional moves retain rounding (source catalog).
- `test/battle/move_effect/polished_attack_suite.c:277` — Polished attack suite optional doubles retain native recovery recoil and drain (source catalog).
- `test/battle/move_effect/polished_attack_suite.c:306` — Polished attack suite either active gimmick retains native recovery recoil and drain (source catalog).
- `test/battle/move_effect/polished_moves.c:64` — Polished Fresh Snack floors half HP independently of generation defaults (source catalog).
- `test/battle/move_effect/polished_moves.c:77` — Polished HealingLight uses source weather fractions and Umbrella rules (source catalog).
- `test/battle/move_effect/polished_moves.c:102` — Polished healing moves fail at full HP (source catalog).
- `test/battle/move_effect/polished_suite2_copy.c:4` — Polished Suite 2 Transform keeps the target's current Roost types after end turn (source catalog).
- `test/battle/polished_action_boundaries.c:150` — Polished Rest uses the opponent Umbrella for Leaf Guard independent of generation (source catalog).
- `test/battle/polished_action_boundaries.c:173` — Polished Rest rechecks weather and ability suppression for Leaf Guard (source catalog).
- 2 additional matched entries are listed explicitly in `SUITE2_MOVE_RECONCILIATION.json`; they are not promoted to complete branch coverage.

**Remaining / scope:** Source tests cover exact odd rounding, all weather/Umbrella fractions, full HP and Rest rejection/cure order plus copied Roost types. Animation choice by type/time is existing Suite 3 presentation.

### heal_bell

**Owner:** move-command review with explicit shared dependencies. **Status:** reviewed/live scoped

**Commands:** `healbell`

Clear active status and all six party major-status bytes on the acting side regardless of Soundproof. Source has no Aromatherapy or native Nightmare obligations.

**Authenticated source anchors:** `engine/battle/move_effects/heal_bell.asm:1 BattleCommand_healbell`

**Actual runtime definitions:** `src/battle_script_commands.c:7076 Cmd_healpartystatus`

**Actual test catalog entries:**

- `test/battle/move_effect/heal_bell.c:11` — Sparkly Swirl cures the entire party (native control only).
- `test/battle/move_effect/heal_bell.c:37` — Heal Bell/Aromatherapy cures the entire party of the user from primary status effects (native control only).
- `test/battle/move_effect/heal_bell.c:105` — Heal Bell does not cure Soundproof partners (Gen 4, Gen 6+) (native control only).
- `test/battle/move_effect/heal_bell.c:134` — Heal Bell cures inactive Soundproof Pokemon (Gen5+) (native control only).
- `test/battle/move_effect/heal_bell.c:164` — Heal Bell cures a Soundproof user (Gen5, Gen8+) (native control only).
- `test/battle/move_effect/heal_bell.c:188` — Aromatherapy cure Soundproof battlers regardless of config (native control only).
- `test/battle/move_effect/heal_bell.c:216` — Aromatherapy cures inactive Soundproof Pokemon regardless of config (native control only).
- `test/battle/move_effect/heal_bell.c:240` — Aromatherapy will be blocked on ally by Sap Sipper but not user (native control only).
- `test/battle/move_effect/polished_suite2_copy.c:258` — Polished Suite 2 Heal Bell clears active and benched Soundproof status (source catalog).

**Remaining / scope:** Both acting sides with active/bench Soundproof are source-tested. Invalid status-bearing empty/egg slots are not legal source battle states.

### substitute

**Owner:** move-command review with explicit shared dependencies. **Status:** source/runtime reconciled; finite closure matrices passed source/native v8

**Commands:** `substitute`

Reject existing Substitute or current HP <= floor(maxHP/4) (minimum one). On success subtract that amount, store doll HP, set bit and clear own binding counter/move; failure leaves HP and existing doll untouched.

**Authenticated source anchors:** `engine/battle/move_effects/substitute.asm:1 BattleCommand_substitute`

**Actual runtime definitions:** `src/battle_script_commands.c:6557 Cmd_setsubstitute`; `data/battle_scripts_1.s:1898 BattleScript_EffectSubstitute`

**Reviewed transitive anchors:** `home/battle.asm:175 GetQuarterMaxHP`; `home/battle.asm:574 CompareHP`

**Actual test catalog entries:**

- `test/battle/move_effect/polished_command_closure.c:339` — Polished command closure Substitute honors minimum equal-cost and repeated creation boundaries (source catalog).
- `test/battle/move_effect/polished_multihit.c:104` — Polished multihit processes intact and broken Substitute separately on each strike (source catalog).
- `test/battle/move_effect/substitute.c:9` — Substitute creates a Substitute at the cost of 1/4 users maximum HP (native control only).
- `test/battle/move_effect/substitute.c:30` — Substitute fails if the user doesn't have enough HP (native control only).
- `test/battle/move_effect/substitute.c:43` — Substitute's HP cost can trigger a berry (native control only).
- `test/battle/move_effect/substitute.c:57` — Substitute's HP cost doesn't trigger effects that trigger on damage taken (native control only).
- `test/battle/move_effect_secondary/polished_binding.c:34` — Polished binding pauses an Infiltrator bind behind Substitute (source catalog).
- `test/battle/move_effect_secondary/polished_binding.c:64` — Polished binding resumes on the turn Substitute breaks (source catalog).
- `test/battle/move_effect_secondary/polished_binding.c:267` — Polished binding preserves native Substitute cleanup in doubles (source catalog).
- `test/battle/move_effect_secondary/polished_binding.c:323` — Polished binding is cleared by creating a new Substitute (source catalog).
- 1 additional matched entries are listed explicitly in `SUITE2_MOVE_RECONCILIATION.json`; they are not promoted to complete branch coverage.

**Remaining / scope:** Both sides, max HP3/4/100/101, equal cost versus one HP above, repeat failure, exact doll HP and PP; binding cleanup remains independently tested. These are executed source/native polished_command_closure.c assertions, not a name-only dispatch inference. The current full-catalog rerun passed; local production acceptance passed; the associated Actions run records verification of the containing commit.

### pain_split

**Owner:** move-command review with explicit shared dependencies. **Status:** source/runtime semantically reconciled; current full source/native catalog execution passed; local production/input acceptance passed; associated Actions records verify the containing commit

**Commands:** `painsplit`

Reject only hidden opponent after shared hastarget/priority; source bypasses ordinary Protect/Substitute and type/accuracy checks. Drain healthier battler by ceil(diff/2), then restore other by floor(diff/2), capping its max HP; update both party HP values.

**Authenticated source anchors:** `engine/battle/move_effects/pain_split.asm:1 BattleCommand_painsplit`

**Actual runtime definitions:** `src/battle_script_commands.c:6768 Cmd_painsplitdmgcalc`; `data/battle_scripts_1.s:2011 BattleScript_PolishedPainSplitTargetFirst`; `src/battle_move_resolution.c:2324 UsesPolishedPreEffectAccuracy`; `src/battle_move_resolution.c:2386 PolishedManualMissMessage`

**Actual test catalog entries:**

- `test/battle/move_effect/polished_stat_command_order.c:359` — Polished command Trick and Pain Split retain distinct manual and priority miss paths (source catalog).
- `test/battle/move_effect/pain_split.c:6` — Polished Pain Split matches source rounding, caps, HP order and party updates (source catalog).
- `test/battle/move_effect/pain_split.c:51` — Polished Pain Split bypasses source Substitute without damaging the doll (source catalog).
- `test/battle/move_effect/pain_split.c:79` — Polished Pain Split bypasses the source Protect failure flag (source catalog).
- `test/battle/move_effect/pain_split.c:102` — Polished Pain Split cannot reach Fly or Dig even with No Guard on either side (source catalog).
- `test/battle/move_effect/pain_split.c:133` — Polished Pain Split cannot use Lock-On to bypass the source Fly and Dig guard (source catalog).
- `test/battle/move_effect/pain_split.c:157` — Polished Pain Split ignores Normal and Ghost damage immunities (source catalog).
- `test/battle/move_effect/pain_split.c:176` — Polished Pain Split ignores accuracy and evasion stage changes (source catalog).
- `test/battle/move_effect/pain_split.c:196` — Polished Pain Split still obeys Prankster immunity on Dark targets (source catalog).
- `test/battle/move_effect/pain_split.c:211` — Polished Pain Split HP redistribution is not reflected by Counter or Mirror Coat (source catalog).

**Remaining / scope:** Nine existing source groups prove arithmetic, party HP, Protect/doll bypass, hidden rejection and type/stage independence. The new v8-passing matrix proves the previously unconnected script branch: ordinary accuracy/Protect flags are ignored by the body, while earlier Prankster-Dark checkpriority failure can end it and consume Policy for an inherited accuracy miss. Native behavior remains controlled.

### callers

**Owner:** move-command review with explicit shared dependencies. **Status:** reviewed/live scoped

**Commands:** `metronome`, `sleeptalk`

Both callers clear last/counter history before success/failure and ResetTurn for called execution. Metronome samples source byte IDs, rejects exclusions/known moves including zero PP/transformed slots. Sleep Talk requires sleep, samples eligible known slots independent of PP, excludes six named effects and disabled identity, preserves cursor.

**Authenticated source anchors:** `engine/battle/move_effects/metronome.asm:1 BattleCommand_metronome`; `engine/battle/move_effects/sleep_talk.asm:1 BattleCommand_sleeptalk`

**Actual runtime definitions:** `src/battle_move_resolution.c:684 CancelerCallSubmove`; `src/battle_move_resolution.c:6977 GetRecordedMove`; `src/battle_script_commands.c:6635 Cmd_setcalledmove`

**Reviewed transitive anchors:** `engine/battle/effect_commands.asm:6160 ClearLastMove`; `engine/battle/effect_commands.asm:593 ResetTurn`; `engine/battle/effect_commands.asm:1042 CheckUserIsCharging`

**Actual test catalog entries:**

- `test/battle/move_effect/polished_metronome_struggle.c:4` — Polished Metronome and Struggle resolve source move 249 as Brick Break on either side (source catalog).
- `test/battle/move_effect/polished_metronome_struggle.c:26` — Polished Metronome and Struggle call source healing aliases without native bans (source catalog).
- `test/battle/move_effect/polished_metronome_struggle.c:46` — Polished Metronome and Struggle call source Fury Strikes (source catalog).
- `test/battle/move_effect/polished_metronome_struggle.c:62` — Polished Metronome and Struggle reject known zero PP moves on both sides (source catalog).
- `test/battle/move_effect/polished_metronome_struggle.c:97` — Polished Metronome and Struggle exclude transformed moves instead of original party slots (source catalog).
- `test/battle/move_effect/polished_metronome_struggle.c:116` — Polished Metronome and Struggle floor quarter maximum HP with a minimum of one (source catalog).
- `test/battle/move_effect/polished_metronome_struggle.c:149` — Polished Metronome and Struggle recoil bypasses Rock Head and Magic Guard (source catalog).
- `test/battle/move_effect/polished_metronome_struggle.c:168` — Polished Metronome and Struggle recoil is independent of dealt damage and target fainting (source catalog).
- `test/battle/move_effect/polished_metronome_struggle.c:186` — Polished Metronome and Struggle recoil applies when damaging a substitute (source catalog).
- `test/battle/move_effect/polished_metronome_struggle.c:205` — Polished Metronome and Struggle recoil can faint the user (source catalog).
- 27 additional matched entries are listed explicitly in `SUITE2_MOVE_RECONCILIATION.json`; they are not promoted to complete branch coverage.

**Remaining / scope:** Source caller PP/Pressure, charging continuation, held-Metronome phase, Magic Bounce later-own-action state, native gimmick transition and enemy Baton Pass history have distinguishing fixtures. Sleeping called Rollout/Curl passed focused source/native. These lifetime branches are resolved under shared_action; final consolidated execution passed; local production acceptance passed; the associated Actions run records verification of the containing commit.

### sketch

**Owner:** move-command review with explicit shared dependencies. **Status:** source/runtime semantically reconciled; current full source/native catalog execution passed; local production/input acceptance passed; associated Actions records verify the containing commit

**Commands:** `sketch`

Reject source link, target Substitute unless effective Infiltrator, transformed user, missing/Struggle/duplicate counter-move history. Replace chosen slot with base PP and zero only its PP-Up bits; permanent party replacement only when original party slot is Sketch; remap locks by identity.

**Authenticated source anchors:** `engine/battle/move_effects/sketch.asm:1 BattleCommand_sketch`

**Actual runtime definitions:** `src/battle_script_commands.c:6887 TryPolishedSketch`; `src/battle_script_commands.c:6931 Cmd_copymovepermanently`; `src/battle_script_commands.c:6405 PolishedRemapMoveLocks`

**Reviewed transitive anchors:** `engine/battle/effect_commands.asm:6160 ClearLastMove`; `engine/battle/effect_commands.asm:6172 ResetActorDisable`; `engine/battle/effect_commands.asm:6235 CheckSubstituteOpp`; `home/battle.asm:471 GetTrueUserAbility`

**Actual test catalog entries:**

- `test/battle/move_effect/polished_sketch.c:5` — Polished Sketch replaces the selected slot and persists base PP on either side (source catalog).
- `test/battle/move_effect/polished_sketch.c:31` — Polished Sketch Substitute rejection honors effective Infiltrator on either side (source catalog).
- `test/battle/move_effect/polished_sketch.c:62` — Polished Sketch cannot copy cleared Sleep Talk or Metronome history (source catalog).
- `test/battle/move_effect/polished_sketch.c:86` — Polished Sketch rejects absent history and already known moves (source catalog).
- `test/battle/move_effect/polished_sketch.c:105` — Polished Sketch can copy an ordinary failed move (source catalog).
- `test/battle/move_effect/polished_sketch.c:121` — Polished Sketch retains the last executed move when the target cannot act (source catalog).
- `test/battle/move_effect/polished_sketch.c:139` — Polished Sketch cannot be used by a transformed battler (source catalog).
- `test/battle/move_effect/polished_sketch.c:156` — Polished Sketch copies survive switching and spend PP from the learned slot (source catalog).
- `test/battle/move_effect/polished_sketch.c:177` — Polished Sketch preserves other move PP and PP Up bonuses (source catalog).
- `test/battle/move_effect/polished_sketch.c:196` — Polished Sketch rejects Struggle after the target exhausts every PP (source catalog).
- 7 additional matched entries are listed explicitly in `SUITE2_MOVE_RECONCILIATION.json`; they are not promoted to complete branch coverage.

**Remaining / scope:** All seventeen Sketch groups passed source/native v8, including the effective-Infiltrator/Gas matrix and original copy/PP-Up/lock/history fixtures. Source hastarget rejects a fainted target before sketch, disproving the proposed post-Explosion gap. Production-boundary temporary-slot and link guards are explicitly not full link playback. Current full source/native execution passed; local production acceptance passed; the associated Actions run records verification of the containing commit.

### transform

**Owner:** move-command review with explicit shared dependencies. **Status:** source/runtime reconciled; finite closure matrices passed source/native v8

**Commands:** `transform`

Clear history; reject transformed target, Armored Mewtwo, Substitute unless effective Infiltrator and hidden target; allow already-transformed user. Copy target current stats/types/DVs/personality/moves/stages while preserving user HP/status/item; Sketch PP1, others5. Clear Disable timer, remap remaining move locks, copy stored Gas ability and invoke eligible entry effects.

**Authenticated source anchors:** `engine/battle/move_effects/transform.asm:1 BattleCommand_transform`

**Actual runtime definitions:** `src/battle_script_commands.c:6436 Cmd_transformdataexecution`; `src/battle_script_commands.c:6405 PolishedRemapMoveLocks`; `src/battle_script_commands.c:6540 BS_PolishedTransformEntry`

**Reviewed transitive anchors:** `engine/battle/effect_commands.asm:6160 ClearLastMove`; `engine/battle/effect_commands.asm:6460 CheckHiddenOpponent`; `engine/battle/effect_commands.asm:6235 CheckSubstituteOpp`; `engine/battle/effect_commands.asm:6172 ResetActorDisable`

**Actual test catalog entries:**

- `test/battle/move_effect/polished_command_closure.c:146` — Polished command closure Transform rejects hidden targets with No Guard under the oldest preset (source catalog).
- `test/battle/move_effect/polished_sketch.c:139` — Polished Sketch cannot be used by a transformed battler (source catalog).
- `test/battle/move_effect/polished_suite2_copy.c:4` — Polished Suite 2 Transform keeps the target's current Roost types after end turn (source catalog).
- `test/battle/move_effect/polished_suite2_copy.c:35` — Polished Suite 2 Transform preserves HP item status and party moves while copying stats types and PP (source catalog).
- `test/battle/move_effect/polished_suite2_copy.c:66` — Polished Suite 2 Transform activates the copied entry ability (source catalog).
- `test/battle/move_effect/polished_suite2_copy.c:87` — Polished Suite 2 Transform permits a second transformation and restores the original party identity (source catalog).
- `test/battle/move_effect/polished_suite2_copy.c:109` — Polished Suite 2 Transform rejects a transformed target (source catalog).
- `test/battle/move_effect/polished_suite2_copy.c:124` — Polished Suite 2 Transform respects Substitute and Infiltrator (source catalog).
- `test/battle/move_effect/polished_suite2_copy.c:139` — Polished Suite 2 Transform and Imposter cannot copy Armored Mewtwo (source catalog).
- `test/battle/move_effect/polished_suite2_copy.c:155` — Polished Suite 2 Transform copies stored Neutralizing Gas and clears its move history (source catalog).
- 1 additional matched entries are listed explicitly in `SUITE2_MOVE_RECONCILIATION.json`; they are not promoted to complete branch coverage.

**Remaining / scope:** Fly/Dig with either No Guard owner under the GEN_1 native preset, plus existing copying, current Roost types, party identity, lock/PP/history and entry ability controls. These are executed source/native polished_command_closure.c assertions, not a name-only dispatch inference. The current full-catalog rerun passed; local production acceptance passed; the associated Actions run records verification of the containing commit.

### skill_swap

**Owner:** move-command review with explicit shared dependencies. **Status:** source/runtime reconciled; finite closure matrices passed source/native v8

**Commands:** `skillswap`

Reject hidden target and NO_ABILITY/NEUTRALIZING_GAS/sentinel. Swap stored abilities, clear both Unburden/Flash Fire/Cud Chew state, then execute user entry ability before target, excluding Imposter and Cloud Nine reentry.

**Authenticated source anchors:** `engine/battle/move_effects/skill_swap.asm:1 BattleCommand_skillswap`

**Actual runtime definitions:** `src/battle_script_commands.c:8031 Cmd_tryswapabilities`; `src/battle_script_commands.c:6540 BS_PolishedTransformEntry`

**Reviewed transitive anchors:** `engine/battle/abilities.asm:8 RunEntryAbilitiesInner_SkillSwap`; `engine/battle/move_effects/skill_swap.asm:59 AbilityCanBeSkillSwapped`; `engine/battle/move_effects/skill_swap.asm:47 NoSkillSwapEntry`

**Actual test catalog entries:**

- `test/battle/move_effect/polished_command_closure.c:167` — Polished command closure Skill Swap clears both earned Flash Fire and Unburden states (source catalog).
- `test/battle/move_effect/polished_command_closure.c:196` — Polished command closure Skill Swap skips Cloud Nine reentry after the swap announcement (source catalog).
- `test/battle/move_effect/polished_command_closure.c:217` — Polished command closure Skill Swap does not activate a newly acquired Imposter (source catalog).
- `test/battle/ability/polished_unburden.c:4` — Polished Unburden is not activated by Thief or Knock Off on either side (source catalog).
- `test/battle/ability/polished_unburden.c:34` — Polished Unburden survives receiving a held item through Thief (source catalog).
- `test/battle/ability/polished_unburden.c:58` — Polished Unburden is not activated when Pickpocket steals its item (source catalog).
- `test/battle/ability/polished_unburden.c:82` — Polished Unburden failed removal behind Substitute leaves its item and flag unchanged (source catalog).
- `test/battle/ability/polished_unburden.c:98` — Polished Unburden Bug Bite activates the berry owner rather than its eater (source catalog).
- `test/battle/ability/polished_unburden.c:121` — Polished Unburden activates after its own berry or White Herb is consumed (source catalog).
- `test/battle/ability/polished_unburden.c:141` — Polished Unburden activates when its Air Balloon bursts (source catalog).
- 15 additional matched entries are listed explicitly in `SUITE2_MOVE_RECONCILIATION.json`; they are not promoted to complete branch coverage.

**Remaining / scope:** Both earned Flash Fire and Unburden states clear; Cloud Nine and newly acquired Imposter skip source entry; existing Gas/hidden/Cud/user-first entry controls remain linked. These are executed source/native polished_command_closure.c assertions, not a name-only dispatch inference. The current full-catalog rerun passed; local production acceptance passed; the associated Actions run records verification of the containing commit.

### conversion

**Owner:** move-command review with explicit shared dependencies. **Status:** reviewed/live scoped

**Commands:** `conversion`

Scan source static listed types, exclude UNKNOWN/current type(s), fail if none. Player selects any eligible known slot (zero PP/disabled permitted), reopens on invalid/cancel and restores cursor. Wild enemy retries among eligible slots; source Smart AI/link transport remain separate existing owners.

**Authenticated source anchors:** `engine/battle/move_effects/conversion.asm:1 BattleCommand_conversion`

**Actual runtime definitions:** `src/battle_script_commands.c:6076 Cmd_tryconversiontypechange`; `src/battle_controller_player.c:678 HandleInputChooseMove`; `src/battle_script_commands.c:6002 TryPolishedConversion`

**Actual test catalog entries:**

- `test/battle/move_effect/polished_suite2_conversion.c:4` — Polished Suite 2 Conversion selects any known moves listed type without using its PP (source catalog).
- `test/battle/move_effect/polished_suite2_conversion.c:25` — Polished Suite 2 Conversion reopens selection after a current type empty slot or cancel (source catalog).
- `test/battle/move_effect/polished_suite2_conversion.c:54` — Polished Suite 2 Conversion fails without opening a menu when no new type exists (source catalog).
- `test/battle/move_effect/polished_suite2_conversion.c:68` — Polished Suite 2 wild Conversion selects a valid later slot (source catalog).

**Remaining / scope:** Four source groups cover all slots, zero PP/disabled eligibility, current/empty/cancel menu retry, no eligible type and wild selection; trainer Smart AI/link control are explicitly Suite 3, not untested Suite 2 effects.

### curse

**Owner:** move-command review with explicit shared dependencies. **Status:** reviewed/live scoped

**Commands:** `curse`

Non-Ghost branch raises Attack/Defense and lowers Speed independently; Ghost branch checks current types/Protean, hidden target and current Substitute via Infiltrator helper, rejects already cursed, pays floor(half maxHP) minimum one and sets opponent Curse even if user faints. Bypasses Protect/reflection; residual Magic Guard/switch/Baton Pass lifetime persists.

**Authenticated source anchors:** `engine/battle/move_effects/curse.asm:1 BattleCommand_curse`

**Actual runtime definitions:** `src/battle_script_commands.c:7174 Cmd_cursetarget`; `src/battle_set_effect.c:1541 SetMoveEffect`; `src/battle_end_turn.c:2192 DoPolishedEndTurnEffects`

**Actual test catalog entries:**

- `test/battle/move_effect/polished_curse_trick.c:4` — Polished Curse respects Substitute with the source Infiltrator exception (source catalog).
- `test/battle/move_effect/polished_curse_trick.c:25` — Polished Curse rejects Fly and Dig even with No Guard (source catalog).
- `test/battle/move_effect/polished_curse_trick.c:43` — Polished Curse after Protean targets the opponent instead of its user (source catalog).
- `test/battle/move_effect/polished_curse_trick.c:100` — Polished Curse uses half maximum HP with floor rounding and a minimum of one (source catalog).
- `test/battle/move_effect/polished_curse_trick.c:118` — Polished Curse can faint its user and persists after replacement (source catalog).
- `test/battle/move_effect/polished_curse_trick.c:141` — Polished Curse cannot charge HP again for an already cursed opponent (source catalog).
- `test/battle/move_effect/polished_curse_trick.c:159` — Polished Curse bypasses Protect and Magic Bounce (source catalog).
- `test/battle/move_effect/polished_curse_trick.c:174` — Polished Curse Magic Guard blocks residual damage but does not block application (source catalog).
- `test/battle/move_effect/polished_curse_trick.c:188` — Polished Curse residual damage ignores a Substitute created afterward (source catalog).
- `test/battle/move_effect/polished_curse_trick.c:201` — Polished Curse switching clears the curse while Baton Pass transfers it (source catalog).
- 5 additional matched entries are listed explicitly in `SUITE2_MOVE_RECONCILIATION.json`; they are not promoted to complete branch coverage.

**Remaining / scope:** Both branches, individual caps/Contrary/Simple, Ghost HP threshold, faint/persistence, hidden/No Guard, protection and optional native scope are distinguishing source tests.

### belly_drum

**Owner:** move-command review with explicit shared dependencies. **Status:** reviewed/live scoped

**Commands:** `bellydrum`

Require current HP greater than floor(half maxHP) minimum one; fail if target Attack stage already at source destination. Set max Attack (min under Contrary), ignoring Simple magnitude; other stages remain. Suppression and optional native Z treatment stay separate.

**Authenticated source anchors:** `engine/battle/move_effects/belly_drum.asm:1 BattleCommand_bellydrum`

**Actual runtime definitions:** `src/battle_move_resolution.c:7404 TryBellyDrum`

**Actual test catalog entries:**

- `test/battle/move_effect/polished_belly_drum.c:4` — Polished Belly Drum stage limits follow the effective direction on both sides (source catalog).
- `test/battle/move_effect/polished_belly_drum.c:54` — Polished Belly Drum requires more than floored half maximum HP and preserves failed PP costs (source catalog).
- `test/battle/move_effect/polished_belly_drum.c:89` — Polished Belly Drum has a minimum one HP cost and never self-faints (source catalog).
- `test/battle/move_effect/polished_belly_drum.c:121` — Polished Belly Drum preserves other stats while reaching the Attack limit (source catalog).
- `test/battle/move_effect/polished_belly_drum.c:158` — Polished Belly Drum rejects a repeated capped attempt after HP is restored (source catalog).
- `test/battle/move_effect/polished_belly_drum.c:202` — Polished Belly Drum remains usable after switching resets the Attack stages (source catalog).
- `test/battle/move_effect/polished_belly_drum.c:242` — Polished Belly Drum uses the normal direction when Neutralizing Gas suppresses Contrary (source catalog).
- `test/battle/move_effect/polished_belly_drum.c:278` — Polished Belly Drum retains native Contrary limits and healing for Z-moves (source catalog).

**Remaining / scope:** Eight source groups cover HP floor/threshold/minimum, cap/repeat, Contrary/Simple/suppression, other stages and switch reset.

### restrictions

**Owner:** move-command review with explicit shared dependencies. **Status:** reviewed/live scoped

**Commands:** `disable`, `encore`

Reject existing active low-nibble restriction, missing/Struggle history, removed identity or exhausted slot. Encore additionally rejects Encore, records slot, overwrites a not-yet-acted target choice and starts four ticks; Disable starts five. Mental Herb clears restriction, not packed Choice high nibble. Expiry/exhaustion/switch/Transform/Sketch/removal preserve required identity history.

**Authenticated source anchors:** `engine/battle/move_effects/encore_disable.asm:12 BattleCommand_disable`; `engine/battle/move_effects/encore_disable.asm:1 BattleCommand_encore`

**Actual runtime definitions:** `src/battle_script_commands.c:6643 Cmd_disablelastusedattack`; `src/battle_script_commands.c:6684 Cmd_trysetencore`; `src/battle_script_commands.c:6405 PolishedRemapMoveLocks`; `src/battle_script_commands.c:6388 PolishedForgetMoveLocks`

**Actual test catalog entries:**

- `test/battle/move_effect/polished_encore_commitment.c:4` — Polished Encore commitment survives Mental Herb in every move slot on both sides (source catalog).
- `test/battle/move_effect/polished_encore_commitment.c:53` — Polished Encore commitment does not carry into the next turn when the target already acted (source catalog).
- `test/battle/move_effect/polished_encore_commitment.c:87` — Polished Encore commitment preserves the active restriction without a curing item (source catalog).
- `test/battle/move_effect/polished_encore_commitment.c:109` — Polished Encore commitment leaves the selected move and herb intact when Encore fails (source catalog).
- `test/battle/move_effect/polished_encore_commitment.c:146` — Polished Encore commitment spends no PP on a paralyzed action and allows the next choice (source catalog).
- `test/battle/move_effect/polished_encore_commitment.c:184` — Polished Encore commitment keeps Disable and Attract herb cures usable under Unnerve (source catalog).
- `test/battle/move_effect/polished_encore_commitment.c:217` — Polished Encore commitment does not persist through switching and reentry after a cure (source catalog).
- `test/battle/move_effect/polished_move_restrictions.c:4` — Polished move restrictions Disable expires after the source action and end-turn ticks (source catalog).
- `test/battle/move_effect/polished_move_restrictions.c:40` — Polished move restrictions Encore always starts at four ticks and overrides a slower target (source catalog).
- `test/battle/move_effect/polished_move_restrictions.c:78` — Polished move restrictions reject an active reapplication without refreshing its counter (source catalog).
- 20 additional matched entries are listed explicitly in `SUITE2_MOVE_RECONCILIATION.json`; they are not promoted to complete branch coverage.

**Remaining / scope:** Source duration, cancellation ticks, PP exhaustion, four-slot commitment, cure, both sides and switch/slot-remapping are actual tests. No name-only closure inferred.

### foresight

**Owner:** move-command review with explicit shared dependencies. **Status:** source/runtime semantically reconciled; current full source/native catalog execution passed; local production/input acceptance passed; associated Actions records verify the containing commit

**Commands:** `foresight`

Reject already identified, missed or hidden opponent even with No Guard; otherwise set identification, bypass Substitute but honor Protect. Suppresses positive evasion only and enables Normal/Fighting versus Ghost through source type rules.

**Authenticated source anchors:** `engine/battle/move_effects/foresight.asm:1 BattleCommand_foresight`

**Actual runtime definitions:** `src/battle_move_resolution.c:1523 CancelerMoveEffectFailureTarget`; `src/battle_script_commands.c:7227 Cmd_setvolatile`; `src/battle_util.c:9161 CalcTypeEffectivenessMultiplier`; `data/battle_scripts_1.s:2167 BattleScript_EffectForesight`

**Actual test catalog entries:**

- `test/battle/ability/polished_magic_bounce_accuracy.c:4` — Polished Magic Bounce accuracy preserves original status misses through reflection (source catalog).
- `test/battle/ability/polished_magic_bounce_accuracy.c:32` — Polished Magic Bounce accuracy preserves original stat misses through reflection (source catalog).
- `test/battle/ability/polished_magic_bounce_accuracy.c:59` — Polished Magic Bounce accuracy preserves confusion seed and source Foresight misses (source catalog).
- `test/battle/ability/polished_magic_bounce_accuracy.c:87` — Polished Magic Bounce accuracy reuses an original status hit without a reflected roll (source catalog).
- `test/battle/ability/polished_magic_bounce_accuracy.c:111` — Polished Magic Bounce accuracy restores ordinary rolls after reflection (source catalog).
- `test/battle/ability/polished_magic_bounce_accuracy.c:132` — Polished Magic Bounce accuracy retains returned status immunities (source catalog).
- `test/battle/ability/polished_magic_bounce_accuracy.c:159` — Polished Magic Bounce accuracy retains original type immunity through reflection (source catalog).
- `test/battle/ability/polished_magic_bounce_accuracy.c:194` — Polished Magic Bounce accuracy keeps ordinary and Mold Breaker move rolls (source catalog).
- `test/battle/ability/polished_magic_bounce_accuracy.c:214` — Polished Magic Bounce accuracy leaves optional moves Magic Coat and gimmicks native (source catalog).
- `test/battle/ability/polished_magic_bounce_accuracy.c:237` — Polished Magic Bounce accuracy leaves optional doubles native (source catalog).
- 15 additional matched entries are listed explicitly in `SUITE2_MOVE_RECONCILIATION.json`; they are not promoted to complete branch coverage.

**Remaining / scope:** The repeat/GEN_3 source control and raw accuracy, negative stages, hidden guards, immunity, PP/switch tests are implemented; finite command-edge execution passed v8. Reflection carries the source accuracy result and the manual body does not consume Policy. Native Foresight auto-hit is separately controlled.

### seed

**Owner:** move-command review with explicit shared dependencies. **Status:** source/runtime semantically reconciled; current full source/native catalog execution passed; local production/input acceptance passed; associated Actions records verify the containing commit

**Commands:** `leechseed`

The source body rejects existing wAttackMissed, then current Substitute via the effective Infiltrator/sound helper, Grass and an existing seed. Ordinary evasion is a manual message and does not consume Blunder Policy; an earlier priority failure is separate. Target Cmd_setseeded uses the same state predicates, with existing-seed versus Grass rejection text in a different order only when both are true; both branches preserve identical battle state and items. Residual Seed uses nominal source damage, Big Root before halving, minimum one, recipient state and Liquid Ooze/Magic Guard behavior.

**Authenticated source anchors:** `engine/battle/move_effects/leech_seed.asm:1 BattleCommand_leechseed`

**Actual runtime definitions:** `src/battle_script_commands.c:5576 Cmd_setseeded`; `src/battle_set_effect.c:833 HandleSetEffectLeechSeed`; `src/battle_end_turn.c:2192 DoPolishedEndTurnEffects`; `src/battle_move_resolution.c:2386 PolishedManualMissMessage`

**Reviewed transitive anchors:** `engine/battle/effect_commands.asm:6235 CheckSubstituteOpp`; `home/battle.asm:512 CheckIfTargetIsGrassType`

**Branch adjudication:**

| Source branch | Production binding | Distinguishing evidence and disposition |
| --- | --- | --- |
| Application predicates and manual miss versus shared failure text | `Cmd_setseeded / PolishedManualMissMessage / shared reflection cache` | Direct unchanged Grass/repeat branches are reviewed against source and native control assertions; source miss/return semantics are separately live-tested. No count or title substitutes for the predicate comparison. Evidence: `test/battle/ability/polished_magic_bounce_accuracy.c:4` — Polished Magic Bounce accuracy preserves original status misses through reflection; `test/battle/ability/polished_magic_bounce_accuracy.c:32` — Polished Magic Bounce accuracy preserves original stat misses through reflection; `test/battle/ability/polished_magic_bounce_accuracy.c:59` — Polished Magic Bounce accuracy preserves confusion seed and source Foresight misses; remaining exact fixture titles are listed in the JSON row |
| Nominal drain, Big Root, minimum one and Liquid Ooze recipient | `DoPolishedEndTurnEffects` | Source execution checks nominal rather than clipped KO damage, the source half-healing fraction and Magic Guard-ignoring Ooze, plus Rapid Spin removal. Evidence: `test/battle/move_effect/polished_rapid_spin.c:16` — Polished Rapid Spin removes every source hazard layer without clearing the opposite side; `test/battle/move_effect/polished_rapid_spin.c:62` — Polished Rapid Spin clears binding Leech Seed and both stacked hazards together; `test/battle/move_effect/polished_rapid_spin.c:106` — Polished Rapid Spin keeps hazards and Leech Seed after misses protection immunity or paralysis; remaining exact fixture titles are listed in the JSON row |

**Actual test catalog entries:**

- `test/battle/ability/polished_magic_bounce_accuracy.c:4` — Polished Magic Bounce accuracy preserves original status misses through reflection (source catalog).
- `test/battle/ability/polished_magic_bounce_accuracy.c:32` — Polished Magic Bounce accuracy preserves original stat misses through reflection (source catalog).
- `test/battle/ability/polished_magic_bounce_accuracy.c:59` — Polished Magic Bounce accuracy preserves confusion seed and source Foresight misses (source catalog).
- `test/battle/ability/polished_magic_bounce_accuracy.c:87` — Polished Magic Bounce accuracy reuses an original status hit without a reflected roll (source catalog).
- `test/battle/ability/polished_magic_bounce_accuracy.c:111` — Polished Magic Bounce accuracy restores ordinary rolls after reflection (source catalog).
- `test/battle/ability/polished_magic_bounce_accuracy.c:132` — Polished Magic Bounce accuracy retains returned status immunities (source catalog).
- `test/battle/ability/polished_magic_bounce_accuracy.c:159` — Polished Magic Bounce accuracy retains original type immunity through reflection (source catalog).
- `test/battle/ability/polished_magic_bounce_accuracy.c:194` — Polished Magic Bounce accuracy keeps ordinary and Mold Breaker move rolls (source catalog).
- `test/battle/ability/polished_magic_bounce_accuracy.c:214` — Polished Magic Bounce accuracy leaves optional moves Magic Coat and gimmicks native (source catalog).
- `test/battle/ability/polished_magic_bounce_accuracy.c:237` — Polished Magic Bounce accuracy leaves optional doubles native (source catalog).
- 29 additional matched entries are listed explicitly in `SUITE2_MOVE_RECONCILIATION.json`; they are not promoted to complete branch coverage.

**Remaining / scope:** The unchanged direct Grass/repeated-seed predicates are code-equivalent and have native boundary controls; they are explicitly not relabeled as source execution. Source live fixtures establish accuracy/reflection, shared Infiltrator doll semantics, removal and residual arithmetic. The differing combined-rejection wording belongs to presentation, with no unresolved Suite 2 state consequence.

### safeguard

**Owner:** move-command review with explicit shared dependencies. **Status:** source/runtime reconciled; finite closure matrices passed source/native v8

**Commands:** `safeguard`, `checksafeguard`

Set five ticks only when inactive; reapplication fails without refresh. Shared checksafeguard executes before primary target immunity and substitutes, and honors effective user Infiltrator.

**Authenticated source anchors:** `engine/battle/move_effects/safeguard.asm:1 BattleCommand_safeguard`; `engine/battle/move_effects/safeguard.asm:35 BattleCommand_checksafeguard`

**Actual runtime definitions:** `src/battle_script_commands.c:7313 Cmd_setsafeguard`; `src/battle_util.c:5783 IsSafeguardProtected`; `src/battle_util.c:5794 CanSetNonVolatileStatusInternal`

**Actual test catalog entries:**

- `test/battle/move_effect/polished_command_closure.c:469` — Polished command closure Safeguard rejects refresh and expires after exactly five turns (source catalog).
- `test/battle/ability/polished_corrosion_status.c:7` — Polished Corrosion status bypasses prevention before immunity abilities cure the poison (source catalog).
- `test/battle/ability/polished_corrosion_status.c:48` — Polished Corrosion status consumes healing berries before Immunity or Pastel Veil can cure (source catalog).
- `test/battle/ability/polished_corrosion_status.c:91` — Polished Corrosion status retains Protect Safeguard existing status and accuracy gates (source catalog).
- `test/battle/ability/polished_corrosion_status.c:115` — Polished Corrosion status respects suppression of the actual attacker ability (source catalog).
- `test/battle/ability/polished_corrosion_status.c:135` — Polished Corrosion status leaves either active Terastal native (source catalog).
- `test/battle/ability/polished_corrosion_status.c:155` — Polished Corrosion status leaves doubles native (source catalog).
- `test/battle/ability/polished_corrosion_status.c:175` — Polished Corrosion status leaves dynamically retyped moves native (source catalog).
- `test/battle/ability/polished_corrosion_status.c:196` — Polished Corrosion status leaves optional Magic Coat returns native (source catalog).
- `test/battle/ability/polished_corrosion_status.c:220` — Polished Corrosion status leaves other status moves native (source catalog).
- 12 additional matched entries are listed explicitly in `SUITE2_MOVE_RECONCILIATION.json`; they are not promoted to complete branch coverage.

**Remaining / scope:** Reapplication preserves its exact five-turn timer and side flag; shared primary status fixtures cover its earlier guard and Infiltrator behavior. These are executed source/native polished_command_closure.c assertions, not a name-only dispatch inference. The current full-catalog rerun passed; local production acceptance passed; the associated Actions run records verification of the containing commit.

### screens

**Owner:** move-command review with explicit shared dependencies. **Status:** source/runtime reconciled; finite closure matrices passed source/native v8

**Commands:** `screen`, `brickbreak`

Reflect/Light Screen are independent packed counters; set five or item duration and reject active refresh. Brick Break ignores screens during damage stats, then clears both only after successful damage step (including Substitute); immunity/miss/Protect abort before removal.

**Authenticated source anchors:** `engine/battle/move_effects/reflect_light_screen.asm:1 BattleCommand_screen`; `engine/battle/move_effects/brick_break.asm:1 BattleCommand_brickbreak`

**Actual runtime definitions:** `src/battle_script_commands.c:1680 TrySetReflect`; `src/battle_script_commands.c:1701 TrySetLightScreen`; `src/battle_set_effect.c:1326 HandleSetEffectBreakScreen`; `src/polished_damage.c:236 PolishedDamageBeforeRoll`; `src/battle_util.c:2393 IsPolishedBrickBreak`; `src/battle_move_resolution.c:3106 CancelerPreAttackMoveEffect`; `src/battle_script_commands.c:1862 CanApplyAdditionalEffect`; `data/battle_scripts_1.s:2623 BattleScript_PolishedBreakScreens`

**Reviewed transitive anchors:** `engine/battle/effect_commands.asm:6186 GetItemBoostedDuration`; `engine/battle/effect_commands.asm:4003 GetOpponentActiveScreens`

**Actual test catalog entries:**

- `test/battle/move_effect/polished_command_closure.c:364` — Polished command closure screen durations and failed refresh stay independent with Light Clay (source catalog).
- `test/battle/move_effect/polished_command_closure.c:394` — Polished command closure Brick Break removes both screens only after a successful hit (source catalog).
- `test/battle/move_effect/polished_command_closure.c:431` — Polished command closure Brick Break clears screens after HP damage including a lethal hit (source catalog).
- `test/battle/move_effect/light_screen.c:9` — Light Screen reduces special damage (native control only).
- `test/battle/move_effect/light_screen.c:29` — Light Screen applies for 5 turns (native control only).
- `test/battle/move_effect/light_screen.c:66` — Light Screen fails if already active (native control only).
- `test/battle/move_effect/reflect.c:9` — Reflect reduces physical damage (native control only).
- `test/battle/move_effect/reflect.c:29` — Reflect applies for 5 turns (native control only).
- `test/battle/move_effect/reflect.c:66` — Reflect fails if already active (native control only).
- `test/battle/move_effect_secondary/break_screens.c:15` — Brick Break, Psychic Fangs, and Raging Bull remove Light Screen, Reflect and Aurora Veil from the target's side of the field (native control only).
- 11 additional matched entries are listed explicitly in `SUITE2_MOVE_RECONCILIATION.json`; they are not promoted to complete branch coverage.

**Remaining / scope:** Light Clay/no-refresh/independent duration; Brick Break ignores screens for damage and removes both counters only after successful HP/doll damage, including lethal hits; miss/Protect/immunity retain screens. These are executed source/native polished_command_closure.c assertions, not a name-only dispatch inference. The current full-catalog rerun passed; local production acceptance passed; the associated Actions run records verification of the containing commit.

### weather

**Owner:** move-command review with explicit shared dependencies. **Status:** reviewed/live scoped

**Commands:** `starthail`, `startrain`, `startsandstorm`, `startsun`

Same weather rejects before timer mutation; otherwise set requested weather and item-boosted duration. Active weather and effective user/target weather through Utility Umbrella are separate concepts; source uses Hail, not Snow.

**Authenticated source anchors:** `engine/battle/move_effects/weather.asm:13 BattleCommand_starthail`; `engine/battle/move_effects/weather.asm:5 BattleCommand_startrain`; `engine/battle/move_effects/weather.asm:9 BattleCommand_startsandstorm`; `engine/battle/move_effects/weather.asm:1 BattleCommand_startsun`

**Actual runtime definitions:** `src/battle_util.c:2144 TryChangeBattleWeather`; `src/battle_set_effect.c:1029 HandleSetEffectWeather`; `src/battle_util.c:10314 GetWeather`

**Actual test catalog entries:**

- `test/battle/move_effect/polished_moves.c:77` — Polished HealingLight uses source weather fractions and Umbrella rules (source catalog).
- `test/battle/polished_battle_core.c:197` — Polished core Speed applies paralysis before weather abilities and held items (source catalog).
- `test/battle/polished_battle_core.c:270` — Polished core odd paralysis weather and Quick Powder rounding changes live action order (source catalog).
- `test/battle/polished_battle_core.c:288` — Polished core weather abilities use source finite durations under older native presets (source catalog).
- `test/battle/polished_battle_core.c:311` — Polished core Snow Warning creates source Hail rather than native Snow (source catalog).
- `test/battle/polished_battle_core.c:329` — Polished core Hail residual ability gates include source Slush Rush (source catalog).
- `test/battle/polished_battle_core.c:344` — Polished core Safety Goggles and Cloud Nine prevent Hail residual without extending duration (source catalog).
- `test/battle/polished_battle_core.c:356` — Polished core source weather replaces overworld weather and installs its own timer (source catalog).
- `test/battle/polished_battle_core.c:370` — Polished core reapplying the same weather does not refresh its timer (source catalog).
- `test/battle/polished_battle_core.c:381` — Polished core doubles keep native Snow Warning Slush Rush and critical odds (source catalog).
- 3 additional matched entries are listed explicitly in `SUITE2_MOVE_RECONCILIATION.json`; they are not promoted to complete branch coverage.

**Remaining / scope:** Source same-weather no-refresh, overworld replacement, finite old-preset ability duration, Hail, suppression and user/target Umbrella rules have live fixtures. Duration rocks and on-hit/endturn weather items are explicitly bound in the ability/item ledger, while source weather Speed/stat/accuracy consumers are shared_accuracy/shared_damage/room. Source move semantics have no unclassified remaining branch.

### room

**Owner:** move-command review with explicit shared dependencies. **Status:** source/runtime semantically reconciled; current full source/native catalog execution passed; local production/input acceptance passed; associated Actions records verify the containing commit

**Commands:** `trickroom`

Toggle active Trick Room off, else install five ticks and run Room Service in source speed order, consuming each item only on a successful Speed drop. Expiry changes later end-turn order.

**Authenticated source anchors:** `engine/battle/move_effects/trick_room.asm:1 BattleCommand_trickroom`

**Actual runtime definitions:** `src/battle_script_commands.c:8008 Cmd_setroom`; `src/battle_script_commands.c:7992 HandleRoomMove`; `src/battle_script_commands.c:12661 BS_TryRoomService`; `src/battle_script_commands.c:12634 BS_NextRoomServiceBattler`; `src/battle_end_turn.c:159 SortPolishedBattlersBySpeed`

**Reviewed transitive anchors:** `engine/battle/move_effects/trick_room.asm:24 HandleRoomService`; `home/battle.asm:135 SetFastestTurn`

**Actual test catalog entries:**

- `test/battle/move_effect/polished_command_identity.c:100` — Polished command identity Room Service snapshots Speed order after direct Trick Room activation (source catalog).
- `test/battle/move_effect/polished_command_identity.c:130` — Polished command identity capped Room Service holders keep their items on activation (source catalog).
- `test/battle/move_effect/polished_command_identity.c:157` — Polished command identity Room Service does not activate when Trick Room is toggled off or expires (source catalog).
- `test/battle/hold_effect/room_service.c:9` — Room Serive decreases the holder's seep by one stage (native control only).
- `test/battle/move_effect/polished_suite2_copy.c:294` — Polished Suite 2 voluntary switches obey Trick Room Lagging Tail and Custap (source catalog).
- `test/battle/move_effect/trick_room.c:9` — Trick Room doesn't print its ending message twice when used again (native control only).
- `test/battle/move_effect/trick_room.c:30` — Trick Room reverses move order for 5 turns including the turn it is used (native control only).
- `test/battle/move_effect/trick_room.c:58` — Trick Room does not affect move priority (native control only).
- `test/battle/move_effect/trick_room.c:76` — Trick Room does not fail if the chosen AI target has fainted (native control only).
- `test/battle/polished_lifecycle.c:358` — Polished lifecycle Trick Room expiry refreshes Speed before block B (source catalog).
- 1 additional matched entries are listed explicitly in `SUITE2_MOVE_RECONCILIATION.json`; they are not promoted to complete branch coverage.

**Remaining / scope:** All three direct Room Service groups passed source/native v8: both Speed orders and actors, capped holders retaining items, and no consumption on toggling off or expiry. Source entry and expiry scheduling fixtures remain separately linked. No remaining move-specific order/cap branch was found; consolidated execution passed; production/CI acceptance still applies.

### sucker_punch

**Owner:** move-command review with explicit shared dependencies. **Status:** source/runtime reconciled; finite closure matrices passed source/native v8

**Commands:** `suckerpunch`

Source fails if opponent already acted or selected a status category; otherwise normal hit pipeline. Native Me First is an optional exception, not source content.

**Authenticated source anchors:** `engine/battle/move_effects/sucker_punch.asm:1 BattleCommand_suckerpunch`

**Actual runtime definitions:** `src/battle_move_resolution.c:1523 CancelerMoveEffectFailureTarget`; `src/battle_script_commands.c:1591 HasBattlerActedThisTurn`

**Actual test catalog entries:**

- `test/battle/move_effect/polished_command_closure.c:119` — Polished command closure Sucker Punch checks attack status earlier action and item selection (source catalog).
- `test/battle/move_effect/sucker_punch.c:9` — Sucker Punch hits targets that are about to attack (native control only).
- `test/battle/move_effect/sucker_punch.c:25` — Sucker Punch doesn't hit targets using status moves (native control only).
- `test/battle/move_effect/sucker_punch.c:42` — Sucker Punch hits targets using Me First (native control only).
- `test/battle/move_effect/sucker_punch.c:61` — Sucker Punch doesn't hit targets that has already moved (native control only).
- `test/battle/move_effect/sucker_punch.c:79` — Sucker Punch fails if the target has attempted to act even if previously successful (native control only).

**Remaining / scope:** Selected attack/status, already-acted opponent and actual item action; source accuracy-blocker tests preserve the precondition before any hit roll. These are executed source/native polished_command_closure.c assertions, not a name-only dispatch inference. The current full-catalog rerun passed; local production acceptance passed; the associated Actions run records verification of the containing commit.

### splash

**Owner:** move-command review with explicit shared dependencies. **Status:** reviewed/trivial shared execution

**Commands:** `splash`

Animate, display Nothing Happened, perform no battle-state mutation after ordinary action PP/history.

**Authenticated source anchors:** `engine/battle/move_effects/splash.asm:1 BattleCommand_splash`

**Actual runtime definitions:** `data/battle_scripts_1.s:1945 BattleScript_EffectDoNothing`

**Actual test catalog entries:**

- `test/battle/move_effect/polished_attack_suite.c:7` — Polished attack suite dispatches every source move on either side (source catalog).
- `test/battle/polished_move_history.c:4` — Polished move history Metronome records the disobedient alternate action (source catalog).
- `test/battle/polished_move_history.c:33` — Polished move history Metronome retains attempted repeats across Protect and misses (source catalog).
- `test/battle/polished_move_history.c:59` — Polished move history Metronome ignores a flinched selection of another move (source catalog).
- `test/battle/polished_move_history.c:83` — Polished move history Metronome counts the caller when its called move changes (source catalog).
- `test/battle/polished_move_history.c:107` — Polished move history Metronome saturates after five increments (source catalog).
- `test/battle/polished_move_history.c:127` — Polished move history Metronome resets for an executed different move and switching (source catalog).
- `test/battle/polished_move_history.c:150` — Polished move history Rage survives an interrupted repeat without starting on failure (source catalog).
- `test/battle/polished_move_history.c:171` — Polished move history using a player item clears Rage before the opponent attacks (source catalog).
- `test/battle/polished_move_history.c:187` — Polished move history Magic Bounce preserves source charging through a later own action (source catalog).
- 2 additional matched entries are listed explicitly in `SUITE2_MOVE_RECONCILIATION.json`; they are not promoted to complete branch coverage.

**Remaining / scope:** All-move dispatch and caller/history cases execute Splash; PP/history are checked by shared action contracts. Animation wording/timing is Suite 3.

### payday

**Owner:** move-command review with explicit shared dependencies. **Status:** fixed/focused source and native validation passed

**Commands:** `payday`

After damage, CheckSubstituteOpp rejects a surviving doll except effective Infiltrator; a breaking hit permits coins. Add actor level five times into three little-endian bytes, on either side, wrapping modulo 2^24 with no saturation. Optional native effects retain native player-only 16-bit cap.

**Authenticated source anchors:** `engine/battle/move_effects/pay_day.asm:1 BattleCommand_payday`

**Actual runtime definitions:** `src/battle_set_effect.c:264 HandleSetEffectPayday`; `src/battle_set_effect.c:1127 HandleSetEffectConfusePayDaySide`; `src/battle_main.c:207 gPaydayMoney`

**Reviewed transitive anchors:** `engine/battle/effect_commands.asm:6235 CheckSubstituteOpp`

**Actual test catalog entries:**

- `test/battle/move_effect/polished_pay_day.c:6` — Polished Pay Day accumulates coins from either actor and rejects misses and Protect (source catalog).
- `test/battle/move_effect/polished_pay_day.c:26` — Polished Pay Day checks the surviving Substitute and honors Infiltrator (source catalog).
- `test/battle/move_effect/polished_pay_day.c:58` — Polished Pay Day still accumulates coins when its damage faints the target (source catalog).
- `test/battle/move_effect/polished_pay_day.c:81` — Polished Pay Day production effect wraps its source 24 bit accumulator and retains native caps (source catalog).

**Remaining / scope:** Parent reports four focused groups passed in both profiles: live hit/miss/Protect, intact/broken/Infiltrator and faint cases plus production overflow boundary probe. Final monetary reward delivery remains Suite 3 by existing plan; no claim that 24-bit accumulation fixes presentation/rewards.

### item_moves

**Owner:** move-command review with explicit shared dependencies. **Status:** source/runtime semantically reconciled; current full source/native catalog execution passed; local production/input acceptance passed; associated Actions records verify the containing commit

**Commands:** `thief`, `trick`, `knockoff`, `bugbite`

Theft checks current Substitute helper, living Sticky Hold, essential/mail eligibility, trainer existing-item vs wild player Bag delivery and full Bag, and forbids wild opponent theft. Trick swaps eligible items and overwrites wild backup. Knock Off marks pending loss before HP berry sweep and removes after hit effects. Bug Bite has NO Substitute guard at either pending/consume phase; checks Unnerve, living Sticky Hold, six no-Bug-Bite EV berries and berry pocket, consumes even no-effect berries. UserCanLoseItem compares original party species/form, so only party Armored Mewtwo protects Armor Suit; transformed-away original identity remains decisive.

**Authenticated source anchors:** `engine/battle/move_effects/thief.asm:1 BattleCommand_thief`; `engine/battle/move_effects/trick.asm:1 BattleCommand_trick`; `engine/battle/move_effects/knock_off.asm:1 BattleCommand_knockoff`; `engine/battle/move_effects/bug_bite.asm:1 BattleCommand_bugbite`

**Actual runtime definitions:** `src/battle_util.c:10184 CanStealItem`; `src/battle_script_commands.c:1625 StealTargetItem`; `src/battle_util.c:9631 CanBattlerGetOrLoseItem`; `src/battle_script_commands.c:7730 Cmd_tryswapitems`; `src/battle_util.c:10162 PolishedCanEatTargetBerry`; `src/battle_move_resolution.c:3482 TryMoveDamageUpdate`; `src/battle_set_effect.c:582 HandleSetEffectBugBite`; `src/battle_set_effect.c:1651 DoesSubstituteBlockMoveEffectOnTarget`

**Reviewed transitive anchors:** `engine/battle/move_effects/thief.asm:107 CanStealItem`; `engine/battle/move_effects/thief.asm:52 ReceiveBattleItem`; `engine/battle/move_effects/thief.asm:82 CheckStickyHold`; `engine/battle/effect_commands.asm:1381 UserCanLoseItem`; `engine/battle/effect_commands.asm:1379 OpponentCanLoseItem`; `engine/battle/move_effects/knock_off.asm:38 CheckPendingItemLoss`; `engine/battle/effect_commands.asm:6471 GetOpponentItem`; `engine/battle/effect_commands.asm:6235 CheckSubstituteOpp`; `home/pokemon.asm:428 CompareSpeciesForm`; `home/battle.asm:42 UserPartyAttr`

**Actual test catalog entries:**

- `test/battle/move_effect/polished_stat_command_order.c:296` — Polished command manual miss text does not consume Blunder Policy (source catalog).
- `test/battle/move_effect/polished_stat_command_order.c:359` — Polished command Trick and Pain Split retain distinct manual and priority miss paths (source catalog).
- `test/battle/polished_suite2_boss.c:50` — Polished Suite 2 Armor Suit is essential only for either party Armored Mewtwo form (source catalog).
- `test/battle/polished_suite2_boss.c:84` — Polished Suite 2 Armor Suit eligibility retains the party form after Transform (source catalog).
- `test/battle/ability/polished_unburden.c:4` — Polished Unburden is not activated by Thief or Knock Off on either side (source catalog).
- `test/battle/ability/polished_unburden.c:34` — Polished Unburden survives receiving a held item through Thief (source catalog).
- `test/battle/ability/polished_unburden.c:58` — Polished Unburden is not activated when Pickpocket steals its item (source catalog).
- `test/battle/ability/polished_unburden.c:82` — Polished Unburden failed removal behind Substitute leaves its item and flag unchanged (source catalog).
- `test/battle/ability/polished_unburden.c:98` — Polished Unburden Bug Bite activates the berry owner rather than its eater (source catalog).
- `test/battle/ability/polished_unburden.c:121` — Polished Unburden activates after its own berry or White Herb is consumed (source catalog).
- 41 additional matched entries are listed explicitly in `SUITE2_MOVE_RECONCILIATION.json`; they are not promoted to complete branch coverage.

**Remaining / scope:** The v5 source/native doll/pending-loss fixes passed, and the direct Armor Suit matrix passed v8. The transformed identity fixture needed explicit speeds after Transform; the corrected source/native v9 groups passed, resolving the fixture gate and confirming the party-form predicate. Trick now has source ordinary-miss/no-Policy versus earlier priority-failure evidence. Berry handlers, posthit consumption and six-slot backups are concretely tied to the ability/item and switching cleanup ledgers; no generic item-command gap is left unclassified.

### binding

**Owner:** move-command review with explicit shared dependencies. **Status:** reviewed/live scoped

**Commands:** `traptarget`

Source binding applies after a successful nonblocked hit only to live/unbound target; its initial CheckSubstituteOpp honors effective Infiltrator. Duration uses Grip Claw or 4/5. The separate residual routine checks the raw current Substitute bit even for an Infiltrator binding and pauses both damage and duration; creating an own Substitute clears binding.

**Authenticated source anchors:** `engine/battle/effect_commands.asm:5840 BattleCommand_traptarget`

**Actual runtime definitions:** `src/battle_set_effect.c:306 HandleSetEffectWrap`; `src/battle_script_commands.c:6557 Cmd_setsubstitute`; `src/battle_end_turn.c:2192 DoPolishedEndTurnEffects`

**Actual test catalog entries:**

- `test/battle/move_effect_secondary/polished_binding.c:6` — Polished binding releases on the last countdown turn without residual damage (source catalog).
- `test/battle/move_effect_secondary/polished_binding.c:34` — Polished binding pauses an Infiltrator bind behind Substitute (source catalog).
- `test/battle/move_effect_secondary/polished_binding.c:64` — Polished binding resumes on the turn Substitute breaks (source catalog).
- `test/battle/move_effect_secondary/polished_binding.c:96` — Polished binding pauses the countdown while Magic Guard prevents damage (source catalog).
- `test/battle/move_effect_secondary/polished_binding.c:120` — Polished binding resumes when Magic Guard is suppressed (source catalog).
- `test/battle/move_effect_secondary/polished_binding.c:148` — Polished binding checks the live Binding Band including under Unnerve (source catalog).
- `test/battle/move_effect_secondary/polished_binding.c:178` — Polished binding does not refresh an existing countdown (source catalog).
- `test/battle/move_effect_secondary/polished_binding.c:198` — Polished binding keeps damaging and counting during Fly or Dig (source catalog).
- `test/battle/move_effect_secondary/polished_binding.c:223` — Polished binding preserves native duration for optional binding moves (source catalog).
- `test/battle/move_effect_secondary/polished_binding.c:245` — Polished binding preserves native duration with either battler Terastallized (source catalog).
- 4 additional matched entries are listed explicitly in `SUITE2_MOVE_RECONCILIATION.json`; they are not promoted to complete branch coverage.

**Remaining / scope:** Source fractions/timers, Grip Claw, suppression, intact/broken doll/Infiltrator application, raw-doll residual pause and creation cleanup have scoped live fixtures. Shared hit/faint order is adjudicated under shared_damage_faint and the switching ledger.

### perish_song

**Owner:** move-command review with explicit shared dependencies. **Status:** reviewed/live scoped

**Commands:** `perishsong`

Apply independent user/opponent perish counters at four unless already affected or Soundproof; source Prankster-Dark can reject opponent without exempting user, while Protect/Substitute/hidden state do not stop ordinary song. Tick and simultaneous faint order are lifecycle-owned.

**Authenticated source anchors:** `engine/battle/move_effects/perish_song.asm:1 BattleCommand_perishsong`

**Actual runtime definitions:** `src/battle_script_commands.c:7250 Cmd_trysetperishsong`; `src/battle_end_turn.c:2192 DoPolishedEndTurnEffects`

**Actual test catalog entries:**

- `test/battle/move_effect/polished_perish_song.c:4` — Polished Perish Song separates user countdown from opponent ability immunity (source catalog).
- `test/battle/move_effect/polished_perish_song.c:38` — Polished Perish Song keeps Prankster Dark immunity during Fly and Dig (source catalog).
- `test/battle/move_effect/polished_perish_song.c:57` — Polished Perish Song countdown reaches zero on the fourth turn including use (source catalog).
- `test/battle/move_effect/polished_perish_song.c:89` — Polished Perish Song repeat fails without refreshing either countdown (source catalog).
- `test/battle/move_effect/polished_perish_song.c:111` — Polished Perish Song clears on switching and can afflict only the new opponent (source catalog).
- `test/battle/move_effect/polished_perish_song.c:136` — Polished Perish Song bypasses Protect Substitute and hiding without Prankster (source catalog).
- `test/battle/move_effect/polished_perish_song.c:158` — Polished Perish Song still afflicts its Soundproof user (source catalog).
- `test/battle/move_effect/polished_perish_song.c:176` — Polished Perish Song retains native ability blocking during active Terastal (source catalog).
- `test/battle/move_effect/polished_perish_song.c:194` — Polished Perish Song retains native ability blocking in optional doubles (source catalog).

**Remaining / scope:** Application/duplicate/side, Soundproof, Prankster, four-turn switching and actual sound-item paths are source-tested. Simultaneous final-party outcomes and replacement boundaries are explicitly adjudicated in the switching/faint ledger; exact display wording remains presentation.

### destiny_bond

**Owner:** root shared pipeline review. **Status:** source/runtime semantically reconciled; current full source/native catalog execution passed; local production/input acceptance passed; associated Actions records verify the containing commit

**Commands:** `destinybond`

Source consecutive Destiny Bond clears the old bit and fails. Selecting/executing another action or CantMove clears it at the source action boundary. On direct opposing KO postfainteffects consumes the bond instead of Moxie/Aftermath, reduces the attacker to zero before either faint animation and follows source first-faint selection. Future Sight is explicitly excluded and Explosion removes the opposing bond before self-destruction proceeds.

**Authenticated source anchors:** `engine/battle/move_effects/destiny_bond.asm:1 BattleCommand_destinybond`

**Actual runtime definitions:** `src/battle_move_resolution.c:1523 CancelerMoveEffectFailureTarget`; `src/battle_move_resolution.c:3772 DoAttackCanceler`; `src/battle_move_resolution.c:4403 MoveEndFaintBlock`; `src/battle_main.c:4781 SetActionsAndBattlersTurnOrder`; `src/battle_move_resolution.c:5000 MoveEndPolishedPostFaint`

**Branch adjudication:**

| Source branch | Production binding | Distinguishing evidence and disposition |
| --- | --- | --- |
| Lifetime and KO consequence are separate source phases | `CancelerMoveEffectFailureTarget / SetActionsAndBattlersTurnOrder / MoveEndPolishedPostFaint` | Fixtures assert actual bit lifetime and HP/faint ordering, including no Moxie/Aftermath and no mutation from AI eligibility queries; external Future Sight never consumes the bond. Evidence: `test/battle/polished_action_resolution.c:109` — Polished action Destiny Bond replaces the faint ability branch; `test/battle/polished_action_resolution.c:132` — Polished action Destiny Bond repeated use fails independently of native generation; `test/battle/polished_action_resolution.c:288` — Polished action Destiny Bond suppresses Moxie as well as Aftermath; remaining exact fixture titles are listed in the JSON row |

**Actual test catalog entries:**

- `test/battle/polished_action_resolution.c:109` — Polished action Destiny Bond replaces the faint ability branch (source catalog).
- `test/battle/polished_action_resolution.c:132` — Polished action Destiny Bond repeated use fails independently of native generation (source catalog).
- `test/battle/polished_action_resolution.c:288` — Polished action Destiny Bond suppresses Moxie as well as Aftermath (source catalog).
- `test/battle/polished_action_resolution.c:301` — Polished action Destiny Bond is cleared by flinch even when the repeated choice is Destiny Bond (source catalog).
- `test/battle/polished_action_resolution.c:410` — Polished action Destiny Bond damage precedes both faint animations (source catalog).
- `test/battle/polished_action_resolution.c:428` — Polished action Explosion does not trigger an existing Destiny Bond (source catalog).
- `test/battle/polished_action_resolution.c:489` — Polished action AI Destiny Bond failure queries cannot clear the live volatile (source catalog).
- `test/battle/polished_dynamic_damage.c:347` — Polished dynamic damage Future Sight does not take its user down through Destiny Bond (source catalog).
- `test/battle/polished_obedience.c:196` — Polished action sleep cancellation clears a repeated Destiny Bond (source catalog).

**Remaining / scope:** Actual repeated-use, flinch, source self/alternate action, Moxie/Aftermath exclusion, Explosion, delayed-hit exclusion and animation-order fixtures resolve the move-specific branch review. Global final-party/replacement choices are connected to the explicit switching ledger rather than left as unspecified faint ownership.

### selfdestruct

**Owner:** move-command review with explicit shared dependencies. **Status:** source/runtime reconciled; explicit terminal input gates retained

**Commands:** `selfdestruct`

Damp rejects before mutation; otherwise source clears user status/seed and opposing Destiny Bond, reserves first faint, releases Unnerve berry sweeps before attack damage, and proceeds even though user HP is zero.

**Authenticated source anchors:** `engine/battle/move_effects/explosion.asm:1 BattleCommand_selfdestruct`

**Actual runtime definitions:** `src/battle_script_commands.c:5467 Cmd_setatkhptozero`; `data/battle_scripts_1.s:1606 BattleScript_Explosion`; `src/battle_end_turn.c:2119 PolishedHandleUnnerveExit`

**Actual test catalog entries:**

- `test/battle/polished_action_boundaries.c:115` — Polished Explosion releases Unnerve berries before attack damage (source catalog).
- `test/battle/polished_action_boundaries.c:134` — Polished Explosion clears status before fainting and respects Damp prevention (source catalog).
- `test/battle/polished_action_resolution.c:428` — Polished action Explosion does not trigger an existing Destiny Bond (source catalog).

**Remaining / scope:** Damp, source status/seed/bond clearing and Unnerve berry release before damage are distinguishing execution tests. Direct/faint consequence is shared_damage_faint; the wild faint prompt/trapped-victim production-input gate passed in both profiles, as recorded in the switching ledger and SUITE2_VALIDATION.json. Broader EXP/rewards/context remains original Suite 3.

### drain_recoil

**Owner:** root shared pipeline review. **Status:** source/runtime semantically reconciled; current full source/native catalog execution passed; local production/input acceptance passed; associated Actions records verify the containing commit

**Commands:** `draintarget`, `eatdream`, `recoil`

SapHealth returns at full user HP unless Liquid Ooze, uses assigned real/doll damage, computes Draining Kiss damage+floor(half damage) before Big Root 13/10 and final halving/minimum one, and caps actual restoration. Dream Eater requires sleep before drain; source Ooze still hurts a full-HP Magic Guard user. Ordinary recoil uses the authenticated quarter/third fraction and minimum one, with Rock Head/Magic Guard gates; Struggle is quarter user max HP, ignores those abilities and cannot be transformed into an ordinary power move. Direct hit clipping, recoil/berry recovery and later contact/faint order are part of the source action pipeline.

**Authenticated source anchors:** `engine/battle/effect_commands.asm:5090 BattleCommand_draintarget`; `engine/battle/move_effects/dream_eater.asm:1 BattleCommand_eatdream`; `engine/battle/effect_commands.asm:5896 BattleCommand_recoil`

**Actual runtime definitions:** `src/battle_set_effect.c:169 HandleSetEffectAbsorb`; `src/battle_move_resolution.c:4892 MoveEndMoveBlockRecoil`; `src/battle_move_resolution.c:1523 CancelerMoveEffectFailureTarget`; `src/battle_move_resolution.c:5000 MoveEndPolishedPostFaint`; `data/battle_scripts_1.s:1581 BattleScript_PolishedAbsorbLiquidOoze`

**Reviewed transitive anchors:** `engine/battle/effect_commands.asm:5093 SapHealth`

**Branch adjudication:**

| Source branch | Production binding | Distinguishing evidence and disposition |
| --- | --- | --- |
| Exact drain/recoil arithmetic, full HP and ability gates | `HandleSetEffectAbsorb / MoveEndMoveBlockRecoil` | Source vectors bind each move fraction and rounding; live cases distinguish full-HP Ooze, Magic Guard, clipped target HP, lethal recoil and berry recovery before subsequent effects. Evidence: `test/battle/move_effect/polished_attack_suite.c:166` — Polished attack suite recoil uses source fractions and exact division; `test/battle/move_effect/polished_attack_suite.c:194` — Polished attack suite drain applies source Kiss and Big Root arithmetic in order; `test/battle/move_effect/polished_attack_suite.c:233` — Polished attack suite recoil remains blocked by Rock Head and Magic Guard; remaining exact fixture titles are listed in the JSON row |

**Actual test catalog entries:**

- `test/battle/move_effect/polished_attack_suite.c:166` — Polished attack suite recoil uses source fractions and exact division (source catalog).
- `test/battle/move_effect/polished_attack_suite.c:194` — Polished attack suite drain applies source Kiss and Big Root arithmetic in order (source catalog).
- `test/battle/move_effect/polished_attack_suite.c:233` — Polished attack suite recoil remains blocked by Rock Head and Magic Guard (source catalog).
- `test/battle/move_effect/polished_attack_suite.c:277` — Polished attack suite optional doubles retain native recovery recoil and drain (source catalog).
- `test/battle/move_effect/polished_attack_suite.c:306` — Polished attack suite either active gimmick retains native recovery recoil and drain (source catalog).
- `test/battle/move_effect/polished_metronome_struggle.c:4` — Polished Metronome and Struggle resolve source move 249 as Brick Break on either side (source catalog).
- `test/battle/move_effect/polished_metronome_struggle.c:26` — Polished Metronome and Struggle call source healing aliases without native bans (source catalog).
- `test/battle/move_effect/polished_metronome_struggle.c:46` — Polished Metronome and Struggle call source Fury Strikes (source catalog).
- `test/battle/move_effect/polished_metronome_struggle.c:62` — Polished Metronome and Struggle reject known zero PP moves on both sides (source catalog).
- `test/battle/move_effect/polished_metronome_struggle.c:97` — Polished Metronome and Struggle exclude transformed moves instead of original party slots (source catalog).
- 17 additional matched entries are listed explicitly in `SUITE2_MOVE_RECONCILIATION.json`; they are not promoted to complete branch coverage.

**Remaining / scope:** Source calculator and live full-HP/Ooze/Dream Eater/HP-clipping/berry/KO-order fixtures bind these branches. Artificial extended HP that can overflow a wider intermediate is not a legal pinned default input and is not used to claim source arithmetic equivalence. Current full-catalog execution passed; terminal input gates remain separate and pending.

### future_sight

**Owner:** root shared pipeline review. **Status:** reviewed; Weakness Policy focused and full source/native execution passed; local production/input acceptance passed; associated Actions records verify the containing commit

**Commands:** `checkfuturesight`, `futuresight`

checkfuturesight rejects a second pending strike; scheduling stores source target and original party identity with three ticks. At impact GetFutureSightUser uses the active original user when present, otherwise the saved party slot, including a fainted/out-of-field user. External replay suppresses replacement ability/item/stage state; an active original user retains its ability and therefore Parental Bond. Impact reruns the authenticated hit script, including the second Bond hit, immediate item checkpoints and criticaltext, while preserving current action PP/last/counter/selected history and Rollout/rampage/Metronome counts. Protect/hidden/No Guard, current target type/stages, original stats/STAB, external crit ownership and source Speed ordering are resolved at their specified phases.

**Authenticated source anchors:** `engine/battle/move_effects/future_sight.asm:1 BattleCommand_checkfuturesight`; `engine/battle/move_effects/future_sight.asm:7 BattleCommand_futuresight`

**Actual runtime definitions:** `src/battle_script_commands.c:4537 Cmd_futuresighttargetfailure`; `src/battle_end_turn.c:454 HandleEndTurnFutureSight`; `src/battle_move_resolution.c:6977 GetRecordedMove`; `src/polished_damage.c:236 PolishedDamageBeforeRoll`; `src/polished_damage.c:11 IsPolishedFutureSightImpact`; `src/battle_util.c:3307 UsesPolishedHitCheckpoint`; `src/battle_move_resolution.c:4777 MoveEndMultihitMove`; `data/battle_scripts_1.s:3759 BattleScript_PolishedFutureAttackHit`

**Reviewed transitive anchors:** `engine/battle/move_effects/future_sight.asm:37 GetFutureSightUser`

**Branch adjudication:**

| Source branch | Production binding | Distinguishing evidence and disposition |
| --- | --- | --- |
| Delayed source identity, impact defenses and no current-action history mutation | `HandleEndTurnFutureSight / IsPolishedFutureSightImpact / PolishedDamageBeforeRoll / GetRecordedMove` | Existing source cases assert original party stats/types, active stages, external accuracy/crit exclusions, duplicate/reopen scheduling, Protect/dolls/hiding, both Metronome counters, no Rollout/rampage advance and batched lethal impacts. Evidence: `test/battle/move_effect/polished_command_closure.c:318` — Polished command closure Future Sight cannot finish a running rampage; `test/battle/polished_dynamic_damage.c:167` — Polished dynamic damage Future Sight uses original party stats and STAB after switching; `test/battle/polished_dynamic_damage.c:192` — Polished dynamic damage Future Sight takes active stages at impact; remaining exact fixture titles are listed in the JSON row |
| Active original ability and second Parental Bond impact retain source item/critical phases | `MoveEndMultihitMove / BattleScript_PolishedFutureAttackHit / UsesPolishedHitCheckpoint` | Current fixtures require both active Bond hits, intermediate Sitrus and canceled Berserk, plus immediate Liechi before criticaltext/Anger Point. External user ability suppression remains separately covered. Evidence: `test/battle/ability/polished_hit_stat_abilities.c:211` — Polished hit stats Future Sight consumes Liechi before critical text and Anger Point; `test/battle/ability/polished_hit_stat_abilities.c:238` — Polished hit stats Future Sight Parental Bond repeats and cancels Berserk after intermediate healing |

**Actual test catalog entries:**

- `test/battle/ability/polished_hit_stat_abilities.c:211` — Polished hit stats Future Sight consumes Liechi before critical text and Anger Point (source catalog).
- `test/battle/ability/polished_hit_stat_abilities.c:238` — Polished hit stats Future Sight Parental Bond repeats and cancels Berserk after intermediate healing (source catalog).
- `test/battle/move_effect/polished_command_closure.c:318` — Polished command closure Future Sight cannot finish a running rampage (source catalog).
- `test/battle/polished_dynamic_damage.c:167` — Polished dynamic damage Future Sight uses original party stats and STAB after switching (source catalog).
- `test/battle/polished_dynamic_damage.c:192` — Polished dynamic damage Future Sight takes active stages at impact (source catalog).
- `test/battle/polished_dynamic_damage.c:209` — Polished dynamic damage Future Sight never receives Analytic or Metronome boosts (source catalog).
- `test/battle/polished_dynamic_damage.c:228` — Polished dynamic damage Future Sight blocks Dark targets after either side switches (source catalog).
- `test/battle/polished_dynamic_damage.c:248` — Polished dynamic damage Future Sight bypasses Substitute at impact (source catalog).
- `test/battle/polished_dynamic_damage.c:261` — Polished dynamic damage Future Sight checks Fly Dig and No Guard at impact (source catalog).
- `test/battle/polished_dynamic_damage.c:281` — Polished dynamic damage Future Sight preserves Endure through impact (source catalog).
- 12 additional matched entries are listed explicitly in `SUITE2_MOVE_RECONCILIATION.json`; they are not promoted to complete branch coverage.

**Remaining / scope:** The earlier one-hit assumption for active Parental Bond was false. The corrected candidate loops to the source impact label rather than the native scheduling script, preserving hit-2 criticaltext. Its corrected focused source/native v9 group passed. The consolidated dynamic/history/lifecycle suites passed in the current full-catalog run. Active versus external original user is not collapsed into a generic delayed-hit exclusion. The finite Weakness Policy phase correction passed all eight source/native focused v12 groups; current full-catalog execution also passed. The original late-only source runtime was wrong for 59 early supereffectivetext families (162 mapped moves), changing Justified/Weak Armor immediate Mirror Herb totals. GetPolishedWeaknessPolicyPhase and the two BS_PolishedSuperEffectiveItem hooks now use the authenticated early/late/absent classification; IsOnTargetHitActivation suppresses every early/absent late retry, including an initially capped holder whose stat later drops. The two ordinary multihit families (seven moves) remain after hit effects on the final strike, with the source attacker-faint termination guard; PB second hit returns. Future Sight scheduling is excluded, while its actual impact uses UsesPolishedHitCheckpoint. Holder life, current item, self-stat caps/Contrary, Substitute bypass and zero-damage False Swipe admission match the source. Independent bounded static review passed, followed by eight/eight source and eight/eight native groups in weakness-policy-v12-{1,0}.log. These are the distinguishing phase fixtures, not a promotion of prior conditional-item tests. The v11 ordinary contact fixture used unmapped optional Rock Smash; replacing it with mapped Low Kick corrected that expectation without a runtime change. Full-catalog execution passed; local production gates passed; associated Actions records verify the containing commit.

### attract

**Owner:** move-command review with explicit shared dependencies. **Status:** source/runtime semantically reconciled; current full source/native catalog execution passed; local production/input acceptance passed; associated Actions records verify the containing commit

**Commands:** `attract`

The body checks original party gender (including transformed species), current hidden state and existing attraction before effective Oblivious, then applies the inherited miss result. Attract manual failure omits ordinary Blunder Policy. Destiny Knot checks the original actor immunity before reflected attraction; both held Mental Herb/cure passes retain source Speed order. CheckOppositeGender is also shared by Rivalry and Cute Charm, so these use original party identity rather than the transformed displayed species.

**Authenticated source anchors:** `engine/battle/move_effects/attract.asm:1 BattleCommand_attract`

**Actual runtime definitions:** `src/battle_script_commands.c:6238 Cmd_tryinfatuating`; `src/battle_util.c:10470 AreBattlersOfOppositeGender`; `src/battle_util.c:10481 AreBattlersOfSameGender`; `src/pokemon.c:1796 GetMonGender`; `src/polished_damage.c:122 OffensiveAbility`

**Reviewed transitive anchors:** `engine/battle/move_effects/attract.asm:178 CheckOppositeGender`; `engine/battle/effect_commands.asm:6460 CheckHiddenOpponent`; `engine/battle/move_effects/attract.asm:100 CheckMentalHerb`; `engine/battle/abilities.asm:29 RunStatusHealAbilities`; `home/battle.asm:24 TrueUserPartyAttr`; `home/battle.asm:63 OpponentPartyAttr`; `engine/battle/abilities.asm:1702 RivalryAbility`; `engine/battle/abilities.asm:713 CuteCharmAbility`

**Actual test catalog entries:**

- `test/battle/ability/polished_contact_volatile.c:6` — Polished volatile contact abilities use the exact Substitute break boundary (source catalog).
- `test/battle/ability/polished_contact_volatile.c:34` — Polished volatile contact Cute Charm uses source three-in-ten activation under every preset (source catalog).
- `test/battle/ability/polished_contact_volatile.c:53` — Polished volatile contact Tangling Hair cannot lower stats after its holder faints (source catalog).
- `test/battle/ability/polished_contact_volatile.c:71` — Polished volatile contact Tangling Hair respects the attacking Pokemon's Substitute (source catalog).
- `test/battle/ability/polished_contact_volatile.c:90` — Polished volatile contact Perish Body starts the attacker's count even when its holder faints (source catalog).
- `test/battle/ability/polished_contact_volatile_matrix.c:7` — Polished volatile contact classification covers all 124 direct attacks for all three abilities (source catalog).
- `test/battle/ability/polished_contact_volatile_matrix.c:30` — Polished volatile contact Protective Pads Punching Glove and optional Long Reach prevent activation (source catalog).
- `test/battle/ability/polished_contact_volatile_matrix.c:48` — Polished volatile contact rejects misses Protect noncontact damage and status moves (source catalog).
- `test/battle/ability/polished_contact_volatile_matrix.c:68` — Polished volatile contact Mold Breaker does not suppress these abilities but Neutralizing Gas does (source catalog).
- `test/battle/ability/polished_contact_volatile_matrix.c:88` — Polished volatile contact attacker Substitute blocks Tangling Hair but not Cute Charm or Perish Body (source catalog).
- 41 additional matched entries are listed explicitly in `SUITE2_MOVE_RECONCILIATION.json`; they are not promoted to complete branch coverage.

**Remaining / scope:** Attract/Cute Charm transformed-gender and rejection-order groups passed both v8 profiles. Rivalry v8 source failure was an unintended Tauros Intimidate baseline from Ability(NONE), not a source-gender bug; the inert Battle Armor baseline and explicit stages/RNG now isolate the intended effect. Corrected source/native v9 execution passed; the later full-catalog result also passed in the canonical runner. Destiny Knot and Mental Herb timing is linked to the existing contact-volatile matrices and ability/item ledger.

### rapid_spin

**Owner:** move-command review with explicit shared dependencies. **Status:** reviewed/live scoped

**Commands:** `clearhazards`

After damage and posthit effects clear acting side Seed, Spikes, Toxic Spikes and binding; source success/failure, faint-user and current Substitute boundaries determine whether cleanup runs. Source Speed raise uses separate secondary/self-effect rules.

**Authenticated source anchors:** `engine/battle/move_effects/rapid_spin.asm:1 BattleCommand_clearhazards`

**Actual runtime definitions:** `src/battle_script_commands.c:7420 Cmd_rapidspinfree`; `src/battle_script_commands.c:1906 Cmd_setadditionaleffects`

**Actual test catalog entries:**

- `test/battle/move_effect/polished_rapid_spin.c:16` — Polished Rapid Spin removes every source hazard layer without clearing the opposite side (source catalog).
- `test/battle/move_effect/polished_rapid_spin.c:62` — Polished Rapid Spin clears binding Leech Seed and both stacked hazards together (source catalog).
- `test/battle/move_effect/polished_rapid_spin.c:106` — Polished Rapid Spin keeps hazards and Leech Seed after misses protection immunity or paralysis (source catalog).
- `test/battle/move_effect/polished_rapid_spin.c:149` — Polished Rapid Spin cleanup works through intact and breaking Substitutes (source catalog).
- `test/battle/move_effect/polished_rapid_spin.c:192` — Polished Rapid Spin cleanup still runs after knocking out the target (source catalog).
- `test/battle/move_effect/polished_rapid_spin.c:230` — Polished Rapid Spin does not clean hazards after its user faints from contact (source catalog).
- `test/battle/move_effect/polished_rapid_spin.c:271` — Polished Rapid Spin preserves Sheer Force suppression of Life Orb and Shell Bell (source catalog).
- `test/battle/move_effect/polished_rapid_spin.c:310` — Polished Rapid Spin keeps ordinary cleanup and Speed when Neutralizing Gas suppresses Sheer Force (source catalog).
- `test/battle/move_effect/polished_rapid_spin.c:348` — Polished Rapid Spin repeats cleanup through its last PP and leaves later moves usable (source catalog).
- `test/battle/move_effect/polished_rapid_spin.c:392` — Polished Rapid Spin preserves native Sheer Force cleanup for optional moves and active gimmicks (source catalog).
- 1 additional matched entries are listed explicitly in `SUITE2_MOVE_RECONCILIATION.json`; they are not promoted to complete branch coverage.

**Remaining / scope:** Eleven source groups cover all hazards, miss/immunity/protect, intact/breaking Substitute, target KO/user contact KO, Sheer Force/Gas, repeat and native controls.

## All 129 commands

| Source command | Authenticated implementation | Review contract |
| --- | --- | --- |
| `applydamage` | `engine/battle/effect_commands.asm:2656` | `shared_damage_faint` |
| `arenatrap` | `engine/battle/move_effects/mean_look.asm:1` | `switching` |
| `attract` | `engine/battle/move_effects/attract.asm:1` | `attract` |
| `batonpass` | `engine/battle/move_effects/baton_pass.asm:1` | `switching` |
| `bellydrum` | `engine/battle/move_effects/belly_drum.asm:1` | `belly_drum` |
| `bounceback` | `engine/battle/move_effects/magic_bounce.asm:1` | `shared_accuracy` |
| `brickbreak` | `engine/battle/move_effects/brick_break.asm:1` | `screens` |
| `bugbite` | `engine/battle/move_effects/bug_bite.asm:1` | `item_moves` |
| `burn` | `engine/battle/effect_commands.asm:5412` | `shared_status` |
| `burntarget` | `engine/battle/effect_commands.asm:5178` | `shared_status` |
| `charge` | `engine/battle/effect_commands.asm:5743` | `charge` |
| `checkfuturesight` | `engine/battle/move_effects/future_sight.asm:1` | `future_sight` |
| `checkhit` | `engine/battle/effect_commands.asm:1948` | `shared_accuracy` |
| `checkobedience` | `engine/battle/effect_commands.asm:725` | `shared_action` |
| `checkpowder` | `engine/battle/effect_commands.asm:1837` | `shared_accuracy` |
| `checkpriority` | `engine/battle/effect_commands.asm:2332` | `shared_accuracy` |
| `checksafeguard` | `engine/battle/move_effects/safeguard.asm:35` | `safeguard` |
| `clearhazards` | `engine/battle/move_effects/rapid_spin.asm:1` | `rapid_spin` |
| `clearmissdamage` | `engine/battle/effect_commands.asm:4123` | `shared_action` |
| `cleartext` | `home/battle.asm:825` | `presentation` |
| `conditionalboost` | `engine/battle/effect_commands.asm:6303` | `shared_damage` |
| `confuse` | `engine/battle/effect_commands.asm:5979` | `shared_status` |
| `confusetarget` | `engine/battle/effect_commands.asm:5955` | `shared_status` |
| `constantdamage` | `engine/battle/effect_commands.asm:4511` | `shared_damage` |
| `conversion` | `engine/battle/move_effects/conversion.asm:1` | `conversion` |
| `counter` | `engine/battle/move_effects/counter.asm:1` | `counter` |
| `critical` | `engine/battle/effect_commands.asm:1172` | `shared_damage` |
| `criticaltext` | `engine/battle/effect_commands.asm:2888` | `presentation` |
| `curl` | `engine/battle/move_effects/defense_curl.asm:1` | `curl` |
| `curse` | `engine/battle/move_effects/curse.asm:1` | `curse` |
| `damagecalc` | `engine/battle/effect_commands.asm:4248` | `shared_damage` |
| `damagestats` | `engine/battle/effect_commands.asm:3894` | `shared_damage` |
| `damagevariation` | `engine/battle/effect_commands.asm:1894` | `shared_damage` |
| `destinybond` | `engine/battle/move_effects/destiny_bond.asm:1` | `destiny_bond` |
| `disable` | `engine/battle/move_effects/encore_disable.asm:12` | `restrictions` |
| `doturn` | `engine/battle/effect_commands.asm:1052` | `shared_action` |
| `doubleflyingdamage` | `engine/battle/effect_commands.asm:6349` | `shared_damage` |
| `doubleminimizedamage` | `engine/battle/effect_commands.asm:6445` | `shared_damage` |
| `doubleundergrounddamage` | `engine/battle/effect_commands.asm:6355` | `shared_damage` |
| `draintarget` | `engine/battle/effect_commands.asm:5090` | `drain_recoil` |
| `eatdream` | `engine/battle/move_effects/dream_eater.asm:1` | `drain_recoil` |
| `effectchance` | `engine/battle/effect_commands.asm:2380` | `shared_status` |
| `encore` | `engine/battle/move_effects/encore_disable.asm:1` | `restrictions` |
| `endloop` | `engine/battle/effect_commands.asm:5658` | `multihit` |
| `endmove` | `engine/battle/effect_commands.asm:138` | `shared_action` |
| `endure` | `engine/battle/move_effects/endure.asm:1` | `protect` |
| `failuretext` | `engine/battle/effect_commands.asm:2615` | `shared_action` |
| `falseswipe` | `engine/battle/move_effects/false_swipe.asm:1` | `shared_damage` |
| `flinchtarget` | `engine/battle/effect_commands.asm:5693` | `shared_status` |
| `focusenergy` | `engine/battle/move_effects/focus_energy.asm:1` | `focus` |
| `forcelowerstat` | `engine/battle/effect_commands.asm:5280` | `shared_stats` |
| `forceraisestat` | `engine/battle/effect_commands.asm:5273` | `shared_stats` |
| `foresight` | `engine/battle/move_effects/foresight.asm:1` | `foresight` |
| `freezetarget` | `engine/battle/effect_commands.asm:5210` | `shared_status` |
| `futuresight` | `engine/battle/move_effects/future_sight.asm:7` | `future_sight` |
| `getmagnitude` | `engine/battle/move_effects/magnitude.asm:1` | `shared_damage` |
| `growth` | `engine/battle/move_effects/growth.asm:1` | `growth` |
| `gyroball` | `engine/battle/move_effects/gyro_ball.asm:1` | `shared_damage` |
| `happinesspower` | `engine/battle/move_effects/return.asm:1` | `shared_damage` |
| `hastarget` | `engine/battle/effect_commands.asm:1082` | `shared_action` |
| `heal` | `engine/battle/effect_commands.asm:6092` | `healing` |
| `healbell` | `engine/battle/move_effects/heal_bell.asm:1` | `heal_bell` |
| `healweather` | `engine/battle/move_effects/healinglight.asm:1` | `healing` |
| `hiddenpower` | `engine/battle/move_effects/hidden_power.asm:1` | `shared_damage` |
| `knockoff` | `engine/battle/move_effects/knock_off.asm:1` | `item_moves` |
| `leechseed` | `engine/battle/move_effects/leech_seed.asm:1` | `seed` |
| `loweroppstat` | `engine/battle/effect_commands.asm:5328` | `shared_stats` |
| `loweroppstathit` | `engine/battle/effect_commands.asm:5344` | `shared_stats` |
| `lowerstat` | `engine/battle/effect_commands.asm:5265` | `shared_stats` |
| `lowersub` | `engine/battle/effect_commands.asm:2440` | `presentation` |
| `lowkick` | `engine/battle/move_effects/low_kick.asm:1` | `shared_damage` |
| `metronome` | `engine/battle/move_effects/metronome.asm:1` | `callers` |
| `minimize` | `engine/battle/move_effects/minimize.asm:1` | `minimize` |
| `moveanim` | `engine/battle/effect_commands.asm:2464` | `presentation` |
| `moveanimnosub` | `engine/battle/effect_commands.asm:2481` | `presentation` |
| `painsplit` | `engine/battle/move_effects/pain_split.asm:1` | `pain_split` |
| `paralyze` | `engine/battle/effect_commands.asm:5408` | `shared_status` |
| `paralyzetarget` | `engine/battle/effect_commands.asm:5204` | `shared_status` |
| `payday` | `engine/battle/move_effects/pay_day.asm:1` | `payday` |
| `perishsong` | `engine/battle/move_effects/perish_song.asm:1` | `perish_song` |
| `poison` | `engine/battle/effect_commands.asm:5402` | `shared_status` |
| `poisontarget` | `engine/battle/effect_commands.asm:5198` | `shared_status` |
| `postfainteffects` | `engine/battle/effect_commands.asm:3198` | `shared_damage_faint` |
| `posthiteffects` | `engine/battle/effect_commands.asm:3238` | `shared_damage_faint` |
| `protect` | `engine/battle/move_effects/protect.asm:1` | `protect` |
| `pursuit` | `engine/battle/move_effects/pursuit.asm:1` | `switching` |
| `rage` | `engine/battle/move_effects/rage.asm:1` | `rage` |
| `raiseoppstat` | `engine/battle/effect_commands.asm:5320` | `shared_stats` |
| `raisestathit` | `engine/battle/effect_commands.asm:5288` | `shared_stats` |
| `raisesub` | `engine/battle/effect_commands.asm:2598` | `presentation` |
| `rampage` | `engine/battle/effect_commands.asm:5494` | `rampage` |
| `rechargenextturn` | `engine/battle/effect_commands.asm:6057` | `recharge` |
| `recoil` | `engine/battle/effect_commands.asm:5896` | `drain_recoil` |
| `resetstats` | `engine/battle/effect_commands.asm:6071` | `haze` |
| `resettypematchup` | `engine/battle/effect_commands.asm:1870` | `shared_damage` |
| `roar` | `engine/battle/move_effects/roar.asm:1` | `switching` |
| `rolloutpower` | `engine/battle/move_effects/rollout.asm:3` | `rollout` |
| `roost` | `engine/battle/move_effects/roost.asm:1` | `healing` |
| `safeguard` | `engine/battle/move_effects/safeguard.asm:1` | `safeguard` |
| `screen` | `engine/battle/move_effects/reflect_light_screen.asm:1` | `screens` |
| `selfdestruct` | `engine/battle/move_effects/explosion.asm:1` | `selfdestruct` |
| `selfeffectchance` | `engine/battle/effect_commands.asm:2400` | `shared_status` |
| `sketch` | `engine/battle/move_effects/sketch.asm:1` | `sketch` |
| `skillswap` | `engine/battle/move_effects/skill_swap.asm:1` | `skill_swap` |
| `sleep` | `engine/battle/effect_commands.asm:4852` | `shared_status` |
| `sleeptalk` | `engine/battle/move_effects/sleep_talk.asm:1` | `callers` |
| `spikes` | `engine/battle/move_effects/spikes.asm:1` | `switching` |
| `splash` | `engine/battle/move_effects/splash.asm:1` | `splash` |
| `stab` | `engine/battle/effect_commands.asm:1458` | `shared_damage` |
| `starthail` | `engine/battle/move_effects/weather.asm:13` | `weather` |
| `startloop` | `engine/battle/effect_commands.asm:2955` | `multihit` |
| `startrain` | `engine/battle/move_effects/weather.asm:5` | `weather` |
| `startsandstorm` | `engine/battle/move_effects/weather.asm:9` | `weather` |
| `startsun` | `engine/battle/move_effects/weather.asm:1` | `weather` |
| `substitute` | `engine/battle/move_effects/substitute.asm:1` | `substitute` |
| `suckerpunch` | `engine/battle/move_effects/sucker_punch.asm:1` | `sucker_punch` |
| `supereffectivetext` | `engine/battle/effect_commands.asm:3008` | `presentation` |
| `switchout` | `engine/battle/effect_commands.asm:5615` | `switching` |
| `teleport` | `engine/battle/move_effects/teleport.asm:1` | `switching` |
| `thief` | `engine/battle/move_effects/thief.asm:1` | `item_moves` |
| `thunderaccuracy` | `engine/battle/move_effects/thunder.asm:1` | `shared_accuracy` |
| `toxic` | `engine/battle/move_effects/toxic.asm:1` | `shared_status` |
| `toxicspikes` | `engine/battle/move_effects/spikes.asm:21` | `switching` |
| `transform` | `engine/battle/move_effects/transform.asm:1` | `transform` |
| `traptarget` | `engine/battle/effect_commands.asm:5840` | `binding` |
| `trick` | `engine/battle/move_effects/trick.asm:1` | `item_moves` |
| `trickroom` | `engine/battle/move_effects/trick_room.asm:1` | `room` |
| `tristatuschance` | `engine/battle/move_effects/tri_attack.asm:1` | `shared_status` |
| `usedmovetext` | `engine/battle/effect_commands.asm:1039` | `shared_action` |

## All 133 used effect families

Every row binds the actual authenticated `data/moves/effects.asm` script, its source move identities and the command contracts above. The complete ordered command stream is in `SUITE2_MOVE_RECONCILIATION.json`. Shared-command references point to the concrete branch adjudications above. All used scripts retain their complete ordered command stream; `complete_behavior_parity` remains false as a conservative whole-context gate that includes original Suite 3 obligations; Suite 2 implementation and local acceptance are closed in SUITE2_VALIDATION.json, and the documented nontermination deviation remains explicit.

| Source family and script anchor | Source move identities | Contracts derived from its actual script |
| --- | --- | --- |
| `EFFECT_NORMAL_HIT` (`NormalHit`, effects.asm:5) | KARATE_CHOP, DRAGON_CLAW, SCRATCH, X_SCISSOR, NIGHT_SLASH, CUT, WING_ATTACK, DAZZLINGLEAM, VINE_WHIP, STONE_EDGE, HORN_ATTACK, TACKLE, WATER_GUN, HYDRO_PUMP, SURF, PECK, DRILL_PECK, STRENGTH, RAZOR_LEAF, ROCK_THROW, DRAGON_PULSE, AQUA_TAIL, CRABHAMMER, SLASH, HYPER_VOICE, AEROBLAST, SEED_BOMB, POWER_GEM, POWER_WHIP, MEGAHORN, SHADOW_CLAW, CROSS_CHOP | `shared_action`, `shared_accuracy`, `shared_damage`, `presentation`, `shared_damage_faint` |
| `EFFECT_PRIORITY_HIT` (`NormalHit`, effects.asm:5) | QUICK_ATTACK, BULLET_PUNCH, ICE_SHARD, MACH_PUNCH, AQUA_JET, EXTREMESPEED | `shared_action`, `shared_accuracy`, `shared_damage`, `presentation`, `shared_damage_faint` |
| `EFFECT_ALWAYS_HIT` (`NormalHit`, effects.asm:5) | AERIAL_ACE, SWIFT, FEINT_ATTACK, AURA_SPHERE, DISARM_VOICE | `shared_action`, `shared_accuracy`, `shared_damage`, `presentation`, `shared_damage_faint` |
| `EFFECT_LEECH_HIT` (`LeechHit`, effects.asm:48) | ABSORB, MEGA_DRAIN, LEECH_LIFE, DRAINING_KISS, DRAIN_PUNCH, GIGA_DRAIN | `shared_action`, `shared_accuracy`, `shared_damage`, `presentation`, `shared_damage_faint`, `drain_recoil` |
| `EFFECT_DOUBLE_HIT` (`MultiHit`, effects.asm:92) | DOUBLE_KICK, BONEMERANG | `shared_action`, `multihit`, `presentation`, `shared_accuracy`, `shared_damage`, `shared_damage_faint` |
| `EFFECT_MULTI_HIT` (`MultiHit`, effects.asm:92) | DOUBLE_SLAP, PIN_MISSILE, ROCK_BLAST, ICICLE_SPEAR, FURY_STRIKES | `shared_action`, `multihit`, `presentation`, `shared_accuracy`, `shared_damage`, `shared_damage_faint` |
| `EFFECT_RECOIL_HIT` (`RecoilHit`, effects.asm:119) | TAKE_DOWN, DOUBLE_EDGE, BRAVE_BIRD, WILD_CHARGE, STRUGGLE | `shared_action`, `shared_accuracy`, `shared_damage`, `presentation`, `shared_damage_faint`, `drain_recoil` |
| `EFFECT_FLINCH_HIT` (`FlinchHit`, effects.asm:141) | AIR_SLASH, HEADBUTT, BITE, WATERFALL, ICICLE_CRASH, IRON_HEAD, EXTRASENSORY, ROCK_SLIDE, HYPER_FANG, ZEN_HEADBUTT, ASTONISH, DARK_PULSE | `shared_action`, `shared_accuracy`, `shared_damage`, `presentation`, `shared_damage_faint`, `shared_status` |
| `EFFECT_POISON_HIT` (`PoisonHit`, effects.asm:164) | POISON_STING, GUNK_SHOT, POISON_JAB, SLUDGE_BOMB | `shared_action`, `shared_accuracy`, `shared_damage`, `presentation`, `shared_damage_faint`, `shared_status` |
| `EFFECT_BURN_HIT` (`BurnHit`, effects.asm:187) | FIRE_PUNCH, EMBER, FLAMETHROWER, SCALD, FIRE_BLAST | `shared_action`, `shared_accuracy`, `shared_damage`, `presentation`, `shared_damage_faint`, `shared_status` |
| `EFFECT_PARALYZE_HIT` (`ParalyzeHit`, effects.asm:210) | THUNDERPUNCH, THUNDERSHOCK, THUNDERBOLT, LICK, ZAP_CANNON, SPARK, DRAGONBREATH | `shared_action`, `shared_accuracy`, `shared_damage`, `presentation`, `shared_damage_faint`, `shared_status` |
| `EFFECT_FREEZE_HIT` (`FreezeHit`, effects.asm:280) | ICE_PUNCH, ICE_BEAM, BLIZZARD | `shared_action`, `shared_accuracy`, `shared_damage`, `presentation`, `shared_damage_faint`, `shared_status` |
| `EFFECT_CONFUSE_HIT` (`ConfuseHit`, effects.asm:303) | PSYBEAM, CONFUSION, WATER_PULSE, DIZZY_PUNCH, DYNAMICPUNCH, HURRICANE | `shared_action`, `shared_accuracy`, `shared_damage`, `presentation`, `shared_damage_faint`, `shared_status` |
| `EFFECT_ATTACK_UP_HIT` (`AttackUpHit`, effects.asm:326) | METAL_CLAW | `shared_action`, `shared_accuracy`, `shared_damage`, `presentation`, `shared_damage_faint`, `shared_status`, `shared_stats` |
| `EFFECT_DEFENSE_UP_HIT` (`DefenseUpHit`, effects.asm:349) | STEEL_WING | `shared_action`, `shared_accuracy`, `shared_damage`, `presentation`, `shared_damage_faint`, `shared_status`, `shared_stats` |
| `EFFECT_SPEED_UP_HIT` (`SpeedUpHit`, effects.asm:372) | FLAME_CHARGE | `shared_action`, `shared_accuracy`, `shared_damage`, `presentation`, `shared_damage_faint`, `shared_status`, `shared_stats` |
| `EFFECT_ALL_UP_HIT` (`AllUpHit`, effects.asm:395) | ANCIENTPOWER | `shared_action`, `shared_accuracy`, `shared_damage`, `presentation`, `shared_damage_faint`, `shared_status`, `shared_stats` |
| `EFFECT_ATTACK_DOWN_HIT` (`AttackDownHit`, effects.asm:423) | AURORA_BEAM, PLAY_ROUGH | `shared_action`, `shared_accuracy`, `shared_damage`, `presentation`, `shared_damage_faint`, `shared_status`, `shared_stats` |
| `EFFECT_DEFENSE_DOWN_HIT` (`DefenseDownHit`, effects.asm:446) | IRON_TAIL, CRUNCH | `shared_action`, `shared_accuracy`, `shared_damage`, `presentation`, `shared_damage_faint`, `shared_status`, `shared_stats` |
| `EFFECT_SPEED_DOWN_HIT` (`SpeedDownHit`, effects.asm:492) | BUBBLE_BEAM, BULLDOZE, ICY_WIND | `shared_action`, `shared_accuracy`, `shared_damage`, `presentation`, `shared_damage_faint`, `shared_status`, `shared_stats` |
| `EFFECT_SP_ATK_DOWN_HIT` (`SpecialAttackDownHit`, effects.asm:515) | MOONBLAST | `shared_action`, `shared_accuracy`, `shared_damage`, `presentation`, `shared_damage_faint`, `shared_status`, `shared_stats` |
| `EFFECT_SP_DEF_DOWN_HIT` (`SpecialDefenseDownHit`, effects.asm:538) | FOCUS_BLAST, ACID, PSYCHIC_M, FLASH_CANNON, EARTH_POWER, ENERGY_BALL, BUG_BUZZ, SHADOW_BALL | `shared_action`, `shared_accuracy`, `shared_damage`, `presentation`, `shared_damage_faint`, `shared_status`, `shared_stats` |
| `EFFECT_ACCURACY_DOWN_HIT` (`AccuracyDownHit`, effects.asm:561) | MUD_SLAP, OCTAZOOKA | `shared_action`, `shared_accuracy`, `shared_damage`, `presentation`, `shared_damage_faint`, `shared_status`, `shared_stats` |
| `EFFECT_STATIC_DAMAGE` (`StaticDamage`, effects.asm:630) | SONIC_BOOM, DRAGON_RAGE | `shared_action`, `shared_accuracy`, `shared_damage`, `presentation`, `shared_damage_faint` |
| `EFFECT_LEVEL_DAMAGE` (`StaticDamage`, effects.asm:630) | SEISMIC_TOSS, NIGHT_SHADE | `shared_action`, `shared_accuracy`, `shared_damage`, `presentation`, `shared_damage_faint` |
| `EFFECT_POISON` (`DoPoison`, effects.asm:646) | POISONPOWDER | `shared_action`, `shared_accuracy`, `safeguard`, `shared_status` |
| `EFFECT_BURN` (`DoBurn`, effects.asm:672) | WILL_O_WISP | `shared_action`, `shared_accuracy`, `shared_damage`, `safeguard`, `shared_status` |
| `EFFECT_PARALYZE` (`DoParalyze`, effects.asm:686) | STUN_SPORE, THUNDER_WAVE, GLARE | `shared_action`, `shared_accuracy`, `safeguard`, `shared_status` |
| `EFFECT_SLEEP` (`DoSleep`, effects.asm:699) | SING, SLEEP_POWDER, HYPNOSIS, SPORE | `shared_action`, `shared_accuracy`, `safeguard`, `shared_status` |
| `EFFECT_CONFUSE` (`DoConfuse`, effects.asm:724) | SUPERSONIC, CONFUSE_RAY, SWEET_KISS | `shared_action`, `shared_accuracy`, `safeguard`, `shared_status` |
| `EFFECT_TOXIC` (`Toxic`, effects.asm:659) | TOXIC | `shared_action`, `shared_accuracy`, `safeguard`, `shared_status` |
| `EFFECT_EVASION_UP` (`EvasionUp`, effects.asm:778) | DOUBLE_TEAM | `shared_action`, `shared_stats` |
| `EFFECT_FOCUS_ENERGY` (`FocusEnergy`, effects.asm:785) | FOCUS_ENERGY | `shared_action`, `focus` |
| `EFFECT_ATTACK_UP_2` (`AttackUp2`, effects.asm:792) | SWORDS_DANCE | `shared_action`, `shared_stats` |
| `EFFECT_DEFENSE_UP_2` (`DefenseUp2`, effects.asm:799) | BARRIER | `shared_action`, `shared_stats` |
| `EFFECT_SPEED_UP_2` (`SpeedUp2`, effects.asm:806) | AGILITY | `shared_action`, `shared_stats` |
| `EFFECT_SP_ATK_UP_2` (`SpecialAttackUp2`, effects.asm:813) | NASTY_PLOT | `shared_action`, `shared_stats` |
| `EFFECT_SP_DEF_UP_2` (`SpecialDefenseUp2`, effects.asm:820) | AMNESIA | `shared_action`, `shared_stats` |
| `EFFECT_BULK_UP` (`BulkUp`, effects.asm:841) | BULK_UP | `shared_action`, `shared_stats` |
| `EFFECT_CALM_MIND` (`CalmMind`, effects.asm:849) | CALM_MIND | `shared_action`, `shared_stats` |
| `EFFECT_GROWTH` (`Growth`, effects.asm:857) | GROWTH | `shared_action`, `growth` |
| `EFFECT_DRAGON_DANCE` (`DragonDance`, effects.asm:864) | DRAGON_DANCE | `shared_action`, `shared_stats` |
| `EFFECT_HONE_CLAWS` (`HoneClaws`, effects.asm:872) | HONE_CLAWS | `shared_action`, `shared_stats` |
| `EFFECT_DEFENSE_CURL` (`DefenseCurl`, effects.asm:880) | DEFENSE_CURL | `shared_action`, `shared_stats`, `curl` |
| `EFFECT_MINIMIZE` (`Minimize`, effects.asm:888) | MINIMIZE | `shared_action`, `shared_stats`, `minimize` |
| `EFFECT_SHELL_SMASH` (`ShellSmash`, effects.asm:896) | SHELL_SMASH | `shared_action`, `shared_stats` |
| `EFFECT_ATTACK_DOWN` (`AttackDown`, effects.asm:907) | GROWL | `shared_action`, `shared_accuracy`, `shared_stats` |
| `EFFECT_DEFENSE_DOWN` (`DefenseDown`, effects.asm:918) | LEER | `shared_action`, `shared_accuracy`, `shared_stats` |
| `EFFECT_ACCURACY_DOWN` (`AccuracyDown`, effects.asm:962) | SMOKESCREEN, FLASH | `shared_action`, `shared_accuracy`, `shared_stats` |
| `EFFECT_ATTACK_DOWN_2` (`AttackDown2`, effects.asm:984) | CHARM | `shared_action`, `shared_accuracy`, `shared_stats` |
| `EFFECT_DEFENSE_DOWN_2` (`DefenseDown2`, effects.asm:995) | SCREECH | `shared_action`, `shared_accuracy`, `shared_stats` |
| `EFFECT_SPEED_DOWN_2` (`SpeedDown2`, effects.asm:1006) | STRING_SHOT, SCARY_FACE | `shared_action`, `shared_accuracy`, `shared_stats` |
| `EFFECT_HAZE` (`ResetStats`, effects.asm:1061) | HAZE | `shared_action`, `haze` |
| `EFFECT_SAFEGUARD` (`Safeguard`, effects.asm:1068) | SAFEGUARD | `shared_action`, `safeguard` |
| `EFFECT_REFLECT` (`Screen`, effects.asm:1075) | REFLECT | `shared_action`, `screens` |
| `EFFECT_LIGHT_SCREEN` (`Screen`, effects.asm:1075) | LIGHT_SCREEN | `shared_action`, `screens` |
| `EFFECT_HEAL` (`Heal`, effects.asm:1089) | RECOVER, FRESH_SNACK, REST | `shared_action`, `healing` |
| `EFFECT_RAMPAGE` (`Rampage`, effects.asm:1104) | THRASH, PETAL_DANCE, OUTRAGE | `shared_action`, `rampage`, `shared_accuracy`, `shared_damage`, `presentation`, `shared_damage_faint` |
| `EFFECT_TRAP` (`Trap`, effects.asm:1127) | WRAP, FIRE_SPIN, WHIRLPOOL | `shared_action`, `shared_accuracy`, `shared_damage`, `presentation`, `shared_damage_faint`, `binding` |
| `EFFECT_EXPLOSION` (`Explosion`, effects.asm:1150) | EXPLOSION | `shared_action`, `shared_accuracy`, `shared_damage`, `selfdestruct`, `presentation`, `shared_damage_faint` |
| `EFFECT_DREAM_EATER` (`DreamEater`, effects.asm:1172) | DREAM_EATER | `shared_action`, `shared_accuracy`, `shared_damage`, `presentation`, `shared_damage_faint`, `drain_recoil` |
| `EFFECT_ROAR` (`Roar`, effects.asm:1194) | ROAR | `shared_action`, `shared_accuracy`, `switching` |
| `EFFECT_CONVERSION` (`Conversion`, effects.asm:1224) | CONVERSION | `shared_action`, `conversion` |
| `EFFECT_PAY_DAY` (`PayDay`, effects.asm:1231) | PAY_DAY | `shared_action`, `shared_accuracy`, `shared_damage`, `presentation`, `shared_damage_faint`, `payday` |
| `EFFECT_TRI_ATTACK` (`TriAttack`, effects.asm:1302) | TRI_ATTACK | `shared_action`, `shared_accuracy`, `shared_damage`, `presentation`, `shared_damage_faint`, `shared_status` |
| `EFFECT_SUPER_FANG` (`StaticDamage`, effects.asm:630) | SUPER_FANG | `shared_action`, `shared_accuracy`, `shared_damage`, `presentation`, `shared_damage_faint` |
| `EFFECT_TRANSFORM` (`Transform`, effects.asm:1325) | TRANSFORM | `shared_action`, `transform` |
| `EFFECT_SUBSTITUTE` (`Substitute`, effects.asm:1333) | SUBSTITUTE | `shared_action`, `substitute` |
| `EFFECT_HYPER_BEAM` (`HyperBeam`, effects.asm:1340) | HYPER_BEAM, GIGA_IMPACT | `shared_action`, `shared_accuracy`, `shared_damage`, `presentation`, `shared_damage_faint`, `recharge` |
| `EFFECT_RAGE` (`Rage`, effects.asm:1362) | RAGE | `shared_action`, `shared_accuracy`, `shared_damage`, `presentation`, `rage`, `shared_damage_faint` |
| `EFFECT_METRONOME` (`Metronome`, effects.asm:1384) | METRONOME | `shared_action`, `callers` |
| `EFFECT_LEECH_SEED` (`LeechSeed`, effects.asm:1391) | LEECH_SEED | `shared_action`, `shared_accuracy`, `seed` |
| `EFFECT_SPLASH` (`Splash`, effects.asm:1402) | SPLASH | `shared_action`, `splash` |
| `EFFECT_DISABLE` (`Disable`, effects.asm:1409) | DISABLE | `shared_action`, `shared_accuracy`, `restrictions` |
| `EFFECT_COUNTER` (`Counter`, effects.asm:1421) | COUNTER, MIRROR_COAT | `shared_action`, `shared_accuracy`, `counter`, `presentation`, `shared_damage_faint` |
| `EFFECT_ENCORE` (`Encore`, effects.asm:1435) | ENCORE | `shared_action`, `shared_accuracy`, `restrictions` |
| `EFFECT_PAIN_SPLIT` (`PainSplit`, effects.asm:1447) | PAIN_SPLIT | `shared_action`, `shared_accuracy`, `pain_split` |
| `EFFECT_SKETCH` (`Sketch`, effects.asm:1457) | SKETCH | `shared_action`, `sketch` |
| `EFFECT_SLEEP_TALK` (`SleepTalk`, effects.asm:1465) | SLEEP_TALK | `shared_action`, `callers` |
| `EFFECT_DESTINY_BOND` (`DestinyBond`, effects.asm:1472) | DESTINY_BOND | `shared_action`, `shared_accuracy`, `destiny_bond` |
| `EFFECT_REVERSAL` (`Reversal`, effects.asm:1480) | REVERSAL | `shared_action`, `shared_accuracy`, `shared_damage`, `presentation`, `shared_damage_faint` |
| `EFFECT_FALSE_SWIPE` (`FalseSwipe`, effects.asm:1498) | FALSE_SWIPE | `shared_action`, `shared_accuracy`, `shared_damage`, `presentation`, `shared_damage_faint` |
| `EFFECT_HEAL_BELL` (`HealBell`, effects.asm:1520) | HEAL_BELL | `shared_action`, `heal_bell` |
| `EFFECT_THIEF` (`Thief`, effects.asm:1527) | THIEF | `shared_action`, `shared_accuracy`, `shared_damage`, `presentation`, `shared_damage_faint`, `item_moves` |
| `EFFECT_MEAN_LOOK` (`MeanLook`, effects.asm:1550) | MEAN_LOOK | `shared_action`, `shared_accuracy`, `switching` |
| `EFFECT_CURSE` (`Curse`, effects.asm:1560) | CURSE | `shared_action`, `curse` |
| `EFFECT_PROTECT` (`Protect`, effects.asm:1567) | PROTECT | `shared_action`, `protect` |
| `EFFECT_SPIKES` (`Spikes`, effects.asm:1574) | SPIKES | `shared_action`, `shared_accuracy`, `switching` |
| `EFFECT_FORESIGHT` (`Foresight`, effects.asm:1592) | FORESIGHT | `shared_action`, `shared_accuracy`, `foresight` |
| `EFFECT_PERISH_SONG` (`PerishSong`, effects.asm:1603) | PERISH_SONG | `shared_action`, `perish_song` |
| `EFFECT_SANDSTORM` (`Sandstorm`, effects.asm:1610) | SANDSTORM | `shared_action`, `weather` |
| `EFFECT_HAIL` (`Hail`, effects.asm:1617) | HAIL | `shared_action`, `weather` |
| `EFFECT_ENDURE` (`Endure`, effects.asm:1624) | ENDURE | `shared_action`, `protect` |
| `EFFECT_ROLLOUT` (`Rollout`, effects.asm:1631) | ROLLOUT | `shared_action`, `shared_accuracy`, `shared_damage`, `rollout`, `presentation`, `shared_damage_faint` |
| `EFFECT_SWAGGER` (`Swagger`, effects.asm:1653) | SWAGGER | `shared_action`, `shared_accuracy`, `shared_stats`, `shared_status` |
| `EFFECT_CONDITIONAL_BOOST` (`ConditionalBoostHit`, effects.asm:1665) | ACROBATICS, HEX, AVALANCHE, VENOSHOCK, FACADE | `shared_action`, `shared_accuracy`, `shared_damage`, `presentation`, `shared_damage_faint` |
| `EFFECT_ATTRACT` (`Attract`, effects.asm:1687) | ATTRACT | `shared_action`, `shared_accuracy`, `attract` |
| `EFFECT_RETURN` (`Return`, effects.asm:1698) | RETURN | `shared_action`, `shared_accuracy`, `shared_damage`, `presentation`, `shared_damage_faint` |
| `EFFECT_SACRED_FIRE` (`SacredFire`, effects.asm:1720) | SACRED_FIRE | `shared_action`, `shared_accuracy`, `shared_damage`, `presentation`, `shared_damage_faint`, `shared_status` |
| `EFFECT_MAGNITUDE` (`Magnitude`, effects.asm:1743) | MAGNITUDE | `shared_action`, `shared_accuracy`, `shared_damage`, `presentation`, `shared_damage_faint` |
| `EFFECT_BATON_PASS` (`BatonPass`, effects.asm:1810) | BATON_PASS | `shared_action`, `switching` |
| `EFFECT_PURSUIT` (`Pursuit`, effects.asm:1817) | PURSUIT | `shared_action`, `switching`, `shared_accuracy`, `shared_damage`, `presentation`, `shared_damage_faint` |
| `EFFECT_RAPID_SPIN` (`RapidSpin`, effects.asm:1840) | RAPID_SPIN | `shared_action`, `shared_accuracy`, `shared_damage`, `presentation`, `shared_damage_faint`, `shared_status`, `shared_stats`, `rapid_spin` |
| `EFFECT_HEALING_LIGHT` (`HealingLight`, effects.asm:1864) | HEALINGLIGHT | `shared_action`, `healing` |
| `EFFECT_HIDDEN_POWER` (`HiddenPower`, effects.asm:1871) | HIDDEN_POWER | `shared_action`, `shared_accuracy`, `shared_damage`, `presentation`, `shared_damage_faint` |
| `EFFECT_RAIN_DANCE` (`RainDance`, effects.asm:1893) | RAIN_DANCE | `shared_action`, `weather` |
| `EFFECT_SUNNY_DAY` (`SunnyDay`, effects.asm:1900) | SUNNY_DAY | `shared_action`, `weather` |
| `EFFECT_BELLY_DRUM` (`BellyDrum`, effects.asm:1907) | BELLY_DRUM | `shared_action`, `belly_drum` |
| `EFFECT_EARTHQUAKE` (`Earthquake`, effects.asm:1914) | EARTHQUAKE | `shared_action`, `shared_accuracy`, `shared_damage`, `presentation`, `shared_damage_faint` |
| `EFFECT_FUTURE_SIGHT` (`FutureSight`, effects.asm:1936) | FUTURE_SIGHT | `future_sight`, `shared_action`, `shared_accuracy`, `shared_damage`, `presentation`, `shared_damage_faint` |
| `EFFECT_GUST` (`Gust`, effects.asm:1959) | GUST | `shared_action`, `shared_accuracy`, `shared_damage`, `presentation`, `shared_damage_faint` |
| `EFFECT_STOMP` (`Stomp`, effects.asm:1981) | STOMP | `shared_action`, `shared_accuracy`, `shared_damage`, `presentation`, `shared_damage_faint`, `shared_status` |
| `EFFECT_SOLAR_BEAM` (`SolarBeam`, effects.asm:2005) | SOLAR_BEAM | `shared_action`, `charge`, `shared_accuracy`, `shared_damage`, `presentation`, `shared_damage_faint` |
| `EFFECT_THUNDER` (`Thunder`, effects.asm:2027) | THUNDER | `shared_action`, `shared_accuracy`, `shared_damage`, `presentation`, `shared_damage_faint`, `shared_status` |
| `EFFECT_TELEPORT` (`Teleport`, effects.asm:2051) | TELEPORT | `shared_action`, `switching` |
| `EFFECT_FLY` (`Fly`, effects.asm:2058) | FLY, DIG | `shared_action`, `charge`, `shared_accuracy`, `shared_damage`, `presentation`, `shared_damage_faint` |
| `EFFECT_FLARE_BLITZ` (`FlareBlitz`, effects.asm:2082) | FLARE_BLITZ | `shared_action`, `shared_accuracy`, `shared_damage`, `presentation`, `shared_damage_faint`, `drain_recoil`, `shared_status` |
| `EFFECT_PSYSTRIKE` (`NormalHit`, effects.asm:5) | PSYSTRIKE | `shared_action`, `shared_accuracy`, `shared_damage`, `presentation`, `shared_damage_faint` |
| `EFFECT_JUMP_KICK` (`NormalHit`, effects.asm:5) | HI_JUMP_KICK | `shared_action`, `shared_accuracy`, `shared_damage`, `presentation`, `shared_damage_faint` |
| `EFFECT_SWITCH_HIT` (`SwitchHit`, effects.asm:70) | VOLT_SWITCH, U_TURN | `shared_action`, `shared_accuracy`, `shared_damage`, `presentation`, `shared_damage_faint`, `switching` |
| `EFFECT_SUCKER_PUNCH` (`SuckerPunch`, effects.asm:26) | SUCKER_PUNCH | `shared_action`, `sucker_punch`, `shared_accuracy`, `shared_damage`, `presentation`, `shared_damage_faint` |
| `EFFECT_TOXIC_SPIKES` (`ToxicSpikes`, effects.asm:1583) | TOXIC_SPIKES | `shared_action`, `shared_accuracy`, `switching` |
| `EFFECT_ROOST` (`Roost`, effects.asm:1096) | ROOST | `shared_action`, `healing` |
| `EFFECT_CLOSE_COMBAT` (`CloseCombat`, effects.asm:607) | CLOSE_COMBAT | `shared_action`, `shared_accuracy`, `shared_damage`, `presentation`, `shared_damage_faint`, `shared_stats` |
| `EFFECT_SKILL_SWAP` (`SkillSwap`, effects.asm:1205) | SKILL_SWAP | `shared_action`, `shared_accuracy`, `skill_swap` |
| `EFFECT_TRICK` (`Trick`, effects.asm:1214) | TRICK | `shared_action`, `shared_accuracy`, `item_moves` |
| `EFFECT_KNOCK_OFF` (`KnockOff`, effects.asm:1253) | KNOCK_OFF | `shared_action`, `shared_accuracy`, `shared_damage`, `presentation`, `item_moves`, `shared_damage_faint` |
| `EFFECT_BUG_BITE` (`BugBite`, effects.asm:1278) | BUG_BITE | `shared_action`, `shared_accuracy`, `shared_damage`, `presentation`, `item_moves`, `shared_damage_faint` |
| `EFFECT_GYRO_BALL` (`GyroBall`, effects.asm:1766) | GYRO_BALL | `shared_action`, `shared_accuracy`, `shared_damage`, `presentation`, `shared_damage_faint` |
| `EFFECT_BODY_SLAM` (`BodySlam`, effects.asm:233) | BODY_SLAM | `shared_action`, `shared_accuracy`, `shared_damage`, `presentation`, `shared_damage_faint`, `shared_status` |
| `EFFECT_LOW_KICK` (`LowKick`, effects.asm:1788) | LOW_KICK | `shared_action`, `shared_accuracy`, `shared_damage`, `presentation`, `shared_damage_faint` |
| `EFFECT_BRICK_BREAK` (`BrickBreak`, effects.asm:469) | BRICK_BREAK | `shared_action`, `shared_accuracy`, `shared_damage`, `presentation`, `shared_damage_faint`, `screens` |
| `EFFECT_TRICK_ROOM` (`TrickRoom`, effects.asm:1082) | TRICK_ROOM | `shared_action`, `room` |

## Deviations, scope boundaries and true inventory exclusions

- Native-only tests are explicitly labeled; catalog membership does not assert execution. The parent runner records current source/native/AI/ROM results in the canonical closure review.
- The complete ordered scripts are taken from the authenticated attack-suite source, including data/moves/effects.asm, which is not included in the decoded battle-prefix directory. Command bodies and transitive helpers come from the authenticated full battle suite.
- Exact shared RNG stream, animation/text and existing trainer AI/link/rewards remain Suite 3 by the original plan. No action-state branch is reclassified to hide an open gap.
- Light Metal with a source weight-one target is a reachable authenticated source nontermination. Retaining bounded native arithmetic for that input is a deliberate semantic deviation, not a true inventory exclusion or a passing source result.
- Sketch after target self-KO is not a gap because its source script executes hastarget before sketch.
- CheckSubstituteOpp honors effective Infiltrator through GetTrueUserAbility, external Future Sight and eligible sound moves. A direct call is not an unconditional Substitute rejection. Sketch and item-command interpretations were corrected after tracing this helper.
- UserCanLoseItem compares party species/form through UserPartyAttr and CompareSpeciesForm; the explicit Armored Mewtwo form is not a wildcard for all Mewtwo.
- The 18 finite command-closure groups resolved ten earlier move-specific evidence rows and passed both profiles in v8/v9. The corrected broader command/identity/thaw/reflection groups also passed focused v9, with status/reactive-item followups passing v10. The current full source/native/shared Cud/AI runs passed; production-linked prompt probes also passed in both profiles; SUITE2_VALIDATION.json records the completed local acceptance gates.

The following 19 effect constants have no move in the pinned default 255-move table. They are not missing default move content:

`EFFECT_ACCURACY_DOWN_2`, `EFFECT_ACCURACY_UP`, `EFFECT_ACCURACY_UP_2`, `EFFECT_ATTACK_UP`, `EFFECT_DEFENSE_UP`, `EFFECT_EVASION_DOWN`, `EFFECT_EVASION_DOWN_2`, `EFFECT_EVASION_DOWN_HIT`, `EFFECT_EVASION_UP_2`, `EFFECT_FREEZE`, `EFFECT_SLEEP_HIT`, `EFFECT_SPEED_DOWN`, `EFFECT_SPEED_UP`, `EFFECT_SP_ATK_DOWN`, `EFFECT_SP_ATK_DOWN_2`, `EFFECT_SP_ATK_UP`, `EFFECT_SP_DEF_DOWN`, `EFFECT_SP_DEF_DOWN_2`, `EFFECT_SP_DEF_UP`
