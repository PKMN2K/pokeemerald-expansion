"""Basic trainer decisions from AI_Redundant in the default v3.2.3 ROM."""
from itertools import product
import re

RECOVERY = {'RECOVER', 'REST', 'ROOST', 'FRESH_SNACK', 'HEALINGLIGHT'}
SETTERS = {'RAIN_DANCE': 'rain', 'SUNNY_DAY': 'sun', 'SANDSTORM': 'sand', 'HAIL': 'hail'}
SCREENS = {'REFLECT': 'reflect', 'LIGHT_SCREEN': 'lightScreen', 'SAFEGUARD': 'ownSafeguard'}
CONFUSION = {'SUPERSONIC', 'CONFUSE_RAY', 'SWEET_KISS'}
SCENARIOS = ('basic_screens', 'basic_confusion', 'basic_weather')
BATTLES = {name + suffix for name in SCENARIOS for suffix in ('', '_next')}


def check_source(assembly, move_records):
    table = dict(re.findall(r'^\s*dbw EFFECT_(\w+),\s+\.(\w+)$', assembly, re.M))
    expected = {
        'Heal': {'RECOVER', 'REST', 'FRESH_SNACK'}, 'HealingLight': {'HEALINGLIGHT'}, 'Roost': {'ROOST'},
        'RainDance': {'RAIN_DANCE'}, 'SunnyDay': {'SUNNY_DAY'}, 'Sandstorm': {'SANDSTORM'}, 'Hail': {'HAIL'},
        'Reflect': {'REFLECT'}, 'LightScreen': {'LIGHT_SCREEN'}, 'Safeguard': {'SAFEGUARD'},
        'Confuse': CONFUSION, 'Swagger': {'SWAGGER'},
    }
    found = {handler: set() for handler in expected}
    for line in move_records:
        match = re.match(r'move (\w+),\s*EFFECT_(\w+),', line)
        if match and (handler := table.get(match[2])) in found:
            found[handler].add(match[1])
    assert found == expected, found


def penalty(move, hp, maximum, weather, flags):
    if move in ('SPORE', 'HYPNOSIS', 'SING', 'SLEEP_POWDER'):
        redundant = flags.get('targetStatus') or flags.get('safeguard')
    elif move == 'DREAM_EATER':
        status = flags.get('targetStatus', 0)
        redundant = not (isinstance(status, int) and status & 7)
    elif move in RECOVERY:
        redundant = hp == maximum
    elif move in SETTERS:
        redundant = weather == SETTERS[move]
    elif move in SCREENS:
        redundant = flags.get(SCREENS[move])
    elif move in CONFUSION or move == 'SWAGGER':
        redundant = flags.get('targetConfused') or (move in CONFUSION and flags.get('safeguard'))
    else:
        redundant = False
    return -10 if redundant else 0


def cases(row, state_type):
    def emit(slots, flags, mask, hp=301, weather='clear', state=None):
        return row(slots, hp, 301, 256, weather, 'clear', mask, 7, target_max=301,
                   state=state or state_type(), tactics=flags)

    screens = ('REFLECT', 'LIGHT_SCREEN', 'SAFEGUARD', 'CONFUSE_RAY')
    for bits, mask in product(product((0, 1), repeat=4), (0, 2, 9, 15)):
        flags = dict(zip(('reflect', 'lightScreen', 'ownSafeguard', 'safeguard'), bits))
        yield emit(screens, flags, mask)

    for confused, guard, own_guard, mask in product((0, 1, 3), (0, 1), (0, 1), (0, 1, 5, 15)):
        yield emit(('SUPERSONIC', 'CONFUSE_RAY', 'SWEET_KISS', 'SWAGGER'),
                   dict(targetConfused=confused, safeguard=guard, ownSafeguard=own_guard), mask)

    for hp, status, mask in product((256, 300, 301), (0, 'STATUS1_POISON'), (0, 2, 15)):
        for slots in (('REST', 'RECOVER', 'ROOST', 'FRESH_SNACK'), ('HEALINGLIGHT', 'REST', 'RECOVER', 'ROOST')):
            yield emit(slots, {}, mask, hp, state=state_type(status=status))

    for weather, mask in product(('clear', 'sun', 'rain', 'sand', 'hail'), (0, 1, 6, 15)):
        yield emit(tuple(SETTERS), {}, mask, weather=weather)


def check_battles(results):
    def check(name, scores, moves):
        battle = results[name]
        if tuple(battle['scores']) != scores or battle['move'] not in moves:
            raise ValueError(f'{name}: {battle}')

    check('basic_screens', (100, 100), ('Reflect', 'Light Screen'))
    if results['basic_screens']['move'] == 'Reflect':
        check('basic_screens_next', (90, 100), ('Light Screen',))
    else:
        check('basic_screens_next', (100, 90), ('Reflect',))
    check('basic_confusion', (100, 100), ('Confuse Ray', 'Swagger'))
    check('basic_confusion_next', (90, 100), ('Swagger',))
    check('basic_weather', (90, 100), ('Rain Dance',))
    check('basic_weather_next', (100, 90), ('Sunny Day',))
