#!/usr/bin/env python3
"""Audit every pinned move/machine against production-linked Johto accessors.

This is a data/dispatch/compatibility audit, not a battle-effect or UI replay.
Default exit status is nonzero for parity gaps; --report-only records unfinished
port work without making a claim of parity. No shipping game code is changed.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import tempfile

import build_pokemon_learnsets as source
import build_pokemon_learning as learning
import build_move_pp as pp
import build_pain_split as pain_split
import build_fixed_damage as fixed_damage
import build_retaliation_accuracy as retaliation
import build_self_move_accuracy as self_accuracy
import build_typeless_moves as typeless
import build_metronome_struggle as metronome_struggle
import build_secondary_stat_drops as stat_drops
import build_sleep_talk_selection as sleep_talk
import build_move_restrictions as restrictions
import build_encore_commitment as encore_commitment
import build_cursed_body as cursed_body
from test_johto_demo_emulator import Game

ROOT = source.ROOT


def inputs():
    files = source.sources()  # Verifies every fixture against its pinned blob SHA.
    identities = source.move_identities(files)[1:]
    rows = [[v.strip() for v in line[5:].split(',')] for line in
            source.default_lines(files['data/moves/moves.asm'].split('Moves::', 1)[1])
            if line.startswith('move ')]
    assert len(rows) == len(identities) == 255
    moves = []
    for identity, row in zip(identities, rows, strict=True):
        name, effect, power, type_, accuracy, base_pp, chance, category = row
        assert name == identity['source_move']
        moves.append(dict(**identity, source_effect=effect, power=int(power),
                          type=type_, accuracy=0 if accuracy == '-1' else int(accuracy),
                          pp=int(base_pp), chance=int(chance), category=category))
    outputs = learning.build()
    for path, expected in {**outputs, **pp.build(), **pain_split.build(), **fixed_damage.build(), **retaliation.build(), **self_accuracy.build(), **typeless.build(), **metronome_struggle.build(), **stat_drops.build(), **sleep_talk.build(), **restrictions.build(), **encore_commitment.build(), **cursed_body.build()}.items():
        assert path.read_text() == expected, f'Stale imported source data: {path}'
    catalog = json.loads(outputs[learning.MANIFEST])
    assert [r['kind'] for r in catalog['teaching_catalog']] == ['TM']*75 + ['HM']*6 + ['MT']*31
    return moves, catalog['teaching_catalog'], catalog['records']


def probe_code(moves, catalog, species):
    # Expected type/category constants are compiled with the target headers;
    # source and target numeric encodings are never assumed equal.
    definitions = [
        '#include "global.h"', '#include "main.h"', '#include "pokemon.h"',
        '#include "item.h"', '#include "item_use.h"', '#include "move.h"',
        '#include "polished_tm.h"', '#include "battle.h"', '#include "battle_stat_change.h"',
        'struct Expected { u16 move, type, category; };',
        'static const struct Expected expected[] = {',
    ]
    definitions += [f"{{{m['target_move']}, TYPE_{'MYSTERY' if m['type'] == 'UNKNOWN_T' else m['type']}, DAMAGE_CATEGORY_{m['category']}}}," for m in moves]
    definitions += ['};', 'static const u16 teaching[] = {']
    definitions += [m['target_move'] + ',' for m in catalog]
    definitions += ['};', 'static const u16 machines[] = {']
    definitions += [f"ITEM_{m['kind']}{m['number']:02d}," for m in catalog if m['kind'] != 'MT']
    definitions += ['};', 'static const u16 species[] = {']
    definitions += [s['target_species'] + ',' for s in species]
    definitions += ['};', r'''
// Only one row at a time is resident; this does not change the save layout or
// require room for the entire compatibility matrix in the shipping EWRAM.
EWRAM_DATA volatile u32 gAuditResult[128] = {0};
EWRAM_DATA volatile u32 gAuditCursor = 0;
EWRAM_DATA volatile u32 gAuditKind = 0;
EWRAM_DATA volatile u32 gAuditReady = 0;
void CB2_MoveMachineAudit(void)
{
    if (gAuditReady)
    {
        if (!(gMain.newKeys & A_BUTTON))
            return;
        gAuditCursor++;
        u32 limit = gAuditKind == 0 ? ARRAY_COUNT(expected) :
                    gAuditKind == 1 ? ARRAY_COUNT(machines) : ARRAY_COUNT(species);
        if (gAuditCursor == limit)
        {
            gAuditCursor = 0;
            gAuditKind++;
        }
        gAuditReady = 0;
        return;
    }
    if (gAuditKind == 0 && gAuditCursor < ARRAY_COUNT(expected))
    {
        u32 i = gAuditCursor;
        enum Move m = expected[i].move;
        if ((u32)m >= ARRAY_COUNT(gMovesInfo))
            m = MOVE_NONE;
        gAuditResult[0] = m;
        gAuditResult[1] = GetMovePower(m);
        gAuditResult[2] = GetMoveAccuracy(m);
        gAuditResult[3] = GetMovePP(m);
        gAuditResult[4] = GetMoveType(m) == expected[i].type;
        gAuditResult[5] = GetMoveCategory(m) == expected[i].category;
        gAuditResult[6] = GetMoveEffect(m) == EFFECT_PLACEHOLDER;
        gAuditResult[7] = gMovesInfo[m].battleAnimScript != NULL;
        gAuditResult[8] = gBattleMoveEffects[GetMoveEffect(m)].battleScript != NULL;
        for (u32 n = 0; n < 4; n++)
            gAuditResult[9+n] = CalculatePPWithBonus(m, n << 6, 3);
        gAuditResult[13] = GetMoveType(m);
        gAuditResult[14] = MoveIgnoresSubstitute(m);
        gAuditResult[15] = MoveIgnoresProtect(m);
        gAuditResult[16] = GetMoveEffect(m) == EFFECT_FIXED_HP_DAMAGE;
        gAuditResult[17] = gAuditResult[16] ? GetMoveFixedHPDamage(m) : 0;
        gAuditResult[18] = GetMoveDisplayPower(m);
        gAuditResult[30] = !IsMoveSleepTalkBanned(m) && !gBattleMoveEffects[GetMoveEffect(m)].twoTurnEffect;
        for (u32 n = 19; n < 30; n++)
            gAuditResult[n] = 0;
        gAuditResult[19] = GetMoveAdditionalEffectCount(m);
        if (gAuditResult[19])
        {
            const struct AdditionalEffect *effect = GetMoveAdditionalEffectById(m, 0);
            gAuditResult[20] = effect->moveEffect == MOVE_EFFECT_STAT_MINUS;
            gAuditResult[21] = effect->chance;
            gAuditResult[22] = effect->self | effect->preAttackEffect << 1 | effect->onChargeTurnOnly << 2
                | effect->onSide << 3 | effect->onlyIfTargetRaisedStats << 4 | effect->pledgeCombo << 5
                | effect->sheerForceOverride << 6;
            for (u32 stat = STAT_ATK; stat <= STAT_EVASION; stat++)
                gAuditResult[23 + stat - STAT_ATK] = GetStatStage(stat, effect);
        }
    }
    else if (gAuditKind == 1 && gAuditCursor < ARRAY_COUNT(machines))
    {
        enum Item item = machines[gAuditCursor];
        enum Move m = GetItemTMHMMoveId(item);
        gAuditResult[0] = item;
        gAuditResult[1] = m;
        gAuditResult[2] = GetMachineDisplayNumber(item);
        gAuditResult[3] = IsItemHM(item);
        gAuditResult[4] = GetItemFieldFunc(item) == ItemUseOutOfBattle_TMHM;
        gAuditResult[5] = GetItemPocket(item) == POCKET_TM_HM;
        gAuditResult[6] = GetItemImportance(item);
        gAuditResult[7] = GetItemPrice(item);
        gAuditResult[8] = IsPolishedTM(item);
        gAuditResult[9] = GetTMHMItemIdFromMoveId(teaching[gAuditCursor]);
    }
    else if (gAuditKind == 2 && gAuditCursor < ARRAY_COUNT(species))
    {
        for (u32 n = 0; n < ARRAY_COUNT(teaching); n++)
            gAuditResult[n] = CanLearnTeachableMove(species[gAuditCursor], teaching[n]);
    }
    gAuditReady = 1;
}
''']
    return '\n'.join(definitions)


def compile_probe(build_root, directory, code):
    c = directory / 'probe.c'
    obj = directory / 'probe.o'
    elf = directory / 'probe.elf'
    linker = directory / 'probe.ld'
    c.write_text(code)
    subprocess.run(['arm-none-eabi-gcc', '-c', '-Wa,-meabi=5', '-mthumb',
                    '-mthumb-interwork', '-O2', '-mabi=apcs-gnu', '-march=armv4t',
                    '-std=gnu17', '-DMODERN=1', '-DTESTING=0', '-DEMERALD',
                    '-DPOLISHED_JOHTO_DEMO=1', '-Wall', '-Werror',
                    '-iquote', 'include', '-iquote', '.', str(c), '-o', str(obj)],
                   cwd=build_root, check=True)
    linker.write_text((build_root/'ld_script_modern.ld').read_text().replace(
        'gInitialMainCB2 = CB2_InitCopyrightScreenAfterBootup;',
        'gInitialMainCB2 = CB2_MoveMachineAudit;'))
    objects = [line[5:] for line in (build_root/'pokeemerald-johto-demo.map').read_text().splitlines()
               if line.startswith('LOAD ') and line != 'LOAD linker stubs']
    assert objects, 'Build the production Johto ROM first'
    subprocess.run(['arm-none-eabi-ld', '--gc-sections', '-T', str(linker),
                    '-o', str(elf), str(obj), *objects],
                   cwd=build_root/'build/emerald-johto-demo', check=True)
    subprocess.run([str(build_root/'tools/gbafix/gbafix'), str(elf), '-tPOKEMON EMER',
                    '-cBPEE', '-m01', '-r0', '-d0', '--silent'], check=True)
    return elf


def sample(game, kind, cursor, size):
    # A acknowledges a completed probe row. No gameplay RAM/code is patched.
    for _ in range(120):
        game.frames(1)
        if game.u32(game.sym['gAuditReady']) == 1:
            assert game.u32(game.sym['gAuditKind']) == kind
            assert game.u32(game.sym['gAuditCursor']) == cursor
            result = list(struct.unpack('<' + 'I'*size, game.read(game.sym['gAuditResult'], size*4)))
            game.frames(1, 1)
            game.frames(1, 0)
            return result
    raise AssertionError(f'Production-linked API probe stalled: {kind}/{cursor}')


def fixed_damage_differences(source_power, actual):
    """Accept only the explicit fixed-HP adapter, never an ordinary power row."""
    differences = {}
    expected = dict(power=1, fixed_hp_effect=True, fixed_hp_payload=source_power)
    observed = dict(power=actual[1], fixed_hp_effect=bool(actual[16]), fixed_hp_payload=actual[17])
    if observed != expected:
        differences['fixed_damage_adapter'] = dict(source=expected, target=observed)
    if actual[18] != source_power:
        differences['display_power'] = dict(source=source_power, target=actual[18])
    return differences


def run(game, moves, catalog, species):
    pain_split_contract = json.loads((ROOT/'tools/polished_port/manifests/pain_split.json').read_text())
    secondary_contract = {r['target_move']: r for r in stat_drops.records()}
    sleep_contract = {r['target_move']: r['eligible'] for r in json.loads(
        (ROOT/'tools/polished_port/manifests/sleep_talk_selection.json').read_text())['candidates']}
    stats = ['STAT_ATK', 'STAT_DEF', 'STAT_SPEED', 'STAT_SPATK', 'STAT_SPDEF', 'STAT_ACC', 'STAT_EVASION']
    move_results = []
    ids = {}
    native_names = {int(value): name for name, value in re.findall(
        r'\b(MOVE_\w+)\s*=\s*(\d+)\s*,', (ROOT/'include/constants/moves.h').read_text())}
    type_names = {int(value): name for name, value in re.findall(
        r'\bTYPE_(\w+)\s*=\s*(\d+)\s*,', (ROOT/'include/constants/pokemon.h').read_text())}
    for i, m in enumerate(moves):
        actual = sample(game, 0, i, 31)
        assert actual[0] > 0 and actual[0] not in ids.values(), 'Invalid or duplicate move ID'
        ids[m['target_move']] = actual[0]
        differences = {}
        fixed = m['source_effect'] == 'EFFECT_STATIC_DAMAGE'
        if fixed:
            differences.update(fixed_damage_differences(m['power'], actual))
        for field, index in [('power', 1), ('accuracy', 2), ('pp', 3)]:
            # Source power 1 is a sentinel for fixed/variable damage; target
            # storage may use 0 instead. Record that separately, not as a bug.
            if field == 'power' and (m[field] == 1 or fixed):
                continue
            if m[field] != actual[index]:
                differences[field] = dict(source=m[field], target=actual[index])
        for field, index in [('type', 4), ('category', 5)]:
            if not actual[index]:
                differences[field] = dict(source=m[field], target=type_names.get(actual[13], actual[13]) if field == 'type' else 'different')
        maximum = [m['pp'] + min(m['pp']//5, 7)*n for n in range(4)]
        if actual[9:13] != maximum:
            differences['pp_up'] = dict(source=maximum, target=actual[9:13])
        if actual[6] or not actual[7] or not actual[8]:
            differences['dispatch'] = dict(placeholder=bool(actual[6]), animation=bool(actual[7]), battle_script=bool(actual[8]))
        if m['target_move'] == 'MOVE_PAIN_SPLIT':
            for field, key, index in [('ignores_substitute', 'bypasses_substitute', 14),
                                      ('ignores_protect', 'bypasses_protect', 15)]:
                if bool(actual[index]) != pain_split_contract[key]:
                    differences[field] = dict(source=pain_split_contract[key], target=bool(actual[index]))
        secondary = None
        if m['target_move'] in sleep_contract and bool(actual[30]) != sleep_contract[m['target_move']]:
            differences['sleep_talk_eligible'] = dict(source=sleep_contract[m['target_move']], target=bool(actual[30]))
        if m['target_move'] in secondary_contract:
            contract = secondary_contract[m['target_move']]
            expected = dict(count=1, stat_minus=True, chance=contract['chance'], flags=0,
                            stages={stat: int(stat == contract['target_stat']) for stat in stats})
            secondary = dict(count=actual[19], stat_minus=bool(actual[20]), chance=actual[21], flags=actual[22],
                             stages=dict(zip(stats, actual[23:30], strict=True)))
            if secondary != expected:
                differences['secondary_stat_drop'] = dict(source=expected, target=secondary)
        move_results.append(dict(source_move=m['source_move'], target_move=m['target_move'],
                                 source_effect=m['source_effect'], source_chance=m['chance'],
                                 actual=dict(id=actual[0], power=actual[1], accuracy=actual[2], pp=actual[3],
                                             type=type_names.get(actual[13], actual[13]), pp_up=actual[9:13], type_matches=bool(actual[4]), category_matches=bool(actual[5]),
                                             placeholder=bool(actual[6]), animation=bool(actual[7]), battle_script=bool(actual[8]),
                                             ignores_substitute=bool(actual[14]), ignores_protect=bool(actual[15]),
                                             fixed_hp_effect=bool(actual[16]), fixed_hp_payload=actual[17], display_power=actual[18],
                                             secondary_stat_drop=secondary,
                                             sleep_talk_eligible=bool(actual[30]) if m['target_move'] in sleep_contract else None),
                                 fixed_damage_adapter=fixed,
                                 variable_power_requires_behavior_test=m['power'] == 1,
                                 differences=differences))
    machines = []
    for i, m in enumerate(row for row in catalog if row['kind'] != 'MT'):
        actual = sample(game, 1, i, 10)
        correct_identity = (actual[1] == ids[m['target_move']] and actual[2] == m['number'] and
                            bool(actual[3]) == (m['kind'] == 'HM'))
        machines.append(dict(**m, item_id=actual[0], actual_move_id=actual[1],
                             actual_move=next((name for name, id_ in ids.items() if id_ == actual[1]), native_names.get(actual[1], str(actual[1]))),
                             display_number=actual[2], is_hm=bool(actual[3]),
                             teaching_handler=bool(actual[4]), machine_pocket=bool(actual[5]),
                             importance=actual[6], price=actual[7], source_lesson_adapter=bool(actual[8]),
                             reverse_item=actual[9], identity_matches=correct_identity))
    mismatches = []
    for i, s in enumerate(species):
        actual = sample(game, 2, i, len(catalog))
        for m, result in zip(catalog, actual, strict=True):
            expected = m['target_move'] in s['teaching_moves']
            if bool(result) != expected:
                mismatches.append(dict(species=s['target_species'], move=m['target_move'], expected=expected, actual=bool(result)))
    return dict(moves=move_results, machines=machines, compatibility=dict(
        combinations=len(species)*len(catalog), mismatches=mismatches))


def markdown(report):
    s = report['summary']
    lines = ['# Move and machine audit', '',
             f"Production source: `{report['target_commit']}`.",
             f"Pinned Polished Crystal: `{report['source_commit']}`.",
             f"Production ROM SHA-256: `{report['production_rom_sha256']}`.", '',
             'This probe links unchanged production Johto objects with a separate test boot.',
             'It checks the real accessors, PP calculations, teaching dispatch and compatibility.',
             'It does not play each move, teach every machine, or certify source battle effects,',
             'all secondary families, priorities, animation playback, field uses, inventory ownership,',
             'UI, save persistence or campaign acquisition. Engine regression tests and controller',
             'teaching scenarios are separate checks. A matching table row is not complete parity.', '',
             'Secondary data coverage is limited to the 18 source damaging target stat-drop moves:',
             'chance, stat payload, amount and target flags. RNG and gate behavior have separate tests.', '',
             '| Check | Result |', '| --- | --- |',
             f"| Move PP and all four PP Up levels | {s['move_pp_matches']}/{s['moves']} match |",
             f"| Non-placeholder effects and script/animation bindings | {s['dispatch_bound']}/{s['moves']} present |",
             f"| Move rows with audited data differences | {s['move_data_differences']} |",
             f"| TM number/move/category identity | {sum(m['identity_matches'] for m in report['machines'] if m['kind']=='TM')}/75 match |",
             f"| HM number/move/category identity | {sum(m['identity_matches'] for m in report['machines'] if m['kind']=='HM')}/6 match |",
             f"| Target stat-drop secondary data | {s['secondary_stat_drop_matches']}/{s['secondary_stat_drop_moves']} match |",
             f"| Sleep Talk source known-move eligibility | {s['sleep_talk_eligibility_matches']}/{s['sleep_talk_known_moves']} match |",
             f"| TMs routed through the explicit source lesson adapter | {s['source_tm_lesson_adapters']}/75 |",
             f"| Species/form × TM/HM/tutor compatibility | {s['compatibility_checks']-s['compatibility_mismatches']}/{s['compatibility_checks']} match |", '',
             '## Audited move-data differences', '',
             'These are table differences requiring source-behavior review, not all confirmed',
             'gameplay bugs. Typeless move sentinels and',
             'accuracy on moves that bypass accuracy may use different representations.',
             'Sonic Boom/Dragon Rage require the explicit power-1 fixed-HP adapter with',
             'payload and display power 20/40; raw runtime power remains visible in JSON.',
             'Source power 1 denotes variable/fixed damage and is recorded separately in JSON.', '',
             '| Move | Field | Source | Target |', '| --- | --- | --- | --- |']
    for m in report['moves']:
        for field, diff in m['differences'].items():
            lines.append(f"| {m['source_move']} | {field} | {diff.get('source', '')} | {diff.get('target', diff)} |")
    lines += ['', '## Machine identity gaps', '',
              'Stable target item IDs are preserved. Correcting these requires source adapters;',
              'do not renumber saved items. MOVE_NONE means this numbered slot has no move mapping.',
              'Legacy HM05 Flash remains intentionally readable; a source HM05 Whirlpool adapter',
              'must preserve that compatibility. Tutors have compatibility checks, not acquisition/UI tests.', '',
              '| Machine | Source move | Current target move |', '| --- | --- | --- |']
    for m in report['machines']:
        if not m['identity_matches']:
            lines.append(f"| {m['kind']}{m['number']:02d} | {m['target_move']} | {m['actual_move']} |")
    lines += ['', '## Reproduce', '', 'Build the Johto ROM, compile the existing mGBA driver, then run:', '',
              '```sh', 'python3 tools/polished_port/scripts/audit_moves_machines.py \\',
              '  --driver /tmp/johto-demo-driver --output /tmp/move-machine-audit', '```', '',
              'Exit 1 means observed parity gaps. `--report-only` emits the same findings with',
              'exit 0 for the staged migration CI; it does not label them passed. Runtime/compiler',
              'errors and source fixture failures still fail the job. CI uses `--known-gaps`',
              'to permit only the explicitly recorded differences; new differences fail it.',
              'Detailed rows are in',
              '`moves-machines.json` alongside this report.', '']
    return '\n'.join(lines)


def findings(report):
    result = {}
    for m in report['moves']:
        for field, difference in m['differences'].items():
            result[f"move/{m['target_move']}/{field}"] = difference
    for m in report['machines']:
        key = f"{m['kind']}{m['number']:02d}"
        if not m['identity_matches']:
            result[f'machine/{key}/identity'] = {k: m[k] for k in
                ('target_move', 'actual_move', 'display_number', 'is_hm')}
        if not m['teaching_handler'] or not m['machine_pocket']:
            result[f'machine/{key}/dispatch'] = {k: m[k] for k in ('teaching_handler', 'machine_pocket')}
        if m['identity_matches'] and m['reverse_item'] != m['item_id']:
            result[f'machine/{key}/reverse_lookup'] = {k: m[k] for k in ('reverse_item', 'item_id')}
        if m['kind'] == 'TM' and not m['source_lesson_adapter']:
            result[f'machine/{key}/source_lesson_adapter'] = False
    for m in report['compatibility']['mismatches']:
        result[f"compatibility/{m['species']}/{m['move']}"] = m
    return result


def unexpected_findings(report, baseline):
    assert baseline['source_commit'] == source.PIN
    return {key: value for key, value in findings(report).items()
            if key not in baseline['findings'] or baseline['findings'][key] != value}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-root', type=Path, default=ROOT)
    parser.add_argument('--driver', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--report-only', action='store_true')
    parser.add_argument('--known-gaps', type=Path,
                        help='Fail new/changed gaps, permit only explicitly recorded unfinished work')
    args = parser.parse_args()
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    build_root = args.build_root.resolve()
    # The tested object tree may be another checkout, but its tracked source
    # must exactly match this checkout. This catches accidental old-header use.
    expected_head = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip()
    build_head = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=build_root, text=True).strip()
    subprocess.run(['git', 'diff', '--exit-code', build_head, expected_head, '--',
                    'src', 'include', 'data', 'graphics', 'sound', 'Makefile', 'config.mk'], cwd=ROOT, check=True,
                   stdout=subprocess.DEVNULL)
    assert not subprocess.check_output(['git', 'diff', 'HEAD', '--', 'src', 'include', 'data', 'graphics', 'sound', 'Makefile', 'config.mk'], cwd=build_root), 'Dirty production source'
    moves, catalog, species = inputs()
    with tempfile.TemporaryDirectory(prefix='move-machine-audit-') as tmp:
        directory = Path(tmp)
        elf = compile_probe(build_root, directory, probe_code(moves, catalog, species))
        game = Game(args.driver.resolve(), elf, elf, args.output, directory)
        try:
            report = run(game, moves, catalog, species)
        finally:
            game.process.terminate()
            game.process.wait(timeout=10)
    report.update(source_commit=source.PIN, target_commit=build_head,
                  production_rom_sha256=hashlib.sha256((build_root/'pokeemerald-johto-demo.gba').read_bytes()).hexdigest(),
                  scope='Production-linked API/data audit; no battle effect, animation playback, controller teaching, save/reload or acquisition certification.')
    summary = dict(moves=len(report['moves']), move_data_differences=sum(bool(m['differences']) for m in report['moves']),
                   move_pp_matches=sum('pp' not in m['differences'] and 'pp_up' not in m['differences'] for m in report['moves']),
                   dispatch_bound=sum(not m['actual']['placeholder'] and m['actual']['animation'] and m['actual']['battle_script'] for m in report['moves']),
                   secondary_stat_drop_moves=sum(m['actual']['secondary_stat_drop'] is not None for m in report['moves']),
                   secondary_stat_drop_matches=sum(m['actual']['secondary_stat_drop'] is not None and 'secondary_stat_drop' not in m['differences'] for m in report['moves']),
                   sleep_talk_known_moves=sum(m['actual']['sleep_talk_eligible'] is not None for m in report['moves']),
                   sleep_talk_eligibility_matches=sum(m['actual']['sleep_talk_eligible'] is not None and 'sleep_talk_eligible' not in m['differences'] for m in report['moves']),
                   machine_identities_matching=sum(m['identity_matches'] for m in report['machines']),
                   machines=len(report['machines']), source_tm_lesson_adapters=sum(m['source_lesson_adapter'] for m in report['machines'] if m['kind'] == 'TM'),
                   compatibility_checks=report['compatibility']['combinations'], compatibility_mismatches=len(report['compatibility']['mismatches']))
    report['summary'] = summary
    (args.output/'moves-machines.json').write_text(json.dumps(report, indent=2)+'\n')
    (args.output/'moves-machines.md').write_text(markdown(report))
    print(json.dumps(summary, indent=2))
    gaps = findings(report)
    print('PARITY GAPS FOUND' if gaps else 'DATA CHECKS PASS; behavior/UI coverage remains separate')
    if args.known_gaps:
        unexpected = unexpected_findings(report, json.loads(args.known_gaps.read_text()))
        print(f'{len(gaps)} observed gap entries; {len(unexpected)} new or changed gaps')
        if unexpected:
            print(json.dumps(unexpected, indent=2))
        raise SystemExit(bool(unexpected))
    raise SystemExit(0 if args.report_only or not gaps else 1)


if __name__ == '__main__':
    main()
