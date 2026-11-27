#!/usr/bin/env python3
"""Run the whole Polished battle regression catalog and report remaining parity.

Exit 0 means the requested regression checks passed, never complete parity.
--strict-completeness exits 2 while any domain/block/effect remains unverified.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys

import build_battle_suite as coverage
import run_attack_suite as attacks

ROOT = coverage.ROOT


def markdown(report):
    review = report['suite2_reconciliation']
    rows = ['# Whole battle parity suite', '',
            f'Source: `{report["source_commit"]}`. Target: `{report["target_commit"]}`.', '',
            f'Regression execution: **{report["execution"]}**. Complete battle parity: **OPEN**.', '',
            f'{report["source_file_count"]} authenticated source files; {report["source_block_count"]} lexical global blocks; '
            f'{report["source_moves"]} moves; {report["source_effects"]} effects; {report["source_test_groups"]} source test groups.', '',
            'Inventory and passing scenarios do not certify every branch. Lexical blocks include data labels, aliases and conditional variants. '
            'The six AI groups measure frame budgets, not source decision parity.', '',
            '## Reviewed Suite 2 implementation', '',
            f'Mechanical implementation: **{review["implementation_status"]}**. '
            f'Review: `{review["closure_review"]}`. Validation: `{review["validation_record"]}`.', '',
            review['interpretation'], '',
            '## Documented source deviation', '']
    for deviation in review['documented_deviations']:
        rows.append(f'- {deviation["source_behavior"]} {deviation["target_behavior"]} {deviation["disposition"]} '
                    f'Source review: `{deviation["review"]}`.')
    rows += ['', '## Original Suite 3 obligations remaining open', '']
    rows += ['- ' + obligation for obligation in report['shared_open_obligations']]
    rows += ['', '## Conservative whole-battle inventory', '',
             report['inventory_status_interpretation'], '',
             '| Domain | Whole-battle status | Original inventory scope |', '| --- | --- | --- |']
    for domain in report['domains']:
        rows.append(f'| {domain["title"]} | {domain["status"]} | {" ".join(domain["obligations"])} |')
    rows += ['', 'The machine-readable report contains the exact test catalog, source manifest hash, logs and execution results.', '']
    return '\n'.join(rows)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    parser.add_argument('--keep-going', action='store_true', help='Run independent catalogs after a failure')
    parser.add_argument('--strict-completeness', action='store_true')
    parser.add_argument('--native-regression', action='store_true')
    parser.add_argument('--output', type=Path, default=Path('/tmp/polished-battle-suite'))
    parser.add_argument('--jobs', type=int, default=8)
    parser.add_argument('--timeout', type=int, default=1800)
    parser.add_argument('--driver')
    args = parser.parse_args()
    assert args.jobs > 0 and args.timeout > 0
    args.output.mkdir(parents=True, exist_ok=True)
    try:
        for path, expected in coverage.build().items():
            assert path.read_text() == expected, f'Regenerate {path}'
        inventory = json.loads(coverage.MANIFEST.read_text())
    except (AssertionError, KeyError, OSError, ValueError) as error:
        (args.output/'battle-suite.json').write_text(json.dumps(dict(execution='failed', complete_behavior_parity=False, error=str(error)), indent=2)+'\n')
        raise
    report = dict(source_commit=coverage.source.PIN,
        target_commit=subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip(),
        tracked_worktree_diff_sha256=hashlib.sha256(subprocess.check_output(['git', 'diff', 'HEAD', '--'], cwd=ROOT)).hexdigest(),
        manifest_sha256=hashlib.sha256(coverage.MANIFEST.read_bytes()).hexdigest(),
        source_file_count=len(inventory['source_files']), source_block_count=sum(len(f['blocks']) for f in inventory['source_files']),
        source_moves=inventory['source_moves'], source_effects=inventory['source_effects'],
        source_test_groups=sum(t['name'].startswith('Polished') for t in inventory['regression_tests']),
        domains=inventory['domains'], unverified_effects=inventory['unverified_effects'],
        suite2_reconciliation=inventory['suite2_reconciliation'],
        original_shared_obligations=inventory['original_shared_obligations'],
        shared_open_obligations=inventory['shared_open_obligations'],
        inventory_status_interpretation=inventory['inventory_status_interpretation'],
        complete_behavior_parity=False, execution='not run' if args.check else 'running', runs=[])
    def save():
        (args.output/'battle-suite.json').write_text(json.dumps(report, indent=2)+'\n')
        (args.output/'battle-suite.md').write_text(markdown(report))
    save()
    # Retain the existing execution engine and its attack-suite.json artifact so
    # current consumers keep working. It selects every Polished unit/battle test.
    command = [sys.executable, str(ROOT/'tools/polished_port/scripts/run_attack_suite.py'),
               '--output', str(args.output), '--jobs', str(args.jobs), '--timeout', str(args.timeout)]
    for flag, enabled in (('--check', args.check), ('--native-regression', args.native_regression),
                          ('--keep-going', args.keep_going)):
        if enabled:
            command.append(flag)
    if args.driver:
        command += ['--driver', args.driver]
    code = subprocess.call(command, cwd=ROOT)
    legacy_path = args.output/'attack-suite.json'
    if legacy_path.exists():
        legacy = json.loads(legacy_path.read_text())
        report['runs'] = legacy['runs']
        report['tests'] = legacy['tests']
        report['execution'] = legacy['execution'] if code == 0 else 'failed'
    else:
        report['execution'] = 'failed'
    save()
    if code != 0 or report['execution'] == 'failed':
        return code or 1
    print(args.output/'battle-suite.md')
    try:
        coverage.require_complete(inventory)
    except AssertionError as error:
        print('Whole-battle strict completeness remains OPEN; the reviewed Suite 2 implementation is recorded separately above.')
        print('Conservative inventory detail:', error)
        return 2 if args.strict_completeness else 0
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
