# Whole battle parity suite

Run every registered source unit/battle group and the six AI timing groups:

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

The source archive contains **326 files authenticated back to the pinned Git
commit**. It includes every assembly file under the battle engine/data,
trainer/facility data, item/Pokemon engines, shared home routines, constants and
macros. Missing or edited blobs/trees fail authentication. The generated ledger
records **3,927 global lexical blocks**, their exact source hashes, local-label
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

## Foundation corrections in this suite

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
self-confusion damage and complete damage/action/event ordering remain open.

Regenerate the ledger after adding tests:

```sh
python3 tools/polished_port/scripts/build_battle_foundations.py
python3 tools/polished_port/scripts/build_battle_suite.py
python3 tools/polished_port/scripts/test_battle_suite.py
```

To reproduce the authenticated archive, supply a checkout containing the exact
pinned commit to `build_battle_suite.py --import-source /path/to/source`.
Import reads immutable Git objects, not checkout files. CI needs no source
network fetch. The negative tests reject deleted/corrupt source objects and
false completion claims even when regression execution is marked passed.
