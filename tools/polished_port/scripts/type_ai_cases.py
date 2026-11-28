"""Trainer type scores from AI_Types and the default release's two matchup tables."""
from collections import Counter
from itertools import product
import re

from build_battle_suite import ROOT, authenticated_sources, source


SCORED_TYPES = {'ICE': 11, 'DARK': 11, 'FIGHTING': 17, 'GHOST': 10, 'PSYCHIC': 21}


def read_chart(text):
    entries = {}
    after_foresight = False
    for line in source.default_lines(text):
        if line == 'db $fe':
            after_foresight = True
        elif match := re.fullmatch(r'db (\w+),\s*(\w+),\s*(\w+)', line):
            entries.setdefault(match[1], []).append((match[2], match[3], after_foresight))
    return entries


def matchup(entries, defenses, bypass_ghost, ring):
    value = 10  # EFFECTIVE in the source.
    for defense, multiplier, after_foresight in entries:
        if after_foresight and bypass_ghost:
            break
        if defense not in defenses:
            continue
        if multiplier == 'SUPER_EFFECTIVE':
            value *= 2
        elif multiplier == 'NOT_VERY_EFFECTIVE':
            value //= 2
        elif multiplier == 'NO_EFFECT':
            if not ring:
                return 0
        else:
            raise ValueError(multiplier)
    return value


def cases():
    files = source.sources()
    mapped = source.move_targets(files)
    mapped['NONE'] = 'MOVE_NONE'
    moves = {}
    for line in source.default_lines(files['data/moves/moves.asm']):
        if line.startswith('move '):
            name, _, power, kind, *_ = (x.strip() for x in line[5:].split(','))
            moves[name] = (kind, int(power))
    group = [name for name, (kind, _) in moves.items() if kind in SCORED_TYPES]
    assert Counter(moves[name][0] for name in group) == SCORED_TYPES
    fixtures = ROOT / 'tools/polished_port/fixtures/battle_logic'
    charts = (read_chart(files['data/types/type_matchups.asm']), read_chart((fixtures / 'inverse_type_matchups.asm').read_text()))
    types = sorted(charts[0])
    assert len(types) == 18 and set(types) == charts[1].keys()
    assert [sum(map(len, chart.values())) for chart in charts] == [119, 120]
    sounds = set(re.findall(r'^\s*db (\w+)$', files['data/moves/sound_moves.asm'], re.M))
    bullets = set(re.findall(r'^\s*db (\w+)$', (fixtures / 'bullet_moves.asm').read_text(), re.M))
    mold_breaker = authenticated_sources()['data/abilities/mold_breaker_suppressed_abilities.asm']['text']
    suppressed = set(re.findall(r'^\s*db (\w+)$', mold_breaker, re.M))

    def row(slots, defenses, inverse=False, mask=0, previous='NONE', attacker='NONE', defender='NONE',
            ring=False, identified=False):
        bypass_ghost = identified or (defender != 'NEUTRALIZING_GAS' and attacker in ('SCRAPPY', 'MINDS_EYE'))
        nullified = ((defender == 'SOUNDPROOF' and previous in sounds) or (defender == 'BULLETPROOF' and previous in bullets))
        if attacker == 'NEUTRALIZING_GAS' or (attacker == 'MOLD_BREAKER' and defender in suppressed):
            nullified = False
        attacks = {moves[move][0] for j, move in enumerate(slots) if move in moves and moves[move][1] and not mask & (1 << j)}
        deltas = []
        for i, move in enumerate(slots):
            kind, power = moves.get(move, (None, 0))
            delta = 0
            if kind in SCORED_TYPES and not mask & (1 << i):
                value = matchup(charts[inverse][kind], defenses, bypass_ghost, ring)
                if nullified or value == 0:
                    delta = -10
                elif value > 10 and power:
                    delta = 1
                elif value < 10 and attacks - {kind}:
                    delta = -1
            deltas.append(str(delta))
        ids = ', '.join(mapped[m] for m in slots)
        return ('{{' + ids + '}, {' + ', '.join(deltas) + '}, '
                + f'TYPE_{defenses[0]}, TYPE_{defenses[1]}, {int(inverse)}, {mask}, {mapped[previous]}, '
                + f'ABILITY_{attacker}, ABILITY_{defender}, {int(ring)}, {int(identified)}' + '},')

    for name, defense, inverse in product(group, types, (False, True)):
        yield row((name, 'TACKLE', 'NONE', 'NONE'), (defense, defense), inverse)
    for defenses, mask, inverse in product((('WATER', 'PSYCHIC'), ('DRAGON', 'FLYING'), ('FIRE', 'STEEL')),
                                           (0, 2, 4, 6, 8, 14, 15), (False, True)):
        for slots in (('ICE_BEAM', 'NONE', 'CRUNCH', 'HAZE'), ('NASTY_PLOT', 'ICE_PUNCH', 'BITE', 'HAIL'),
                      ('ICY_WIND', 'HAZE', 'HAIL', 'ICE_BEAM'), ('BULK_UP', 'COUNTER', 'MIRROR_COAT', 'NIGHT_SHADE'),
                      ('CURSE', 'CALM_MIND', 'SEISMIC_TOSS', 'FUTURE_SIGHT')):
            yield row(slots, defenses, inverse, mask)
    for pair in (('GHOST', 'ICE'), ('NORMAL', 'PSYCHIC'), ('DARK', 'FIGHTING')):
        for defenses, inverse, ring, identified, abilities in product(
                (pair, pair[::-1]), (False, True), (False, True), (False, True),
                (('NONE', 'NONE'), ('SCRAPPY', 'NONE'), ('MINDS_EYE', 'NONE'), ('SCRAPPY', 'NEUTRALIZING_GAS'),
                 ('MINDS_EYE', 'NEUTRALIZING_GAS'))):
            yield row(('BRICK_BREAK', 'SHADOW_BALL', 'PSYCHIC_M', 'BULK_UP'), defenses, inverse,
                      ring=ring, identified=identified, attacker=abilities[0], defender=abilities[1])
    for previous, defender, attacker in product(('NONE', 'GROWL', 'SHADOW_BALL', 'AURA_SPHERE', 'FOCUS_BLAST'),
                                               ('SOUNDPROOF', 'BULLETPROOF'), ('NONE', 'MOLD_BREAKER', 'NEUTRALIZING_GAS')):
        for slots in (('ICE_BEAM', 'HAZE', 'DARK_PULSE', 'HONE_CLAWS'), ('AURA_SPHERE', 'HEX', 'FOCUS_BLAST', 'TRICK')):
            yield row(slots, ('GRASS', 'GHOST'), previous=previous, attacker=attacker, defender=defender)
