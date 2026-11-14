#!/usr/bin/env python3
"""Pin the source Disable/Encore counter transitions and ordinary exit rules."""
import argparse
import json
import re

import build_pokemon_learnsets as source


def build():
    files = source.sources()
    command = files['engine/battle/move_effects/encore_disable.asm']
    match = re.search(r'cp DISABLE\n\tld hl, WasDisabledText\n\tld a, (\d+)\n\tjr z, \.got_text_and_duration\n\tld hl, GotAnEncoreText\n\tdec a', command)
    assert match
    disable, encore = int(match[1]), int(match[1]) - 1
    assert (disable, encore) == (5, 4)
    assert 'DoEncoreDisable:\n\tld a, [de]\n\tand $f\n\tjr nz, .failed' in command
    assert 'call GetBattleVar\n\tand a\n\tjr z, .failed\n\tcp STRUGGLE\n\tjr z, .failed' in command
    assert 'call UserKnowsMove\n\tret nz' in command
    assert 'and $3f\n\tjr z, .failed' in command
    assert 'ld a, BATTLE_VARS_MOVE_OPP\n\tcall GetBattleVarAddr\n\tld a, BATTLE_VARS_LAST_COUNTER_MOVE_OPP\n\tcall GetBattleVar\n\tld [hl], a' in command
    assert 'jmp CheckOpponentMentalHerb' in command
    end = files['engine/battle/endturn.asm']
    tick = end.split('\nTickDisableAfterMove:', 1)[1].split('\nHandleDisable:', 1)[0]
    assert 'call HasUserFainted\n\tret z' in tick
    assert f'and $f\n\tcp {disable}\n\tret nz\n\tdec [hl]' in tick
    core = files['engine/battle/core.asm']
    perform = core.split('\nPerformMove:', 1)[1].split('\nResolveFaints:', 1)[0]
    assert perform.index('farcall DoTurn') < perform.index('farcall TickDisableAfterMove')
    assert 'call HandleEncore\n\tcall HandleDisable' in end
    assert 'EndturnEncoreDisable:\n\tld a, [hl]\n\tand $f\n\tret z\n\tdec [hl]\n\tld a, [hl]\n\tand $f\n\tret nz' in end
    assert 'and $3f\n\tpop hl\n\tld de, BattleText_UserEncoreEnded\n\tjr z, EndturnEncoreDisable_End' in end
    # Switching clears both counters. Encore preserves the source's separate
    # high-nibble Choice bookkeeping when its own low-nibble timer ends.
    for side in ('Player', 'Enemy'):
        assert f'ld [w{side}DisableCount], a\n\tld [w{side}EncoreCount], a' in core
    herb = files['engine/battle/move_effects/attract.asm']
    assert 'cp HELD_MENTAL_HERB' in herb
    assert 'and $f0\n\tld [hl], a' in herb
    assert 'set 2, b\n\tld [hl], 0' in herb
    header = f'/* Generated from Polished Crystal {source.PIN} by build_move_restrictions.py. */\n'
    header += '#ifndef GUARD_CONSTANTS_POLISHED_MOVE_RESTRICTIONS_H\n#define GUARD_CONSTANTS_POLISHED_MOVE_RESTRICTIONS_H\n\n'
    header += f'#define POLISHED_DISABLE_INITIAL_TICKS {disable}\n#define POLISHED_ENCORE_INITIAL_TICKS {encore}\n\n#endif\n'
    traces = {}
    for acted in (False, True):
        counter = disable
        if not acted:
            counter -= 1  # TickDisableAfterMove, including a blocked action.
        traces['target_already_acted' if acted else 'target_still_to_act'] = {
            'disable_end_of_turn': [max(counter - t - 1, 0) for t in range(5)],
            'encore_end_of_turn': [max(encore - t - 1, 0) for t in range(4)],
        }
    report = dict(source_repository='Rangi42/polishedcrystal', source_commit=source.PIN,
        source_variant='default (FAITHFUL undefined)', profile='B_POLISHED_MOVE_BALANCE',
        disable_initial_ticks=disable, encore_initial_ticks=encore, traces=traces,
        scope='Ordinary singles Disable/Encore duration, action/end-turn ticks, active/no-history/no-PP rejection, Encore PP exhaustion, switching and Mental Herb regressions.',
        rules={'disable_first_action_tick': 'Only 5 -> 4, after the affected battler acts, including failed or interrupted moves; no tick if fainted.',
               'end_turn': 'Decrement each active timer once; clear the restriction at zero.',
               'encore': 'Always starts at 4; no adjustment for whether the target has acted. Clear early if the move has no PP.',
               'active_reapplication': 'Fails without refreshing the counter.'},
        gaps=['Full called-move history/eligibility, disobedience, move replacement and every interaction with other move restrictions remain separate audits.',
              'Mental Herb tests use an already-matching selected move: the source prewritten same-turn move after an Encore cure, and Unnerve interactions, remain outside this timer suite.',
              'Cursed Body application gates, ability/item interactions beyond the tested cases, smart AI and source presentation remain separate audits.',
              'Global source action order, end-turn order, RNG streams and optional Expansion formats/gimmicks are not imported by this timer adapter.'])
    return {source.ROOT/'include/constants/polished_move_restrictions.h': header,
            source.ROOT/'tools/polished_port/manifests/move_restrictions.json': json.dumps(report, indent=2)+'\n'}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    for path, expected in build().items():
        if args.check:
            assert path.read_text() == expected, f'Regenerate {path}'
        else:
            path.write_text(expected)
    print('PASS: pinned Disable/Encore initial counters, post-action tick and end-turn/exit rules')
