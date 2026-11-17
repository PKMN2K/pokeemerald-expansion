#!/usr/bin/env python3
"""Pin Substitute protection against Synchronize after source direct status moves."""
import argparse
import json

import build_pokemon_learnsets as source
import build_corrosion_substitute as corrosion
import build_magic_bounce as reflection


def build():
    corrosion.build()
    files = source.sources()
    commands = {'EFFECT_POISON': 'poison', 'EFFECT_TOXIC': 'toxic',
                'EFFECT_PARALYZE': 'paralyze', 'EFFECT_BURN': 'burn'}
    rows = [[v.strip() for v in line[5:].split(',')] for line in
            source.default_lines(files['data/moves/moves.asm']) if line.startswith('move ')]
    scripts = {row['source_move']: row for row in reflection.reflection_pool(files)}
    selected = []
    for identity, row in zip(source.move_identities(files)[1:], rows, strict=True):
        assert identity['source_move'] == row[0]
        if row[1] in commands:
            assert row[2] == '0' and row[7] == 'STATUS'
            script = scripts[row[0]]['source_script']
            expected = 'DoBurn' if row[1] == 'EFFECT_BURN' else 'DoParalyze' if row[1] == 'EFFECT_PARALYZE' else 'Toxic' if row[1] == 'EFFECT_TOXIC' else 'DoPoison'
            assert script == expected
            selected.append(dict(**identity, source_effect=row[1], source_type=row[3]))
    assert [row['source_move'] for row in selected] == ['POISONPOWDER', 'STUN_SPORE', 'THUNDER_WAVE', 'TOXIC', 'GLARE', 'WILL_O_WISP']
    effects = files['data/moves/effects.asm']
    for label, after, expected in [
        ('DoBurn', 'DoParalyze', ['checkobedience', 'usedmovetext', 'doturn', 'hastarget', 'checkhit', 'checkpriority', 'resettypematchup', 'bounceback', 'stab', 'checksafeguard', 'burn', 'endmove']),
        ('DoParalyze', 'DoSleep', ['checkobedience', 'usedmovetext', 'doturn', 'hastarget', 'checkhit', 'checkpowder', 'checkpriority', 'bounceback', 'checksafeguard', 'paralyze', 'endmove']),
    ]:
        body = effects.split('\n'+label+':\n', 1)[1].split('\n'+after+':', 1)[0]
        assert [line for line in source.default_lines(body) if line] == expected
    abilities = files['engine/battle/abilities.asm']
    sync = abilities.split('\nRunEnemySynchronizeAbility:', 1)[1].split('\nResolveOpponentBerserk_CheckMultihit:', 1)[0]
    assert [line for line in source.default_lines(sync) if line] == [
        'call SwitchTurn', 'call GetTrueUserAbility', 'cp SYNCHRONIZE',
        'call z, SynchronizeAbility', 'jmp SwitchTurn', 'SynchronizeAbility:',
        'ld a, BATTLE_VARS_STATUS', 'call GetBattleVar', 'and 1 << PAR | 1 << BRN | 1 << PSN',
        'ret z', 'call BeginAbility', 'call ShowPotentialAbilityActivation', 'farcall ResetMiss',
        'ld a, BATTLE_VARS_STATUS', 'call GetBattleVar', 'cp 1 << PAR', 'jr z, .is_par',
        'cp 1 << BRN', 'jr z, .is_brn', 'cp 1 << PSN', 'jr z, .is_psn',
        'farcall BattleCommand_toxic', 'jmp EndAbility', '.is_psn', 'farcall BattleCommand_poison',
        'jmp EndAbility', '.is_par', 'farcall BattleCommand_paralyze', 'jmp EndAbility',
        '.is_brn', 'farcall BattleCommand_burn', 'jmp EndAbility']
    begin = abilities.split('\nBeginAbility:', 1)[1].split('\nEndAbility:', 1)[0]
    assert [line for line in source.default_lines(begin) if line] == [
        'ld a, [wInAbility]', 'and a', 'ret nz', 'push hl', 'push de', 'push bc',
        'call LoadTileMapToTempTileMap', 'pop bc', 'pop de', 'pop hl', 'ld a, 1',
        'ld [wInAbility], a', 'ret']
    commands_text = files['engine/battle/effect_commands.asm']
    status_commands = commands_text.split('\n_BattleCommand_poison:', 1)[1].split('\nStatusTargetVerbose:', 1)[0]
    assert [line for line in source.default_lines(status_commands) if line] == [
        'ld hl, CanPoisonTarget', 'jr StatusTargetVerbose', 'BattleCommand_paralyze:',
        'ld a, 1 << PAR', 'ld hl, CanParalyzeTarget', 'jr StatusTargetVerbose',
        'BattleCommand_burn:', 'ld a, 1 << BRN', 'ld hl, CanBurnTarget']
    verbose = commands_text.split('\nStatusTargetVerbose:', 1)[1].split('\n.failed_ineffective\n', 1)[0]
    assert 'ld b, 0\n\tcall _hl_\n\tjr c, .ability_ok\n\tjr nz, .failed' in verbose
    assert 'call PostStatusWithSynchronize' in verbose
    status = commands_text.split('\nCanStatusTarget:', 1)[1].split('\nBattleCommand_draintarget:', 1)[0]
    assert 'pop af ; "and a" was performed earlier.\n\tjr nz, .no_mold_breaker\n\tcall CheckSubstituteOpp\n\tld hl, ButItFailedText\n\tjr nz, .end' in status
    sub = commands_text.split('\nCheckSubstituteOpp:', 1)[1].split('\nCheckUserMove:', 1)[0]
    assert 'call GetTrueUserAbility\n\tcp INFILTRATOR\n\tret z' in sub
    assert 'ld a, [wInAbility]\n\tand a\n\tjr nz, .no_sound_move' in sub
    assert '.no_sound_move\n\tld a, BATTLE_VARS_SUBSTATUS4_OPP\n\tcall GetBattleVar\n\tbit SUBSTATUS_SUBSTITUTE, a\n\tret' in sub
    post = commands_text.split('\nPostStatusWithSynchronize:', 1)[1].split('\nBattleCommand_sleep:', 1)[0]
    assert [line for line in source.default_lines(post) if line] == [
        'farcall RunEnemySynchronizeAbility', 'PostStatus:',
        'farcall UseOpponentHeldStatusHealingItem', 'farjp RunEnemyStatusHealAbilities']
    report = dict(source_repository='Rangi42/polishedcrystal', source_commit=source.PIN,
        source_variant='default (FAITHFUL undefined)', profile='B_POLISHED_MOVE_BALANCE',
        scope='Synchronize returns after the six source direct poison/burn/paralysis moves with original battle type in singles without active gimmicks.',
        moves=selected,
        rules=['Synchronize switches perspective and checks the actual holder ability before returning poison, bad poison, paralysis or burn.',
               'Returned statuses enter StatusTargetVerbose mode zero and its Substitute check.',
               'The reflector has Synchronize, not the original attacker Infiltrator or Corrosion. Ability processing also skips move-based sound bypasses.',
               'The original attacker Substitute remains intact and blocks the returned status; Synchronize activation remains visible.',
               'Synchronize is checked before the holder healing berry. A protected original attacker retains its healing berry.'],
        adapter='Use a generated source move/type table to add the Substitute gate at Synchronize activation. Preserve the existing failed-activation popup and ordinary status eligibility; do not change shared AI/status queries.',
        gaps=['This covers the Substitute return gate for direct source status moves, not full Synchronize behavior after damaging effects, contact abilities or other status sources.',
              'Full status ordering, failure text, source animation/presentation, RNG and AI decisions remain separate audits.',
              'Optional status moves, dynamically retyped moves, Magic Coat returns, doubles and active gimmicks retain native paths.'])
    header = f'/* Generated by build_synchronize_substitute.py from Polished Crystal {source.PIN}. */\n'
    header += '/* Zero excludes a move; original source types are stored plus one. */\n'
    header += 'static const u8 sPolishedSynchronizeStatusTypes[MOVES_COUNT] = {\n'
    header += ''.join(f"    [{row['target_move']}] = TYPE_{row['source_type']} + 1,\n" for row in selected)
    header += '};\n'
    return {source.ROOT/'src/data/polished_synchronize_substitute.h': header,
            source.ROOT/'tools/polished_port/manifests/synchronize_substitute.json': json.dumps(report, indent=2)+'\n'}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    for path, expected in build().items():
        if args.check:
            assert path.read_text() == expected, f'Regenerate {path}'
        else:
            path.write_text(expected)
    print('PASS: six pinned direct status moves retain Substitute protection against Synchronize')
