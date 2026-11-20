# Consolidated source attack suite

The [whole battle suite](BATTLE_SUITE.md) now wraps this runner and accounts for
all remaining battle domains. CI uses that entry point and uploads
`polished-battle-suite`; the attack JSON remains included for compatibility.

The pinned default Polished Crystal release has **255 moves, 133 used effect
families and 129 distinct script commands**. The generated
[catalog](manifests/attack_suite.json) includes every source ID, its target ID,
source effect, ordered source script and the new matrix's precise test scopes.
Source ID 249 correctly resolves the default Brick Break/Rock Smash alias.

Run the entire source battle suite and AI frame budgets:

```sh
python3 tools/polished_port/scripts/run_attack_suite.py
```

Include the full native engine regression suite:

```sh
python3 tools/polished_port/scripts/run_attack_suite.py --native-regression
```

The source pass selects **all** `Polished` battle groups, including existing
groups omitted by the previous hand-maintained prefix list. CI also runs the
full native tests separately. Each invocation records the exact test catalog,
target/source commits, manifest hash, commands, counts and logs. A failed
command, timeout, missing summary, skipped source group or source expected
failure fails the run. CI uploads `polished-attack-suite` even on failure.
`--output`, `--jobs`, `--timeout` and `--driver` customize the local run.

The new twelve-group matrix executes **1,680 cases per profile**:

| Matrix | Cases | What it establishes |
| --- | ---: | --- |
| All 255 moves, either side | 510 | First-turn dispatch and PP accounting; prerequisite-dependent moves may fail normally |
| 124 direct attacks, either side | 248 | Actual target HP loss in an unblocked hit |
| Same attacks into Protect | 248 | No target HP/status/stat changes |
| Same attacks into Substitute | 248 | Substitute interception or the source sound-move bypass |
| 21 explicit stat moves | 84 | Source deltas and saturation after one/seven uses, on both sides |
| Recover, Roost, Fresh Snack | 6 | Odd maximum HP rounding |
| Five recoil moves | 50 | Source fractions and exact integer division |
| Six draining moves | 240 | Kiss/Big Root order, with/without Liquid Ooze, on both sides |
| Recoil immunity controls | 20 | Rock Head and Magic Guard |
| Recovery controls | 8 | Full-HP rejection and optional Slack Off rounding |
| Doubles controls | 6 | Existing native arithmetic remains selected |
| Either active gimmick | 12 | Existing native arithmetic remains selected |

Three arithmetic groups failed against the parent runtime and now cover the
corrections: Recover/Roost floor half maximum HP; Double-Edge/Flare Blitz divide
damage by three exactly; Brave Bird uses source quarter-damage recoil; and
Big Root applies before the final halving of drain. Draining Kiss first adds
floor(damage/2), as the source does. These corrections are restricted to their
mapped source moves in ordinary singles without either active gimmick.

## Multi-hit follow-up

Thirty additional groups cover all seven source multi-hit moves with 530
parameter combinations per profile and repeated probability trials. The
[source manifest](manifests/multihit.json) pins selection slots, fixed-strike and
Skill Link/Loaded Dice priority, PP/accuracy boundaries and the per-hit loop.
The runtime now uses the source two/three/four/five-hit distribution for all five
variable-hit moves and allows a mapped attack already in progress to finish
when Effect Spore causes sleep. Initial sleep still prevents the action.

The tests include both sides, native generation presets, initial rejection,
Pressure, target/attacker fainting, Substitute damage, contact statuses,
Cursed Body, powder immunity, Weak Armor damage changes, critical/ordinary
sequences and Metronome. Native mode, optional moves, doubles and either active
gimmick are controls. Full contact/proc eligibility and ordering, source status
probabilities/duration, damage/critical/RNG formulas, deferred switching, AI and
presentation remain separate obligations. See the phase record in
[PARITY_AUDIT.md](PARITY_AUDIT.md) for validation and limits.

## Contact-status follow-up

Thirty-one additional groups exercise 545 parameter combinations per profile,
including live contact classification for all 124 direct attacks. The
[source manifest](manifests/contact_status.json) records all 255 contact
identities and the five affected abilities. The scoped adapter imports Effect
Spore's byte selection, separate activation and sleep counters; defensive
status activation on Substitute-breaking hits; and Poison Touch's source
order, Protective Pads prevention and actual Shield Dust/Covert Cloak branch.

Native/optional moves, doubles and active gimmicks have separate controls.
Remaining global contact, status/Synchronize/curing, fainting, RNG, presentation
and AI obligations stay open in [PARITY_AUDIT.md](PARITY_AUDIT.md).

## Volatile-contact follow-up

Thirty-two additional groups cover 947 cases per profile, including all 124
direct attacks against Cute Charm, Tangling Hair and Perish Body on both sides.
The [source manifest](manifests/contact_volatile.json) records the shared
Substitute boundary, Cute Charm's three-in-ten chance, Tangling Hair's living-
holder stat gate and Perish Body's activation after holder knockout. Countdown
asymmetry, switching, gender/protection gates, stat reactions, multi-hit and
Poison Touch order have dedicated checks, alongside native/optional controls.
Global contact/faint/curing order, exact RNG, presentation and AI remain open.

## What a passing run does not establish

This is a consolidated regression and coverage suite, **not a declaration that
all remaining attack behavior has been ported**. Dispatch-only coverage is
explicitly labeled. The earlier focused suites retain their documented limits
in [PARITY_AUDIT.md](PARITY_AUDIT.md). No whole effect family is yet certified
across every shared mechanic and interaction.

The catalog keeps the remaining obligations visible: full damage/stat and RNG
rules, every effect's prerequisites and failure branches, duration/cancellation
and turn order, immunity/ability/item interactions, broken Substitute and
fainting order, switching/called moves, multi-hit/multi-turn effects, weather,
end-turn effects, presentation and AI. The arithmetic checks in this suite do
not establish source damage inputs, KO/Substitute caps or effect ordering;
Dream Eater and full-HP Liquid Ooze gates remain separate audits.

For a completion gate that must reject those gaps:

```sh
python3 tools/polished_port/scripts/run_attack_suite.py --check --strict-completeness
```

That command currently returns **2**, deliberately. Ordinary regression mode
can pass without turning unverified behavior into a parity claim. Fixture or
execution errors still fail separately. To check reproducibility without
running the emulator, use `--check` alone.
