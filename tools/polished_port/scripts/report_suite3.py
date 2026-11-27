#!/usr/bin/env python3
"""Verify Suite 3 against the build, emulator and battle evidence from one CI run."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess

import build_battle_completion_plan as plan
import build_battle_suite as coverage
import build_trainer_attributes as trainers
import run_attack_suite as battle

# Paths below are relative to the production emulator artifact. Each probe
# writes its result only after its assertions pass. The full battle catalog is
# required in addition to these distinguishing cases from each owned domain.
DOMAIN_CHECKS = {
    'setup': (
        ['trainer-roster/report.json', 'suite3-contexts/report.json', 'battle-systems/report.json', 'new-bark/sprout-validation.json', 'new-bark/falkner-validation.json',
         'shift-source/shift-switch-validation.json', 'shift-native/shift-switch-validation.json'],
        ['Polished systems trainer generation preserves source slots stats gender and friendship',
         'Polished systems trainer ability selection uses source species byte trainer ID and one based slot']),
    'capture_escape': (
        ['suite3-contexts/report.json', 'capture-storage/report.json', 'escape-storage/report.json', 'shift-source/shift-switch-validation.json',
         'shift-native/shift-switch-validation.json'],
        ['Polished Dusk Ball triples catch probability in Dark Cave and uses normal odds in Sprout Tower',
         'Polished systems Friend Ball delivers source friendship with capture EXP',
         'Polished escape resolves inclusive random boundaries and repeated attempts through Run']),
    'rewards': (
        ['suite3-contexts/report.json', 'factory-points/report.json', 'friendship-storage/report.json', 'new-bark/battle-loss.json'],
        ['Polished systems EXP combines participant and held share fractions before rounding',
         'Polished systems Safari rewards exclude the unsent lead and retain held shares']),
    'ai': (
        ['suite3-contexts/report.json'],
        ['Polished action AI Destiny Bond failure queries cannot clear the live volatile',
         'Polished escape roamer AI and action execution both reject sleep and freeze']),
    'facility_link': (
        ['suite3-contexts/report.json', 'tower-parties/report.json', 'factory-rentals/report.json', 'factory-points/report.json', 'link-conversion/report.json'],
        ["Polished linked Conversion plays both trainers' choices and leaves the next turn aligned",
         'Polished affection excludes actual link and frontier battles',
         'Polished critical evolution counter excludes link battles',
         'Polished Sketch boundary rejects link mode and invalid or duplicate Sketch history without mutation']),
    'presentation_rng': (
        ['mon-pictures/report.json', 'trainer-roster/report.json', 'battle-art/report.json', 'link-conversion/report.json', 'conversion-selection/report.json', 'metronome-pool/report.json', 'new-bark/validation.json',
         'shift-source/shift-switch-validation.json', 'shift-native/shift-switch-validation.json'],
        ['Polished Safari bait and rock consume every rejected draw and one accepted draw']),
}
EVOLUTION_REPORTS = ('evolution-input-validation.json', 'source-rules/evolution-input-validation.json',
                     'forms/form-evolution-input-validation.json')
EVIDENCE_ERRORS = (OSError, ValueError, KeyError, TypeError, AssertionError)


def check_catalog(directory, inventory, target, errors):
    passed = set()
    counts = {}
    try:
        counts['native'] = battle.summary((directory / 'native.log').read_text())
    except EVIDENCE_ERRORS as error:
        errors.append(f'Native battle catalog: {error}')
    try:
        report = json.loads((directory / 'battle-suite.json').read_text())
        if report['target_commit'] != target or report['source_commit'] != inventory['source_commit']:
            raise ValueError('battle report belongs to a different source or target commit')
        if report['manifest_sha256'] != hashlib.sha256(coverage.MANIFEST.read_bytes()).hexdigest():
            raise ValueError('battle report refers to a different test inventory')
    except EVIDENCE_ERRORS as error:
        errors.append(f'Battle report: {error}')
        return passed, counts
    if report['execution'] != 'passed':
        errors.append(f"Battle catalog execution was {report['execution']}")

    names = {case['name'] for case in inventory['regression_tests']}
    source = {name for name in names if name.startswith('Polished')}
    groups = {'source': source, 'ai': names - source}
    groups.update({test['label']: {test['name']} for test in battle.REVIEWED_SHARED_TESTS})
    for label, expected in groups.items():
        try:
            log = battle.ANSI.sub('', (directory / (label + '.log')).read_text())
            actual = set(re.findall(r'^(?:\[\d+\] )?(.+): PASS[ \t]*$', log, re.M))
            passed.update(actual & expected)
            missing = expected - actual
            if missing:
                raise ValueError(f'{len(missing)} missing passing result(s), including {min(missing)}')
            counts[label] = battle.summary(log, expected=len(expected))
        except EVIDENCE_ERRORS as error:
            errors.append(f'{label} battle catalog: {error}')
    return passed, counts


def check_probe(path, rom_hash, pin):
    data = json.loads(path.read_text())
    if not isinstance(data, dict) or not data:
        raise ValueError('missing probe results')
    if data.get('source_commit', pin) != pin:
        raise ValueError('unexpected source commit')
    if path.parent.name == 'mon-pictures':
        if (data['identities'], data['egg'], data['palettes'], data['playback_sequences'],
                data['expansion_control']) != (391, True, 784, 784, 'RAYQUAZA') or (data['playback_steps'], data['stationary_egg_from_cycle']) != (12733, 11):
            raise ValueError('missing source picture, palette or animation evidence')
    if path.parent.name == 'battle-art':
        if (len(set(data['verified_items'])) != 244
                or not {'ITEM_POLISHED_ARMOR_SUIT', 'ITEM_POLISHED_MINT_LEAF'} <= set(data['verified_items'])
                or not data['item_vram_checked'] or data['registered_trainer_portraits'] != ['GIOVANNI1'] or not data['expansion_item_fallback']):
            raise ValueError('missing decoded source item tiles, palettes or Giovanni portrait')
    if path.parent.name == 'link-conversion':
        if (data['choices'] != [1, 2] or len(set(data['types'])) != 2
                or len(data['tackle_pp_before']) != 2 or len(data['tackle_pp_after']) != 2
                or any(before - after != 1 for before, after in zip(data['tackle_pp_before'], data['tackle_pp_after']))):
            raise ValueError('missing serial choice retry or next-turn synchronization')
    if path.parent.name == 'factory-points':
        cases = data['cases']
        expected = {'loss': 5, 'factory-head': 26, 'interruption': 16,
                    'pause-resume': 7, 'wallet-and-wrap': 16, 'long-streak': 56}
        if (data['prefix_cases'] != 64 or data['complete_set_awards'] != [12, 19, 26, 33, 40, 47, 53, 56]
                or {name: case['earned'] for name, case in cases.items()} != expected
                or any(case['verified_reloads'] != 2 for case in cases.values())
                or {name: case['mint_leaves'] for name, case in cases.items()} != {name: int(name == 'factory-head') for name in expected}
                or {name: (case['ability_patches'], case['earned'], case['verified_reloads']) for name, case in data['tower'].items()}
                   != {'win': (1, 20, 2), 'full': (0, 20, 2), 'loss': (0, 15, 2)}
                or cases['factory-head']['streak'] != 21 or cases['long-streak']['record'] != 10003
                or cases['wallet-and-wrap']['wallet'] != 50000 or cases['wallet-and-wrap']['card'] != 65535):
            raise ValueError('missing Factory BP, streak, lobby or Flash evidence')
    if path.parent.name == 'factory-rentals':
        if (data['drafts'] < 40 or data['draft_rows'] != list(range(2, 9))
                or not data['repeated_item_drafts'] or not data['whole_draft_retries']
                or data['tower_factory_carry_cases'] != 6 or data['flash_rentals'] != 3
                or data['selection_order'] != [5, 2, 4] or data['opponent_teams'] != data['drafts'] * 7
                or len(data['scout_messages']) != 6 or min(data['scout_messages']) <= 0
                or sum(data['scout_messages']) != data['opponent_teams']
                or not all(data[k] for k in ('rental_rejections', 'history_rejections', 'third_slot_repeats', 'other_shared_type_ignored'))
                or data['attendant_dialogue'] != 'You can expect to see Sneasel, Mr. Rime and Yanma.'):
            raise ValueError('missing Factory draft, opponents, scouting, streak, Flash or selection evidence')
    if path.parent.name == 'tower-parties':
        if (data['sets'] != 432 or data['generated_teams'] < 700 or data['retained_history_slots'] != 9
                or data['player_registration_cases'] != 11 or data['player_battle_levels'] != [50, 50, 50]
                or data['restored_roster_levels'] != [1, 33, 44, 100, 55, 77]
                or data['streak_cases'] != 10
                or data['streak_board_after_resets'] != [['Streak: 0 wins', 'Record: 10005 wins']] * 2
                or data['point_award_cases'] != 65 or data['flash_bp'] != 50000
                or data['difficulty_rows'] != 8 or not data['third_slot_history_repeats']):
            raise ValueError('missing Tower sets, tier, third-slot repeat or Flash history evidence')
    if path.parent.name == 'trainer-roster':
        if data['trainer_badge_policies'] != 16252:
            raise ValueError('missing source trainer class and badge policy combinations')
        if (data['source_parties'], data['roster_entries'], data['party_builds'],
                data['portraits_and_palettes'], data['inver_omissions'], data['saved_trainer_flags'], data['inverse_matchups']) != (956, 2659, 1912, 154, 7, 934, 324):
            raise ValueError('missing full source trainer, portrait, or saved flag evidence')
    if path.parent.name == 'suite3-contexts':
        smart = data['smart_field_ai']
        if (smart['cases'] != 861 or {name: case['scores'] for name, case in smart['battles'].items()} !=
                {'low': [102, 99], 'full': [98, 101], 'low_control': [100, 100], 'full_control': [100, 100],
                 'sun': [103, 100], 'sun_umbrella': [102, 100]}
                or smart['battles']['low']['move'] != 'Recover' or smart['battles']['full']['move'] != 'Rain Dance'):
            raise ValueError('missing Smart trainer field and recovery score or move-selection evidence')
        if (not all(row['link_records_reloaded'] for row in data['outcomes'].values())
                or data['trainer_setup_cases'] != 640 or data['legendary_blocked'] != ['Run', 'Teleport', 'Roar']):
            raise ValueError('missing Pay Day, link standings, Setup scoring or legendary exit evidence')
        if (data['seeded_reward_cases'] < 127 or data['wild_factory_cases'] < 96
                or data['roamer_movement_cases'] < 280 or data['roamer_encounter_bytes'] != 256
                or data['simultaneous_beasts'] != 2 or data['wild_gender_cases'] != 576 or data['trainer_tie_cases'] != 192 or data['trainer_policy_cases'] != 256
                or data['trainer_medicine_cases'] < 15 or data['flash_reloads'] != 4 or data['payout_cases'] != 70
                or {name: row['wallet_after_reset'] for name, row in data['outcomes'].items()}
                   != {'win': 1200923, 'capture': 1200923, 'escape': 1200123, 'loss': 1188243}
                or {name: row['outcome'] for name, row in data['outcomes'].items()}
                   != {'win': 1, 'capture': 7, 'escape': 4, 'loss': 2}):
            raise ValueError('missing wild generation, medicine, outcome or Flash results')
    if path.name == 'shift-switch-validation.json':
        profile = path.parent.name.removeprefix('shift-')
        if data['profile'] != profile or bool(data['fixture_source_rules']) != (profile == 'source'):
            raise ValueError('incorrect Shift battle profile')
        default = 'set' if profile == 'source' else 'shift'
        required = {'set', 'switch', 'predict'} if profile == 'source' else {'set', 'shift'}
        options = [check['setup']['options'] for check in data['checks'] if 'options' in check['setup']]
        if {o['selected'] for o in options if o['flash_reload'] and o['default'] == default} != required:
            raise ValueError('missing battle style menu and Flash reload evidence')
        if profile == 'native':
            if data['production_rom'] != 'pokeemerald.gba' or not data['checks']:
                raise ValueError('missing native Shift control')
            return
    if data.get('production_rom_sha256', data.get('rom_sha256', rom_hash)) != rom_hash:
        raise ValueError('different production ROM')
    if path.parent.name == 'new-bark' and path.name == 'validation.json':
        if not data['warps'] or not any('continue' in check.lower() for check in data['checks']):
            raise ValueError('campaign Save/Continue evidence missing')


def collect(inputs, jobs, target):
    inventory = json.loads(coverage.MANIFEST.read_text())
    completion = json.loads(plan.MANIFEST.read_text())
    owned = next(s['domains'] for s in completion['suites'] if s['id'] == 3)
    if len(owned) != len(DOMAIN_CHECKS) or set(owned) != set(DOMAIN_CHECKS):
        raise ValueError('Missing or unexpected Suite 3 domain in evidence mapping')
    domains = {row['id']: row for row in inventory['domains']}
    errors = []
    for name in ('build-emerald', 'test'):
        if jobs.get(name) != 'success':
            errors.append(f"{name} job was {jobs.get(name, 'missing')}")

    passed, counts = check_catalog(inputs / 'polished-battle-suite', inventory, target, errors)
    production = inputs / ('polished-johto-demo-' + target)
    rom_hash = None
    try:
        rom, = production.rglob('pokeemerald-johto-demo.gba')
        rom_hash = hashlib.sha256(rom.read_bytes()).hexdigest()
    except (ValueError, OSError) as error:
        errors.append(f'Expected one production Johto ROM in this run: {error}')

    trainer_coverage = json.loads(trainers.MANIFEST.read_text())['party_coverage']
    missing_trainers = sum(map(len, trainer_coverage['unbound_by_class'].values()))
    if trainer_coverage['bound_parties'] + missing_trainers != trainer_coverage['source_parties']:
        errors.append('trainer bindings and unbound parties do not reconcile')
    if missing_trainers and (inventory['complete_behavior_parity'] or domains['setup']['status'] == 'complete'):
        errors.append('battle setup was marked complete with unbound trainer parties')
    rows = []
    for key in owned:
        paths, cases = DOMAIN_CHECKS[key]
        checks = {case: 'passed' if case in passed else 'missing pass' for case in cases}
        for name in paths:
            try:
                if not rom_hash:
                    raise ValueError('production ROM unavailable')
                check_probe(production / 'tmp/johto-validation' / name, rom_hash, inventory['source_commit'])
                checks[name] = 'passed'
            except EVIDENCE_ERRORS as error:
                checks[name] = str(error)
        if key == 'rewards':
            for name in EVOLUTION_REPORTS:
                try:
                    data = json.loads((inputs / 'battle-evolution-evidence' / name).read_text())
                    if {row['result'] for row in data} != {'completed', 'cancelled'}:
                        raise ValueError('missing evolution completion or cancellation')
                    checks[name] = 'passed'
                except EVIDENCE_ERRORS as error:
                    checks[name] = str(error)
        if key == 'ai':
            checks['AI frame budgets'] = 'passed' if 'ai' in counts else 'missing pass'
        failed = [name for name, status in checks.items() if status != 'passed']
        if failed:
            errors.append(f"{key} has incomplete evidence for {', '.join(failed)}")
        rows.append({**domains[key], 'checks': checks, 'execution': 'incomplete' if failed else 'passed'})

    return dict(suite_id=3, target_commit=target, source_commit=inventory['source_commit'],
                run_id=os.getenv('GITHUB_RUN_ID'), run_attempt=os.getenv('GITHUB_RUN_ATTEMPT'),
                execution='failed' if errors else 'passed', jobs=jobs, production_rom_sha256=rom_hash,
                catalog_counts=counts, trainer_party_coverage=trainer_coverage, domains=rows, shared_open_obligations=inventory['shared_open_obligations'],
                complete_behavior_parity=inventory['complete_behavior_parity'], errors=errors)


def markdown(report):
    parity = 'verified' if report['complete_behavior_parity'] else 'open'
    lines = ['## Suite 3 battle contexts and integration', '',
             f"Commit `{report['target_commit'][:12]}`. Validation **{report['execution']}**. "
             f"Whole-battle parity **{parity}**.", '']
    lines += ['| Area | Checks in this run | Reviewed parity |', '| --- | --- | --- |']
    for row in report['domains']:
        lines.append(f"| {row['title']} | {row['execution']} | {row['status']} |")
    trainers = report['trainer_party_coverage']
    lines += ['', f"Trainer party bindings cover **{trainers['bound_parties']} of {trainers['source_parties']}** "
                  "default-source parties, including rematches. The JSON report lists unbound parties by source class."]
    lines += ['', '### Battle catalogs', '', '| Catalog | Passed | Registered |', '| --- | ---: | ---: |']
    titles = {'native': 'Native', 'source': 'Polished Crystal', 'ai': 'AI frame budgets'}
    titles.update({test['label']: test['name'] for test in battle.REVIEWED_SHARED_TESTS})
    for label, counts in report['catalog_counts'].items():
        lines.append(f"| {titles[label]} | {counts['PASSED']:,} | {counts['TOTAL']:,} |")
    if report['shared_open_obligations']:
        lines += ['', '### Remaining work', '']
        lines += ['- ' + item.removeprefix('Suite 3: ') for item in report['shared_open_obligations']]
    if report['errors']:
        lines += ['', '### Execution failures', ''] + ['- ' + error for error in report['errors']]
    lines += ['', 'The `polished-suite-3` artifact identifies the individual tests and emulator reports for each area.', '']
    return '\n'.join(lines)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--inputs', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    target = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=coverage.ROOT, text=True).strip()
    jobs = {name: value['result'] for name, value in json.loads(os.environ['SUITE3_JOBS']).items()}
    report = collect(args.inputs, jobs, target)
    args.output.mkdir(parents=True, exist_ok=True)
    (args.output / 'suite-3.json').write_text(json.dumps(report, indent=2) + '\n')
    summary = markdown(report)
    (args.output / 'suite-3.md').write_text(summary)
    if path := os.getenv('GITHUB_STEP_SUMMARY'):
        with open(path, 'a') as stream:
            stream.write(summary)
    print(summary)
    return int(report['execution'] != 'passed')


if __name__ == '__main__':
    raise SystemExit(main())
