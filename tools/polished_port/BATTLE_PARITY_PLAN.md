# Three suites to finish battle parity

Source: Polished Crystal v3.2.3, default non-FAITHFUL build at
`3fa43192379df5c3e7b09a08e4d5d79af4f02f42`.
Starting target: `7e88f8f9f2b68240951c97b2a222429283b7a302`.

The three implementation suites below contain all remaining work. Suite 1
establishes the shared lifecycle runtime; Suite 2 completes action-specific
behavior on top of it; Suite 3 closes context and cross-system obligations.
Implementation and verified complete behavioral parity are separate statuses.
The existing strict completeness gate remains unchanged.

| Suite | Primary completion domains | Deliverable |
| --- | --- | --- |
| 1. End-turn, entry and replacement foundations | field | Source residual/timer phases and Speed order; faint/EXP/outcome checkpoints; deferred replacement and hazard KO loops; shared entry order, Baton Pass state and entry status corrections. |
| 2. Action and attack resolution | actions, damage, moves, status, abilities, items, switching, fainting | Every branch of 133 source effect families and 255 moves; complete voluntary/forced/pivot/Pursuit switching; recoil/contact/Destiny Bond/Aftermath chains; interactions with Suite 1 phases. |
| 3. Battle contexts and integration | setup, capture_escape, rewards, ai, facility_link, presentation_rng | Source AI and construction; capture/rewards/persistence; Tower/link; RNG and observable sequence; final production/campaign/save validation and closure of every outstanding obligation. |

## Suite 1 — shared lifecycle runtime

The initial broad lifecycle draft is refined before publication: complete
move-triggered switching/faint chains belong with the move-resolution pipeline
in Suite 2. No work is dropped. Entry and replacement corrections implemented
here are foundational evidence for those later switching/fainting closures.

- Authenticate the source residual schedule and checkpoint placement. Finish
  both battlers in each phase before faints, EXP and whole-party outcomes;
  retain first-faint order and defer replacements until the source boundary.
- Re-evaluate Speed per phase, including weather/status changes and Trick Room
  expiry. Expire screens by effect, then battler. Keep final Leppa/healing
  sweeps after ability/orb phases while retaining immediate damage/status
  healing and Unnerve-exit sweeps.
- Correct Toxic/Magic Guard/Poison Heal, burn/Heatproof, Black Sludge and
  Leech Seed/Big Root/Liquid Ooze arithmetic. Retain the earlier binding,
  Encore/Disable, Perish Song and weather adapters and their regressions.
- Apply both entrants' hazards/items before entry abilities; Spikes before
  Toxic Spikes; passive Gas before hazards; Speed-ordered entry abilities.
  Preserve source Baton Pass binding/Toxic counters and correctly translate
  the source trap-user bit to Emerald's trapped-target representation.
- Run ordinary and item-forced entry's Toxic Spikes Synchronize before the
  entrant's berry; Roar-style forced-target entry skips reflection. Preserve opposing Substitute and immunity
  checks. Repeat replacements after hazard KOs without repeating residuals.
- Run source/native catalogs, AI budgets, source authentication, both ROM
  builds, production probes and the playable campaign with Save/Continue.

Suite 1 implementation status: implemented and regression-validated; domain closure pending. These are scoped runtime
changes, not certification of all branches in any whole battle domain.

## Suite 2 — actions, attacks and their lifecycle interactions

1. Account for all 129 commands, 133 effects and 255 move identities with
   source interpretation, live bindings and distinguishing branch evidence.
   Include called/copied/transformed moves, multihit/multi-turn state, PP and
   history, protection/substitutes, cancellation and failure paths.
2. Finish arithmetic, stat/critical/STAB/type/weather/screen/accuracy order,
   rounding/saturation and probabilistic decisions. Complete status/volatile
   application, reflection, cure, duration and persistence; every ability/item
   trigger, suppression, recovery, removal and consumption branch.
3. Complete voluntary/forced/pivot/Pursuit behavior, traps, outgoing cleanup,
   state transfer and replacement selection. Finish action-triggered faint
   order, drain/recoil/contact, Destiny Bond/Aftermath, simultaneous outcomes
   and delayed effects. Verify interactions with all Suite 1 boundaries.
4. Retain controls for native optional species, moves, formats and gimmicks.
   Any residual/entry mismatch discovered in these interactions remains an
   explicit dependent obligation and must be fixed before final closure.

Suite 2 implementation status: **closed with the documented source nontermination
deviation**. `SUITE2_CLOSURE_REVIEW.md` records the branch reconciliation and
`SUITE2_VALIDATION.json` records the frozen candidate, complete catalogs and
production acceptance. The GitHub Actions run for the containing commit records
published-head verification. Shared action resolution, deferred switching,
Intimidate entry-item chains and immediate ability stat-item checkpoints are
implemented. The current implementation
covers per-hit recoil/faint/contact order, action-level faint/EXP/outcome
checkpoints, source Destiny Bond lifecycle, Aftermath/Liquid Ooze eligibility,
Substitute boundaries, and first-wins reservations shared by U-turn, Baton
Pass, trainer Teleport/Roar and switch items. Pack checks current negative
stages in source Speed order; switch execution follows the action checkpoint.

The resumed closure work also covers item-backup and Choice history, source
obedience/cancellation, entry-information abilities and X items, forgetting,
rampage/Explosion and in-battle evolution boundaries. Subsequent reviewed
branches correct Metronome/Rage and Protect chains, caller PP and charging,
Counter saturation, Rest/Roost/Transform, end-turn and absorption abilities,
survival items, wild Red Card, item-entry hazards and fainted Mean Look lifetime.
The closure review records source decisions, runtime functions, distinguishing
evidence and final execution. It retains the reachable Light Metal nontermination
exception explicitly; safe source quirks, including packed Leppa wrap, remain reproduced.

Intimidate's private item passes can complete nested replacement chains while
preserving the surrounding action. Download, Tangling Hair, StatUpAbility,
Weak Armor, Moody and Anger Point now have immediate Mirror Herb checkpoints.
Nested stat commands preserve their continuation. Mirror Herb requires both
battlers alive and pending copies clear on switching; generic entry/end-turn
White Herb and end-turn Pack sweeps no longer consume source items early.
An independent Mirror ledger now records actual increments before held-item
and recipient-cap checks, clears failed/itemless copies, and retains pending
boosts across skipped checks. Each healing sweep copies both sides after its
stat berry. Optional Sticky Web entry, native Opportunist, native profiles and
doubles retain controls.

Berserk Gene now runs in the source pre-action phase: both holders finish in
current Speed/Trick Room order, then Mirror Herb runs before AI/player action
selection. Entry and mid-turn acquisitions wait until that boundary. Failed
stat changes preserve the item; successful changes consume it before Own Tempo
and existing-confusion checks. Safeguard is bypassed and new confusion uses
uniform counters of two or three. Native doubles retain entry activation and
permanent confusion.

Sketch now uses cleared source counter-move history, rejects Substitute unless
the effective user ability is Infiltrator, and preserves the distinction between battle-only and permanent
party moves. Base PP replaces only the selected slot's PP-Up bits; Disable/Encore/Choice
follow the surviving move identity. Fourteen live groups cover execution,
failures, switching, PP and native controls; three seeded production-command
boundary groups cover temporary slots, locks and actual link-mode rejection.
Called-move history is reconciled in the consolidated Suite 2 action and move
ledgers. Link construction and playback retain the Suite 3 integration owner.

The consolidated remaining-action batch adds Transform and Skill Swap state
and ability activation, player-selected Conversion, Cud Chew's separate berry
record, Heal Bell, wild theft/Pickpocket, pending Knock Off/Bug Bite consumption,
and Giovanni1's Armored Mewtwo opening checkpoint. Equal-priority voluntary
switches now use source Speed/Trick Room and turn-order items, completing each
entry sequentially. The former "simultaneous voluntary entry" wording was
incorrect: the shared entry pass applies to initial entry and simultaneous
faint replacements, not two independently chosen switches. Pursuit interception,
KO/replacement, U-turn and Mean Look interactions have dedicated live controls.

Authenticated contracts and live bindings remain in
`manifests/action_resolution.json`. The consolidated review reconciles all 133
effect families, 129 commands and 255 moves with their live bindings and branch
evidence; the final catalogs validate the frozen implementation. The conservative
whole-battle inventory continues to include the original broader contexts and
observable-sequence obligations. Conversion's
Smart AI and link transport, exact RNG calls, recorded/link compatibility, and
source presentation remain Suite 3 obligations. The appended Giovanni1 trainer
binding is available for the battle checkpoint and factory probe; its overworld
story binding and source trainer/item art are not claimed complete.

The current reconciliation also covers all used commands/effect scripts in
`SUITE2_MOVE_RECONCILIATION.md` and all source ability/held-effect identities in
`SUITE2_ABILITY_ITEM_RECONCILIATION.md`. These ledgers identify named behavior
fixtures and the final execution record. Selectable source affection and
type-based physical/special categories are included in Suite 2 mechanics; their
source option menus retain the existing Suite 3 setup owner. Likewise the
accepted Shift/Predict replacement order and wild faint prompt transitions
retain Suite 2 ownership and passed production real-input probes in both profiles.
Neither the default Set style nor a default-disabled option excludes a reachable
mechanical branch from this suite.

## Suite 3 — battle contexts and final closure

A push to `polished-crystal-port` runs all six areas in the existing CI workflow.
The **Suite 3 battle contexts and integration** job checks the source and native
battle catalogs, AI timing, production emulator probes, campaign, and
Save/reset/Continue results from that run. Its summary gives a result and the
reviewed parity status for each area. The `polished-suite-3` artifact identifies
the individual checks and remaining obligations. Manual CI runs can also select
**Require reviewed whole-battle parity** to enforce the existing strict gate.

1. Finish trainer/wild generation and all special context bindings; source AI
   scoring, knowledge, selection, item use, switching and tie decisions.
2. Finish capture/escape/roamers and reward/persistence flows: party/box/Dex
   delivery, EXP/growth, learning/evolution cancellation, friendship event
   dispatch, money/Pay Day, losses and restoration.
3. Finish Tower/link construction, restrictions, outcomes and synchronization.
   Retain native extra facilities/doubles/gimmicks as optional capabilities.
4. Verify shared RNG stream/call counts and observable event/message order.
   Source artwork, animations and timing remain explicit obligations; a
   logic-only test cannot remove them from the existing completeness gate.
5. Re-run all source/native regressions and meaningful production-linked
   probes, campaign and save/reset/Continue checks. Close every outstanding
   domain/block/effect obligation with source evidence before strict
   completeness is allowed to pass.

Johto's Battle Style follows `DefaultOptions` and `Options_BattleStyle` in
Polished Crystal v3.2.3. New games start in Set. The menu cycles through Set,
Switch and Predict. After a trainer's Pokémon faints, Switch names the incoming
party member and Predict asks the same question without revealing its identity.
The prompt uses the trainer's source class for singular or plural wording.
Existing saves keep their Set or Switch choice.
[Source menu](https://github.com/Rangi42/polishedcrystal/blob/3fa43192379df5c3e7b09a08e4d5d79af4f02f42/engine/menus/options_menu.asm#L208-L259)
and [trainer prompt](https://github.com/Rangi42/polishedcrystal/blob/3fa43192379df5c3e7b09a08e4d5d79af4f02f42/engine/battle/core.asm#L2873-L2928).

The production emulator exercises the Options menu in both directions, writes
cartridge Flash, resets and reopens the selected style before battle. It checks
the incoming nickname in Switch, the full Predict message on the initial and
repeated offer, and the immediate send-out in Set. The native profile exercises
Shift and Set. The combined job requires these reports for setup and presentation.

### Current Suite 3 implementation

| Area | Implemented in the GBA port | Remaining closure work |
| --- | --- | --- |
| Setup | Wild, scripted and roaming encounters generate six source DVs. Legendary encounters guarantee three perfect DVs. Roamers preserve their ability slot across Flash reload. Sprout Tower Bellsprout can carry Growth. | The source defines 956 trainer parties when Inver’s separate pool is included. The adapter binds 22. The CI report lists the 934 unbound class and party IDs. Other trainer parties, special encounter types, form-specific gender and the full encounter lifecycle remain open. |
| Capture and escape | Dusk Ball applies the cave bonus in Dark Cave. The source time comparison cannot activate the bonus in outdoor maps. Sprout Tower is a dungeon in the source. Raikou and Entei share the source 16-route graph, use grass encounter odds of 25/256 each when present, and persist separately in Flash. | Burned Tower release, 13 outstanding GBA route bindings, remaining scripted capture outcomes and special battle exits. |
| Rewards | After a win or capture, the party is processed in reverse slot order after held items are restored. Natural Cure, Pickup and Honey Gather use the source conditions. | Special scripted rewards, all experience and loss branches, and their persistence. |
| AI | Ordinary wild move selection filters unavailable moves. Registered trainers with medicine use source priority, health and status thresholds and the party's maximum level. Conversion rerolls invalid slots for wild and Smart trainers. | Source move scoring, switching, opponent knowledge, and item policies for unregistered trainers. |
| Facility and link | Tower battles force Set for that battle. Compatible two-player link singles carry both players' Conversion choices and preserve them in the recording. | Tower party generation and streak progression, all facility rules, and synchronization across the remaining link actions. |
| Presentation and RNG | Pickup and Honey Gather show the party member and awarded item. Seeded production probes check item results and the next draw. | The shared Game Boy RNG stream, complete draw order, message sequence, artwork, animation and timing. |

The combined run requires the production contexts report, the Dusk Ball map test
and recorded and live two-player Conversion alongside the existing campaign and catalogs.
The serial probe connects two emulated GBAs, negotiates the protocol over the cable,
retries an invalid guest choice and checks both battlers' Tackle PP on the following turn.
The contexts probe verifies 127 seeded reward cases, 15 trainer medicine decisions,
96 wild and scripted encounters, all 280 adjacent and random-jump cases with the player on Route 29, and every possible roaming encounter byte. Four Flash reloads cover win, capture, escape and defeat. Capture checks the caught Pokémon in the party and Honey in the bag.

## Completed milestone and next suite

The consolidated Suite 2 pass requested from `8e340802` and resumed from
`2f285962` is complete as an implementation milestone. Its source scope remains
actions, damage, moves, status, abilities, items, switching and fainting. The
complete move/command, ability/item and switching/fainting ledgers provide the
source interpretations, live bindings and distinguishing evidence. None of
these mechanical obligations was reassigned to Suite 3.

`SUITE2_VALIDATION.json` records the complete source/native runs, reviewed shared
fixture, AI performance controls, source guards, reproducibility, ARM probes,
both shipping builds and production/campaign/Save/reset/Continue checks. The
associated Actions run verifies the exact published implementation commit.
Inventory counts and passing tests are supporting evidence for the reconciliation.

Nineteen unused effect constants are true default-build inventory exclusions.
The reachable weight-one/Light Metal nontermination is a deliberate terminating
deviation, retained in the closure record and final parity assessment. Source
selectable mechanics were included despite their default-disabled settings.

The **next planned implementation suite is Suite 3**, with the six original
domains and work listed above. Suite 1 retains its implemented-foundations status;
complete battle parity and the strict completeness gate remain OPEN.

## Ownership and evidence

`manifests/battle_completion_suites.json` assigns all fifteen major domains
exactly once by primary completion owner. Shared foundations can be implemented
in an earlier suite; that does not close their owner's remaining obligations.
New gaps go into one of these three suites, never an unplanned fourth suite.
The authenticated inventory in `manifests/battle_suite.json` remains the full,
conservative audit universe, including unused source variants.

Closure requires a source anchor and interpretation, live target binding,
success/failure/boundary tests and optional-feature controls. Generic native
regressions and source-only fixtures are supporting evidence. Complete battle
parity remains OPEN until Suite 3 closes the remaining ledger.
