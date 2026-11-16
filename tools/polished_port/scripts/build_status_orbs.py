#!/usr/bin/env python3
"""Verify pinned Toxic/Flame Orb status gates and end-turn activation order."""
import argparse
import json

import build_pokemon_learnsets as source


def build():
    files = source.sources()
    endturn = files['engine/battle/endturn.asm']
    orb = endturn.split('\nHandleStatusOrbs:', 1)[1].split('\nHandleRoost:', 1)[0]
    assert [line for line in source.default_lines(orb) if line] == [
        'call HasOpponentFainted', 'ret z', 'ld a, EFFECTIVE', 'ld [wTypeModifier], a',
        'farcall GetOpponentItemAfterUnnerve', 'ld a, b', 'cp HELD_SELF_BRN',
        'ld b, 1 << BRN', 'jr z, .burn', 'cp HELD_SELF_PSN',
        'ld b, 1 << PSN | 1 << TOX', 'ret nz', 'push bc', 'ld b, 2',
        'farcall CanPoisonTarget', 'jr .do_status', '.burn', 'push bc', 'ld b, 2',
        'farcall CanBurnTarget', '.do_status', 'pop bc', 'ret nz', 'farjp StatusTarget']
    block = endturn.split('\nHandleEndturnBlockB:', 1)[1].split('\nHandleWeather:', 1)[0]
    assert [line for line in source.default_lines(block) if line] == [
        'call SetFastestTurn', 'call .do_it', 'call SwitchTurn', '.do_it',
        'farcall EndturnAbilitiesB', 'call SwitchTurn', 'call HandleStatusOrbs', 'jmp SwitchTurn']
    between = endturn.split('\nHandleBetweenTurnEffects:', 1)[1].split('\n.endturn_loop\n', 1)[0]
    assert between.index('call HandlePoison') < between.index('call HandleEndturnBlockB')
    assert between.index('call HandleBurn') < between.index('call HandleEndturnBlockB')
    commands = files['engine/battle/effect_commands.asm']
    poison = commands.split('\nCanPoisonTarget:', 1)[1].split('\nCanParalyzeTarget:', 1)[0]
    assert 'lb bc, POISON, STEEL\n\tlb de, IMMUNITY, HELD_PREVENT_POISON' in poison
    assert 'lb bc, FIRE, FIRE\n\tlb de, WATER_VEIL, HELD_PREVENT_BURN' in poison
    status = commands.split('\nCanStatusTarget:', 1)[1].split('\nBattleCommand_draintarget:', 1)[0]
    assert 'cp 2\n\tjr z, .no_safeguard\n\tcall SafeCheckSafeguard' in status
    no_sub = status.split('\n.no_safeguard\n', 1)[1].split('\n.got_ability\n', 1)[0]
    assert [line for line in source.default_lines(no_sub) if line] == [
        'pop af', 'jr nz, .no_mold_breaker', 'call CheckSubstituteOpp',
        'ld hl, ButItFailedText', 'jr nz, .end', 'call GetOpponentAbilityAfterMoldBreaker',
        'jr .got_ability', '.no_mold_breaker', 'call GetOpponentAbility']
    corrosion = status.split('; Corrosion bypasses everything except existing status problems\n', 1)[1].split('\n.check_psn_status_move\n', 1)[0]
    assert [line for line in source.default_lines(corrosion) if line] == [
        'ld a, h', 'cp 1 << PSN', 'jr nz, .not_corrosive', 'pop af', 'push af',
        'jr z, .check_psn_status_move', 'dec a', 'jr z, .not_corrosive',
        'call GetOpponentAbility', 'jr .check_corrosion']
    assert '.check_corrosion\n\tcp CORROSION\n\tjr nz, .not_corrosive\n\tpop af\n\tjr .immunities_done' in status
    assert '.immunities_done\n\tld a, BATTLE_VARS_STATUS_OPP\n\tcall GetBattleVar\n\tand a\n\tld hl, ButItFailedText\n\tret' in status
    assert 'cp PASTEL_VEIL\n\tld e, IMMUNITY' in status
    assert 'call DoLeafGuardCheck\n\tjr z, .cant_ability' in status
    attrs = files['data/items/attributes.asm']
    assert '; FLAME ORB\n\titem_attribute 4000, HELD_SELF_BRN, 0, ITEM, ITEMMENU_NOUSE, ITEMMENU_NOUSE' in attrs
    assert '; TOXIC ORB\n\titem_attribute 4000, HELD_SELF_PSN, 0, ITEM, ITEMMENU_NOUSE, ITEMMENU_NOUSE' in attrs
    report = dict(source_repository='Rangi42/polishedcrystal', source_commit=source.PIN,
        source_variant='default (FAITHFUL undefined)', profile='B_POLISHED_MOVE_BALANCE',
        scope='Toxic Orb and Flame Orb Safeguard exemption in singles without either battler using an active gimmick.',
        items={'TOXIC_ORB': {'held_effect': 'HELD_SELF_PSN', 'status': 'bad poison'},
               'FLAME_ORB': {'held_effect': 'HELD_SELF_BRN', 'status': 'burn'}},
        rules=['HandleStatusOrbs checks the living holder, clears residual move ineffectiveness and uses CanStatusTarget mode 2.',
               'Mode 2 skips Safeguard, Substitute and Mold Breaker checks while retaining the holder type, actual ability and existing-status gates.',
               'Only the Toxic Orb holder own Corrosion bypasses Poison/Steel immunity; the opposing ability cannot supply that exception, and Fire immunity to Flame Orb remains.',
               'The handler does not consume the orb. It runs after poison/burn damage for that turn, so newly inflicted status first damages on the next turn.'],
        adapter='Reuse one private status eligibility routine with an explicit Safeguard exception. Ordinary move/ability/AI queries always use the normal gate; only the two held-orb callers request the scoped exception. Native type, ability, status, item suppression and activation handlers remain in place.',
        gaps=['This verifies the scoped activation gates, item retention and first residual-damage timing, not all orb item definitions, icons, broader end-turn ordering, damage rounding, status/Synchronize behavior or presentation.',
              'Doubles, active gimmicks, optional item suppression and non-source ability/terrain barriers retain native paths. Broader optional combinations remain separate audits.'])
    return {source.ROOT/'tools/polished_port/manifests/status_orbs.json': json.dumps(report, indent=2)+'\n'}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    for path, expected in build().items():
        if args.check:
            assert path.read_text() == expected, f'Regenerate {path}'
        else:
            path.write_text(expected)
    print('PASS: pinned Toxic/Flame Orb mode-2 status gates, holder Corrosion and activation timing')
