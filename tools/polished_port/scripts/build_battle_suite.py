#!/usr/bin/env python3
"""Inventory the whole battle boundary from Git-authenticated source objects.

This is a coverage ledger, NOT a source interpreter or a parity certificate.
Lexical labels include conditional variants: semantic evidence must select the
release default explicitly. Import never reads the source checkout's worktree.
"""
import argparse
import base64
import gzip
import hashlib
import json
from pathlib import Path
import re
import subprocess

import build_pokemon_learnsets as source
import build_attack_suite as attacks
import build_battle_foundations as foundations
import run_attack_suite as execution

ROOT = source.ROOT
ARCHIVE = ROOT/'tools/polished_port/fixtures/battle_logic/source.json.gz.b64'
MANIFEST = ROOT/'tools/polished_port/manifests/battle_suite.json'
# Entire directories, including unused variants/presentation, deliberately form
# a conservative superset. External field scripts are not certified by this.
PREFIXES = ('engine/battle/', 'data/battle/', 'home/', 'engine/items/',
            'engine/pokemon/', 'data/items/', 'data/abilities/', 'constants/',
            'macros/', 'data/trainers/', 'engine/events/battle_tower/',
            'data/battle_tower/')
EXTRA = ('engine/events/trainer_scripts.asm', 'engine/math/math.asm',
         'data/options/default_options.asm', 'data/moves/critical_hit_moves.asm', 'ram/hram.asm', 'data/moves/priorities.asm',
         'data/moves/punching_moves.asm', 'data/moves/launcher_moves.asm',
         'data/moves/slicing_moves.asm', 'data/moves/low_kick_power.asm',
         'data/pokemon/body_data.asm', 'engine/battle_anims/pokeball_wobble.asm',
         'engine/pokedex/pokedex.asm', 'data/growth_rates.asm', 'engine/events/whiteout.asm',
         'engine/events/happiness_egg.asm', 'data/events/happiness_changes.asm')

DOMAINS = {
    'setup': ('Battle setup and trainer/wild construction', ['engine/battle/core.asm', 'engine/battle/read_trainer_party.asm', 'engine/battle/read_trainer_dvs.asm', 'engine/battle/read_trainer_attributes.asm'],
              ['All source trainer parties, attributes, DV/stat/ability selection, wild generation, battle types, start/end persistence and special scripted opponents.']),
    'actions': ('Action selection, order and cancellation', ['engine/battle/core.asm', 'engine/battle/effect_commands.asm', 'home/battle.asm'],
                ['Full priority/speed ordering, ties, switching/items/run ordering, obedience, recharge/flinch/status/Disable/Encore/PP interactions and random call order.']),
    'damage': ('Damage, criticals, types, accuracy and stats', ['engine/battle/effect_commands.asm', 'engine/battle/stats.asm', 'engine/battle/hidden_power.asm'],
               ['Every arithmetic boundary and modifier order: stat stages, criticals, STAB, effectiveness, burn, items, abilities, weather, screens, integer rounding and saturation.', 'Accuracy/evasion combination, immunities, hit exceptions and all byte/range RNG decisions.']),
    'moves': ('Every move and effect command', ['engine/battle/effect_commands.asm', 'data/battle/effect_command_pointers.asm'],
              ['All 255 source identities, 133 effects and 129 commands: all success/failure branches, called/multi-turn moves, copy/transform/history, PP, recoil/drain, Substitute/Protect, switches and fainting.']),
    'status': ('Major status and volatile state', ['engine/battle/effect_commands.asm', 'engine/battle/endturn.asm'],
               ['Application, immunity, cure/reflection order, duration, action blocking, self-confusion damage, switch/party persistence and simultaneous conditions.']),
    'abilities': ('Abilities and suppression', ['engine/battle/abilities.asm', 'data/abilities/mold_breaker_suppressed_abilities.asm'],
                  ['Every source ability, trigger and rejection branch; active/stored/suppressed ability identity; contact, faint, entry, end-turn, Unnerve, Neutralizing Gas, Mold Breaker and copying.']),
    'items': ('Held and trainer/player-used items', ['engine/battle/core.asm', 'engine/items/item_effects.asm'],
              ['All held-effect parameter values and consumptions, cure/berry gates, damage/reactive items, removal/recovery, in-battle use, restrictions and inventory/party persistence.']),
    'field': ('Weather, screens and residual event order', ['engine/battle/endturn.asm', 'home/battle.asm'],
              ['Weather durations, suppression and immunity; screen timers; each residual fraction and ordered event; cures, items, delayed attacks and replacement after every faint boundary.']),
    'switching': ('Switching, trapping and entry hazards', ['engine/battle/core.asm', 'engine/battle/abilities.asm'],
                  ['Voluntary/forced/pivot switches, traps, Baton Pass, Pursuit, hazard ordering, switch abilities/items, reset/persist state and replacement selection.']),
    'fainting': ('Fainting and battle outcomes', ['engine/battle/core.asm', 'engine/battle/effect_commands.asm', 'engine/battle/endturn.asm'],
                ['Global contact/recoil/residual/faint ordering, Destiny Bond/Aftermath, simultaneous knockouts, last-Pokemon outcomes, forced replacements and cleanup.']),
    'capture_escape': ('Capture and fleeing', ['engine/items/poke_balls.asm', 'engine/battle/core.asm'],
                      ['Every ball modifier and capture arithmetic, status/HP rounding, guaranteed/restricted captures, party/box/Pokedex delivery, run/escape attempts, fleeing and roaming persistence.']),
    'rewards': ('Experience, money and post-battle changes', ['engine/battle/core.asm', 'engine/pokemon/experience.asm', 'engine/battle/evolve_battle.asm'],
                ['Participant/Exp Share distribution, growth/stat changes, learning/evolution cancellation, happiness, money/Pay Day, losses and saved outcome state.']),
    'ai': ('AI decisions and information', ['engine/battle/ai/scoring.asm', 'engine/battle/ai/move.asm', 'engine/battle/ai/switch.asm', 'engine/battle/ai/items.asm'],
           ['Source scoring tiers and every move/effect heuristic, choice tie breaks, item and switch decisions, permitted information and battle modes. Native timing checks prove performance only.']),
    'facility_link': ('Facility and link-specific rules', ['engine/events/battle_tower/battle_tower.asm', 'engine/events/battle_tower/load_trainer.asm'],
                      ['Battle Tower party generation, tier restrictions, outcomes and persistence; source link battle rules and synchronization. Native facilities/doubles/gimmicks remain optional extensions.']),
    'presentation_rng': ('Shared RNG and observable sequencing', ['home/random.asm', 'engine/battle/text.asm', 'engine/battle/used_move_text.asm'],
                         ['Exact random stream/call count and source battle messages/animations/timing. A probability match or native animation does not establish these.']),
}


def git_hash(kind, data):
    return hashlib.sha1(kind.encode()+b' '+str(len(data)).encode()+b'\0'+data).hexdigest()


def tree_entries(raw):
    while raw:
        header, raw = raw.split(b'\0', 1)
        mode, name = header.decode().split(' ', 1)
        assert len(raw) >= 20
        yield mode, name, raw[:20].hex()
        raw = raw[20:]


def selected(path):
    return path.endswith('.asm') and (path.startswith(PREFIXES) or path in EXTRA)


def descend(path):
    return any(root.startswith(path) or path.startswith(root) for root in PREFIXES + EXTRA)


def import_source(repo):
    def obj(kind, sha):
        return subprocess.check_output(['git', '-C', str(repo), 'cat-file', kind, sha])
    commit = obj('commit', source.PIN)
    root = commit.split(b'\n', 1)[0].decode().removeprefix('tree ')
    objects = {'commit': base64.b64encode(commit).decode(), 'trees': {}, 'blobs': {}}
    def visit(sha, prefix=''):
        data = obj('tree', sha)
        objects['trees'][sha] = base64.b64encode(data).decode()
        for mode, name, child in tree_entries(data):
            path = prefix+name
            if mode == '40000' and descend(path+'/'):
                visit(child, path+'/')
            elif selected(path):
                objects['blobs'][child] = base64.b64encode(obj('blob', child)).decode()
    visit(root)
    raw = json.dumps(objects, sort_keys=True, separators=(',', ':')).encode()
    ARCHIVE.parent.mkdir(parents=True, exist_ok=True)
    ARCHIVE.write_text(base64.b64encode(gzip.compress(raw, mtime=0)).decode()+'\n')
    authenticated_sources()  # Prove the archive is complete before returning.


def authenticated_sources(archive=None):
    archive = archive if archive is not None else json.loads(gzip.decompress(base64.b64decode(ARCHIVE.read_bytes())))
    commit = base64.b64decode(archive['commit'])
    assert git_hash('commit', commit) == source.PIN, 'Source commit authentication failed'
    root = commit.split(b'\n', 1)[0].decode().removeprefix('tree ')
    files = {}
    def visit(sha, prefix=''):
        raw = base64.b64decode(archive['trees'][sha])
        assert git_hash('tree', raw) == sha, f'Source tree authentication failed: {prefix}'
        for mode, name, child in tree_entries(raw):
            path = prefix+name
            if mode == '40000' and descend(path+'/'):
                visit(child, path+'/')
            elif selected(path):
                data = base64.b64decode(archive['blobs'][child])
                assert git_hash('blob', data) == child, f'Source blob authentication failed: {path}'
                files[path] = dict(sha=child, text=data.decode())
    visit(root)
    assert all(any(p.startswith(prefix) for p in files) for prefix in PREFIXES)
    assert all(path in files for path in EXTRA)
    # Reject disagreement with older per-feature fixtures of the same pin.
    for path, text in source.sources().items():
        if path in files:
            assert files[path]['text'] == text, f'Conflicting source fixtures: {path}'
    return files


def source_blocks(text):
    # Retain both conditional branches and alias labels; never call this an
    # executable control-flow graph or use it to infer semantic completion.
    matches = list(re.finditer(r'^([A-Za-z_][A-Za-z_0-9]*)(?:::|:)(?:\s|$)', text, re.M))
    blocks = []
    for index, match in enumerate(matches):
        body = text[match.start():matches[index+1].start() if index+1 < len(matches) else len(text)]
        blocks.append(dict(label=match[1], line=text.count('\n', 0, match.start())+1,
            sha256=hashlib.sha256(body.encode()).hexdigest(),
            local_labels=len(re.findall(r'^\.[A-Za-z_0-9]+', body, re.M)),
            branch_sites=len(re.findall(r'^\s*(?:jr|jp|jmp|ret|call)\s+(?:z|nz|c|nc),?', body, re.M)),
            behavior='unverified'))
    return blocks


def build():
    files = authenticated_sources()
    manifest = json.loads(attacks.build()[ROOT/'tools/polished_port/manifests/attack_suite.json'])
    catalog = execution.test_catalog()
    for _, anchors, _ in DOMAINS.values():
        assert all(p in files for p in anchors), [p for p in anchors if p not in files]
    speed = files['engine/battle/core.asm']['text'].split('GetSpeed::', 1)[1].split('\n.paralyze_done', 1)[0]
    assert 'cp QUICK_FEET' in speed and 'ln a, 1, 2' in speed
    umbrella = files['home/battle.asm']['text'].split('GetWeatherAfterUserUmbrella::', 1)[1].split('GetWeatherAfterCloudNine::', 1)[0]
    assert 'cp WEATHER_HAIL\n\tret z\n\tcp WEATHER_SANDSTORM\n\tret z' in umbrella
    rows = [dict(path=p, git_blob_sha1=record['sha'], blocks=source_blocks(record['text'])) for p, record in sorted(files.items())]
    abilities = re.findall(r'^\s*const ([A-Z][A-Z_0-9]*)', files['constants/ability_constants.asm']['text'], re.M)
    holds = re.findall(r'^\s*const (HELD_[A-Z_0-9]+)', files['constants/item_data_constants.asm']['text'], re.M)
    assert len(abilities) > 100 and len(holds) > 50
    report = dict(source_repository='Rangi42/polishedcrystal', source_commit=source.PIN,
        source_variant='default release, FAITHFUL undefined; lexical inventory retains alternate branches as a conservative superset',
        complete_behavior_parity=False,
        suite2_reconciliation=manifest['suite2_reconciliation'],
        original_shared_obligations=manifest['original_shared_obligations'],
        shared_open_obligations=manifest['shared_open_obligations'],
        inventory_status_interpretation='Domain status, lexical block behavior and per-effect flags remain conservative whole-battle certification fields. Domain obligations retain their original inventory scope. Reviewed Suite 2 mechanical branches are reconciled in the closure review and validation record, while original Suite 3 contexts, AI, exact RNG and presentation remain open; the unchanged inventory flags do not mean every reviewed mechanical branch is unexamined.',
        inventory_scope=dict(directory_prefixes=list(PREFIXES), additional_files=list(EXTRA),
            limitation='This is the battle boundary and shared dependencies, not a proof of transitive call-graph closure. External field scripts, UI and data references still require end-to-end evidence.'),
        domains=[dict(id=key, title=title, source_anchors=anchors, status='open', obligations=obligations) for key, (title, anchors, obligations) in DOMAINS.items()],
        source_files=rows, source_abilities=abilities, source_held_effects=holds,
        source_moves=len(manifest['moves']), source_effects=len(manifest['effects']),
        source_commands=sorted({c for e in manifest['effects'] for c in (line.split()[0] for line in e['commands'])}),
        unverified_effects=[e['effect'] for e in manifest['effects'] if not e['complete_behavior_parity']],
        regression_tests=catalog,
        semantic_evidence_policy='Existing tests establish only their asserted scenarios. Names, dispatch, inventory, code presence, native passes and AI frame budgets do not close a source block or domain. Suite 2 implementation reconciliation additionally records authenticated branch review, runtime bindings and distinguishing evidence in its linked ledger; this does not certify every whole-battle context or the documented reachable deviation.',
        scoped_evidence=[dict(manifest=str(p.relative_to(ROOT)), sha256=hashlib.sha256(p.read_bytes()).hexdigest())
            for p in sorted((ROOT/'tools/polished_port/manifests').glob('*.json'))
            if p.name in ('attack_suite.json', 'multihit.json', 'contact_status.json', 'contact_volatile.json', 'battle_foundations.json', 'battle_core.json', 'accuracy_order.json', 'battle_systems.json', 'trainer_attributes.json', 'action_resolution.json')])
    return {MANIFEST: json.dumps(report, indent=2)+'\n', **foundations.build()}


def require_complete(report):
    assert {d['id'] for d in report['domains']} == set(DOMAINS), 'Missing battle domain'
    assert len(report['source_files']) == len(authenticated_sources()), 'Missing source files'
    pending = [d['id'] for d in report['domains'] if d['status'] != 'verified']
    blocks = sum(b['behavior'] != 'verified' for f in report['source_files'] for b in f['blocks'])
    assert not pending and not blocks and not report['unverified_effects'], f'Parity remains open: {len(pending)} domains, {blocks} source blocks awaiting behavioral review, {len(report["unverified_effects"])} effect families'


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--import-source', type=Path)
    parser.add_argument('--check', action='store_true')
    parser.add_argument('--strict-completeness', action='store_true')
    args = parser.parse_args()
    if args.import_source:
        import_source(args.import_source)
    outputs = build()
    for path, content in outputs.items():
        if args.check:
            assert path.read_text() == content, f'Regenerate {path}'
        else:
            path.write_text(content)
    report = json.loads(outputs[MANIFEST])
    print(f'PASS: {len(report["source_files"])} authenticated source files; {len(report["domains"])} battle domains; {len(report["regression_tests"])} registered test groups')
    print('Reviewed Suite 2 implementation: reconciled; whole-battle inventory/strict completeness remains OPEN. See SUITE2_CLOSURE_REVIEW.md and SUITE2_VALIDATION.json.')
    if args.strict_completeness:
        try:
            require_complete(report)
        except AssertionError as error:
            print(error)
            raise SystemExit(2)
