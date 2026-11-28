"""Ice and Dark trainer type scores from AI_Types and the two pinned matchup tables."""
from itertools import product
import re

from build_battle_suite import authenticated_sources, source


def cases():
    files = source.sources()
    mapped = source.move_targets(files)
    moves = {}
    for line in source.default_lines(files['data/moves/moves.asm']):
        if line.startswith('move '):
            name, _, power, kind, *_ = (x.strip() for x in line[5:].split(','))
            moves[name] = (kind, int(power))
    group = [name for name, (kind, _) in moves.items() if kind in ('ICE', 'DARK')]
    assert len(group) == 22
    entries = re.findall(r'^\s*db (\w+),\s*(\w+),\s*(\w+)', files['data/types/type_matchups.asm'], re.M)
    chart = {(attack, defense): multiplier for attack, defense, multiplier in entries}
    types = sorted({d for _, d in chart})
    assert len(types) == 18
    sounds = set(re.findall(r'^\s*db (\w+)$', files['data/moves/sound_moves.asm'], re.M))

    mold_breaker = authenticated_sources()['data/abilities/mold_breaker_suppressed_abilities.asm']['text']
    suppressed = set(re.findall(r'^\s*db (\w+)$', mold_breaker, re.M))

    def row(slots, defenses, inverse=False, mask=0, previous='NONE', attacker='NONE', defender='NONE'):
        deltas = []
        for i, move in enumerate(slots):
            kind, power = moves.get(move, (None, 0))
            delta = 0
            if kind in ('ICE', 'DARK') and not mask & (1 << i):
                # Neither attacking type has a 0x chart entry. Multipliers in the inverse table swap 2x and 1/2x.
                steps = sum({'SUPER_EFFECTIVE': 1, 'NOT_VERY_EFFECTIVE': -1, None: 0}[chart.get((kind, d))]
                            for d in set(defenses))
                if inverse:
                    steps = -steps
                if (attacker != 'NEUTRALIZING_GAS' and not (attacker == 'MOLD_BREAKER' and defender in suppressed)
                        and ((defender == 'SOUNDPROOF' and previous in sounds)
                             or (defender == 'BULLETPROOF' and previous == 'SHADOW_BALL'))):
                    delta = -10
                elif steps > 0 and power:
                    delta = 1
                elif steps < 0 and any(other in moves and moves[other][0] != kind and moves[other][1]
                                       for j, other in enumerate(slots) if not mask & (1 << j)):
                    delta = -1
            deltas.append(str(delta))
        ids = ', '.join(mapped.get(m, 'MOVE_NONE') for m in slots)
        return ('{{' + ids + '}, {' + ', '.join(deltas) + '}, '
                + f'TYPE_{defenses[0]}, TYPE_{defenses[1]}, {int(inverse)}, {mask}, MOVE_{previous}, '
                + f'ABILITY_{attacker}, ABILITY_{defender}' + '},')

    for name, defense, inverse in product(group, types, (False, True)):
        yield row((name, 'TACKLE', 'NONE', 'NONE'), (defense, defense), inverse)
    for defenses, mask, inverse in product((('WATER', 'PSYCHIC'), ('DRAGON', 'FLYING'), ('FIRE', 'STEEL')),
                                           (0, 2, 4, 6, 8, 14, 15), (False, True)):
        for slots in (('ICE_BEAM', 'NONE', 'CRUNCH', 'HAZE'), ('NASTY_PLOT', 'ICE_PUNCH', 'BITE', 'HAIL'),
                      ('ICY_WIND', 'HAZE', 'HAIL', 'ICE_BEAM')):
            yield row(slots, defenses, inverse, mask)
    for previous, defender, attacker in product(('NONE', 'GROWL', 'SHADOW_BALL'), ('SOUNDPROOF', 'BULLETPROOF'),
                                               ('NONE', 'MOLD_BREAKER', 'NEUTRALIZING_GAS')):
        yield row(('ICE_BEAM', 'HAZE', 'DARK_PULSE', 'HONE_CLAWS'), ('GRASS', 'GHOST'),
                  previous=previous, attacker=attacker, defender=defender)
