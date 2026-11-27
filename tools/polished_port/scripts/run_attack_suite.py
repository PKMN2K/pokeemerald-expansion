#!/usr/bin/env python3
"""Run all Polished battle tests together, with fail-closed coverage accounting.

Default: source-profile tests and AI frame budgets. --native-regression also
runs the full native suite. --check checks fixtures/catalog without emulation.
--strict-completeness additionally fails for unverified source behavior; a
green regression suite must never be mistaken for complete port parity.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import signal
import subprocess
import time

import build_attack_suite as vectors

ROOT = vectors.source.ROOT
ANSI = re.compile(r'\x1b\[[0-9;]*m')

# These existing native-named fixtures were individually reconciled with the
# source path. Run them under the source build too; their native run alone is
# not evidence for that path. Keep exact names and expected counts fail-closed.
REVIEWED_SHARED_TESTS = [
    dict(label='source-cud-kee', name='Cud Chew will activate Kee Berry effect again on the next turn',
         path='test/battle/ability/cud_chew.c', count=1),
]


def test_catalog():
    result = []
    for path in sorted((ROOT/'test').rglob('*.c')):
        for match in re.finditer(r'\b(TEST|\w*BATTLE_TEST)\("(Polished[^"\n]*|AI thinking time[^"\n]*)"', path.read_text()):
            assert not match[1].startswith('TO_DO'), f'Unimplemented suite case: {match[2]}'
            result.append(dict(name=match[2], kind=match[1], path=str(path.relative_to(ROOT))))
    assert len({r['name'] for r in result}) == len(result), 'Duplicate test names'
    return result


def summary(log, expected=None):
    plain = ANSI.sub('', log)
    counts = {label: int(number.replace(',', '')) for label, number in
              re.findall(r'Tests ([A-Z_ ]+?):\s+([\d,]+)', plain)}
    assert counts.get('PASSED', 0) > 0 and counts.get('TOTAL', 0) > 0, 'Missing/empty test summary'
    assert counts.get('FAILED', 0) == 0, 'Battle tests failed'
    if expected is not None:
        assert counts['PASSED'] == counts['TOTAL'] == expected, (
            f'Expected {expected} executed passing groups; got {counts}. Skips, TODOs and expected failures do not count as source evidence.')
    return counts


def run(command, log_path, timeout, expected):
    started = time.monotonic()
    print('RUN ' + ' '.join(command), flush=True)
    with log_path.open('w') as output:
        child = subprocess.Popen(command, cwd=ROOT, stdout=output, stderr=subprocess.STDOUT, start_new_session=True)
        try:
            code = child.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            os.killpg(child.pid, signal.SIGKILL)
            child.wait()
            return dict(status='failed', error=f'Timed out after {timeout}s', log=str(log_path), command=command)
    result = dict(command=command, log=str(log_path), returncode=code,
                  seconds=round(time.monotonic()-started, 2), status='failed')
    try:
        assert code == 0, f'Command exited {code}'
        result['counts'] = summary(log_path.read_text(), expected)
        result['status'] = 'passed'
    except AssertionError as error:
        result['error'] = str(error)
    print(f"{result['status'].upper()}: {log_path.name}", flush=True)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    parser.add_argument('--keep-going', action='store_true', help='Run independent catalogs after a failure')
    parser.add_argument('--strict-completeness', action='store_true')
    parser.add_argument('--native-regression', action='store_true')
    parser.add_argument('--output', type=Path, default=Path('/tmp/polished-attack-suite'))
    parser.add_argument('--jobs', type=int, default=min(os.cpu_count() or 1, 8))
    parser.add_argument('--timeout', type=int, default=1800)
    parser.add_argument('--driver', help='Optional ROMTESTHYDRA override')
    args = parser.parse_args()
    assert args.jobs > 0 and args.timeout > 0
    for path, expected in vectors.build().items():
        assert path.read_text() == expected, f'Regenerate {path}'
    manifest_path = ROOT/'tools/polished_port/manifests/attack_suite.json'
    manifest = json.loads(manifest_path.read_text())
    catalog = test_catalog()
    for test in REVIEWED_SHARED_TESTS:
        body = (ROOT / test['path']).read_text()
        names = re.findall(r'\b(?:TEST|\w*BATTLE_TEST)\("([^"\n]+)"', body)
        assert names.count(test['name']) == test['count'], f'Changed reviewed shared fixture: {test["name"]}'
    source_count = sum(t['name'].startswith('Polished') for t in catalog)
    ai_count = len(catalog) - source_count
    assert source_count >= 503 and ai_count == 6
    report = dict(source_commit=vectors.source.PIN,
                  target_commit=subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip(),
                  manifest_sha256=hashlib.sha256(manifest_path.read_bytes()).hexdigest(),
                  source_moves=len(manifest['moves']), source_effects=len(manifest['effects']),
                  source_test_groups=source_count, ai_test_groups=ai_count, tests=catalog,
                  reviewed_shared_source_tests=REVIEWED_SHARED_TESTS,
                  suite2_reconciliation=manifest['suite2_reconciliation'],
                  original_shared_obligations=manifest['original_shared_obligations'],
                  complete_behavior_parity=False, unverified_effects=[e['effect'] for e in manifest['effects']
                      if not e['complete_behavior_parity']], shared_open_obligations=manifest['shared_open_obligations'],
                  execution='not run' if args.check else 'running', runs=[])
    args.output.mkdir(parents=True, exist_ok=True)
    report_path = args.output/'attack-suite.json'
    def save():
        report_path.write_text(json.dumps(report, indent=2)+'\n')
    save()
    if not args.check:
        base = ['make', f'-j{args.jobs}', '-O', 'TEST=1', 'UNUSED_ERROR=1', 'DEPRECATED_ERROR=1', 'check']
        if args.driver:
            base.append('ROMTESTHYDRA='+args.driver)
        jobs = []
        if args.native_regression:
            jobs.append(('native', ['POLISHED_MOVE_BALANCE_TEST=0', 'TESTS='], None))
        jobs += [('source', ['POLISHED_MOVE_BALANCE_TEST=1', 'TESTS=Polished'], source_count)]
        jobs += [(test['label'], ['POLISHED_MOVE_BALANCE_TEST=1', 'TESTS=' + test['name']], test['count'])
                 for test in REVIEWED_SHARED_TESTS]
        jobs += [('ai', ['POLISHED_MOVE_BALANCE_TEST=1', 'TESTS=AI thinking time'], ai_count)]
        for label, extra, count in jobs:
            result = run(base+extra, args.output/(label+'.log'), args.timeout, count)
            report['runs'].append(result)
            save()
            if result['status'] != 'passed':
                report['execution'] = 'failed'
                save()
                print(result['error'])
                if not args.keep_going:
                    return 1
        if report['execution'] == 'failed':
            return 1
        report['execution'] = 'passed'
        save()
    print(f"255 moves / 133 effects; {source_count} source groups + {ai_count} AI groups; execution: {report['execution']}")
    review = report['suite2_reconciliation']
    print(f"Reviewed Suite 2 mechanical implementation: {review['implementation_status'].upper()}; review: {review['closure_review']}; validation: {review['validation_record']}.")
    print(f"Complete source attack parity: OPEN ({len(report['unverified_effects'])} conservative full-context flags; Suite 3 contexts, AI, exact RNG and presentation remain open).")
    print('Documented deviation: reachable Light Metal zero-weight nontermination uses a bounded target fallback; literal source parity is not claimed for that input.')
    print(report_path)
    return 2 if args.strict_completeness and report['unverified_effects'] else 0


if __name__ == '__main__':
    raise SystemExit(main())
