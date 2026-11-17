#!/usr/bin/env python3
"""Pin the secondary-status move pool that returns through source Synchronize."""
import argparse
import json
import re

import build_pokemon_learnsets as source
import build_synchronize_substitute as direct
import build_magic_bounce as reflection


def build():
    # Authenticate the reflector's direct-status mode, Substitute gate and healing order.
    direct.build()
    files = source.sources()
    parts = re.split(r'^([A-Za-z][A-Za-z0-9_]*):[^\n]*\n', files['data/moves/effects.asm'], flags=re.M)
    scripts = dict(zip(parts[1::2], parts[2::2], strict=True))
    statuses = {'poisontarget': ['POISON'], 'burntarget': ['BURN'],
                'paralyzetarget': ['PARALYSIS'], 'tristatuschance': ['BURN', 'PARALYSIS']}
    rows = [[v.strip() for v in line[5:].split(',')] for line in
            source.default_lines(files['data/moves/moves.asm']) if line.startswith('move ')]
    pool = reflection.reflection_pool(files)
    selected = []
    base = ['checkobedience', 'usedmovetext', 'doturn', 'hastarget', 'checkhit',
            'checkpriority', 'critical', 'damagestats', 'damagecalc', 'stab',
            'damagevariation', 'moveanim', 'failuretext', 'applydamage',
            'criticaltext', 'supereffectivetext', 'postfainteffects', 'effectchance']
    for identity, row, entry in zip(source.move_identities(files)[1:], rows, pool, strict=True):
        assert identity['source_move'] == row[0] == entry['source_move']
        commands = [line for line in source.default_lines(scripts[entry['source_script']]) if line]
        status_commands = [command for command in commands if command in statuses]
        if not status_commands:
            continue
        assert len(status_commands) == 1 and int(row[2]) > 0 and row[7] != 'STATUS'
        command = status_commands[0]
        expected = base + [command, 'posthiteffects', 'endmove']
        if entry['source_script'] == 'BodySlam':
            expected.insert(expected.index('moveanim'), 'doubleminimizedamage')
        elif entry['source_script'] == 'Thunder':
            expected.insert(expected.index('moveanim'), 'thunderaccuracy')
        elif entry['source_script'] == 'FlareBlitz':
            expected.insert(expected.index('postfainteffects'), 'recoil')
            expected.append('endmove')  # Preserve the pinned source's trailing hotfix command.
        assert commands == expected, (entry['source_script'], commands)
        selected.append(dict(**identity, source_effect=row[1], source_script=entry['source_script'],
                             source_type=row[3], secondary_percent=int(row[6]),
                             reflected_statuses=statuses[command]))
    assert [row['source_move'] for row in selected] == [
        'FIRE_PUNCH', 'THUNDERPUNCH', 'FLARE_BLITZ', 'BODY_SLAM', 'POISON_STING',
        'EMBER', 'FLAMETHROWER', 'THUNDERSHOCK', 'THUNDERBOLT', 'THUNDER', 'SCALD',
        'LICK', 'GUNK_SHOT', 'FIRE_BLAST', 'POISON_JAB', 'TRI_ATTACK', 'SLUDGE_BOMB',
        'ZAP_CANNON', 'SPARK', 'SACRED_FIRE', 'DRAGONBREATH']
    commands = files['engine/battle/effect_commands.asm']
    for label, following, can, status in [
        ('BurnTarget', 'BattleCommand_poisontarget', 'CanBurnTarget', 'BRN'),
        ('BattleCommand_poisontarget', 'BattleCommand_paralyzetarget', 'CanPoisonTarget', 'PSN'),
        ('BattleCommand_paralyzetarget', 'BattleCommand_freezetarget', 'CanParalyzeTarget', 'PAR'),
    ]:
        body = commands.split('\n'+label+':', 1)[1].split('\n'+following+':', 1)[0]
        assert [line for line in source.default_lines(body) if line] == [
            'ld b, 0', 'call '+can, 'ret nz', 'ld b, 1 << '+status, 'jr StatusTarget']
    body = commands.split('\nStatusTarget:', 1)[1].split('\nDefrostOpponent:', 1)[0]
    assert [line for line in source.default_lines(body) if line] == [
        'ld a, [wTypeModifier]', 'and a', 'ret z', 'ld a, [wEffectFailed]', 'and a', 'ret nz',
        'DisplayAndSetStatusProblem:', 'xor a', 'ld [wBattleAnimParam], a',
        'ld a, BATTLE_VARS_STATUS_OPP', 'call GetBattleVarAddr', 'ld [hl], b',
        'call UpdateOpponentInParty', 'call DisplayStatusProblem', 'call RefreshBattleHuds',
        'jmp PostStatusWithSynchronize']
    assert [line for line in source.default_lines(files['engine/battle/move_effects/tri_attack.asm']) if line] == [
        'BattleCommand_tristatuschance:', 'ld a, 3', 'call BattleRandomRange', 'call StackJumpTable',
        '.Jumptable:', 'dw BattleCommand_paralyzetarget', 'dw BattleCommand_freezetarget', 'dw BurnTarget']
    report = dict(source_repository='Rangi42/polishedcrystal', source_commit=source.PIN,
        source_variant='default (FAITHFUL undefined)', profile='B_POLISHED_MOVE_BALANCE',
        scope='Synchronize returns from the 21 source secondary-status moves with original battle type in singles without active gimmicks.',
        moves=selected,
        rules=['Successful secondary burn, poison and paralysis pass through StatusTarget and PostStatusWithSynchronize after damage.',
               'Synchronize returns those statuses through the direct status gate, where the original attacker Substitute blocks them.',
               'Tri Attack burn and paralysis return through Synchronize; its freeze outcome does not activate Synchronize.',
               'The original attacker Infiltrator or Corrosion does not become the reflector ability.',
               'The return is checked before the Synchronize holder healing berry; a protected attacker retains its berry.'],
        adapter='Extend the source Synchronize Substitute gate with generated move/type/status membership. Carry the status trigger into activation so contact-ability effects during the same move remain outside this adapter.',
        gaps=['This covers the Substitute return gate, not full secondary-effect RNG, status eligibility, damage/recoil order, animations or AI parity.',
              'Contact-ability status sources, optional moves, dynamic retyping, Magic Coat, doubles and active gimmicks retain their existing paths.'])
    header = f'/* Generated by build_synchronize_hit.py from Polished Crystal {source.PIN}. */\n'
    header += 'static const struct { u8 sourceTypePlusOne; u8 statuses; } sPolishedSynchronizeHitRules[MOVES_COUNT] = {\n'
    for row in selected:
        mask = ' | '.join('STATUS1_'+status for status in row['reflected_statuses'])
        header += f"    [{row['target_move']}] = {{TYPE_{row['source_type']} + 1, {mask}}},\n"
    header += '};\n'
    return {source.ROOT/'src/data/polished_synchronize_hit.h': header,
            source.ROOT/'tools/polished_port/manifests/synchronize_hit.json': json.dumps(report, indent=2)+'\n'}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    for path, expected in build().items():
        if args.check:
            assert path.read_text() == expected, f'Regenerate {path}'
        else:
            path.write_text(expected)
    print('PASS: 21 pinned secondary-status moves retain Substitute protection against Synchronize')
