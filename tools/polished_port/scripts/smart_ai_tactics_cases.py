"""Source score and next-draw cases for the Smart sleep, trapping and volatile handlers."""
from itertools import product

HANDLERS = {
    'Sleep': {'SING', 'SLEEP_POWDER', 'HYPNOSIS', 'SPORE'}, 'DreamEater': {'DREAM_EATER'},
    'Bind': {'WRAP', 'FIRE_SPIN', 'WHIRLPOOL'}, 'HealBell': {'HEAL_BELL'},
    'RapidSpin': {'RAPID_SPIN'}, 'FutureSight': {'FUTURE_SIGHT'}, 'Rollout': {'ROLLOUT'},
    'Rage': {'RAGE'}, 'Endure': {'ENDURE'}, 'Haze': {'HAZE'}, 'Foresight': {'FORESIGHT'},
}
MOVES = set().union(*HANDLERS.values())


BATTLE_OUTCOMES = {
    'awake_target': ('Spore', (100, 102), (90, 93)),
    'sleeping_target': ('Dream Eater', (90, 92), (100, 103)),
    'no_dream_pp': ('Spore', (100,), (0,)),
    'awake_basic': ('Spore', (100,), (90,)),
    'sleeping_basic': ('Dream Eater', (90,), (100,)),
    'poisoned_reserve': ('Heal Bell', (100,), (99,)),
    'fainted_poisoned_reserve': ('Destiny Bond', (90,), (99,)),
}


def check_battles(battles):
    for name, (move, first, second) in BATTLE_OUTCOMES.items():
        result = battles[name]
        left, right = result['scores']
        if result['move'] != move or left not in first or right not in second:
            raise ValueError(f'{name}: unexpected trainer choice or scores: {result}')


def score(move, flags, usable, hp, maximum, first, speeds, room, state, stream):
    """Higher scores win in the GBA engine."""
    def roll():
        return next(stream) & 255

    target_status = flags.get('targetStatus', 0)
    if move in HANDLERS['Sleep']:
        return 2 if 'DREAM_EATER' in usable and roll() >= 128 else 0
    if move == 'DREAM_EATER':
        return 3 * int(roll() >= 25)
    if move in HANDLERS['Bind']:
        favorable = (first or target_status == 'STATUS1_TOXIC_POISON' or flags.get('targetInLove')
                     or flags.get('targetIdentified') or flags.get('targetRolling'))
        if flags.get('ghost') or flags.get('targetWrapped') or not favorable:
            return -int(roll() >= 128)
        return 2 if hp > maximum // 4 and roll() >= 128 else 0
    if move == 'RAPID_SPIN':
        eligible = flags.get('userSeeded') or flags.get('userWrapped') or flags.get('spikes') == 3
        return 2 if eligible and roll() >= 50 else 0
    if move == 'FUTURE_SIGHT':
        if speeds[0] == speeds[1]:
            roll()  # CheckSpeed randomizes ties, but AICompareSpeed never treats a tie as faster.
        faster = speeds[0] < speeds[1] if room else speeds[0] > speeds[1]
        return 2 if faster and flags.get('hidden') in ('STATE_ON_AIR', 'STATE_UNDERGROUND') else -1
    if move == 'HEAL_BELL':
        if not flags.get('partyStatus'):
            return 0 if state.status else -10
        asleep_or_frozen = (isinstance(state.status, int) and state.status & 7) or state.status == 'STATUS1_FREEZE'
        return int(bool(state.status)) + (2 if asleep_or_frozen and roll() >= 128 else 0)
    if move == 'HAZE':
        unfavorable = (min(state.accuracy, state.sp_defense, flags.get('userOffset', 0)) <= -3
                       or max(state.evasion, flags.get('targetOffset', 0)) >= 3)
        return int(roll() >= 40) if unfavorable else -1
    if move == 'FORESIGHT':
        if flags.get('ghost') or state.accuracy <= -3 or state.evasion >= 3:
            return 2 * int(roll() >= 100)
        return -int(roll() >= 20)
    if move == 'RAGE':
        if flags.get('userRage'):
            return int(roll() >= 128)
        return int(roll() < 50) if hp > maximum // 2 else -1
    if move == 'ROLLOUT':
        if (flags.get('userInLove') or flags.get('userConfused') or state.status == 'STATUS1_PARALYSIS'
                or hp <= maximum // 4 or state.accuracy < 0 or state.evasion > 0):
            return -int(roll() >= 50)
        return 2 * int(roll() < 200)
    assert move == 'ENDURE'
    if flags.get('userProtected') or hp == maximum:
        return -2
    if hp > maximum // 4:
        return -1
    if 'REVERSAL' in usable:
        return 3 * int(roll() >= 50)
    return 2 if flags.get('targetLockOn') and roll() >= 128 else 0


def cases(row, draws, state_type):
    def emit(moves, hp=50, mask=0, first=False, seed=7, speeds=(80, 120), room=False, state=None, **flags):
        return row(moves, hp, 100, 100, 'clear', 'clear', mask, seed, speeds, room, first,
                   state=state or state_type(), tactics=flags)

    for slots in (('SING', 'DREAM_EATER', 'SLEEP_POWDER', 'HYPNOSIS'),
                  ('DREAM_EATER', 'SPORE', 'SING', 'DREAM_EATER'),
                  ('SPORE', 'HYPNOSIS', 'SLEEP_POWDER', 'SING')):
        for target, guard, mask in product((0, 1, 3, 'STATUS1_POISON'), (0, 1), (0, 1, 2, 5, 9, 15)):
            yield emit(slots, mask=mask, targetStatus=target, safeguard=guard)
    binding = ('WRAP', 'FIRE_SPIN', 'WHIRLPOOL', 'RAPID_SPIN')
    for flags, hp, first in product(
            ({}, {'ghost': 1}, {'targetWrapped': 1}, {'targetInLove': 1}, {'targetIdentified': 1},
             {'targetRolling': 1}, {'targetStatus': 'STATUS1_TOXIC_POISON'}, {'targetStatus': 'STATUS1_POISON'},
             {'targetStatus': 'STATUS1_TOXIC_POISON', 'ghost': 1}), (25, 26), (False, True)):
        yield emit(binding, hp=hp, first=first, **flags)
    for spikes, wrapped, seeded in product(range(4), (0, 1), (0, 1)):
        yield emit(binding[::-1], spikes=spikes, userWrapped=wrapped, userSeeded=seeded)
    for status, party in product((0, 1, 2, 7, 'STATUS1_FREEZE', 'STATUS1_POISON', 'STATUS1_PARALYSIS'), (0, 1)):
        yield emit(('HEAL_BELL', 'RAGE', 'ROLLOUT', 'ENDURE'), state=state_type(status=status), partyStatus=party)
    for hp, flags, mask in product((24, 25, 26, 50, 51, 99, 100),
                                   ({}, {'userProtected': 1}, {'targetLockOn': 1}, {'userRage': 1},
                                    {'userInLove': 1}, {'userConfused': 1}), (0, 2)):
        yield emit(('ENDURE', 'REVERSAL', 'ROLLOUT', 'RAGE'), hp=hp, mask=mask, **flags)
    for stat, user, target in product(('ATK', 'DEF', 'SPEED', 'SPATK', 'SPDEF', 'ACC', 'EVASION'),
                                      (-3, -2, 3), (-3, 2, 3)):
        yield emit(('HAZE', 'HAZE', 'RAGE', 'WRAP'), stat='STAT_' + stat, userOffset=user, targetOffset=target)
    for acc, eva, ghost in product((-3, -2, -1, 0), (0, 1, 2, 3), (0, 1)):
        yield emit(('FORESIGHT', 'ROLLOUT', 'HAZE', 'WHIRLPOOL'), state=state_type(accuracy=acc, evasion=eva), ghost=ghost)
    for speeds, room, hidden in product(((80, 120), (120, 80), (100, 100)), (False, True),
                                        ('STATE_NONE', 'STATE_ON_AIR', 'STATE_UNDERGROUND', 'STATE_UNDERWATER')):
        yield emit(('FUTURE_SIGHT', 'SPORE', 'DREAM_EATER', 'RAGE'), speeds=speeds, room=room, hidden=hidden)

    trials = [('SPORE', 128, {}), ('DREAM_EATER', 25, {}), ('WRAP', 128, {'first': True}),
              ('FORESIGHT', 100, {'ghost': 1}), ('FORESIGHT', 20, {}), ('RAPID_SPIN', 50, {'spikes': 3}),
              ('HAZE', 40, {'state': state_type(evasion=3)}), ('ROLLOUT', 200, {}),
              ('RAGE', 50, {'hp': 51}), ('ENDURE', 50, {'hp': 25}),
              ('HEAL_BELL', 128, {'state': state_type(status=1), 'partyStatus': 1})]
    for move, cutoff, flags in trials:
        for byte in (cutoff - 1, cutoff):
            seed = next(n for n in range(65536) if next(draws(n)) & 255 == byte)
            yield emit((move, 'DREAM_EATER', 'REVERSAL', 'FORESIGHT'), seed=seed, **flags)
