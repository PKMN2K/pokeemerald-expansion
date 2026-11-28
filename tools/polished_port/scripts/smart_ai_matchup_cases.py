"""Pinned Smart AI rules for draining moves, accuracy changes and two-turn moves."""
from itertools import product

HANDLERS = {
    'LeechHit': {'ABSORB', 'MEGA_DRAIN', 'GIGA_DRAIN', 'DRAIN_PUNCH', 'LEECH_LIFE', 'DRAINING_KISS'},
    'AccuracyDown': {'SMOKESCREEN', 'FLASH'}, 'EvasionUp': {'DOUBLE_TEAM'},
    'Fly': {'FLY', 'DIG'}, 'JumpKick': {'HI_JUMP_KICK'}, 'SolarBeam': {'SOLAR_BEAM'},
    'Magnitude': {'MAGNITUDE'}, 'Earthquake': {'EARTHQUAKE'}, 'Gust': {'GUST'},
}
MOVES = set().union(*HANDLERS.values())

# Chart entries for the defending types used in these cases. Each pair lists
# super-effective and resisted types. Fighting's Ghost immunity is handled below.
DRAIN_CHART = {
    'DRAIN_PUNCH': ({'NORMAL', 'DARK', 'ROCK'}, {'BUG', 'FAIRY', 'FLYING'}),
    'LEECH_LIFE': ({'GRASS', 'DARK'}, {'GHOST', 'FAIRY', 'FLYING'}),
    'DRAINING_KISS': ({'DARK'}, set()),
    'grass': ({'WATER', 'ROCK'}, {'BUG', 'GRASS', 'FLYING'}),
}


def score(move, flags, usable, hp, maximum, target, target_max, weather, speeds, room, stream):
    def roll():
        return next(stream) & 255

    def faster():
        if speeds[0] == speeds[1]:
            roll()
            return False
        return speeds[0] < speeds[1] if room else speeds[0] > speeds[1]

    if move in HANDLERS['LeechHit']:
        key = move if move in DRAIN_CHART else 'grass'
        strong, weak = DRAIN_CHART[key]
        effectiveness = 1
        for field in {flags.get('type1', 'TYPE_NORMAL'), flags.get('type2', 'TYPE_NORMAL')}:
            kind = field.removeprefix('TYPE_')
            if key == 'DRAIN_PUNCH' and kind == 'GHOST':
                if flags.get('inverse'):
                    effectiveness *= 2
                elif not (flags.get('ring') or flags.get('targetIdentified') or
                          flags.get('userAbility') in ('ABILITY_SCRAPPY', 'ABILITY_MINDS_EYE')):
                    effectiveness = 0
            else:
                ratio = 2 if kind in strong else 0.5 if kind in weak else 1
                effectiveness *= 1 / ratio if flags.get('inverse') else ratio
        if key == 'grass' and flags.get('targetAbility') == 'ABILITY_SAP_SIPPER' and flags.get('userAbility') != 'ABILITY_MOLD_BREAKER':
            effectiveness = 0
        if effectiveness < 1:
            return -int(roll() >= 100)
        return int(effectiveness > 1 and hp < maximum and roll() >= 50)

    if move in ('DOUBLE_TEAM', 'FLASH', 'SMOKESCREEN'):
        evasion = move == 'DOUBLE_TEAM'
        if evasion and flags.get('userEvasion') == 6:
            return -10
        current, full = (hp, maximum) if evasion else (target, target_max)
        toxic = flags.get('targetStatus') == 'STATUS1_TOXIC_POISON'
        delta = 0
        if current == full and (evasion or hp > maximum // 2):
            if toxic or roll() < 178:
                return 2
        elif current <= full // 4:
            delta = -2
        else:
            if roll() < 10:
                return 2
            byte = roll()
            if current > full // 2 and byte < 50:
                return 2
            if current <= full // 2 and byte >= 128:
                delta = -2
        if toxic:
            return delta + 2 * (roll() >= 80)
        if flags.get('targetSeeded'):
            return delta + (roll() >= 128)
        if flags.get('targetRolling') and flags.get('targetAccuracy', 0) >= flags.get('userEvasion', 0):
            return delta + 2
        return delta - 1

    hidden = flags.get('hidden')
    in_air_or_ground = hidden in ('STATE_UNDERGROUND', 'STATE_ON_AIR')
    protected = flags.get('seenProtection') and not flags.get('powerHerb')
    if move in ('FLY', 'DIG'):
        return 3 * faster() if in_air_or_ground else (-10 if protected else 0)
    if move == 'SOLAR_BEAM':
        if weather == 'sun':
            return 0
        return -2 if 'SUNNY_DAY' in usable else (-10 if protected else 0)
    if move == 'HI_JUMP_KICK':
        if 'ABILITY_NO_GUARD' in (flags.get('userAbility'), flags.get('targetAbility')):
            return 0
        return -10 if faster() and in_air_or_ground else 0
    assert move in ('GUST', 'MAGNITUDE', 'EARTHQUAKE')
    gust = move == 'GUST'
    if flags.get('selected') != ('MOVE_FLY' if gust else 'MOVE_DIG'):
        return 0
    outruns = faster()
    if hidden == ('STATE_ON_AIR' if gust else 'STATE_UNDERGROUND'):
        return 2 * outruns
    return int(not outruns and roll() >= 128)


def cases(row, draws):
    def emit(moves, hp=100, target=100, mask=0, seed=7, speeds=(90, 100), room=False, weather='clear', **flags):
        return row(moves, hp, 101, target, weather, weather, mask, seed, speeds, room, target_max=101, tactics=flags)

    groups = [('ABSORB', 'MEGA_DRAIN', 'GIGA_DRAIN', 'DRAIN_PUNCH'),
              ('DRAIN_PUNCH', 'LEECH_LIFE', 'DRAINING_KISS', 'ABSORB')]
    for slots, types, hp, inverse in product(groups, [('WATER', 'WATER'), ('ROCK', 'DARK'), ('WATER', 'GRASS'),
            ('GRASS', 'BUG'), ('FLYING', 'FAIRY'), ('GHOST', 'NORMAL'), ('DARK', 'GHOST')], (50, 100, 101), (0, 1)):
        yield emit(slots, hp=hp, type1='TYPE_' + types[0], type2='TYPE_' + types[1], inverse=inverse)
    for flags, inverse in product(({}, {'ring': 1}, {'targetIdentified': 1}, {'userAbility': 'ABILITY_SCRAPPY'},
            {'userAbility': 'ABILITY_MINDS_EYE'}, {'targetAbility': 'ABILITY_SAP_SIPPER'},
            {'targetAbility': 'ABILITY_SAP_SIPPER', 'userAbility': 'ABILITY_MOLD_BREAKER'}), (0, 1)):
        yield emit(groups[1], type1='TYPE_GHOST', type2='TYPE_WATER', inverse=inverse, **flags)
    accuracy = ('DOUBLE_TEAM', 'SMOKESCREEN', 'FLASH', 'DOUBLE_TEAM')
    for hp, target, flags, mask in product((25, 26, 50, 51, 100, 101), (25, 26, 50, 51, 100, 101),
            ({}, {'targetStatus': 'STATUS1_TOXIC_POISON'}, {'targetSeeded': 1}, {'userEvasion': 6},
             {'targetRolling': 1, 'userEvasion': 1}, {'targetRolling': 1, 'userEvasion': 1, 'targetAccuracy': 1}),
            (0, 5)):
        yield emit(accuracy, hp=hp, target=target, mask=mask, **flags)
    for speeds, room, selected, hidden in product(((80, 120), (100, 100), (120, 80)), (0, 1),
            ('MOVE_NONE', 'MOVE_FLY', 'MOVE_DIG'), ('STATE_NONE', 'STATE_ON_AIR', 'STATE_UNDERGROUND', 'STATE_UNDERWATER')):
        yield emit(('GUST', 'MAGNITUDE', 'EARTHQUAKE', 'HI_JUMP_KICK'), speeds=speeds, room=room, selected=selected, hidden=hidden)
        for ability in ('userAbility', 'targetAbility'):
            yield emit(('FLY', 'DIG', 'HI_JUMP_KICK', 'GUST'), speeds=speeds, room=room, selected=selected,
                       hidden=hidden, seenProtection=1, **{ability: 'ABILITY_NO_GUARD'})
    for herb, seen, weather, mask, hidden in product((0, 1), (0, 1), ('clear', 'sun'), (0, 1, 4, 15),
                                                    ('STATE_NONE', 'STATE_ON_AIR')):
        yield emit(('SOLAR_BEAM', 'DIG', 'SUNNY_DAY', 'FLY'), powerHerb=herb, seenProtection=seen, weather=weather,
                   mask=mask, hidden=hidden)

    for move, args, pattern in [
            ('ABSORB', {'type1': 'TYPE_GRASS', 'type2': 'TYPE_GRASS'}, [100]),
            ('ABSORB', {'type1': 'TYPE_WATER', 'type2': 'TYPE_WATER'}, [50]),
            ('DOUBLE_TEAM', {'hp': 101}, [178]), ('FLASH', {'target': 26}, [10]),
            ('FLASH', {'target': 51}, [10, 50]), ('FLASH', {'target': 26}, [10, 128]),
            ('SMOKESCREEN', {'target': 25, 'targetStatus': 'STATUS1_TOXIC_POISON'}, [80]),
            ('SMOKESCREEN', {'target': 25, 'targetSeeded': 1}, [128]),
            ('GUST', {'selected': 'MOVE_FLY', 'speeds': (100, 100)}, [0, 128])]:
        for byte in (pattern[-1] - 1, pattern[-1]):
            for seed in range(65536):
                stream = draws(seed)
                if all(next(stream) & 255 >= n for n in pattern[:-1]) and next(stream) & 255 == byte:
                    break
            else:
                raise AssertionError((move, byte))
            yield emit((move, 'TACKLE', 'TACKLE', 'TACKLE'), seed=seed, **args)


PROTECTION_BATTLES = ('unseen_protect', 'shown_protect', 'shown_endure', 'herb_protect')
LOCKED_BATTLES = {'player_fly': 'Gust', 'player_dig': 'Earthquake'}
BATTLES = {'drain_water', 'drain_sipper'} | {
    name + suffix for name in (*PROTECTION_BATTLES, *LOCKED_BATTLES) for suffix in ('', '_next')
}


def check_battles(results):
    for name in PROTECTION_BATTLES:
        first = results[name]
        assert first['move'] == 'Recover' and tuple(first['scores']) == (100, 102), (name, first)
        second = results[name + '_next']
        blocked = name in ('shown_protect', 'shown_endure')
        assert tuple(second['scores']) == (90 if blocked else 100, 98), (name, second)
        assert second['move'] == ('Recover' if blocked else 'Solar Beam'), (name, second)
    for name in ('drain_water', 'drain_sipper'):
        battle = results[name]
        drain, bond = battle['scores']
        assert bond == 99, (name, battle)
        assert drain in ((100, 101) if name == 'drain_water' else (99, 100)), (name, battle)
        if drain > bond:
            assert battle['move'] == 'Giga Drain', (name, battle)
    assert results['drain_water']['scores'][0] > results['drain_sipper']['scores'][0]
    for name, move in LOCKED_BATTLES.items():
        for turn, score in ((name, 100), (name + '_next', 102)):
            battle = results[turn]
            assert battle['move'] == move and tuple(battle['scores']) == (score, 99), (turn, battle)
