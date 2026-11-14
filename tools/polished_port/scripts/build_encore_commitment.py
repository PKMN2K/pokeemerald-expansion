#!/usr/bin/env python3
"""Verify Encore's source pending-move commitment across a Mental Herb cure."""
import argparse
import json
import re

import build_pokemon_learnsets as source


def build():
    files = source.sources()
    command = files['engine/battle/move_effects/encore_disable.asm']
    write = 'ld a, BATTLE_VARS_MOVE_OPP\n\tcall GetBattleVarAddr\n\tld a, BATTLE_VARS_LAST_COUNTER_MOVE_OPP\n\tcall GetBattleVar\n\tld [hl], a'
    assert write in command
    assert command.index(write) < command.index('jmp CheckOpponentMentalHerb')
    assert command.index('call UserKnowsMove\n\tret nz') < command.index(write)
    assert command.index('and $3f\n\tjr z, .failed') < command.index(write)
    variables = files['home/battle_vars.asm']
    assert '.CurMoveOpp:     db ENEMY_CUR_MOVE,        PLAYER_CUR_MOVE' in variables
    assert 'dw wCurPlayerMove,                wCurEnemyMove' in variables
    herb = files['engine/battle/move_effects/attract.asm'].split('\nCheckMentalHerb:', 1)[1].split('\nCheckOppositeGender:', 1)[0]
    assert 'predef GetUserItemAfterUnnerve' in herb
    assert 'cp HELD_MENTAL_HERB\n\tret nz' in herb
    assert 'and $f0\n\tld [hl], a' in herb  # Clear Encore, retain Choice's slot.
    assert 'jmp ConsumeUserItem' in herb
    assert not any(name in herb for name in ('BATTLE_VARS_MOVE', 'wCurPlayerMove', 'wCurEnemyMove'))
    commands = files['engine/battle/effect_commands.asm']
    turn = commands.split('\nDoTurn:', 1)[1].split('\nDoMove:', 1)[0]
    assert 'call CheckTurn' in turn and 'call UpdateMoveData' in turn
    assert turn.index('call CheckTurn') < turn.index('call UpdateMoveData')
    update = commands.split('\nUpdateMoveData:', 1)[1].split('\nIsOpponentLeafGuardActive:', 1)[0]
    assert 'ld a, BATTLE_VARS_MOVE\n\tcall GetBattleVar' in update
    assert 'call UserKnowsMove' in update
    assert 'ld hl, wCurMoveNum' in update and 'ld hl, wCurEnemyMoveNum' in update
    assert '.got_move_num\n\tld [hl], c' in update
    pp = commands.split('\nBattleConsumePP:', 1)[1].split('\n.end', 1)[0]
    assert 'ld bc, wCurMoveNum' in pp and 'ld bc, wCurEnemyMoveNum' in pp
    assert 'ld a, [bc]\n\tld c, a' in pp and '\tdec [hl]' in pp
    unnerve = commands.split('\nGetUserItemAfterUnnerve::', 1)[1].split('\nNoItem:', 1)[0]
    assert 'cp UNNERVE\n\tret nz' in unnerve
    assert 'ld hl, EdibleBerries\n\tcall IsInByteArray' in unnerve and '\tret nc' in unnerve
    berries = re.findall(r'^\s*db (\w+)', files['data/items/edible_berries.asm'], re.M)
    assert len(berries) == 22 and 'MENTAL_HERB' not in berries
    report = dict(source_repository='Rangi42/polishedcrystal', source_commit=source.PIN,
        source_variant='default (FAITHFUL undefined)', profile='B_POLISHED_MOVE_BALANCE',
        scope='Ordinary singles Encore pending move after a Mental Herb cure, correct move-slot PP, subsequent free selection and Mental Herb immunity to Unnerve.',
        rules=dict(encore_writes_pending_move_before_cure=True,
                   cure_clears_encore_without_restoring_pending_move=True,
                   move_execution_updates_pp_slot_from_pending_move=True,
                   unnerve_blocks_mental_herb=False),
        adapter='Commit an unacted ordinary move action to the validated Encore slot before item resolution. Existing action selection replaces that slot on the next turn; no new saved or volatile fields.',
        gaps=['Full called-move eligibility/history, disobedience, charging/recharge and priority/order edge cases remain separate audits.',
              'Mental Herb cure ordering with simultaneous conditions, full item/ability suppression, and source presentation remain separate audits.',
              'Expansion-only formats and gimmicks retain their native framework; pending selection is only rewritten for ordinary move actions without an active gimmick.'])
    return {source.ROOT/'tools/polished_port/manifests/encore_commitment.json': json.dumps(report, indent=2)+'\n'}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    for path, expected in build().items():
        if args.check:
            assert path.read_text() == expected, f'Regenerate {path}'
        else:
            path.write_text(expected)
    print('PASS: source Encore pending move, Mental Herb cure, PP slot and Unnerve contract')
