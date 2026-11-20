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

The source archive contains **331 files authenticated back to the pinned Git
commit**. It includes every assembly file under the battle engine/data,
trainer/facility data, item/Pokemon engines, shared home routines, constants and
macros. Missing or edited blobs/trees fail authentication. The generated ledger
records **4,062 global lexical blocks**, their exact source hashes, local-label
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
python3 tools/polished_port/scripts/build_battle_suite.py
python3 tools/polished_port/scripts/test_battle_suite.py
```

To reproduce the authenticated archive, supply a checkout containing the exact
pinned commit to `build_battle_suite.py --import-source /path/to/source`.
Import reads immutable Git objects, not checkout files. CI needs no source
network fetch. The negative tests reject deleted/corrupt source objects and
false completion claims even when regression execution is marked passed.
