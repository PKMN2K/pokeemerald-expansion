#!/usr/bin/env python3
"""Import the pinned Protect/Endure successive-use probability and counter cap."""
import argparse
import json
import re

import build_pokemon_learnsets as source


def build():
    files = source.sources()
    protect = files['engine/battle/move_effects/protect.asm']
    endure = files['engine/battle/move_effects/endure.asm']
    assert 'call ProtectChance\n\tret nz' in protect and 'call ProtectChance\n\tret nz' in endure
    assert 'set SUBSTATUS_PROTECT, [hl]' in protect and 'set SUBSTATUS_ENDURE, [hl]' in endure
    chance = protect.split('\nProtectChance:', 1)[1]
    assert 'call CheckOpponentWentFirst\n\tjr nz, .failed' in chance
    cap = re.search(r'ld a, \[de\]\n\tinc a\n\tcp (\d+)\n\tjr z, \.no_overflow\n\tld \[de\], a', chance)
    roll = re.search(r'ld a, (\d+)\n\tcall BattleRandomRange\n\tand a\n\tjr z, \.loop', chance)
    assert cap and roll
    cap, base = int(cap[1]) - 1, int(roll[1])
    assert (cap, base) == (6, 3)
    assert '.no_overflow\n\tld c, a\n.loop\n\tdec c\n\tret z' in chance
    assert '.failed\n\txor a\n\tld [de], a' in chance
    core = files['engine/battle/core.asm']
    for side in ('Player', 'Enemy'):
        assert f'ld [w{side}EncoreCount], a\n\tld [w{side}ProtectCount], a' in core
    assert 'cp EFFECT_PROTECT\n\tjr z, .continue_protect\n\tcp EFFECT_ENDURE\n\tjr z, .continue_protect\n\txor a\n\tld [wPlayerProtectCount], a' in core
    assert 'cp EFFECT_PROTECT\n\tret z\n\tcp EFFECT_ENDURE\n\tret z\n\txor a\n\tld [wEnemyProtectCount], a' in core
    commands = files['engine/battle/effect_commands.asm']
    order = commands.split('\nCheckOpponentWentFirst:', 1)[1].split('\nBattleCommand_charge:', 1)[0]
    assert 'ld hl, wEnemyGoesFirst\n\tldh a, [hBattleTurn]\n\txor [hl]' in order
    moves = '\n'.join(source.default_lines(files['data/moves/moves.asm']))
    assert re.findall(r'move (\w+),\s+EFFECT_(?:PROTECT|ENDURE),', moves) == ['PROTECT', 'ENDURE']
    denominators = [base ** n for n in range(cap + 1)]
    header = f'/* Generated from Polished Crystal {source.PIN} by build_protect_chain.py. */\n'
    header += '#ifndef GUARD_POLISHED_PROTECT_CHAIN_H\n#define GUARD_POLISHED_PROTECT_CHAIN_H\n\n'
    header += f'#define POLISHED_PROTECT_MAX_USES {cap}\n'
    header += '#define POLISHED_PROTECT_DENOMINATORS {' + ', '.join(map(str, denominators)) + '}\n\n#endif\n'
    report = dict(source_repository='Rangi42/polishedcrystal', source_commit=source.PIN,
        source_variant='default (FAITHFUL undefined)', profile='B_POLISHED_MOVE_BALANCE',
        scope='Ordinary singles Protect/Endure successive-use probabilities, shared saturated counter and selected reset cases.',
        moves=['PROTECT', 'ENDURE'], maximum_stored_successes=cap,
        success_denominators_by_prior_successes=denominators,
        rules=dict(shared_counter=True, reset_on_failed_attempt=True, reset_on_other_selected_move=True,
                   reset_on_switch=True, fail_if_opponent_went_first=True),
        adapter='Use a tagged weighted chance with numerator one and the imported denominator, and saturate the existing counter at six for source Protect/Endure. Native profiles, other protection moves and active gimmicks retain their existing paths; no new state fields.',
        gaps=['Exact source RNG stream and the sequence of up to six ternary draws are not reproduced; the weighted roll preserves the total probability.',
              'Interrupted actions, called-move sequencing, disobedience, item/switch action ordering, global priority/AI, every protection interaction and original presentation remain separate audits.',
              'Expansion-only protection moves, formats and gimmicks are retained without certifying them as source behavior.'])
    return {source.ROOT/'include/constants/polished_protect_chain.h': header,
            source.ROOT/'tools/polished_port/manifests/protect_chain.json': json.dumps(report, indent=2)+'\n'}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    for path, expected in build().items():
        if args.check:
            assert path.read_text() == expected, f'Regenerate {path}'
        else:
            path.write_text(expected)
    print('PASS: pinned Protect/Endure chances, six-success cap and reset contract')
