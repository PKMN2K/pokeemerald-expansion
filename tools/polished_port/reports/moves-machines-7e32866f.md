# Move and machine audit

Production source: `7e32866f06f02d8328ffb6dc5ede7b6e46021cd2`.
Pinned Polished Crystal: `3fa43192379df5c3e7b09a08e4d5d79af4f02f42`.
Production ROM SHA-256: `cb159439b932cc6cb0cb29e2d8f7e11e074d6a950dca0e4577e0fc7f2f376fd5`.

This probe links unchanged production Johto objects with a separate test boot.
It checks the real accessors, PP calculations, teaching dispatch and compatibility.
It does not play each move, teach every machine, or certify source battle effects,
secondary chances, priorities, animation playback, field uses, inventory ownership,
UI, save persistence or campaign acquisition. Engine regression tests and controller
teaching scenarios are separate checks. A matching table row is not complete parity.

| Check | Result |
| --- | --- |
| Move PP and all four PP Up levels | 255/255 match |
| Non-placeholder effects and script/animation bindings | 255/255 present |
| Move rows with basic-data differences | 22 |
| TM number/move/category identity | 40/75 match |
| HM number/move/category identity | 4/6 match |
| TMs routed through the explicit source lesson adapter | 3/75 |
| Species/form × TM/HM/tutor compatibility | 43792/43792 match |

## Basic move-data differences

These are table differences requiring source-behavior review, not all confirmed
gameplay bugs. Fixed damage (Sonic Boom/Dragon Rage), typeless move sentinels and
accuracy on moves that bypass accuracy may use different representations.
Source power 1 denotes variable/fixed damage and is recorded separately in JSON.

| Move | Field | Source | Target |
| --- | --- | --- | --- |
| CUT | power | 60 | 50 |
| CUT | accuracy | 100 | 95 |
| CUT | type | STEEL | NORMAL |
| FLY | accuracy | 100 | 95 |
| TAKE_DOWN | accuracy | 100 | 85 |
| SING | accuracy | 75 | 55 |
| SUPERSONIC | accuracy | 65 | 55 |
| SONIC_BOOM | power | 20 | 1 |
| COUNTER | accuracy | 0 | 100 |
| STRENGTH | type | FIGHTING | NORMAL |
| DRAGON_RAGE | power | 40 | 1 |
| FIRE_SPIN | power | 40 | 35 |
| FIRE_SPIN | accuracy | 90 | 85 |
| DIG | power | 90 | 80 |
| METRONOME | type | UNKNOWN_T | NORMAL |
| SCALD | power | 70 | 80 |
| DRAGON_DANCE | accuracy | 100 | 0 |
| CRABHAMMER | accuracy | 95 | 90 |
| OCTAZOOKA | power | 75 | 65 |
| OCTAZOOKA | accuracy | 95 | 85 |
| SLEEP_TALK | accuracy | 100 | 0 |
| PAIN_SPLIT | type | GHOST | NORMAL |
| HIDDEN_POWER | power | 70 | 60 |
| HIDDEN_POWER | type | UNKNOWN_T | NORMAL |
| MIRROR_COAT | accuracy | 0 | 100 |
| WHIRLPOOL | power | 40 | 35 |
| WHIRLPOOL | accuracy | 90 | 85 |
| STRUGGLE | type | UNKNOWN_T | NORMAL |

## Machine identity gaps

Stable target item IDs are preserved. Correcting these requires source adapters;
do not renumber saved items. MOVE_NONE means this numbered slot has no move mapping.
Legacy HM05 Flash remains intentionally readable; a source HM05 Whirlpool adapter
must preserve that compatibility. Tutors have compatibility checks, not acquisition/UI tests.

| Machine | Source move | Current target move |
| --- | --- | --- |
| TM01 | MOVE_DYNAMICPUNCH | MOVE_FOCUS_PUNCH |
| TM03 | MOVE_CURSE | MOVE_WATER_PULSE |
| TM09 | MOVE_VENOSHOCK | MOVE_BULLET_SEED |
| TM12 | MOVE_HONE_CLAWS | MOVE_TAUNT |
| TM21 | MOVE_BULLDOZE | MOVE_FRUSTRATION |
| TM34 | MOVE_FLASH_CANNON | MOVE_SHOCK_WAVE |
| TM39 | MOVE_SWIFT | MOVE_ROCK_TOMB |
| TM41 | MOVE_SUBSTITUTE | MOVE_TORMENT |
| TM43 | MOVE_FLAME_CHARGE | MOVE_SECRET_POWER |
| TM48 | MOVE_ROCK_SLIDE | MOVE_SKILL_SWAP |
| TM49 | MOVE_DAZZLING_GLEAM | MOVE_SNATCH |
| TM50 | MOVE_LEECH_LIFE | MOVE_OVERHEAT |
| TM52 | MOVE_FOCUS_BLAST | MOVE_NONE |
| TM53 | MOVE_ENERGY_BALL | MOVE_NONE |
| TM54 | MOVE_FALSE_SWIPE | MOVE_NONE |
| TM55 | MOVE_SCALD | MOVE_NONE |
| TM56 | MOVE_X_SCISSOR | MOVE_NONE |
| TM57 | MOVE_WILD_CHARGE | MOVE_NONE |
| TM58 | MOVE_DARK_PULSE | MOVE_NONE |
| TM59 | MOVE_DRAGON_PULSE | MOVE_NONE |
| TM60 | MOVE_DRAIN_PUNCH | MOVE_NONE |
| TM61 | MOVE_WILL_O_WISP | MOVE_NONE |
| TM62 | MOVE_ACROBATICS | MOVE_NONE |
| TM63 | MOVE_WATER_PULSE | MOVE_NONE |
| TM64 | MOVE_EXPLOSION | MOVE_NONE |
| TM65 | MOVE_SHADOW_CLAW | MOVE_NONE |
| TM66 | MOVE_POISON_JAB | MOVE_NONE |
| TM67 | MOVE_AVALANCHE | MOVE_NONE |
| TM68 | MOVE_GIGA_IMPACT | MOVE_NONE |
| TM69 | MOVE_U_TURN | MOVE_NONE |
| TM71 | MOVE_STONE_EDGE | MOVE_NONE |
| TM72 | MOVE_VOLT_SWITCH | MOVE_NONE |
| TM73 | MOVE_THUNDER_WAVE | MOVE_NONE |
| TM74 | MOVE_GYRO_BALL | MOVE_NONE |
| TM75 | MOVE_SWORDS_DANCE | MOVE_NONE |
| HM05 | MOVE_WHIRLPOOL | MOVE_FLASH |
| HM06 | MOVE_WATERFALL | MOVE_ROCK_SMASH |

## Reproduce

Build the Johto ROM, compile the existing mGBA driver, then run:

```sh
python3 tools/polished_port/scripts/audit_moves_machines.py \
  --driver /tmp/johto-demo-driver --output /tmp/move-machine-audit
```

Exit 1 means observed parity gaps. `--report-only` emits the same findings with
exit 0 for the staged migration CI; it does not label them passed. Runtime/compiler
errors and source fixture failures still fail the job. CI uses `--known-gaps`
to permit only the explicitly recorded differences; new differences fail it.
Detailed rows are in
`moves-machines.json` alongside this report.

## Additional runtime validation at this revision

- The complete native engine regression run passed 5,349 tests, with nine expected
  failures and 586 existing TODO entries (5,944 total). This uses the native test
  profile and selected source-specific overrides; it is not a full source battle profile.
- All fourteen TM31/TM51 and six TM70 production-linked controller scenarios passed
  on the same production ROM hash above. They cover learning into empty/replacement
  slots, reuse, incompatible/already-known/Egg rejection, cancellation, source PP,
  friendship, ordinary forgetting, Save/Continue and original icon pixels/colors;
  TM31/TM51 also test refusal to give the nontradable machine.
- The comprehensive data audit correctly exited 1 in strict mode for the recorded
  gaps, and exited 0 with the explicit known-gap inventory.
- Injected PP and reverse-lookup regressions were rejected by the gate; resolving
  recorded findings was accepted. Python compilation and workflow parsing passed.

The result is **not all moves/TMs verified correct**. Complete the numbered machine
identity/lesson adapters, investigate the move data differences against each effect's
source code, then add effect-specific source battle tests and controller teaching
scenarios for the remaining machines. Preserve existing save IDs during those ports.
