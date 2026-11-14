#!/usr/bin/env python3
"""Verify source Cursed Body's shared Disable gate and called-move history."""
import argparse
import json
import re

import build_pokemon_learnsets as source
import build_move_restrictions as restrictions


def build():
    files = source.sources()
    restrictions.build()
    abilities = files['engine/battle/abilities.asm']
    hit = abilities.split('\nRunHitAbilities:', 1)[1].split('\nCursedBodyAbility:', 1)[0]
    assert 'call GetOpponentAbilityAfterMoldBreaker' in hit
    assert hit.index('cp CURSED_BODY\n\tjr z, CursedBodyAbility') < hit.index('call HasUserFainted')
    cursed = abilities.split('\nCursedBodyAbility:', 1)[1].split('\nRunContactAbilities:', 1)[0]
    assert 'call SwitchTurn\n\tfarcall GetFutureSightUser\n\tcall SwitchTurn\n\tret nc' in cursed
    chance = re.search(r'ld a, (\d+)\n\tcall BattleRandomRange\n\tcp (\d+)\n\tret nc', cursed)
    assert chance and (int(chance[2]), int(chance[1])) == (3, 10)
    assert 'farcall BattleCommand_disable\n\tjmp EndAbility' in cursed
    assert 'CURSED_BODY' not in files['data/abilities/mold_breaker_suppressed_abilities.asm']
    future = files['engine/battle/move_effects/future_sight.asm']
    assert '; c|z: Regular user in a (Future Sight not involved)' in future
    assert '.future_sight_offline\n\txor a' in future
    assert future.rstrip().endswith('\tscf\n\tret')
    command = files['engine/battle/move_effects/encore_disable.asm']
    assert command.index('BATTLE_VARS_LAST_COUNTER_MOVE_OPP') < command.index('call UserKnowsMove')
    assert command.index('and $3f\n\tjr z, .failed') < command.index('farcall ShowPotentialAbilityActivation')
    assert 'jmp CheckOpponentMentalHerb' in command
    assert 'ld a, [wInAbility]\n\tand a\n\tret nz' in command
    commands = files['engine/battle/effect_commands.asm']
    post = commands.split('\nBattleCommand_posthiteffects:', 1)[1].split('\n\tfarcall RunHitAbilities', 1)[0]
    assert 'call nz, CheckSubstituteOpp\n\tret nz' in post
    assert 'ld a, [wAttackMissed]\n\tand a\n\tret nz' in post
    # Damage is applied before post-hit abilities. A breaking Substitute clears
    # its active flag immediately; post-hit uses that flag, not CheckSubHit.
    normal = files['data/moves/effects.asm'].split('\nNormalHit:', 1)[1].split('\nSuckerPunch:', 1)[0]
    assert normal.index('\tapplydamage') < normal.index('\tposthiteffects')
    apply = commands.split('\nBattleCommand_applydamage:', 1)[1].split('\nBattleCommand_', 1)[0]
    assert '\tcall TakeDamage' in apply
    damage = commands.split('\nTakeDamage:', 1)[1].split('\nSelfInflictDamageToSubstitute:', 1)[0]
    assert 'bit SUBSTATUS_SUBSTITUTE, a\n\tjr nz, SelfInflictDamageToSubstitute' in damage
    sub_damage = commands.split('\nSelfInflictDamageToSubstitute:', 1)[1].split('\nUpdateMoveData:', 1)[0]
    assert sub_damage.startswith('\n\tcall SetSubHit')
    assert 'sub [hl]\n\tjr z, .broke\n\tjr c, .broke\n\tld [de], a\n\tret' in sub_damage
    broken = sub_damage.split('\n.broke', 1)[1]
    assert broken.index('res SUBSTATUS_SUBSTITUTE, [hl]') < broken.index('ld hl, SubFadedText')
    check_sub = commands.split('\nCheckSubstituteOpp:', 1)[1].split('\nCheckUserMove:', 1)[0]
    assert 'cp INFILTRATOR\n\tret z' in check_sub
    assert 'ld hl, SoundMoves\n\tcall IsInByteArray' in check_sub
    assert 'ld a, BATTLE_VARS_SUBSTATUS4_OPP\n\tcall GetBattleVar\n\tbit SUBSTATUS_SUBSTITUTE, a' in check_sub
    assert 'CheckSubHit' not in post and 'CheckSubHit' not in check_sub
    substitute = files['engine/battle/move_effects/substitute.asm']
    assert '\tcall GetQuarterMaxHP' in substitute
    assert 'predef SubtractHPFromUser' in substitute and 'set SUBSTATUS_SUBSTITUTE, [hl]' in substitute
    assert '\tdb HYPER_VOICE\n' in files['data/moves/sound_moves.asm']
    for name, label in (('metronome', 'metronome'), ('sleep_talk', 'sleeptalk')):
        caller = files[f'engine/battle/move_effects/{name}.asm']
        assert caller.startswith(f'BattleCommand_{label}:\n\tcall ClearLastMove\n')
        assert '\tjmp ResetTurn' in caller
    clear = commands.split('\nClearLastMove:', 1)[1].split('\nResetActorDisable:', 1)[0]
    assert all(x in clear for x in ('BATTLE_VARS_LAST_COUNTER_MOVE', 'BATTLE_VARS_LAST_MOVE', 'xor a\n\tld [hl], a'))
    reset = commands.split('\nResetTurn:', 1)[1].split('\nMoveDisabled:', 1)[0]
    assert reset.startswith('\n\tld a, 1') and '\tld [hl], a' in reset and '\tcall DoMove' in reset
    used = files['engine/battle/used_move_text.asm']
    assert 'dec a\n\tjr z, .charging' in used
    assert used.index('jr z, .charging') < used.index('; update last move') < used.index('\n.charging')
    report = dict(source_repository='Rangi42/polishedcrystal', source_commit=source.PIN,
        source_variant='default (FAITHFUL undefined)', profile='B_POLISHED_MOVE_BALANCE',
        scope='Ordinary singles Cursed Body known-move/remaining-PP gate, called-move history, Substitute breaking/bypass, duration and exits.',
        rules=dict(chance_numerator=3, chance_denominator=10, uses_shared_disable_command=True,
                   requires_known_move_with_remaining_pp=True, sleep_talk_and_metronome_clear_history=True,
                   works_on_holder_knockout=True, suppressed_by_mold_breaker=False,
                   triggers_on_future_sight=False, mental_herb_cures_disable=True,
                   substitute_break_allows_activation=True, intact_substitute_blocks_activation=True,
                   infiltrator_and_sound_bypass_substitute=True),
        adapter='Use the source-profile recorded move and validate its live known slot/PP before applying Disable. Import the existing source five-tick initialization; the post-action tick reduces it to four. Metronome joins Sleep Talk in clearing the source history record at PP deduction and move end. Recognize a damaging Substitute-breaking hit by its exhausted HP before the native delayed flag cleanup; retain direct HP hits and native/gimmick behavior.',
        gaps=['Exact source RNG stream and call ordering are not ported; native hit/ability/item scheduling remains in use.',
              'Other post-hit abilities still use native Substitute gates; this adapter only changes Cursed Body.',
              'All multi-hit, recoil/self-faint, charging/recharge, disobedience, reflected moves, simultaneous cure ordering, other history consumers and original presentation remain separate audits.',
              'Expansion-only move callers, battle formats and gimmicks are not certified as source behavior; active gimmicks retain native Cursed Body selection and duration.'])
    return {source.ROOT/'tools/polished_port/manifests/cursed_body.json': json.dumps(report, indent=2)+'\n'}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    for path, expected in build().items():
        if args.check:
            assert path.read_text() == expected, f'Regenerate {path}'
        else:
            path.write_text(expected)
    print('PASS: source Cursed Body Disable gates, called history, Substitute breaking/bypass, chance and exits')
