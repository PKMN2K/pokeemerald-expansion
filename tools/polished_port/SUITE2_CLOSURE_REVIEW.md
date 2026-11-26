# Suite 2 closure review

Source: Polished Crystal v3.2.3, default non-FAITHFUL build,
`3fa43192379df5c3e7b09a08e4d5d79af4f02f42`.

This is the consolidated closure record for the pass resumed from `2f285962`.
Suite 2 is closed as an implementation milestone, with the documented reachable
source nontermination deviation retained below. All local acceptance gates have
passed. The 129-command / 133-effect / 255-move inventory and its eight owned
domains are unchanged. A fixture name or inventory row is not evidence that a branch passed;
the review records source interpretation, runtime binding and distinguishing
assertions, and the final validation section records actual execution.

The detailed reconciliation is split by the existing implementation ownership:

| Review | Scope |
| --- | --- |
| `SUITE2_MOVE_RECONCILIATION.md` and `.json` | Every used command and ordered effect script; all 255 source moves; transitive action, accuracy, damage, status and stat helpers. |
| `SUITE2_ABILITY_ITEM_RECONCILIATION.md` | All 155 source ability identities including NONE, all 89 held-effect constants, the 76 named default held effects, used medicines and acquisition/reconsumption branches. |
| `SUITE2_SWITCHING_FAINTING_RECONCILIATION.md` | Voluntary/forced/pivot/Pursuit switching, faint and replacement chains, terminal item cleanup, selectable Shift and wild faint prompts. |

These records share the source pin above. The command JSON records the complete
ordered source scripts and named fixtures; it does not treat a lexical match as
semantic closure. The following table records the source decisions and their
runtime evidence from the earlier internal checkpoints.

| Reviewed source branch | Runtime binding | Distinguishing evidence |
| --- | --- | --- |
| `home/battle.asm:RestoreBattleItems` restores all six backups. `_ConsumeUserItem` clears only a matching consumed player berry. Removal and nonberry consumption leave the backup. | `PolishedConsumeBackupItem`, `TryRestoreHeldItems` | `polished_persistence.c`: matching/different berries; Knock Off; nonberries; popped Air Balloon; switched slots. |
| `SetBackupItem` fills an empty backup; wild Trick overwrites one. Wild theft uses the held slot even when the acquired item went to the Bag. | `PolishedSetBackupItem`, `StealTargetItem`, `Cmd_tryswapitems`, `Cmd_tryrecycleitem` | Trainer/wild Trick, held/bagged Thief, Harvest/Pickup, and capture cases in `polished_persistence.c`. |
| Temporary optional native rules must not corrupt the source backup when normal rules resume. This is an Expansion compatibility invariant, not new Polished Crystal content. | Backup bookkeeping remains active during temporary gimmicks; acquisition eligibility and final restoration retain their respective rule gates. | Two `polished_persistence.c` regressions reproduce berry duplication and erased wild Pickup acquisitions before their fixes. Each checks an active gimmick, Dynamax expiration and switching, with native controls. |
| `core.asm:SetChoiceLock` records selection before action resolution, without requiring a Choice item. Encore overwrites the shared history; Mental Herb and expiry retain its high nibble. | `SetActionsAndBattlersTurnOrder`, `GetChoiceLockedMove`, `Cmd_trysetencore` | `polished_choice.c`: selection/failure/flinch, four slots, acquisition/removal, Encore cure/expiry, switching, Transform, Metronome and Gorilla Tactics. |
| `pokemon/learn.asm` clears active-player packed locks matching the forgotten slot, including for a benched learner. | `Cmd_yesnoboxlearnmove` calls `PolishedForgetMoveLocks` before replacement. | `polished_action_boundaries.c`: every forgotten slot and active/bench recipients, unrelated locks preserved. This is a production-boundary test; full selection-menu traversal is not asserted here. |
| `CheckTurn` checks recharge, flinch, sleep/freeze, confusion, attraction, Disable and paralysis before obedience/PP. `CantMove` clears Destiny Bond even on a repeat choice. | `DoAttackCanceler`, `CancelerAsleepOrFrozen`, cancellation cleanup | `polished_obedience.c` and the corrected `polished_action_resolution.c` flinch case; existing sleep/freeze/self-thaw catalogs remain controls. |
| `BattleCommand_checkobedience` compares the 16-bit trainer ID and Johto caps 10/20/30/50/70/101. Alternate moves require another PP-bearing slot and no Disable, ignoring Choice/Encore. | `GetPolishedObedience`, `GetPolishedObedienceLevel`, `CancelerObedience` | `polished_obedience.c`: IDs, badges, level/roll boundaries, alternate/no-PP/Disable branches, flinch precedence, self-damage and final history clearing. |
| Disobedience napping overwrites status with a nonzero three-bit timer and does not invoke `PostStatus`. | `BattleScript_PolishedDisobedienceNap`; generic move/end-turn cure exclusions, cleared on switching | Poison replacement, uniform timer boundaries, Insomnia/Vital Spirit remaining asleep into the following action. Genuine entry/ability-change cures retain their existing hooks. |
| `ForewarnAbility` uses source listed power, Counter/Mirror Coat 120, ignores zero power, and samples ties uniformly. `AnticipationAbility` skips status moves. | Generated `polished_entry_abilities.h`, `ForewarnChooseMove`, Anticipation handler | `polished_entry_information.c`: dynamic/fixed powers, Counter vs Overheat, status-only sets, ties, optional Fissure fallback and Thunder Wave. The source comment claiming 160 is not its implementation. |
| `XItemEffect` requests two stages and clears pending Mirror Herb boosts on success. | `BS_ItemIncreaseStat`, `BS_PolishedBattleItemFriendship` | `polished_x_items.c`: all six items under an older native preset; a later Mirror Herb checkpoint cannot copy the boost. |
| `BattleCommand_rampage` does not start while asleep; two/three actions, nonfinal miss cancellation, final cancellation confusion, existing confusion/Own Tempo and faint/Future Sight exclusions. | `HandleSetEffectThrash`, `CancelMultiTurnMoves`, `MoveEndRampage`; source `HandleRampage`/`CantMove` at effect_commands.asm:5494/454 | `polished_action_boundaries.c` covers Sleep Talk, middle-turn Protect unlocking, final-turn flinch and Own Tempo under GEN_3. The four both-side `polished_command_closure.c` groups explicitly cover preserving existing confusion, final Protect under GEN_3, a final contact-user KO with no confusion, and active Future Sight preserving the running counter/lock. These seven named groups passed in both full-catalog logs. Ordinary Thrash tests supply native duration, immunity and failed-action controls. Remaining predicate equivalence is explicit: inactive rampage returns; the final counter-zero branch precedes the common missed/unaffected check, so accuracy miss and type immunity do not add a distinct source final-action branch; `GetFutureSightUser` returns nc for active and external impacts, both rejected by the same `IsPolishedFutureSightImpact` guard. External-impact rampage is code-equivalent here, not claimed as a separate live fixture. No rampage rejection predicate remains unclassified in this bounded review. |
| `selfdestruct` clears status/Seed/opposing Destiny Bond, reserves the first faint and clears stored Unnerve before Leppa/healing sweeps and attack damage. Damp returns before these mutations. | `Cmd_setatkhptozero`, `BattleScript_Explosion`, `PolishedHandleUnnerveExit` | `polished_action_boundaries.c`: berry healing before Explosion animation/damage, poison clearing and Damp prevention; existing Destiny Bond/outcome scenarios. |
| `EvolveDuringBattle` skips Item when reloading the active mon. Temporary party consumption does not clear the battle-start backup. | `PolishedReloadBattleMonAfterEvolution`, source backup restoration | Corrected live evolution regression; real evolution completion and B cancellation in `test_battle_evolution_emulator.py --polished-rules`, with native mode retained. Post-battle evolution still consumes normally. |
| `IncreaseMetronomeCount` runs after `CheckTurn`, before obedience and PP; counts attempts without requiring the item, caps at five, and skips called reentry/Future Sight. Disobedient `.UseInstead` re-enters `DoMove`. | `PolishedIncreaseMetronomeCount`, `CancelerObedience`, source move history in `BattleStruct` | `polished_move_history.c`: misses/Protect, flinched alternate selection, changing called moves, disobedient alternates, saturation, different executed moves and switching. Damage and PP distinguish the histories. |
| Enemy Baton Pass skips `NewEnemyMonStatus`, retaining selected-move history; player sendout explicitly clears its selected move. Source pre-action and native post-action Metronome counters have different phases. | Separate `polishedMetronomeCount`, source damage adapter, native counter maintenance and `SwitchInClearSetData` | `polished_move_history.c`: Metronome-called Baton Pass followed by replacement Metronome on both sides; either-side Tera activation with item/no-item damage controls. `polished_dynamic_damage.c` verifies Future Sight preserves both independent histories. |
| Magic Bounce calls `ResetTurn` without restoring `wCharging`; a later own action still skips obedience, PP and usage/history updates. Its `IN_ABILITY` path also preserves the reflector's earlier history. | `polishedCalledMoveBattlers`, `MoveEndBouncedMove`, action cancellation/PP and `GetRecordedMove` | `polished_move_history.c`: both reflector sides, repeated/different later move, an outsider whose later obedience is bypassed, retained history, PP and actual damage. Caller and charging tests retain explicit history-clearing controls. |
| Source selection resets the shared Protect/Endure counter for other moves and nonmove actions. `CantMove` does not reset it. | `SetActionsAndBattlersTurnOrder`, `TryResetConsecutiveUseCounter`, `CanUseMoveConsecutively`, `Cmd_setprotectlike` | `polished_protect_chain.c`: all capped odds, alternation, failure/last-action rejection, item actions, interrupted selections/callers and disobedient alternatives, with optional Detect/doubles/gimmick controls. |
| `counter.asm` doubles the last matching current-turn strike and saturates unsigned 16-bit damage; the Parental Bond reentry does not double again. | `DoFixedDamageMoveCalc` and incoming-damage history | `polished_retaliation.c` plus `polished_action_boundaries.c`: category/turn/immunity/zero rejection, multi-hit and Parental Bond last-strike history, and direct production-calculator boundaries 32767/32768/65534. The calculator test makes saturation observable independently of HP-bar clipping. |
| Metronome/Sleep Talk scripts have `doturn` without `hastarget`; called `ResetTurn` preserves `wCharging=1`. Direct attacks pay Pressure through `hastarget`; called scripts cannot spend PP again. | `CancelerCallSubmove`, `CancelerPPDeduction`, `GetRecordedMove`, selection/replacement cleanup | `polished_action_boundaries.c`: both-side caller PP against Pressure; direct Fly/Dig/Solar Beam with/without Herb, sun/Umbrella, continuation and cancellation; called history cleanup and initial charge after the opponent faints. |
| `failuretext` ends failed Rage before its effect command, so failure cannot start Rage or erase an existing repeat. Selection clears Rage for other moves/items. | `SetOrClearRageVolatile`, `SetActionsAndBattlersTurnOrder` | `polished_move_history.c`: first failure versus failed repeat, miss/Protect, a faster following attack and item selection. |
| `heal` rejects full HP, existing sleep, Insomnia/Vital Spirit and Leaf Guard after the opponent Umbrella; successful Rest heals before `PostStatus`. | `Cmd_trysetrest`, source status and recovery hooks | `polished_action_boundaries.c`: both-side rejection, source Leaf Guard/Umbrella/suppression and status-berry timing, independent native generation presets. Existing healing/weather arithmetic catalogs remain relevant. |
| `roost.asm` changes the current types, normalizes pure Flying and returns at full HP; `transform.asm` copies the current stats-through-struct-end including those types. | `GetBattlerTypes`, `Cmd_transformdataexecution` | `polished_suite2_copy.c`: successful and rejected Roost, Transform copying the temporary type, persistence after the original restores its Flying type; native raw-type control. |
| Shed Skin compares a byte against `1 + (30 percent)`, where `percent` scales by 255. Weather healing checks the holder Umbrella and HP without a Fly/Dig exclusion; Moody selects only five stats. | `AbilityBattleEffects`, source end-turn phase | `polished_endturn_abilities.c`: exhaustive tagged 77/256 outcomes, native 1/3 control, both sides/statuses, Gas, Ice Body Fly/Dig, weather/Umbrella, flooring/full HP, Hydration and five-stat Moody. |
| `CheckNullificationAbilities` excludes ordinary status moves except its explicit exceptions. `CheckTypeMatchup` returns on immunity first. The source Mold Breaker table excludes Bulletproof; Weak Armor raises Speed by two. | `CanAbilityAbsorbMove`, `IsAbilitySuppressedByMoldBreaker`, Weak Armor handler | `polished_absorption.c`: Sap Sipper powders/Seed, electric status, Ground/Ring Target, legacy Lightning Rod, Bulletproof versus Mold Breaker/Gas, Weak Armor and native doubles. All 50 source suppression-table entries were compared with target metadata; this is not all-ability branch closure. |
| `applydamage` checks Endure, enabled affection, Focus Band/Sash, then Sturdy. Band uses item byte 30 and cannot fall back to Sturdy; Sash consumes first. `falseswipe` precedes `applydamage`. | `GetPolishedSurvivalDamage`, `GetAdjustedDamage` | `polished_survival_items.c` and `polished_affection.c`: exhaustive source outcomes and strict cutoffs, item/ability precedence, full/injured HP, Endure/False Swipe, old presets/Mold Breaker and native doubles. Native untagged RNG is not used as evidence for the source distribution. |
| `ForceDeferredSwitch` checks life/reserves before consuming items, then Pursuit before departure abilities. Red Card uses reserve selection in wild battles too. | `MoveEndPolishedDeferredSwitch`, `Cmd_forcerandomswitch`, Pursuit switch hooks | `polished_deferred_switch.c`, `polished_suite2_switching.c`, `polished_switch_closure.c`: first reservation, no/fainted reserves, wild Card, actual replacement identity, pending-action PP cancellation, Pursuit KO versus surviving Regenerator departure. |
| `GetUserSwitchTarget.random_select` excludes the current party slot and fainted members; eligible reserves are uniform. | Eligible-member array and `RNG_FORCE_RANDOM_SWITCH` in `Cmd_forcerandomswitch` | `polished_switch_closure.c`: trainer and wild draws, current slot 0/1, interleaved fainted slot, each of two living reserves at exactly one half, both trainer attacker sides, native wild termination controls. Exact shared RNG state/call counts are not asserted. |
| Only exact `DEFERRED | TARGET | FORCED` flags select `SpikesDamage_CheckMoldBreaker`; item-forced entries use ordinary `SpikesDamage`. | `UpdateSentMonFlags`, source deferred switch and entry-status hooks | `polished_entry_status.c` and `polished_switch_closure.c`: Roar suppression controls, Card trainer/wild Toxic Spikes Synchronize before Pecha cure, Card/Button entry with Levitate. |
| `FaintUserPokemon` clears major status, both Wrap counters and attraction, but does not clear `CANT_RUN`. Replacement does. | `FaintClearSetData`, `SwitchInClearSetData` | `polished_switch_closure.c`: pending wild Teleport after the trapping user self-KOs, separate replacement release, and final-party termination. Existing lifecycle tests cover Baton Pass and hazard replacement loops. |

`build_action_resolution.py` authenticates these source anchors and emits the
contracts alongside the entry-ability data. Its mutation tests reject removal
of the relevant source conditions. Passing those guards proves fixture drift
is detected; it does not replace live behavior tests.

## Consolidated branch additions

| Reviewed source decision | Runtime binding and distinguishing evidence |
| --- | --- |
| Direct damage runs immediate healing before critical text. Ordinary holders run status/confusion and HP/stat berries before Enigma; Berserk holders run Enigma and single-hit Berserk before ordinary healing. Enigma has no source alive or pending-loss rejection. | Resumable source damage phases, isolated Enigma eligibility and hit-stat fixtures distinguish Liechi before Anger Point, maximum-stage Mirror Herb, lethal recovery and faint dependencies. The canonical full source/native execution passed for this frozen candidate. |
| Active Future Sight users retain effective Parental Bond. The second hit repeats the impact script from `checkhit`, including critical text; external original users have no ability. Intermediate healing can cancel deferred Berserk. | Source-only Future Sight impact reentry and the existing hit-stat matrix assert both damage events, intermediate Sitrus, the second critical checkpoint and the final stat result. Native reentry remains unchanged. |
| Target-stat scripts roll accuracy before priority/nullification and stat guards. Magic Bounce retains the original miss or type-immunity result through reflected reentry. Guard Spec still blocks Infiltrator; defensive stat abilities precede Clear Amulet; Illuminate is unconditional in source rules. | Early source accuracy state, source reflection and stat helpers; primary-stat and corrected Magic Bounce fixtures distinguish actual Blunder Policy consumption, early doll exceptions, guard precedence, suppression and native controls. The canonical full source/native execution passed for this frozen candidate. |
| `checkpriority` preserves an earlier miss, including eligible damaging attacks. Sing's earlier `checkpowder` Soundproof check instead overwrites that miss. Manual effect-command miss text does not run the Blunder Policy helper; Pain Split ignores its ordinary miss after priority checks. | Source failure routing distinguishes Sing from Growl/Screech, ordinary versus priority-blocked Trick/Pain Split, and Quick Attack/Hyper Voice/Sucker Punch blockers. Called attacks use the actual script move's priority; ordinary unblocked damaging and multihit paths stay uncached. Ion Deluge, Magic Coat and other optional boundaries retain native behavior. |
| Source affection is a selectable mechanic, despite being off by default. True party friendship gives levels at 180/220/255; critical odds double at the maximum level; evasion is an independent 1/10 after perfect-accuracy bypasses; level 2 self-cure is 1/5 before Future Sight; endurance is `(level + 1)/20` before held survival items. | `GetPolishedAffectionLevel`, critical/accuracy/survival adapters and `HandleEndTurnAffection`; `polished_affection.c` checks both parties, Transform, the original Future Sight party slot, exact tagged distributions, cutoffs, phase order and native/doubles/gimmick controls. |
| Selectable classic physical/special categories use the original listed move type before ability conversion; optional moves retain their native category. The traded-as-OT option returns before source outsider obedience checks. | `GetBattleMoveCategory`, `GetPolishedObedience`; `polished_category_option.c` checks actual damage and Counter/Mirror Coat, original-type Galvanize and unmapped Weather Ball/Aqua Step; `polished_obedience.c` checks the option, either party and native/gimmick boundaries. All three new option flags default to off. |
| Confusion, disobedient self-hit and Hi Jump Kick crash use the source item-skipping HP path. A later real HP change or explicit Unnerve/healing sweep releases that deferral. Source crash is half maximum HP and is not prevented by Magic Guard. | The source self-hit HP-write masks, cancellation/crash scripts and item eligibility; `polished_self_hit_items.c` distinguishes an immediate fatal follow-up, residual damage, later healing, switching, miss/Protect/immunity and native/gimmick behavior. |
| Leppa adds ten to the packed PP byte before comparing with the old maximum. Unsigned wrap and loss of selected-slot PP-Up bits are observable source behavior. Active battle and party PP have distinct Transform/caller boundaries. | `ItemRestorePp` and the atomic PP battle request; `polished_leppa.c` checks packed 53/54/60 boundaries, zero/nonfull selection, other slot preservation, Cud Chew, Transform, Pressure and optional-move controls. |
| Primary status helpers have source-specific Substitute/Infiltrator, Electric paralysis and guard ordering. Six source all-status medicines clear confusion but not attraction; cures reject fainted recipients. | Source status helper and medicine eligibility/application adapters; `polished_status_gates.c` and `polished_medicine_closure.c` assert active and benched party state, matching/nonmatching status, bag use, PP-Up/Transform restoration and optional items/gimmicks. Manual Persim is consumed by the source fallthrough; it is not reusable. |
| Berserk records crossing half HP before normal berry recovery, then cancels if recovery returns above half. Source Sheer Force suppresses it. Justified/Rattled/Weak Armor use the source current-doll boundary. | Pending Berserk state and hit ability dispatch; `polished_hit_stat_abilities.c` checks both sides, berry and Enigma timing, suppression, multihit/Parental Bond and intact/broken dolls. Anger Point's source critical-text phase is reconciled with thaw and Rage separately. |
| Trace retries only when no permitted ability was copied; source entry, departure and Cud Chew each have their own suppression/history rules. | Entry dispatcher, Natural Cure/Regenerator departure, separate Cud history; `polished_entry_closure.c`, `polished_departure_abilities.c`, `polished_cud_chew.c` and the explicitly source-run generic Kee reconsumption fixture. The Persim retry fixture distinguishes renewed confusion from expiration without a second effect. |
| Stat/critical berries floor their HP threshold at one. Starf selects among raw uncapped stats before Contrary. Weakness Policy requires an actual stage change; reactive category berries use the source doll and final-hit boundaries. | Source held-effect eligibility and change handlers; `polished_item_branch_closure.c` checks Gluttony, zero/one/all candidate masks, Contrary, actual selected-stat distribution, hit category, dolls, Sheer Force, multihit and Parental Bond. |
| Weakness Policy belongs to `supereffectivetext`. The 162 ordinary mapped move identities invoke it before post-faint/post-hit effects; seven multi-hit identities invoke it after post-hit effects on the final strike; 86 identities omit the command. A living super-effective zero-damage hit can still activate it. | The authenticated phase table drives two early script hooks and the final-hit-only late handler. Eight `Polished Weakness Policy` groups passed both profiles in v12, distinguishing Justified/Weak Armor and Mirror Herb totals, capped attempts reopened by secondaries, zero-damage False Swipe, Parental Bond, Future Sight, early Explosion with a fainted user, and late contact-user KO. The complete native-script reachability proof is in the move reconciliation. The canonical full catalogs and both production profiles passed; the validation record below identifies the exact frozen candidate. |
| Solar Power/Dry Skin damage has an immediate holder-item checkpoint. Perish Song's Throat Spray decision follows the source miss latch, including Soundproof and already-active timers. | Weather-damage item script and narrow Perish Song eligibility state; `polished_item_branch_closure.c` asserts actual HP/item order, failed sound moves, caps and both profiles. |
| Party identity survives Transform for source gender and Armor Suit eligibility. Brick Break ignores screens during damage, then clears both counters after successful damage, including a target KO. Direct Room Service uses Speed order after Trick Room changes. | Party identity helpers, screen and room commands; `polished_command_identity.c`, `polished_command_closure.c`, Armor Suit matrices and Sketch/Infiltrator cases include both-side roles, rejection, caps, expiration and native controls. |
| Full-party capture and ordinary loss reach terminal restoration; fainted trapper and trapped-victim state lasts until replacement. Accepted Shift completes departure and both entrants' hazards before abilities, using departed Speed for sendout order. | Cleanup/Shift scripts and faint/entry state; `polished_cleanup_closure.c` reaches real terminal callbacks. `test_shift_switch_emulator.py` links each production profile and drives real acceptance, decline, cancellation, absent/fainted reserves, both trap lifetimes, Natural Cure and participants. |

The source authentication guards in `build_action_resolution.py` bind these
interpretations to the pinned files, and mutation tests reject changed source
conditions. These guards prove provenance and drift detection; the battle and
production probes prove the asserted runtime scenarios.

## Validated candidate and publication evidence

[SUITE2_VALIDATION.json](SUITE2_VALIDATION.json) records the exact commands,
results, input hashes and log hashes. The engine/header/data/headless-test diff
against `2f2859623b217fa03b9b8fa916626ed77cfef7b8` remained frozen at SHA-256
`ba8c3fe373110b51a1021aad77df43aacabb21422d5be774f82b3d4492beef95`
through the full catalogs and production validation.

| Acceptance gate | Observed result |
| --- | --- |
| Full native catalog | 6,746 passed; 585 existing TODOs and nine expected failures; 7,340 registered groups, zero unexpected failures. |
| Full source catalog | 1,524 / 1,524 passed. |
| Reviewed shared source fixture | Kee Berry / Cud Chew: 1 / 1 passed. |
| Source AI performance controls | 6 / 6 passed. This measures frame budgets; source AI decisions retain Suite 3 ownership. |
| Authenticated source action guards | 73 / 73 passed, including 144 new Weakness Policy phase/branch mutations. |
| Reproducibility | 108 commands passed: 77 builder checks and 31 source/test commands. |
| ARM header probes | All 12 passed for the final helper layouts and configured profiles. |
| Production builds | Native and source Johto ROMs built with unused/deprecated warnings treated as errors. |
| Production integration | All 17 checks passed, including storage, arithmetic display, Conversion, Metronome, evolution completion/cancellation and both campaign entry points. |
| Shift and wild faint input | Nine compiled cases / 13 actual input branches passed per profile; 26 source/native branches total. |
| Full New Bark campaign | 102 final checks, all 19 warp events, Sprout/Falkner/rewards and Save/reset/Continue passed against the unchanged source production ROM. |

The production input fixtures use ordinary trainer gimmick eligibility and an
obedience-valid party, then read state and supply real button input. They verify
menu readiness and observe faint state before normal battle cleanup erases the
transient buffers. The campaign driver recognizes the actual optional trainer
Shift script and handles the level-13 Bite move selector with exact before/after
moveset assertions. These are test-driver corrections; the engine remained at
the frozen digest above.

The GitHub Actions run associated with the containing implementation commit is
the published-head verification record. It must match that commit; an earlier
green run cannot substitute. Whole-battle inventory flags and the strict
completeness gate remain unchanged. The next planned implementation work is the
original Suite 3 scope described in `BATTLE_PARITY_PLAN.md`.

## Exclusions, deliberate deviation and preserved ownership

Nineteen unused source effect constants have no default-build move input; they
are genuine inventory exclusions. Optional Expansion moves/species, doubles and
gimmicks remain native capabilities, with explicit boundary controls. Source
selectable affection, classic categories, traded obedience and Shift behavior
remain Suite 2 mechanics even when their default setting bypasses them.

The reachable weight-one plus Light Metal source calculation can fail to
terminate. The target deliberately terminates that calculation; this is a
recorded nontermination deviation, not an unreachable/default-build exclusion.
Safe source quirks, including packed Leppa overflow, are reproduced.

Source option UI/Predict disclosure, AI, capture arithmetic, rewards and
postbattle ability dispatch, evolution/learning options, Tower/link contexts,
exact shared RNG stream/call counts and source presentation retain their original
Suite 3 ownership. The postbattle ability caller was positively found in pinned
`engine/overworld/scripting.asm`; it was not dismissed as dead code. No Suite 2
mechanical obligation has been reassigned. Suite 1 foundation status and the
whole-battle strict completeness gate remain unchanged.
