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
- Run ordinary entry's Toxic Spikes Synchronize before the entrant's berry;
  forced entry skips reflection. Preserve opposing Substitute and immunity
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

Suite 2 implementation status: shared action resolution and deferred switching
implemented; full branch closure pending. The current delivery covers per-hit recoil/faint/contact order,
action-level faint/EXP/outcome checkpoints, source Destiny Bond lifecycle,
Aftermath/Liquid Ooze eligibility, Substitute boundaries for contact and switch
items, U-turn reservation precedence and item/Pursuit ordering. Its authenticated
source contracts and live regression scope are in
`manifests/action_resolution.json`. This does not close all 133 effect families
or the entire switching graph. The next delivery unifies U-turn, Baton Pass,
trainer Teleport/Roar and switch items under one first-wins reservation;
checks Pack's current negative stages in source Speed order; and separates
stat-herb effects from switch execution. Seventeen new source/native groups
cover these boundaries in `test/battle/polished_deferred_switch.c`.
Entry-triggered Pack chains, all voluntary/Pursuit/trap combinations, called
move interactions and remaining effect branches still require closure within
Suite 2. These are existing obligations, not an additional suite.

## Suite 3 — battle contexts and final closure

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
