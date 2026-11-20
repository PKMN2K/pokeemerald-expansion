#!/usr/bin/env python3
"""Inventory every pinned attack; generate source-derived battle test vectors.

Dispatch coverage is deliberately separate from semantic coverage. Neither a
move's name nor an Expansion test is evidence of complete source parity.
"""
import argparse
import collections
import json
import re

import build_pokemon_learnsets as source

STATS = {'ATTACK': 'STAT_ATK', 'DEFENSE': 'STAT_DEF', 'SPEED': 'STAT_SPEED',
         'SP_ATTACK': 'STAT_SPATK', 'SP_DEFENSE': 'STAT_SPDEF',
         'ACCURACY': 'STAT_ACC', 'EVASION': 'STAT_EVASION'}
DIRECT = {'EFFECT_NORMAL_HIT', 'EFFECT_ALWAYS_HIT', 'EFFECT_PRIORITY_HIT',
          'EFFECT_BURN_HIT', 'EFFECT_FREEZE_HIT', 'EFFECT_PARALYZE_HIT',
          'EFFECT_FLINCH_HIT', 'EFFECT_POISON_HIT', 'EFFECT_CONFUSE_HIT',
          'EFFECT_SP_DEF_DOWN_HIT', 'EFFECT_SPEED_DOWN_HIT', 'EFFECT_ATTACK_DOWN_HIT',
          'EFFECT_ACCURACY_DOWN_HIT', 'EFFECT_DEFENSE_DOWN_HIT', 'EFFECT_SP_ATK_DOWN_HIT',
          'EFFECT_ATTACK_UP_HIT', 'EFFECT_DEFENSE_UP_HIT', 'EFFECT_SPEED_UP_HIT',
          'EFFECT_ALL_UP_HIT', 'EFFECT_RECOIL_HIT', 'EFFECT_LEECH_HIT',
          'EFFECT_STOMP', 'EFFECT_GUST', 'EFFECT_BODY_SLAM', 'EFFECT_EARTHQUAKE',
          'EFFECT_FLARE_BLITZ', 'EFFECT_SACRED_FIRE', 'EFFECT_CLOSE_COMBAT',
          'EFFECT_PSYSTRIKE', 'EFFECT_BRICK_BREAK', 'EFFECT_PAY_DAY', 'EFFECT_TRI_ATTACK'}


def inventory():
    files = source.sources()
    effects = [l.split()[1] for l in source.default_lines(files['constants/move_effect_constants.asm'])
               if l.startswith('const EFFECT_')]
    pointers = [l.split()[1] for l in source.default_lines(files['data/moves/effects_pointers.asm'])
                if l.startswith('dw ')]
    mapping = dict(zip(effects, pointers, strict=True))
    lines = list(source.default_lines(files['data/moves/effects.asm']))
    scripts = {}
    for effect, label in mapping.items():
        start = lines.index(label + ':') + 1
        commands = []
        for line in lines[start:]:
            if not line or line.endswith(':'):
                continue
            assert re.fullmatch(r'[a-z]+(?: [A-Z0-9_$ |]+)?', line), (label, line)
            commands.append(line)
            if line == 'endmove':
                break
        assert commands[-1] == 'endmove', label
        scripts[effect] = dict(label=label, commands=commands)
    rows = [[v.strip() for v in l[5:].split(',')] for l in
            source.default_lines(files['data/moves/moves.asm']) if l.startswith('move ')]
    moves = []
    sounds = {l.split()[1] for l in source.default_lines(files['data/moves/sound_moves.asm']) if l.startswith('db ') and l != 'db -1'}
    for identity, row in zip(source.move_identities(files)[1:], rows, strict=True):
        assert identity['source_move'] == row[0]
        effect = row[1]
        changes = []
        for command in scripts[effect]['commands']:
            match = re.fullmatch(r'(forceraisestat|forcelowerstat|loweroppstat) (\$10 \| )?(\w+)', command)
            if match:
                changes.append(dict(stat=STATS[match[3]], stages=(2 if match[2] else 1)
                                    * (-1 if 'lower' in match[1] else 1),
                                    target='opponent' if match[1] == 'loweroppstat' else 'user'))
        # Swagger also confuses; it is not a simple stat-vector test.
        if effect == 'EFFECT_SWAGGER':
            changes = []
        scopes = ['single-turn dispatch and PP on both sides']
        if effect in DIRECT:
            scopes.append('unblocked damage, Protect and Substitute on both sides')
            scopes.append('live contact classification against Flame Body; five contact-status ability interactions are scoped in contact_status.json')
            scopes.append('live contact classification against Cute Charm, Tangling Hair and Perish Body; volatile, stat, faint and countdown interactions are scoped in contact_volatile.json')
        if changes:
            scopes.append('explicit source stat deltas and saturation on both sides')
        if row[0] in ('RECOVER', 'ROOST', 'FRESH_SNACK'):
            scopes.append('half-HP rounding and full-HP rejection')
        if row[0] in ('TAKE_DOWN', 'DOUBLE_EDGE', 'BRAVE_BIRD', 'WILD_CHARGE', 'FLARE_BLITZ'):
            scopes.append('source recoil divisor, rounding and Rock Head/Magic Guard')
        if effect == 'EFFECT_LEECH_HIT':
            scopes.append('source drain rounding, Big Root order and Liquid Ooze')
        if effect in ('EFFECT_MULTI_HIT', 'EFFECT_DOUBLE_HIT'):
            scopes.append('source hit-count slots/probabilities, Skill Link/Loaded Dice, PP, initial rejection, sleep during the loop, faint interruption and per-hit Substitute/damage reactions; see multihit.json for limits')
        moves.append(dict(**identity, source_effect=effect, source_category=row[7],
                          script=scripts[effect]['label'], stat_changes=changes, sound=row[0] in sounds,
                          suite_scopes=scopes, complete_behavior_parity=False))
    assert len(moves) == 255 and len({m['source_effect'] for m in moves}) == 133
    return files, moves, scripts


def build():
    files, moves, scripts = inventory()
    commands = files['engine/battle/effect_commands.asm']
    recoil = commands.split('BattleCommand_recoil:\n', 1)[1].split('BattleCommand_confusetarget:', 1)[0]
    assert 'cp DOUBLE_EDGE\n\tjr z, .OneThirdRecoil' in recoil
    assert 'cp FLARE_BLITZ\n\tjr z, .OneThirdRecoil' in recoil
    assert 'BRAVE_BIRD' not in recoil and 'call HalveBC\n\tcall HalveBC' in recoil
    assert 'cp ROCK_HEAD\n\tret z' in recoil and 'cp MAGIC_GUARD\n\tret z' in recoil
    heal = commands.split('BattleCommand_heal:\n', 1)[1].split('BattleSideCopy:', 1)[0]
    assert '.not_rest\n\tcall GetHalfMaxHP' in heal
    assert 'HalveBC::\n\tsrl b\n\trr c\nFloorBC::' in files['home/battle.asm']
    drain = commands.split('SapHealth:\n', 1)[1].split('BattleCommand_burntarget:', 1)[0]
    assert 'cp DRAINING_KISS' in drain and 'srl b\n\trr c\n\tadd hl, bc' in drain
    assert 'call HandleBigRoot\n\tjmp HalveBC' in drain
    assert 'ld [hl], 13' in drain and 'ld [hl], 10' in drain
    assert {m['source_move'] for m in moves if m['source_effect'] == 'EFFECT_LEECH_HIT'} == {
        'ABSORB', 'MEGA_DRAIN', 'LEECH_LIFE', 'DRAINING_KISS', 'DRAIN_PUNCH', 'GIGA_DRAIN'}
    assert {m['source_move'] for m in moves if m['source_effect'] in ('EFFECT_RECOIL_HIT', 'EFFECT_FLARE_BLITZ')} == {
        'TAKE_DOWN', 'DOUBLE_EDGE', 'BRAVE_BIRD', 'WILD_CHARGE', 'FLARE_BLITZ', 'STRUGGLE'}

    header = ['/* Generated by build_attack_suite.py; source expectations, not target move data. */',
              'struct PolishedAttackVector { enum Move move; u16 sourceId; bool8 directDamage; bool8 sound; };',
              'static const struct PolishedAttackVector sPolishedAttackVectors[] = {']
    for m in moves:
        header.append(f"    {{{m['target_move']}, {m['source_id']}, {'TRUE' if m['source_effect'] in DIRECT else 'FALSE'}, {'TRUE' if m['sound'] else 'FALSE'}}},")
    header += ['};', 'struct PolishedStatVector { enum Move move; bool8 opponent; s8 deltas[NUM_BATTLE_STATS]; };',
               'static const struct PolishedStatVector sPolishedStatVectors[] = {']
    for m in moves:
        if not m['stat_changes']:
            continue
        assert len({c['target'] for c in m['stat_changes']}) == 1
        deltas = ', '.join(f"[{c['stat']}] = {c['stages']}" for c in m['stat_changes'])
        header.append(f"    {{{m['target_move']}, {'TRUE' if m['stat_changes'][0]['target'] == 'opponent' else 'FALSE'}, {{{deltas}}}}},")
    header += ['};', '']
    groups = collections.defaultdict(list)
    for m in moves:
        groups[m['source_effect']].append(m['source_move'])
    report = dict(source_repository='Rangi42/polishedcrystal', source_commit=source.PIN,
                  source_variant='default (FAITHFUL undefined)', moves=moves,
                  effects=[dict(effect=e, moves=names, **scripts[e], complete_behavior_parity=False)
                           for e, names in sorted(groups.items())],
                  shared_open_obligations=[
                      'Complete damage/stat formula, critical hits, type/STAB, accuracy/evasion and RNG distributions.',
                      'Every source effect success/failure branch, prerequisites, durations, cancellation and turn ordering.',
                      'Substitute (including breakage), Protect, immunity, ability/item suppression and fainting order.',
                      'Switching, forced/called/copied moves, multi-turn and multi-hit effects, weather and end-turn state.',
                      'Original battle presentation and source AI decisions.'],
                  arithmetic_scope='Mapped Recover/Roost, six LeechHit moves and five non-Struggle recoil moves in ordinary singles without either active gimmick. Fresh Snack and Struggle retain their earlier adapters. The arithmetic tests do not certify damage inputs, KO/Substitute damage caps, effect order, full-HP Liquid Ooze gates, Dream Eater or AI estimates.',
                  limits='All moves are inventoried and dispatch-tested. Listed semantic scopes and earlier focused suites are partial evidence; no whole effect is certified. Strict completeness must remain failing until all obligations have evidence.')
    return {source.ROOT/'test/battle/move_effect/polished_attack_vectors.h': '\n'.join(header),
            source.ROOT/'tools/polished_port/manifests/attack_suite.json': json.dumps(report, indent=2)+'\n'}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    for path, expected in build().items():
        if args.check:
            assert path.read_text() == expected, f'Regenerate {path}'
        else:
            path.write_text(expected)
    print('PASS: 255 source attacks, 133 effects and ordered scripts inventoried; complete behavior parity remains open')
