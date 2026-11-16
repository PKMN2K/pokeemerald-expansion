#!/usr/bin/env python3
"""Verify source Heavy Boots run before Toxic Spikes absorption."""
import argparse
import json

import build_pokemon_learnsets as source


def build():
    files = source.sources()
    core = files['engine/battle/core.asm']
    entry = core.split('\nSpikesDamage_GotAbility:', 1)[1].split('\n.Spikes:', 1)[0]
    assert [line for line in source.default_lines(entry) if line] == [
        'push de', 'push bc', 'call SetParticipant', 'ld d, 0',
        'farcall CheckAirborne_GotAbility', 'pop bc', 'pop de',
        'jr nz, .end_hazards', 'push bc', 'predef GetUserItemAfterUnnerve',
        'ld a, b', 'cp HELD_HEAVY_BOOTS', 'pop bc', 'ret z',
        'ldh a, [hBattleTurn]', 'and a', 'ld hl, wPlayerHazards',
        'jr z, .ok', 'ld hl, wEnemyHazards', '.ok', 'push hl',
        'call .Spikes', 'pop hl', 'call .ToxicSpikes', '.end_hazards',
        'jmp HandleEntryItems']
    forced = core.split('\nSpikesDamage_CheckMoldBreaker:', 1)[1].split('\nSpikesDamage_GotAbility:', 1)[0]
    assert [line for line in source.default_lines(forced) if line] == [
        'call SwitchTurn', 'call GetOpponentAbilityAfterMoldBreaker',
        'ld b, a', 'call SwitchTurn', 'ld c, 0', 'jr SpikesDamage_GotAbility',
        'SpikesDamage:', 'call GetTrueUserAbility', 'ld b, a', 'ld c, 1']
    toxic = core.split('\n.ToxicSpikes:', 1)[1].split('\n.no_poison_type\n', 1)[0]
    assert [line for line in source.default_lines(toxic) if line] == [
        'ld a, [hl]', 'and HAZARDS_TOXIC_SPIKES', 'ret z', 'push af',
        'push bc', 'push hl', 'call CheckIfUserIsPoisonType', 'pop hl',
        'pop bc', 'jr nz, .no_poison_type', 'pop af', 'xor [hl]', 'ld [hl], a', 'ret']
    assert '; HEAVY BOOTS\n\titem_attribute 4000, HELD_HEAVY_BOOTS, 0, ITEM, ITEMMENU_NOUSE, ITEMMENU_NOUSE' in files['data/items/attributes.asm']
    placement = files['engine/battle/move_effects/spikes.asm'].split('\nBattleCommand_toxicspikes:', 1)[1]
    assert [line for line in source.default_lines(placement) if line] == [
        'ldh a, [hBattleTurn]', 'and a', 'ld hl, wEnemyHazards', 'jr z, .got_screens',
        'ld hl, wPlayerHazards', '.got_screens', 'ld a, [hl]', 'and HAZARDS_TOXIC_SPIKES',
        'cp (HAZARDS_TOXIC_SPIKES / 3) * 2', 'jmp z, FailSpikes',
        'ld a, HAZARDS_TOXIC_SPIKES / 3', 'add [hl]', 'ld [hl], a',
        'call AnimateCurrentMove', 'ld hl, ToxicSpikesText', 'jmp StdBattleTextbox',
        '.failed', 'jmp FailSpikes']
    report = dict(source_repository='Rangi42/polishedcrystal', source_commit=source.PIN,
        source_variant='default (FAITHFUL undefined)', profile='B_POLISHED_MOVE_BALANCE',
        scope='Effective Heavy Boots on entry in singles without either active battler using a gimmick.',
        item={'source_item': 'HEAVY_BOOTS', 'source_hold_effect': 'HELD_HEAVY_BOOTS',
              'target_item': 'ITEM_HEAVY_DUTY_BOOTS', 'target_hold_effect': 'HOLD_EFFECT_HEAVY_DUTY_BOOTS'},
        rules=['The shared hazard entry checks airborne state, then the effective held item, before calling Spikes or Toxic Spikes.',
               'Heavy Boots return before the Poison-type absorption branch, preserving all Toxic Spikes layers.',
               'Ordinary and forced entry both reach the same boots gate.',
               'Without effective boots, grounded Poison types clear Toxic Spikes; airborne Poison types do not.',
               'Toxic Spikes placement caps at two layers; layers preserved by boots remain active for later susceptible entrants.'],
        adapter='Add the scoped boots gate before Poison-type absorption in the switch-in execution path. Retain effective-item suppression, item retention and hazard counter advancement; leave shared hazard/AI queries unchanged.',
        gaps=['This covers the boots/absorption order and selected entry controls, not all source hazard grounding, Mold Breaker interactions, status immunity, rounding, entry-item order, AI decisions or presentation.',
              'The source item identity is mapped to the existing target held effect. Full source name, price, description, icon and inventory migration remain separate work.',
              'Doubles and active gimmicks retain native absorption; optional Klutz, Magic Room and Knock Off controls exercise existing item suppression/removal paths.'])
    return {source.ROOT/'tools/polished_port/manifests/heavy_boots.json': json.dumps(report, indent=2)+'\n'}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    for path, expected in build().items():
        if args.check:
            assert path.read_text() == expected, f'Regenerate {path}'
        else:
            path.write_text(expected)
    print('PASS: pinned Heavy Boots gate precedes Toxic Spikes absorption for ordinary and forced entry')
