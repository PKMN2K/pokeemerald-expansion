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
        scope='Ordinary singles Cursed Body known-move/remaining-PP gate, called-move history, duration and exits.',
        rules=dict(chance_numerator=3, chance_denominator=10, uses_shared_disable_command=True,
                   requires_known_move_with_remaining_pp=True, sleep_talk_and_metronome_clear_history=True,
                   works_on_holder_knockout=True, suppressed_by_mold_breaker=False,
                   triggers_on_future_sight=False, mental_herb_cures_disable=True),
        adapter='Use the source-profile recorded move and validate its live known slot/PP before applying Disable. Import the existing source five-tick initialization; the post-action tick reduces it to four. Metronome joins Sleep Talk in clearing the source history record at PP deduction and move end.',
        gaps=['Exact source RNG stream and call ordering are not ported; native hit/ability/item scheduling remains in use.',
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
    print('PASS: source Cursed Body Disable gates, called history, chance and exits')
