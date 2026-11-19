#!/usr/bin/env python3
"""Pin direct item/Stench flinches ending recharge regardless of turn order."""
import argparse
import json

import build_pokemon_learnsets as source
import build_flinch_recharge as recharge


def build():
    recharge.build()
    files = source.sources()
    commands = files['engine/battle/effect_commands.asm']
    origin = commands.split('\n.rocky_helmet_done\n', 1)[1].split('\n\t; if we fainted', 1)[0]
    assert recharge.lines(origin) == [
        'call GetTrueUserAbility', 'cp STENCH', 'ld c, 10',
        'jr z, .do_flinch_up', 'call GetUserItem', 'push bc',
        'call GetCurItemName', 'pop bc', 'ld a, b', 'cp HELD_FLINCH_UP',
        '.do_flinch_up', 'call z, .flinch_up']
    direct = commands.split('\n.flinch_up:\n', 1)[1].split('\nCheckStatHerbsAfterIntimidate:', 1)[0]
    assert recharge.lines(direct) == [
        'call HasOpponentFainted', 'ret z',
        'call GetOpponentAbilityAfterMoldBreaker', 'cp SHIELD_DUST', 'ret z',
        'ld b, flinchtarget_command', 'call HasBattleCommand', 'ret z',
        'call GetTrueUserAbility', 'cp SERENE_GRACE', 'jr nz, .no_serene_grace',
        'ld a, c', 'add a', 'ld c, a', '.no_serene_grace',
        'ld a, 100', 'call BattleRandomRange', 'cp c', 'call c, FlinchTarget', 'ret']
    # The direct call bypasses flinchtarget's sleep/freeze and acted-turn gates.
    # FlinchTarget and EndRechargeOpp, and the action-time Inner Focus check,
    # are authenticated by recharge.build().
    items = files['data/items/attributes.asm']
    for name in ["KING'S ROCK", 'RAZOR FANG']:
        assert f'; {name}\n\titem_attribute 5000, HELD_FLINCH_UP, 10, ITEM, ITEMMENU_NOUSE, ITEMMENU_NOUSE' in items
    endturn = files['engine/battle/endturn.asm']
    assert '\tld hl, wPlayerSubStatus4\n\tres SUBSTATUS_FLINCHED, [hl]\n\tld hl, wEnemySubStatus4\n\tres SUBSTATUS_FLINCHED, [hl]' in endturn
    report = dict(source_repository='Rangi42/polishedcrystal', source_commit=source.PIN,
        source_variant='default (FAITHFUL undefined)', profile='B_POLISHED_MOVE_BALANCE',
        scope='Recharge clearing after successful existing King\'s Rock, Razor Fang or Stench procs in ordinary singles without either active gimmick.',
        rules=[
            'Both held items use HELD_FLINCH_UP. Active Stench selects the same direct flinch helper instead of adding an item roll.',
            'The direct helper rejects fainted targets, Shield Dust and moves already containing flinchtarget, then calls FlinchTarget on a successful roll.',
            'Unlike move-effect flinchtarget, the direct helper has no sleep/freeze, Inner Focus or acted-turn gate. FlinchTarget sets flinch and clears recharge even if the target already acted.',
            'Pending actions use the existing active Inner Focus recheck, Steadfast and status ordering. Both sides keep the previously selected move when recharge is cleared before their action.',
            'When the target already acted, clearing newly set recharge permits a fresh choice next turn. The source removes flinch at end turn, so it must not leak into that action.'],
        adapter='Tag only the two existing held-item/Stench effect producers with a transient origin flag. After inherited effect eligibility gates, successful tagged flinches clear pending recharge and set flinch without the move-effect status/turn-order gate. Keep optional Fling and intrinsic move effects on their own paths.',
        gaps=[
            'This adapts successful inherited procs, not full source proc eligibility. Broader gates involving Covert Cloak, broken Substitute, Sheer Force, attacker fainting to Rocky Helmet, multi-hit attacks and optional content remain separate audits.',
            'Existing chance calculations, source RNG stream, general recharge application, complete cancellation order, AI and original presentation are not certified by this suite.',
            'Native-profile battles, doubles and either active gimmick retain native behavior. Optional recharge and damage moves inherit this state adapter without certification of their own effects.'])
    return {source.ROOT/'tools/polished_port/manifests/proc_recharge.json': json.dumps(report, indent=2)+'\n'}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    for path, expected in build().items():
        if args.check:
            assert path.read_text() == expected, f'Regenerate {path}'
        else:
            path.write_text(expected)
    print('PASS: pinned item/Stench procs clear recharge before or after the target acts')
