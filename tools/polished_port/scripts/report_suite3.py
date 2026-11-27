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
import run_attack_suite as battle

# Paths below are relative to the production emulator artifact. Each probe
# writes its result only after its assertions pass. The full battle catalog is
# required in addition to these distinguishing cases from each owned domain.
DOMAIN_CHECKS = {
    'setup': (
        ['suite3-contexts/report.json', 'battle-systems/report.json', 'new-bark/sprout-validation.json', 'new-bark/falkner-validation.json',
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
        ['suite3-contexts/report.json', 'friendship-storage/report.json', 'new-bark/battle-loss.json'],
        ['Polished systems EXP combines participant and held share fractions before rounding',
         'Polished systems Safari rewards exclude the unsent lead and retain held shares']),
    'ai': (
        ['suite3-contexts/report.json'],
        ['Polished action AI Destiny Bond failure queries cannot clear the live volatile',
         'Polished escape roamer AI and action execution both reject sleep and freeze']),
    'facility_link': (
        ['link-conversion/report.json'],
        ["Polished linked Conversion plays both trainers' choices and leaves the next turn aligned",
         'Polished affection excludes actual link and frontier battles',
         'Polished critical evolution counter excludes link battles',
         'Polished Sketch boundary rejects link mode and invalid or duplicate Sketch history without mutation']),
    'presentation_rng': (
        ['link-conversion/report.json', 'conversion-selection/report.json', 'metronome-pool/report.json', 'new-bark/validation.json',
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
    if path.parent.name == 'link-conversion':
        if (data['choices'] != [1, 2] or len(set(data['types'])) != 2
                or len(data['tackle_pp_before']) != 2 or len(data['tackle_pp_after']) != 2
                or any(before - after != 1 for before, after in zip(data['tackle_pp_before'], data['tackle_pp_after']))):
            raise ValueError('missing serial choice retry or next-turn synchronization')
    if path.parent.name == 'suite3-contexts':
        if (data['seeded_reward_cases'] < 127 or data['wild_factory_cases'] < 96
                or data['trainer_medicine_cases'] < 15 or data['flash_reloads'] != 4
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
                catalog_counts=counts, domains=rows, shared_open_obligations=inventory['shared_open_obligations'],
                complete_behavior_parity=inventory['complete_behavior_parity'], errors=errors)


def markdown(report):
    parity = 'verified' if report['complete_behavior_parity'] else 'open'
    lines = ['## Suite 3 battle contexts and integration', '',
             f"Commit `{report['target_commit'][:12]}`. Validation **{report['execution']}**. "
             f"Whole-battle parity **{parity}**.", '']
    lines += ['| Area | Checks in this run | Reviewed parity |', '| --- | --- | --- |']
    for row in report['domains']:
        lines.append(f"| {row['title']} | {row['execution']} | {row['status']} |")
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
